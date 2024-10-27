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
		void* prev;
		void* nxt;
		prev = nxt = blk;
		prev = (uint32*)((char*)tmp+get_block_size(blk));
		nxt = (uint32*)((char*)tmp+get_block_size(blk));

		cprintf("(address: %x,size: %d, isFree: %d)\n(address: %x,size: %d, isFree: %d)\n(address: %x,size: %d, isFree: %d)\n",blk, get_block_size(blk), is_free_block(blk)) ;
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

	//panic("set_block_data is not implemented yet");

	// Checking if size greater than 16.
	if (totalSize >= 16)
	{
//		cprintf("h");
		// Checking if the "totalSize" is even or odd.
		if ((totalSize % 2 == 1)) // odd "totalSize"
	    {
	        totalSize++; 		// "totalSize" is even, LSB = 0.
	    }
	    // else, odd no addition needed.

		uint32 maskedLSBSize = totalSize;
//		cprintf("e");
		// Adding the "isAllocated" bit (LSB) to "totalSize".
		if (isAllocated == 1)
	    {
	        totalSize++;		// LSB = 1 -> Allocated.
	    }
	    // else, LSB = 0 -> Free.
//		cprintf("r");
		// setting the block header and footer data.
		va = (uint32*)va;  // original (constant) virtual address.
		uint32* tempVa1 = va;
//		cprintf("e");
//		cprintf("=> VA: %x\n", va);

		uint32* blockHeader = --tempVa1;
		*blockHeader = totalSize;
//
//		cprintf("i");
//		cprintf("=> Header address: %x\n", blockHeader);
//		cprintf("=> H total block size: %u\n", *blockHeader);

		uint32* tempVa2 = va;
//		cprintf("a");
		uint32 metaDataFreeSize = maskedLSBSize - (2 * sizeof(uint32));
//		cprintf("m");
		uint32* blockFooter = (uint32*)((char*)tempVa2 + metaDataFreeSize);
//		cprintf("hehe");
		*blockFooter = totalSize;
//

//		cprintf("=> Footer address: %x\n", blockFooter);
//		cprintf("=> F total block size: %u\n", *blockFooter);
	}
	else
	{
		return;
	}
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
	//panic("alloc_block_FF is not implemented yet");
	//	int sz = LIST_SIZE(&freeBlocksList);
	//	cprintf("%u\n",sz);

	// Add header and footer size to the required "size".
	uint32 totalRequiredSize = (2 * sizeof(int) + size);
	//	cprintf("%u\n",LIST_SIZE(&freeBlocksList));

	// Check if the "totalRequiredSize" is greater than 16.
	if (totalRequiredSize != 0)
	{

		// Checking if the "totalSize" is even or odd.
		if ((totalRequiredSize % 2 == 1)) // odd "totalSize"
		{
			totalRequiredSize++; 		// "totalSize" is even, LSB = 0.
		}

		struct BlockElement* headOfFreeList = LIST_FIRST(&freeBlocksList);
		struct BlockElement* it = headOfFreeList;

		while (it) // it != NULL
		{
			uint32 freeBlockSize = get_block_size(it);
			//cprintf("%u\n",freeBlockSize);

			// In case of large block.
			if (freeBlockSize >= totalRequiredSize)
			{
				// External fragmentation "too large".
				if ((freeBlockSize - totalRequiredSize) >= 16)
				{
					// Alloc block in large free block.
					set_block_data(it, totalRequiredSize, 1);

					// split free block from alloc block in large free block.
					uint32 offset = totalRequiredSize;
					uint32* vaOfNewSplitBlock = (uint32*)((char*)it + offset);
					// Delete large free block.
					LIST_REMOVE(&freeBlocksList, (struct BlockElement*)it);
					uint32 sizeOfNewSplitBlock = freeBlockSize - totalRequiredSize;
					set_block_data(vaOfNewSplitBlock, sizeOfNewSplitBlock, 0);

					// call sort, and insert free splitted block.
					list_insertion_sort((struct BlockElement*)vaOfNewSplitBlock);
				}
				// internal fragmentation "not large enough to split".
				else if ((freeBlockSize - totalRequiredSize) < 16)
				{
					//cprintf("%u\n",(freeBlockSize-totalRequiredSize));
					set_block_data(it, freeBlockSize, 1);
					LIST_REMOVE(&freeBlocksList, (struct BlockElement*)it);
				}
				return it;
			}
			// No space found.
			if (freeBlockSize < totalRequiredSize)
			{
				cprintf("hello pookie i will haunt u  \n");
				it = LIST_NEXT((struct BlockElement*)it);
			}
		}
		void* sbrkk = sbrk(0);
		return NULL;
	}
	else
	{
		return NULL;
	}
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");

	// Add header and footer size to the required "size".
	uint32 totalRequiredSize = (2 * sizeof(int) + size);
	//	cprintf("%u\n",LIST_SIZE(&freeBlocksList));

	// Check if the "totalRequiredSize" is greater than 16.
	if (totalRequiredSize != 0)
	{
		// Checking if the "totalSize" is even or odd.
		if ((totalRequiredSize % 2 == 1)) // odd "totalSize"
		{
			totalRequiredSize++; 		// "totalSize" is even, LSB = 0.
		}

		struct BlockElement* headOfFreeList = LIST_FIRST(&freeBlocksList);
		struct BlockElement* it = headOfFreeList;
		struct BlockElement* bestFit = NULL;
		uint32 freeBlockSize;
		uint32 bestSize;

		while (it) // it != NULL
		{
			freeBlockSize = get_block_size(it);
		    //cprintf("%u\n",freeBlockSize);

			// Space found.
			if (freeBlockSize >= totalRequiredSize)
			{
				// Better space found.
				if (bestSize > freeBlockSize || bestFit == NULL)
				{
					bestFit = it;
					bestSize = get_block_size(bestFit);
				}

				it = LIST_NEXT((struct BlockElement*)it);
			}

			// No space found.
			if (freeBlockSize < totalRequiredSize)
			{
				//cprintf("hello pookie i will haunt u  \n");
				it = LIST_NEXT((struct BlockElement*)it);
			}
		}

		if(bestFit == NULL)
		{
			//cprintf("NULL 1  \n");
			void* sbrkk = sbrk(0);
			return NULL;
		}
		else
		{
			// External fragmentation "too large".
			if ((bestSize - totalRequiredSize) >= 16)
			{
				//cprintf("External  \n");
				// Alloc block in large free block.
				set_block_data(bestFit, totalRequiredSize, 1);

				// split free block from alloc block in large free block.
				uint32 offset = totalRequiredSize;
				uint32* vaOfNewSplitBlock = (uint32*)((char*)bestFit + offset);
				// Delete large free block.
				LIST_REMOVE(&freeBlocksList, (struct BlockElement*)bestFit);
				uint32 sizeOfNewSplitBlock = bestSize - totalRequiredSize;
				set_block_data(vaOfNewSplitBlock, sizeOfNewSplitBlock, 0);

				// call sort, and insert free splitted block.
				list_insertion_sort((struct BlockElement*)vaOfNewSplitBlock);
			}
			// internal fragmentation "not large enough to split".
			else if ((bestSize - totalRequiredSize) < 16)
			{
				//cprintf("Internal  \n");
				//cprintf("%u\n",(freeBlockSize-totalRequiredSize));
				set_block_data(bestFit, bestSize, 1);
				LIST_REMOVE(&freeBlocksList, (struct BlockElement*)bestFit);
			}
			return bestFit;
		}
	}
	else
	{
		//cprintf("NULL 2  \n");
		return NULL;
	}
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
	if(va == NULL) // If block is NULL
	{
		return;
	}

	// Insert block into list (sorted)
	struct BlockElement* free_block = (struct BlockElement*) va;

	list_insertion_sort(free_block);
