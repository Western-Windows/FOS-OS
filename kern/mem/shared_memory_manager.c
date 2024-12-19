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
	for (int i = 0; i < numOfFrames; i++){
		frames_storage[i] = NULL;
	}
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
//	for (int i = 0; i < sizeof(shareName) / sizeof(int); i++){
//		sharedObjectPtr->name[i] = shareName[i];
//	}
	strcpy(sharedObjectPtr->name, shareName);
	//cprintf("Shared object name  %s  sharename %s\n",sharedObjectPtr->name,shareName);
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

	acquire_spinlock(&AllShares.shareslock);
	//cprintf("I'm searching for sem in getshare name %s id %d\n", name, ownerID);
	struct Share *ptr;
	//for (int i = 0; i < LIST_SIZE(&AllShares.shares_list); i++)
	LIST_FOREACH (ptr, &AllShares.shares_list)
	{
		//cprintf("iterator name %s & id %d \n", ptr->name, ptr->ownerID);

		if (ptr->ownerID == ownerID && (strcmp(ptr->name, name) == 0))
		{
			release_spinlock(&AllShares.shareslock);
			//cprintf("Found The object\n");
			return ptr;
		}
	}
	release_spinlock(&AllShares.shareslock);
	//cprintf("Returned Null in get share\n");
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
	acquire_spinlock(&AllShares.shareslock);
	if (isExistingObject != NULL)
	{
		release_spinlock(&AllShares.shareslock);
		return E_SHARED_MEM_EXISTS;
	}

	uint8 perm = isWritable;

	struct Share* ptrToSharedObject = create_share(ownerID, shareName, size, isWritable);
	if (ptrToSharedObject == NULL)
	{
		release_spinlock(&AllShares.shareslock);
		return E_NO_SHARE;
	}


	LIST_INSERT_TAIL(&AllShares.shares_list, ptrToSharedObject);


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
			LIST_REMOVE(&AllShares.shares_list, ptrToSharedObject);
			release_spinlock(&AllShares.shareslock);
			return E_NO_SHARE;
		}
		// Mapping of frames.
		map_frame(myenv->env_page_directory, frame, currentAddress, PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
		if (allocateResult == E_NO_MEM)
		{
		    LIST_REMOVE(&AllShares.shares_list, ptrToSharedObject);
		    free_frame(frame);
		    release_spinlock(&AllShares.shareslock);
		    return E_NO_SHARE;
		}

		ptrToSharedObject->framesStorage[index] = frame;
		currentAddress += PAGE_SIZE;
		index ++;
	}
	release_spinlock(&AllShares.shareslock);
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
		//cprintf("get share returned null\n");
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
			map_frame(myenv->env_page_directory, frameStorage[index], va, PERM_PRESENT|PERM_USER);
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

	kfree(ptrShare->framesStorage);
	acquire_spinlock(&AllShares.shareslock);
	LIST_REMOVE(&AllShares.shares_list,ptrShare);
	release_spinlock(&AllShares.shareslock);
	kfree(ptrShare);
}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("freeSharedObject is not implemented yet");
	//Your Code is Here...
		struct Env* myenv = get_cpu_proc();
	    struct Share *share = NULL;
	    uint32 *ptr_page_table = NULL;
	    struct FrameInfo *frame = get_frame_info(myenv->env_page_directory,(uint32)startVA,&ptr_page_table);
	    startVA = ROUNDDOWN(startVA,PAGE_SIZE);
		if(sharedObjectID == -1){
			bool flag = 1;
			acquire_spinlock(&AllShares.shareslock);
			LIST_FOREACH(share, &AllShares.shares_list) {
			        if (share->framesStorage == NULL) {
			            continue;
			        }
			        int sz = ROUNDUP(share->size,PAGE_SIZE);
			        for (int i = 0; i <(sz>>12); i++) {
			            if (share->framesStorage[i] == frame) {
			                sharedObjectID = share->ID;
			                flag= 0;
			                break;
			            }
			        }
			        if(flag == 0){
			        	break;
			        }
			   }
			release_spinlock(&AllShares.shareslock);

		}
		if (sharedObjectID == -1) {
			return E_SHARED_MEM_NOT_EXISTS;
		}
		uint32 virtual_address = (uint32)startVA;
	    uint32 va = ROUNDDOWN(virtual_address, PAGE_SIZE);
	    uint32 end_va = ROUNDUP(virtual_address + share->size, PAGE_SIZE);
	    for (uint32 i = va; i < end_va; i += PAGE_SIZE) {
	        unmap_frame(myenv->env_page_directory, i);
	    }
	    bool empty = 1;
	    uint32 start_table_va = ROUNDDOWN(va, PAGE_SIZE<<10);
	    uint32 end_table_va = ROUNDUP(end_va, PAGE_SIZE<<10);
	    for (uint32 va = start_table_va; va < end_table_va; va += PAGE_SIZE<<10) {
	        uint32 *page_table = NULL;
	        if (get_page_table(myenv->env_page_directory, va, &page_table) == TABLE_NOT_EXIST) {
	        //	cprintf("NO TABLE EXISTS\n");
	            continue;
	        }
	        for (int i = 0; i < (1<<10); i++) {
	            if (page_table[i] != 0) {
	          //  	cprintf("EMPTY TABLE\n");
	                empty = 0;
	                break;
	            }
	        }
	        if (empty) {
	        	//cprintf("EMPTY = 1 \n");
	            kfree(page_table);
	            pd_clear_page_dir_entry(myenv->env_page_directory, va);
	        }
	        empty = 1;
	    }
	    share->references--;
	    if (share->references == 0) {
	   // 	cprintf("FREE SHARE CALLED\n");
	    	free_share(share);
	    }
	    tlbflush();

	    return 0;
}
