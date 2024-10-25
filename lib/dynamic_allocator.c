/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#include "../inc/queue.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");
	//Your Code is Here...

	//BEG Block address and value
	uint32* beg_block = (uint32*) daStart;
	*beg_block = 1;

	// END Block address and value
	uint32* limit = (uint32*) (daStart + initSizeOfAllocatedSpace);
	uint32* end_block = --limit;
	*end_block = 1;

	// Block Header
	uint32* block_header = ++beg_block;
	*block_header = initSizeOfAllocatedSpace - 2*sizeof(int);

	// Block Footer
	uint32* block_footer = --end_block;
	*block_footer = initSizeOfAllocatedSpace - 2*sizeof(int);

	// List Initialization
	LIST_INIT(&freeBlocksList);

	// Set location of the 1st block in the list after block header
	uint32* block_pos = ++block_header;
	struct BlockElement* block_ptr = (struct BlockElement*)block_pos;
	LIST_INSERT_HEAD(&freeBlocksList, block_ptr);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("set_block_data is not implemented yet");
	//Your Code is Here...
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...

}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...

}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");
	//Your Code is Here...

	if(is_free_block(va)) // If block is already free
	{
		return;
	}
	if(va == 0) // If block is NULL
	{
		return;
	}

	// Insert block into list (sorted)
	struct BlockElement* free_block = (struct BlockElement*) va;
	list_insertion_sort(free_block);

	// Get address of next and previous blocks
	uint32 block_size = get_block_size(va);
	uint32 next_va = (uint32)va + block_size;
	uint32 prev_va = (uint32)va - block_size;
	bool next_empty, prev_empty;

	// Check if consecutive blocks are empty
	if (is_free_block((uint32*) prev_va))
	{
		prev_empty = 1;
	}
	if (is_free_block((uint32*) next_va))
	{
		next_empty = 1;
	}

	uint32* new_va;
	uint32 new_size;
	struct BlockElement* prev =  LIST_PREV(free_block);
	struct BlockElement* next =  LIST_NEXT(free_block);

	// Case: Merge with left and right blocks
	if (prev_empty && next_empty)
	{
		new_va = (uint32*) prev_va; // Set va of merged block to left block
		new_size = block_size + get_block_size(prev) + get_block_size(next);

		// Remove extra va's since they will be merged into one block
		LIST_REMOVE(&freeBlocksList, next);
		LIST_REMOVE(&freeBlocksList, free_block);
	}

	// Case: Merge with left block
	else if(prev_empty)
	{
		new_va = (uint32*) prev_va; // Set va of merged block to left block
		new_size = block_size + get_block_size(prev);

		// Remove extra va since they will be merged into one block
		LIST_REMOVE(&freeBlocksList, free_block);
	}

	// Case: Merge with right block
	else if(next_empty)
	{
		new_va = (uint32*) va; // Set va of merged block to the new (left) block
		new_size = block_size + get_block_size(next);
		// Remove extra va since they will be merged into one block
		LIST_REMOVE(&freeBlocksList, next);
	}

	// Case: No merging
	else
	{
		new_va = (uint32*) va;
		new_size = block_size;
	}

	// Update block data
	set_block_data(new_va, new_size, 0);
	return;
}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

/********************Helper Functions***************************/

void list_insertion_sort(struct BlockElement* free_block)
{
	if (free_block == 0) // If it's NULL
	{
		return;
	}
	if(LIST_EMPTY(&freeBlocksList)) // Empty list
	{
		//cprintf("%x at head\n", free_block);
		LIST_INSERT_HEAD(&freeBlocksList, free_block);
		return;
	}

	struct BlockElement* current_block;
	LIST_FOREACH(current_block, &freeBlocksList)
	{
		// Check Head
		if (current_block == LIST_FIRST(&freeBlocksList))
		{

			if (free_block < current_block) // If block is smaller than head, block becomes head
			{
				//cprintf("%x at head\n", free_block);
				LIST_INSERT_HEAD(&freeBlocksList, free_block);
				break;
			}
			continue; // Else skip the head (to avoid NULL pointers in the checks)
		}
		// Check Tail
		else if (current_block == LIST_LAST(&freeBlocksList)) // If block is greater than tail, block becomes tail
		{
			if (free_block > current_block)
			{
				//cprintf("%x at tail\n", free_block);
				LIST_INSERT_TAIL(&freeBlocksList, free_block);
				break;
			}
			continue; // Else skip the tail (to avoid NULL pointers in the checks)
		}
		else if (free_block == current_block) // If block already exists
		{
			break;
		}
		else
		{
			// If block should be before current block
			if (free_block < current_block && free_block > LIST_PREV(current_block))
			{
				//cprintf("%x before %x \n", free_block, current_block);
				LIST_INSERT_BEFORE(&freeBlocksList, current_block, free_block);
				break;
			}
			// If block should be after current block
			else if(free_block > current_block && free_block < LIST_NEXT(current_block))
			{
				//cprintf("%x after %x \n", free_block, current_block);
				LIST_INSERT_AFTER(&freeBlocksList, current_block, free_block);
				break;
			}
		}
	}

	return;
}