//	cprintf("wakka wakkaaa wa2 wa2%u\n",LIST_SIZE(&freeBlocksList));

    // Get address of next and previous blocks
    uint32 block_size = get_block_size(va);
    uint32 next_va = (uint32)va + block_size;
    uint32 prev_footer = (uint32)va - 2*sizeof(int);

    uint8 is_prev_empty = (~(*((uint32*)prev_footer)) & 0x1);
	bool next_empty=0, prev_empty=0;
	// Check if consecutive blocks are empty
	if (LIST_PREV(free_block) != NULL && is_prev_empty)
	    {
	        prev_empty = 1;
	    }
	if (LIST_NEXT(free_block) !=NULL && is_free_block((uint32*)next_va))
	    {
	        next_empty = 1;
	    }
//	cprintf("%u\n",next_empty);
//	cprintf("%u\n",prev_empty);
	uint32* new_va;
	uint32 new_size;
	struct BlockElement* prev =  LIST_PREV(free_block);
	struct BlockElement* next =  LIST_NEXT(free_block);
//		cprintf("prev1%x\n",prev);
//		cprintf("prev2%x\n",prev);
	// Case: Merge with left and right blocks
	if (prev_empty && next_empty)
	{


		new_va = (uint32*)prev; // Set va of merged block to left block
		new_size = block_size + get_block_size(prev) + get_block_size(next);

		// Remove extra va's since they will be merged into one block
		LIST_REMOVE(&freeBlocksList, next);
		LIST_REMOVE(&freeBlocksList, free_block);
	}

	// Case: Merge with left block
	else if(prev_empty)
	{
		new_va = (uint32*)prev; // Set va of merged block to left block
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
	//	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
	if(va==NULL && new_size){
		return alloc_block_FF(new_size);
	}
	if(va==NULL && (new_size==0)){
		return NULL;
	}
	if((new_size==0)){
		free_block(va);
		return NULL;
	}
	if(new_size%2==1){
		++new_size;
	}
	if(new_size<8){
		new_size=8;
	}
	new_size+=(2*sizeof(uint32));
	uint32 oldSize = get_block_size(va);
	int neededSize = new_size - oldSize;
	cprintf("%d\n",neededSize);
	bool isFree = is_free_block(va);
	uint32* nextHeader = (uint32*)((char*)va + oldSize - sizeof(uint32));
	void* nextVa = (uint32*)((char*)va + oldSize);
	uint32 nextBlockSize = (*nextHeader) & ~(0x1);
	bool isNextBlockFree = (~(*nextHeader) & 0x1);
	if(new_size==oldSize){
		return va;
	}
	if(isFree==1){
		if(oldSize>=new_size){
			neededSize*=-1;
			if(neededSize>=16){
				set_block_data(va,new_size,1);
				void* newVa = (uint32*)((char*)va + new_size);
				LIST_REMOVE(&freeBlocksList, (struct BlockElement*)va);
				set_block_data(newVa,neededSize,0);
				list_insertion_sort((struct BlockElement*)newVa);
				return va;
			}
			set_block_data(va,oldSize,1);
			LIST_REMOVE(&freeBlocksList,(struct BlockElement*) va);
			return va;
		}
		return alloc_block_FF(new_size);
	}
	if(neededSize>0){
		if((isNextBlockFree && nextBlockSize >= neededSize)){
			uint32 newFreeSize;
			if((nextBlockSize - neededSize)>=16){
				set_block_data(va,new_size,1);
				newFreeSize = (nextBlockSize - neededSize);
				void* newVa = (uint32*)((char*)va + new_size);
				LIST_REMOVE(&freeBlocksList, (struct BlockElement*)nextVa);
				set_block_data(newVa,newFreeSize,0);
				list_insertion_sort((struct BlockElement*)newVa);
				return va;
			}

			set_block_data(va,nextBlockSize+oldSize,1);
			LIST_REMOVE(&freeBlocksList, (struct BlockElement*)nextVa);
			return va;

		}
		void* tmpVa = alloc_block_FF(new_size);
		if(tmpVa!=NULL){
			memcpy(tmpVa,va,oldSize);
		}
		free_block(va);
		return tmpVa;
	}else{
		neededSize*=-1;
		if(neededSize>=16){
			set_block_data(va,new_size,1);
			uint32* newFreeVa = (uint32*)((char*)va + new_size);
			set_block_data(newFreeVa,neededSize,1);
			free_block(newFreeVa);
			return va;
		}
		if(isNextBlockFree==1){
			uint32* newFreeVa = (uint32*)((char*)va + new_size);
			set_block_data(newFreeVa,nextBlockSize + neededSize,0);
			LIST_REMOVE(&freeBlocksList, (struct BlockElement*)nextVa);
			list_insertion_sort((struct BlockElement*)newFreeVa);
			return va;
		}
		return va;
	}
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
	bool gg = 0;
	if (free_block == NULL) // If it's NULL
	{
		return;
	}
	if(LIST_EMPTY(&freeBlocksList)) // Empty list
	{
//		cprintf("%x at head\n", free_block);
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
//				cprintf("we want our parks in peace %u\n",LIST_SIZE(&freeBlocksList));
//				cprintf("%x at head\n", free_block);
				LIST_INSERT_BEFORE(&freeBlocksList, current_block, free_block);
				gg=1;
				break;
			}
			continue; // Else skip the head (to avoid NULL pointers in the checks)
		}
		// Check Tail
		else if (current_block == LIST_LAST(&freeBlocksList)) // If block is greater than tail, block becomes tail
		{
			if (free_block > current_block)
			{
				cprintf("%x at tail\n", free_block);
				LIST_INSERT_AFTER(&freeBlocksList, current_block, free_block);
				gg=1;
				break;
			}
			continue; // Else skip the tail (to avoid NULL pointers in the checks)
		}
		else if (free_block == current_block) // If block already exists
		{
			gg=1;
			break;
		}
		else
		{
			// If block should be before current block
			if (free_block < current_block && free_block > LIST_PREV(current_block))
			{
				cprintf("%x before %x \n", free_block, current_block);
				LIST_INSERT_BEFORE(&freeBlocksList, current_block, free_block);
				gg=1;
				break;
			}
			// If block should be after current block
			else if(free_block > current_block && free_block < LIST_NEXT(current_block))
			{
				cprintf("%x after %x \n", free_block, current_block);
				LIST_INSERT_AFTER(&freeBlocksList, current_block, free_block);
				gg=1;
				break;
			}
		}
	}
	if(!gg){
		LIST_INSERT_TAIL(&freeBlocksList,free_block);
	}

	return;
}
