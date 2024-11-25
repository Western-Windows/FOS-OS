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
	struct FrameInfo* frames_storage[numOfFrames];
	for (int i = 0; i < numOfFrames; i++){
		frames_storage[i] = NULL;
	}
	struct FrameInfo** ptr = frames_storage;
	return ptr;

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

	uint32 *va = kmalloc(sizeof(struct Share));
	if (va == NULL){
		return NULL;
	}
	struct Share sharedObject;
	struct Share *sharedObjectPtr;
	sharedObject.ownerID = ownerID;
	for (int i = 0; i < sizeof(shareName) / sizeof(int); i++){
		sharedObject.name[i] = shareName[i];
	}
	sharedObject.size = size;
	sharedObject.isWritable = isWritable;
	sharedObject.references = 1;
	sharedObjectPtr = &sharedObject;
	if (sharedObjectPtr == NULL){
		kfree(va);
		return NULL;
	}
	uint32 id = (uint32)sharedObjectPtr;
	id &= 0x7FFFFFFF;
	sharedObject.ID = id;
	struct FrameInfo ** framesStorage = create_frames_storage(ROUNDUP(size,PAGE_SIZE) / PAGE_SIZE);
	if (framesStorage == NULL){
		kfree(va);
		return NULL;
	}
	sharedObject.framesStorage = framesStorage;
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
	struct Share *ptr = AllShares.shares_list.lh_first;
	for (int i = 0; i < LIST_SIZE(&AllShares.shares_list); i++){
		if (ptr->ownerID == ownerID && (strncmp(ptr->name, name, sizeof(name)/ sizeof(int)) == 0)){
			return ptr;
		}
		ptr = LIST_NEXT(ptr);
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

	struct share* isExistingObject = get_share(ownerID,shareName);
	if (isExistingObject != NULL)
	{
		return E_SHARED_MEM_EXISTS;
	}

	struct Share* ptrToSharedObject = create_share(ownerID, shareName, size, isWritable);

	if (ptrToSharedObject == NULL)
	{
		return E_NO_SHARE;
	}

	LIST_INSERT_TAIL(&AllShares.shares_list, ptrToSharedObject);

	size = ROUNDUP(size,PAGE_SIZE);
	int framesNumber = size/PAGE_SIZE;
	uint32 currentAddress = virtual_address;
	int index = 0;

	while (framesNumber --)
	{
		// Allocation of frames in memory.
		struct FrameInfo*  frame = NULL;
		int allocateResult = allocate_frame(&frame);

		uint32 phys_frame = to_physical_address(frame);
		phys_to_virt[FRAME_NUMBER(phys_frame)] = currentAddress;

		// Mapping of frames.
		allocateResult = map_frame(ptr_page_directory, frame, currentAddress, PERM_WRITEABLE);
		if (allocateResult == E_NO_MEM)
		{
			LIST_REMOVE(&AllShares.shares_list, ptrToSharedObject);
			free_frame(frame);
			return E_NO_SHARE;
		}

		ptrToSharedObject->framesStorage[index] = frame;
		currentAddress += PAGE_SIZE;
		index ++;
	}
}
//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
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
