// User-level Semaphore

#include "inc/lib.h"
#include "inc/queue.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...

	// Create wrapper
	struct semaphore main_semaphore;
	main_semaphore.semdata = NULL;

	// Create shared object
	//void* va = smalloc(semaphoreName, sizeof(struct __semdata), 1);
	struct __semdata* va = (void*) smalloc(semaphoreName, sizeof(struct __semdata), 1);
	//cprintf("Size of struct semdata %d\n",sizeof(struct __semdata));
	//cprintf("Smalloc returned va address %x, name %s \n", va, semaphoreName);
	if (va == NULL)
	{
		//cprintf("Smalloc returned Null\n");
		return main_semaphore;
	}
	struct __semdata* sem = (struct __semdata*) va;

	// Initialize semaphore data
	strcpy(sem->name, semaphoreName);
	sem->count=value;
	sem->lock=0;
	LIST_INIT(&(sem->queue));

	// Populate wrapper
	main_semaphore.semdata = sem;

	return main_semaphore;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...
	//cprintf("I'm searching for semaphore name %s\n", semaphoreName);
	// Create wrapper
	struct semaphore main_semaphore;
	main_semaphore.semdata = NULL;

	// Get shared object
	struct __semdata* va = (void*)sget(ownerEnvID, semaphoreName);
	//void* va = sget(ownerEnvID, semaphoreName);
	//cprintf("VA sget returned %x\n", va);
	if (va == NULL)
	{
		//cprintf("NULL returned from sget\n");
		return main_semaphore;
	}
	//struct __semdata* sem = (struct __semdata*) va;

	//cprintf("Returned semaphore from sget name %s\n", va->name);
	//cprintf("Returned semaphore from sget name %s\n", sem->name);
	// Populate wrapper
	main_semaphore.semdata = va;

	return main_semaphore;
}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wait_semaphore is not implemented yet");
	//Your Code is Here...
//	uint32* semp = (uint32 *)sem.semdata->lock;
//	while(xchg(&(sem.semdata->lock), 1) != 0);
	//cprintf("Wait semaphore.c\n");
	sys_sem_wait(&sem);
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...
//	struct semaphore *semPtr = &sem;
	//cprintf("Signal semaphore.c\n");
	sys_sem_signal(&sem);
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
