/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;
#define false 0
#define true 1
//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//==================
// [1] MAIN HANDLER:
//==================
/*2022*/
uint32 last_eip = 0;
uint32 before_last_eip = 0;
uint32 last_fault_va = 0;
uint32 before_last_fault_va = 0;
int8 num_repeated_fault  = 0;

struct Env* last_faulted_env = NULL;
void fault_handler(struct Trapframe *tf)
{
	/******************************************************/
	// Read processor's CR2 register to find the faulting address
	uint32 fault_va = rcr2();
	//	cprintf("\n************Faulted VA = %x************\n", fault_va);
	//	print_trapframe(tf);
	/******************************************************/

	//If same fault va for 3 times, then panic
	//UPDATE: 3 FAULTS MUST come from the same environment (or the kernel)
	struct Env* cur_env = get_cpu_proc();
	if (last_fault_va == fault_va && last_faulted_env == cur_env)
	{
		num_repeated_fault++ ;
		if (num_repeated_fault == 3)
		{
			print_trapframe(tf);
			panic("Failed to handle fault! fault @ at va = %x from eip = %x causes va (%x) to be faulted for 3 successive times\n", before_last_fault_va, before_last_eip, fault_va);
		}
	}
	else
	{
		before_last_fault_va = last_fault_va;
		before_last_eip = last_eip;
		num_repeated_fault = 0;
	}
	last_eip = (uint32)tf->tf_eip;
	last_fault_va = fault_va ;
	last_faulted_env = cur_env;
	/******************************************************/
	//2017: Check stack overflow for Kernel
	int userTrap = 0;
	if ((tf->tf_cs & 3) == 3) {
		userTrap = 1;
	}
	if (!userTrap)
	{
		struct cpu* c = mycpu();
		//cprintf("trap from KERNEL\n");
		if (cur_env && fault_va >= (uint32)cur_env->kstack && fault_va < (uint32)cur_env->kstack + PAGE_SIZE)
			panic("User Kernel Stack: overflow exception!");
		else if (fault_va >= (uint32)c->stack && fault_va < (uint32)c->stack + PAGE_SIZE)
			panic("Sched Kernel Stack of CPU #%d: overflow exception!", c - CPUS);
#if USE_KHEAP
		if (fault_va >= KERNEL_HEAP_MAX)
			panic("Kernel: heap overflow exception!");
#endif
	}
	//2017: Check stack underflow for User
	else
	{
		//cprintf("trap from USER\n");
		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
			panic("User: stack underflow exception!");
	}

	//get a pointer to the environment that caused the fault at runtime
	//cprintf("curenv = %x\n", curenv);
	struct Env* faulted_env = cur_env;
	if (faulted_env == NULL)
	{
		print_trapframe(tf);
		panic("faulted env == NULL!");
	}
	//check the faulted address, is it a table or not ?
	//If the directory entry of the faulted address is NOT PRESENT then
	if ( (faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT) != PERM_PRESENT)
	{
		// we have a table fault =============================================================
		//		cprintf("[%s] user TABLE fault va %08x\n", curenv->prog_name, fault_va);
		//		print_trapframe(tf);

		faulted_env->tableFaultsCounter ++ ;

		table_fault_handler(faulted_env, fault_va);
	}
	else
	{
		if (userTrap)
		{
			/*============================================================================================*/
			//TODO: [PROJECT'24.MS2 - #08] [2] FAULT HANDLER I - Check for invalid pointers
			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
			//your code is here
            uint32 permissions = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
            if (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX){
            	if ((permissions & (PERM_PRESENT)) == 0 && (permissions & (PERM_AVAILABLE)) == 0){
            		env_exit();
            	}
            }
            else if ((permissions & (PERM_PRESENT))&& (permissions & (PERM_USER)) == 0){
                 env_exit();
            }
            else if ((permissions & (PERM_PRESENT)) && (permissions & (PERM_WRITEABLE)) == 0){
                env_exit();
            }


			/*============================================================================================*/
		}

		/*2022: Check if fault due to Access Rights */
		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
		if (perms & PERM_PRESENT)
			panic("Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n", fault_va) ;
		/*============================================================================================*/


		// we have normal page fault =============================================================
		faulted_env->pageFaultsCounter ++ ;

		//		cprintf("[%08s] user PAGE fault va %08x\n", curenv->prog_name, fault_va);
		//		cprintf("\nPage working set BEFORE fault handler...\n");
		//		env_page_ws_print(curenv);

		if(isBufferingEnabled())
		{
			__page_fault_handler_with_buffering(faulted_env, fault_va);
		}
		else
		{
			//page_fault_handler(faulted_env, fault_va);
			page_fault_handler(faulted_env, fault_va);
		}
		//		cprintf("\nPage working set AFTER fault handler...\n");
		//		env_page_ws_print(curenv);


	}

	/*************************************************************/
	//Refresh the TLB cache
	tlbflush();
	/*************************************************************/
}

//=========================
// [2] TABLE FAULT HANDLER:
//=========================
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//=========================
// [3] PAGE FAULT HANDLER:
//=========================
void page_fault_handler(struct Env * faulted_env, uint32 fault_va)
{
	fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(faulted_env->page_WS_list));
#else
		int iWS =faulted_env->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(faulted_env);
