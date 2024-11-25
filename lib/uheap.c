#include <inc/lib.h>
int user_pages[NUM_OF_UHEAP_PAGES];
int firstTimeSleepLock = 1;

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	cprintf("Sys Sbrk\n");
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	cprintf("Malloc\n");
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
//		if (sys_isUHeapPlacementStrategyFIRSTFIT() == 1) // First Fit
//		{
//			va= alloc_block_FF(size);
//		}
//		else if (sys_isUHeapPlacementStrategyBESTFIT() == 1) // Best Fit
//		{
//			va= alloc_block_BF(size);
//		}
		cprintf("Block Alloc\n");
		va= alloc_block_FF(size);
		return va;
	}
	// Page Allocator
	else
	{
		cprintf("Page\n");
		cprintf("myEnv hardlimit %x\n ", myEnv->hardLimit);
		uint32 givenRange = ROUNDUP(size,PAGE_SIZE);
		uint32 required_pages = givenRange/PAGE_SIZE;
		uint32 start_va= (uint32)(((char*)myEnv->hardLimit)+ PAGE_SIZE);
		int curr_pages=0;

		for (uint32 i=start_va; i<= USER_HEAP_MAX; i+= PAGE_SIZE) // Loop through Page Allocator range
		{
			//cprintf("Loop %x\n", i);

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
				va = (void*)start_va;
				uint32 page_index = (start_va - USER_HEAP_START)>>12;
				mark_pages_arr(va, required_pages);
				sys_allocate_user_mem((uint32)va, required_pages*PAGE_SIZE);
				cprintf("start VA = %x,Size = %d",start_va,required_pages);
				break;
			}
		}

		if (!found) // Required consecutive pages not found
		{
			va = NULL;
		}
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
	if(vaRoundDown>=u_hard_limit && vaRoundDown<=USER_HEAP_MAX)
	{
			cprintf("free user pages..\n");
			unmark_pages_arr((void*)vaRoundDown, (uint32)pages);
			sys_free_user_mem(vaRoundDown, pages*PAGE_SIZE);
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
	//panic("smalloc() is not implemented yet...!!");

	void* va;
	int ret;
	bool found = 0;
	uint32 givenRange = ROUNDUP(size,PAGE_SIZE);
	uint32 required_pages = givenRange/PAGE_SIZE;
	uint32 start_va= (uint32)(((char*)myEnv->hardLimit)+ PAGE_SIZE);
	int curr_pages=0;

	for (uint32 i=start_va; i<= USER_HEAP_MAX; i+= PAGE_SIZE) // Loop through Page Allocator range
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
			va = (void*)start_va;
			uint32 page_index = (start_va - USER_HEAP_START)>>12;
			mark_pages_arr(va, required_pages);
			ret = sys_createSharedObject(sharedVarName, size, isWritable, va);
			break;
		}
	}

	if (!found || ret == E_NO_SHARE || ret == E_SHARED_MEM_EXISTS) // Required consecutive pages not found
	{
		va = NULL;
	}
	return va;
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
	uint32 page_index = ((uint32)va - USER_HEAP_START)>>12;
	if (user_pages[page_index]!= 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void mark_pages_arr(void* va, int pages)
{
	uint32 virtualAddress = (uint32)va;
	int pages_size = pages;
	while(pages--)
	{
		uint32 page_index = (virtualAddress - USER_HEAP_START)>>12;
		user_pages[page_index]= pages_size;
		virtualAddress += PAGE_SIZE;
	}

}
void unmark_pages_arr(void* va, int pages)
{
	uint32 virtualAddress = (uint32)va;
	while(pages--)
	{
		uint32 page_index = (virtualAddress - USER_HEAP_START)>>12;
		user_pages[page_index]= 0;
		virtualAddress += PAGE_SIZE;
	}
}
