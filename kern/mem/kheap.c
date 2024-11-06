#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
	//panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");

	start = (uint32*) daStart;  // Dynamic allocation start address.
	segmentBreak = (uint32*)(daStart + initSizeToAllocate);  // Current Break.
	hardLimit = (uint32*) daLimit;  // The start of the unusable memory.

	uint32 maxRange = daLimit - daStart;  // Maximum size to allocate.

    if (initSizeToAllocate > maxRange)
    {
        panic("Allocation size exceeds the limit.");  // Size exceeded usable memory size.
    }

    uint32 currentAddress = daStart;
    uint32 givenRange = daStart + initSizeToAllocate;

    if (allocateMapFrame(currentAddress,givenRange) == E_NO_MEM)
	{
		panic("No physical memory available for page table.");  // No memory.
	}

    initialize_dynamic_allocator(daStart, initSizeToAllocate);
    return 0;  // Successful initialization.
}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	if (numOfPages == 0) {
		return segmentBreak;
	}
	uint32 available_size = (uint32)hardLimit - (uint32)segmentBreak;
	uint32 available_pages = available_size / PAGE_SIZE;
	uint32 size_added = (numOfPages * PAGE_SIZE);

	void* return_address = segmentBreak;

	//cprintf("number of available pages: %d\n",available_pages);
	//check if number of pages needed exceeds number of pages available
	if (available_pages < numOfPages) {
		return (void *) -1;
	}
	//cprintf("position of previous segment break: %p\n",segmentBreak);
	segmentBreak = (uint32*)((char*)segmentBreak + size_added);

	void* currentAddress = return_address;
	void* givenRange = segmentBreak;
	if (allocateMapFrame((uint32)currentAddress,(uint32)givenRange) == E_NO_MEM)
	{
		return (void *) -1;
	}

	//cprintf("size added : %d\n",size_added);
	uint32* segmentBreak_in_uint32 = (uint32*)segmentBreak;
	uint32* new_end_block = segmentBreak_in_uint32 - 1;
	//cprintf("position of the new end block %p\n",new_end_block);
	//cprintf("position of present segment break: %p\n",segmentBreak);
	*new_end_block = 1;

	set_block_data(return_address,size_added,1);
	//cprintf("size of return address : %d\n",get_block_size(return_address));
	free_block(return_address);
	return return_address;
	// Write your code here, remove the panic and write your code
}
//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator
void* kmalloc(unsigned int size){
	 if(size<=DYN_ALLOC_MAX_BLOCK_SIZE){
			return alloc_block_FF(size);
		}
//	 	cprintf("%d",isKHeapPlacementStrategyFIRSTFIT());
	    size = ROUNDUP(size,PAGE_SIZE);
	    uint32 pagesNumber = size/PAGE_SIZE;
	    int startIndex = -1,tempSize=0;
	    bool checkSegment = 0;
	    for(int i =0; i < 32766;i++){
	    	checkSegment|=pageStatus[i];
	    	if(checkSegment == 0){
	    		if(startIndex==-1)
	    			startIndex = i;
	    		tempSize++;
	    		if(tempSize>=pagesNumber ){
	    			uint32 actualIndx = startIndex*PAGE_SIZE;
	    			void* va = (uint32*)((char*)hardLimit + PAGE_SIZE);
	    			va = (uint32*)((char*)va+actualIndx);
	    			if (allocateMapFrame((uint32) va,(uint32)((char*)va+size)) == E_NO_MEM)
					{
						return NULL;
					}
	    			updatePage(startIndex,pagesNumber);
	    		    return va;
	    		}
	    	}else{
	    		tempSize = 0;
	    		startIndex = -1;
	    		checkSegment = 0;
	    	}
	    }
	    return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

/********************Helper Functions***************************/

int allocateMapFrame(uint32 currentAddress , uint32 limit){
	int x =0;
    while (currentAddress < limit)
    {
    	// Allocation of frames in memory.
    	struct FrameInfo*  frame = NULL;
    	int allocateResult = allocate_frame(&frame);
    	if (allocateResult == E_NO_MEM)
    	{
            panic("No physical memory available to allocate frame.");  // No memory.
    	}

    	// Mapping of frames.
    	allocateResult = map_frame(ptr_page_directory, frame, currentAddress, PERM_WRITEABLE);
    	if (allocateResult == E_NO_MEM)
    	{
            free_frame(frame);
            return E_NO_MEM;
    	}
    	currentAddress += PAGE_SIZE;
    }
    return 0;
}

void updatePage(int index,int size){
	for(int i = index; i < index+size;i++){
		pageStatus[i] = !pageStatus[i];
	}
}




