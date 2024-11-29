#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#define heap_frames ( KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE
#define FRAME_NUMBER(physical_address) ((physical_address) / PAGE_SIZE)
#define RAM_SIZE 0x100000000
#define TOTAL_FRAMES (RAM_SIZE / PAGE_SIZE)
int phys_to_virt[TOTAL_FRAMES];
int pageStatus[32766];
int it = 0;
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

	init();

	start = (uint32*) daStart;  // Dynamic allocation start address.
	segmentBreak = (uint32*)(daStart + initSizeToAllocate);  // Current Break.
	hardLimit = (uint32*) daLimit;  // The start of the unusable memory.

	statusLimit = ((KERNEL_HEAP_MAX - (daLimit+PAGE_SIZE))/PAGE_SIZE);

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

    freePageStatus(0,32767);

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
	//cprintf("entered sbrk kheap\n");
	if (numOfPages == 0) {
		return segmentBreak;
	}
	uint32 available_size = (uint32)hardLimit - (uint32)segmentBreak;
	uint32 available_pages = available_size / PAGE_SIZE;
	uint32 size_added = (numOfPages * PAGE_SIZE);

	void* return_address = segmentBreak;
	//check if number of pages needed exceeds number of pages available
	if (available_pages < numOfPages) {
		return (void *) -1;
	}
	segmentBreak = (uint32*)((char*)segmentBreak + size_added);

	void* currentAddress = return_address;
	void* givenRange = segmentBreak;
	if (allocateMapFrame((uint32)currentAddress,(uint32)givenRange) == E_NO_MEM)
	{
		return (void *) -1;
	}
	uint32* segmentBreak_in_uint32 = (uint32*)segmentBreak;
	uint32* new_end_block = segmentBreak_in_uint32 - 1;
	*new_end_block = 1;


	//cprintf("out of sbrk kheap\n");
	return return_address;
	// Write your code here, remove the panic and write your code
}
//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator
void* kmalloc(unsigned int size){
	if(isKHeapPlacementStrategyFIRSTFIT() == 0){
		setKHeapPlacementStrategyFIRSTFIT();
	}
	if(size > DYN_ALLOC_MAX_BLOCK_SIZE){
		uint32 required_pages = ROUNDUP(size,PAGE_SIZE)>>12;
		int start=-1,temp = 0;
		for(int i = it; i < statusLimit;i++){
			if(~pageStatus[i]){
				if(~start){
					start = -1;
					temp = 0;
				}
				do{
					i+=(pageStatus[i]);
				}while((i < statusLimit) && (~pageStatus[i]));

			}
			if((i < statusLimit) &&pageStatus[i]==-1){
				if(start==-1){
					start = i;
				}
				temp++;
				if(temp == required_pages){
					uint32 va = ((uint32)hardLimit + PAGE_SIZE*(start+1));
					if(allocateMapFrame(va,va+(PAGE_SIZE*required_pages)) == E_NO_MEM){
						return NULL;
					}
					if(start == it)
						it = start + required_pages;
					pageStatus[start] = required_pages;
					return (void*)va;
				}
			}
		}
		return NULL;
	}
	else
		return alloc_block_FF(size);
}


void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
	uint32 va = (uint32) virtual_address;
	if (va>=KERNEL_HEAP_START && va< (uint32)hardLimit)//HARD_LIMIT should be declared in initialize
	{
		free_block((void*)va);
		return;
	}
	uint32 vaRoundDown = ROUNDDOWN(va,PAGE_SIZE);
	uint32 freedVa = (uint32)((vaRoundDown - (uint32)((char*)hardLimit+PAGE_SIZE)))>>12;
	int pages = pageStatus[freedVa];
	uint32 startIndex = (va - ((uint32)((char*)hardLimit+PAGE_SIZE)))>>12;
	if(startIndex < it)
		it = startIndex;
	pageStatus[startIndex] = -1;
//	freePageStatus(startIndex,pages);
	// Pages Range
	if(va>=((uint32)hardLimit + PAGE_SIZE)&&va<=KERNEL_HEAP_MAX)
	{

		for(int i = 0;i < pages;i++){
			uint32 *ptr_page_table = NULL;
			struct FrameInfo *free_frame = get_frame_info(ptr_page_directory, va, &ptr_page_table);

			if(free_frame == NULL) // Frame is free
			{
				return;
			}
			unmap_frame(ptr_page_directory, va);
			va+=PAGE_SIZE;
	    	phys_to_virt[FRAME_NUMBER(to_physical_address(free_frame))] = -1;
		}
	}

	//Invalid address
	else
	{
		panic("failed to free address %x, illegal address", va);
	}

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	uint32 *temp = NULL;
	struct FrameInfo *ans = get_frame_info(ptr_page_directory, virtual_address, &temp);
	if (ans == NULL) return 0;
	uint32 pg = to_physical_address(ans);
	uint32 off = virtual_address & 0xFFF;
	return pg | off;
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	if ( phys_to_virt[FRAME_NUMBER(physical_address)] == -1) return 0;
	uint32 off = physical_address & 0xFFF;
	return phys_to_virt[FRAME_NUMBER(physical_address)] + off;
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

int allocateMapFrame(uint32 currentAddress , uint32 limit)
{
    while (currentAddress < limit)
    {
    	// Allocation of frames in memory.
    	struct FrameInfo*  frame = NULL;
    	int allocateResult = allocate_frame(&frame);
    	if (allocateResult == E_NO_MEM)
		{
			return E_NO_MEM;
		}

    	uint32 phys_frame = to_physical_address(frame);
    	phys_to_virt[FRAME_NUMBER(phys_frame)] = currentAddress;

    	// Mapping of frames.
    	allocateResult = map_frame(ptr_page_directory, frame, currentAddress, PERM_USER|PERM_WRITEABLE|PERM_PRESENT);
    	if (allocateResult == E_NO_MEM)
    	{
			free_frame(frame);
            return E_NO_MEM;
    	}
    	currentAddress += PAGE_SIZE;
    }
    return 0;
}

void freePageStatus(int index,int size){
	for(int i = index; i < index+size;i++){
		pageStatus[i] = -1;
	}
}

void init(){
	for(int i = 0; i < TOTAL_FRAMES;i++){
		phys_to_virt[i] = -1;
	}
}

