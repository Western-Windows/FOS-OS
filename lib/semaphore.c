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
	void* va = smalloc(semaphoreName, sizeof(struct __semdata), 1);
	if (va == NULL)
	{
		return main_semaphore;
	}
	struct __semdata* sem = (struct __semdata*) va;

	// Initialize semaphore data
	strlcpy(sem->name, semaphoreName, sizeof(sem->name));
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

	// Create wrapper
	struct semaphore main_semaphore;
	main_semaphore.semdata = NULL;

	// Get shared object
	void* va = sget(ownerEnvID, semaphoreName);
	if (va == NULL)
	{
		return main_semaphore;
	}
	struct __semdata* sem = (struct __semdata*) va;

	// Populate wrapper
	main_semaphore.semdata = sem;

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
	sys_sem_signal(&sem);
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...
//	struct semaphore *semPtr = &sem;
	sys_sem_wait(&sem);
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
