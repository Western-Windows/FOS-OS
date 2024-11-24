#include <inc/lib.h>

int user_pages[NUM_OF_UHEAP_PAGES];
int firstTimeSleepLock = 1;
struct sleeplock user_pages_lck;

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");
	//return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

	void* va;
	bool found = 0;

	// Dynamic Allocator
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		if (sys_isUHeapPlacementStrategyFIRSTFIT() == 1) // First Fit
		{
			va= alloc_block_FF(size);
		}
		else if (sys_isUHeapPlacementStrategyBESTFIT() == 1) // Best Fit
		{
			va= alloc_block_BF(size);
		}
		return va;
	}
	// Page Allocator
	else
	{
		uint32 required_pages = ROUNDUP(size,PAGE_SIZE)>>12;
		uint32 start_va= (uint32)(((char*)myEnv->hardLimit)+ PAGE_SIZE);
		int curr_pages=0;

		// Sleep lock for User Pages
		if (firstTimeSleepLock)
		{
			firstTimeSleepLock = 0;
			init_sleeplock(&user_pages_lck, "User Pages Sleep Lock");
		}
		acquire_sleeplock(&user_pages_lck);

		for (int i=start_va; i<= USER_HEAP_MAX; i+= PAGE_SIZE) // Loop through Page Allocator range
		{

			if(is_marked((void*)i)==0) // Check if page is unmarked (can be used)
			{
				curr_pages++;
			}
			else // If marked, start over counting from next index
			{
				curr_pages=0;
				start_va=i+PAGE_SIZE;
			}

			if (curr_pages == required_pages) // Found required consecutive pages
			{
				found =1;
				va = start_va;
				uint32 page_index = (va - USER_HEAP_START)>>12;
				user_pages[page_index]= curr_pages;
				sys_allocate_user_mem((uint32)va, required_pages*PAGE_SIZE);
				break;
			}
		}

		if (!found) // Required consecutive pages not found
		{
			va = NULL;
		}

		release_sleeplock(&user_pages_lck);
		return va;
	}
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");

	uint32 va = (uint32) virtual_address;
	uint32 u_hard_limit =(uint32)(((char*)myEnv->hardLimit)+ PAGE_SIZE);

	// Block Range
	if (va>=USER_HEAP_START && va< u_hard_limit)
	{
		cprintf("free user block..\n");
		free_block((void*)va);
		return;
	}


	uint32 vaRoundDown = ROUNDDOWN(va,PAGE_SIZE);
	uint32 page_index = (vaRoundDown - USER_HEAP_START)>>12;
	uint32 pages = user_pages[page_index];

	// Pages Range
	if(va>=u_hard_limit && va<=USER_HEAP_MAX)
	{
			cprintf("free user pages..\n");
			sys_free_user_mem(va,pages*PAGE_SIZE);
			return;
	}

	//Invalid address
	else
	{
		panic("failed to free address %x, illegal address", va);
		return;
	}
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}

bool is_marked(void* va)
{
	int permission= pt_get_page_permissions(myEnv->env_page_directory, (uint32)va);

	if (permission & (PERM_AVAILABLE)==1)
		return 1;
	else
		return 0;
}
