#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...
	struct FrameInfo** frames_storage = (void*)kmalloc((numOfFrames*sizeof(struct FrameInfo*)));
	return frames_storage;
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share *sharedObjectPtr = (void*)kmalloc(sizeof(struct Share));
	if (sharedObjectPtr == NULL){
		return NULL;
	}
	sharedObjectPtr->ownerID = ownerID;
	for (int i = 0; i < sizeof(shareName) / sizeof(int); i++){
		sharedObjectPtr->name[i] = shareName[i];
	}
	sharedObjectPtr->size = size;
	sharedObjectPtr->isWritable = isWritable;
	sharedObjectPtr->references = 1;
	uint32 id = (uint32)sharedObjectPtr;
	id &= 0x7FFFFFFF;
	sharedObjectPtr->ID = id;
	struct FrameInfo ** framesStorage = create_frames_storage(ROUNDUP(size,PAGE_SIZE) / PAGE_SIZE);
	if (framesStorage == NULL){
		kfree((void*)sharedObjectPtr);
		return NULL;
	}
	sharedObjectPtr->framesStorage = framesStorage;
	return sharedObjectPtr;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
	//acquire_spinlock(&AllShares.shareslock);

	struct Share *ptr;
	//for (int i = 0; i < LIST_SIZE(&AllShares.shares_list); i++)
	LIST_FOREACH (ptr, &AllShares.shares_list)
	{
		if (ptr->ownerID == ownerID && (strncmp(ptr->name, name,sizeof(ptr->name)/ sizeof(int)) == 0))
		{
			return ptr;
		}
	}
	return NULL;
}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment

	struct Share* isExistingObject = get_share(ownerID,shareName);
	if (isExistingObject != NULL)
	{
		return E_SHARED_MEM_EXISTS;
	}

	uint8 perm = isWritable;

	struct Share* ptrToSharedObject = create_share(ownerID, shareName, size, isWritable);
	if (ptrToSharedObject == NULL)
	{
		return E_NO_SHARE;
	}

	acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_TAIL(&AllShares.shares_list, ptrToSharedObject);
	release_spinlock(&AllShares.shareslock);

	int framesNumber = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	uint32 currentAddress = (uint32)virtual_address;
	int index = 0;

	while (framesNumber --)
	{
		// Allocation of frames in memory.
		struct FrameInfo*  frame = NULL;
		int allocateResult = allocate_frame(&frame);
		if (allocateResult == E_NO_MEM)
		{
			acquire_spinlock(&AllShares.shareslock);
			LIST_REMOVE(&AllShares.shares_list, ptrToSharedObject);
			release_spinlock(&AllShares.shareslock);
			return E_NO_SHARE;
		}

		// Mapping of frames.
		map_frame(myenv->env_page_directory, frame, currentAddress, PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
		cprintf("Creating shared object '%s' with size = %d, rounded size = %d\n", shareName, size, ROUNDUP(size, PAGE_SIZE));
		cprintf("Mapping frame %p to VA %x\n", frame, currentAddress);

		if (allocateResult == E_NO_MEM)
		{
		    acquire_spinlock(&AllShares.shareslock);
		    LIST_REMOVE(&AllShares.shares_list, ptrToSharedObject);
		    release_spinlock(&AllShares.shareslock);
		    free_frame(frame);
		    return E_NO_SHARE;
		}

		ptrToSharedObject->framesStorage[index] = frame;
		currentAddress += PAGE_SIZE;
		index ++;
	}

	return ptrToSharedObject->ID;
}
//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment

	struct Share* sharedObject = get_share(ownerID, shareName);
	if(sharedObject == NULL)
	{
		return E_SHARED_MEM_NOT_EXISTS;
	}

	struct FrameInfo** frameStorage = sharedObject->framesStorage;
	uint32 size = sharedObject->size;
	uint32 framesNumber = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	uint8 perm = sharedObject->isWritable;

	uint32 va = (uint32)virtual_address;
	int index = 0;

	while(framesNumber --)
	{
		if(perm == 1)
		{
			map_frame(myenv->env_page_directory, frameStorage[index], va, PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
		}
		else
		{
			map_frame(myenv->env_page_directory, frameStorage[index], va, (~PERM_WRITEABLE)|PERM_PRESENT|PERM_USER);
		}

		index ++;
		va += PAGE_SIZE;
	}

	sharedObject->references ++;
	return sharedObject->ID;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("free_share is not implemented yet");
	//Your Code is Here...

}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}