#endif

	if(wsSize < (faulted_env->page_WS_max_size))
	{
		//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		//TODO: [PROJECT'24.MS2 - #09] [2] FAULT HANDLER I - Placement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
		//refer to the project presentation and documentation for details
		uint32 *ptr_table = NULL;
		struct FrameInfo* frame = get_frame_info(faulted_env->env_page_directory, fault_va, &ptr_table);
		int check = allocate_frame(&frame);
		if (check != E_NO_MEM){
			check = map_frame(faulted_env->env_page_directory, frame, fault_va, PERM_USER|PERM_WRITEABLE|PERM_PRESENT);
			if (check != E_NO_MEM){
				check = pf_read_env_page(faulted_env, (void*)fault_va);
				if (check == E_PAGE_NOT_EXIST_IN_PF){
					if (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX){/*Valid address inside heap*/}
					else if (fault_va < USTACKTOP && fault_va >= USTACKBOTTOM){/*Valid address inside stack*/}
					else{
						env_exit();
					}
				}
				struct WorkingSetElement* new_WS = env_page_ws_list_create_element(faulted_env, fault_va);
				if (new_WS != NULL){
					LIST_INSERT_TAIL(&(faulted_env->page_WS_list), new_WS);
					if (faulted_env->page_WS_max_size == LIST_SIZE(&(faulted_env->page_WS_list))){
						faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
					}
					else{
						faulted_env->page_last_WS_element = NULL;
					}
				}
			}
		}
	}
	else
	{
		//refer to the project presentation and documentation for details
		//TODO: [PROJECT'24.MS3] [2] FAULT HANDLER II - Replacement
		//Write your code here, remove the panic and write your code
		//panic("page_fault_handler() Replacement is not implemented yet...!!");

/* sensi was on fire, trying to solve the page replacement problem... but as ideas collided, chaos broke loose! The code? A ticking time bomb of PANIC!!!.
 * Now, the task has fallen to YOU "the chosen one" to restore order and save OS!
 * Here�s the mission: You�ve got a working set of size n and need to implement the Nth-Chance Replacement Algorithm. Your ultimate goal?
 * Replace the victim page, but there�s a catch:
 * Nth Chances: Every page gets N chances before being considered for eviction. You must count up to N accesses for each page.
 * Used Pages: If a page has been accessed recently, reset its status and continue the process.
 * Modified Pages: If a page has been modified, you MUST write its changes to disk before replacing it.
 * The Rules of the Game:
 * LLONG_MIN <= page_WS_max_sweeps < LLONG_MAX -> max number of sweeps!.
 * 0 < N <= page_WS_max_sweeps �> the number of chances each page gets.
 * 0 < n <= page_WS_max_sweeps �> the size of your working set.
 * You�ve got the skills, but can you handle the pressure? Implement it efficiently, dodge the pitfalls, and claim "Winner Winner Chicken Dinner".
 * remember, the OS is counting on you don�t let it down!
 * */
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );

		int N = (page_WS_max_sweeps >= 0) ? page_WS_max_sweeps : -page_WS_max_sweeps;
		bool found = false;
		if (LIST_EMPTY(&(faulted_env->page_WS_list))) {
		    return;
		}

		struct WorkingSetElement *cur_ptr = faulted_env->page_last_WS_element;
		if (cur_ptr == NULL) {
		    cur_ptr = LIST_FIRST(&(faulted_env->page_WS_list));
		}

		for (;!found;) {
		    cur_ptr = faulted_env->page_last_WS_element;
		    if (cur_ptr == NULL) {
		        cur_ptr = LIST_FIRST(&(faulted_env->page_WS_list));
		    }
		    uint32 permissions = pt_get_page_permissions(faulted_env->env_page_directory, cur_ptr->virtual_address);
		    bool used = ((permissions & PERM_USED) == PERM_USED);

		    if (!used) {
		        cur_ptr->sweeps_counter += N;
		    } else {
		        pt_set_page_permissions(faulted_env->env_page_directory, cur_ptr->virtual_address, 0, PERM_USED);
		        cur_ptr->sweeps_counter = 0;
		    }

		    bool modified = ((permissions & PERM_MODIFIED) == PERM_MODIFIED);
		    int checker = (modified && page_WS_max_sweeps<0)? N+1 : N;

		    if (cur_ptr->sweeps_counter >= checker) {
		        uint32 cur_va = cur_ptr->virtual_address;

		        if (modified) {
		            uint32 *table_page = NULL;
		            struct FrameInfo *frame_info = get_frame_info(faulted_env->env_page_directory, cur_va, &table_page);
		            if (frame_info != NULL) {
		                pt_set_page_permissions(faulted_env->env_page_directory, cur_va, 0, PERM_MODIFIED);
		                pf_update_env_page(faulted_env, cur_va, frame_info);
		            }
		        }

		        unmap_frame(faulted_env->env_page_directory, cur_va);
		        struct FrameInfo *frame = NULL;
		        if (allocate_frame(&frame) != 0) {
		        	return;
		        }

		        map_frame(faulted_env->env_page_directory, frame, fault_va, PERM_USER|PERM_WRITEABLE|PERM_PRESENT);
		        pf_read_env_page(faulted_env, (void *)fault_va);

		        cur_ptr->virtual_address = fault_va;
		        cur_ptr->sweeps_counter = 0;
		        found = true;
		    }

		    faulted_env->page_last_WS_element = LIST_NEXT(cur_ptr);
		    if (faulted_env->page_last_WS_element == NULL)
		        faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
		}

	}
}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	//[PROJECT] PAGE FAULT HANDLER WITH BUFFERING
	// your code is here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");
}

