/*
 * phase3a.c
 *
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3Int.h"

P3_VmStats  P3_vmStats;

static void
FaultHandler(int type, void *arg)
{
    /*******************

    if it's an access fault (USLOSS_MmuGetCause)
        terminate faulting process w/ P3_ACCESS_VIOLATION
    else
        add fault information to a queue of pending faults
        let the pager know that there is a pending fault
        wait until the fault has been handled by the pager
        terminate the process if necessary

    *********************/

}

static int 
Pager(void *arg)
{
    /*******************

    loop until P3_VmShutdown is called
        wait for a fault
        if the process does not have a page table
            call USLOSS_Abort with an error message
        rc = P3PageFaultResolve(pid, page, &frame)
        if rc == P3_OUT_OF_SWAP
            mark the faulting process for termination
        else
            if rc == P3_NOT_IMPLEMENTED
                frame = page
            update PTE in page table to map page to frame
       unblock faulting process

    *********************/
    return 0;
}

int
P3_VmInit(int unused, int pages, int frames, int pagers)
{
    // zero P3_vmStats
    // initialize fault queue, lock, and condition variable
    // call P3FrameInit
    // call P3SwapInit
    // fork pager

    USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;
    return P1_SUCCESS;
}

void
P3_VmShutdown(void)
{
    // cause pager to quit
    P3_PrintStats(&P3_vmStats);
}

USLOSS_PTE *
P3_AllocatePageTable(int pid)
{
    USLOSS_PTE  *table = NULL;
    // create a new page table here
    return table;
}

void
P3_FreePageTable(int pid)
{
    // free the page table here
}

int
P3PageTableGet(PID pid, USLOSS_PTE **table)
{
    *table = NULL;
    return P1_SUCCESS;
}

int P3_Startup(void *arg)
{
    int pid;
    int pid4;
    int status;
    int rc;

    rc = Sys_Spawn("P4_Startup", P4_Startup, NULL,  3 * USLOSS_MIN_STACK, 2, &pid4);
    assert(rc == 0);
    assert(pid4 >= 0);
    rc = Sys_Wait(&pid, &status);
    assert(rc == 0);
    assert(pid == pid4);
    Sys_VmShutdown();
    return 0;
}

void
P3_PrintStats(P3_VmStats *stats)
{
    USLOSS_Console("P3_PrintStats:\n");
    USLOSS_Console("\tpages:\t\t%d\n", stats->pages);
    USLOSS_Console("\tframes:\t\t%d\n", stats->frames);
    USLOSS_Console("\tblocks:\t\t%d\n", stats->blocks);
    USLOSS_Console("\tfreeFrames:\t%d\n", stats->freeFrames);
    USLOSS_Console("\tfreeBlocks:\t%d\n", stats->freeBlocks);
    USLOSS_Console("\tfaults:\t\t%d\n", stats->faults);
    USLOSS_Console("\tnewPages:\t%d\n", stats->newPages);
    USLOSS_Console("\tpageIns:\t%d\n", stats->pageIns);
    USLOSS_Console("\tpageOuts:\t%d\n", stats->pageOuts);
    USLOSS_Console("\treplaced:\t%d\n", stats->replaced);
}

