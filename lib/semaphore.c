// User-level Semaphore

#include "inc/lib.h"
#include "inc/queue.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...

	// Create shared object
	void* va = smalloc(semaphoreName, sizeof(struct semaphore), 1);
	if (va == NULL)
	{
		return NULL;
	}
	struct semaphore* sem = (struct semaphore*) va;

	// Initialize semaphore data
	for (int i = 0; i < sizeof(semaphoreName) / sizeof(int); i++)
	{
		sem->semdata->name[i] = semaphoreName[i];
	}
	sem->semdata->count=value;
	sem->semdata->lock=0;

	if(&(sem->semdata->queue) != NULL)
	{
		LIST_INIT(&(sem->semdata->queue));
	}

	return *sem;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("get_semaphore is not implemented yet");
	//Your Code is Here...
}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("wait_semaphore is not implemented yet");
	//Your Code is Here...
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("signal_semaphore is not implemented yet");
	//Your Code is Here...
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
