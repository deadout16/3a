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

int lockId;
int condId;

//TODO: What goes into the fault struct?
typedef struct fault{

    fault *next
} fault;

fault *faultQueueHead;
USLOSS_PTE *tables[P1_MAX_PROC];

static void
FaultHandler(int type, void *arg)
{
    int rc;
    /*******************

    if it's an access fault (USLOSS_MmuGetCause)
        terminate faulting process w/ P3_ACCESS_VIOLATION
    else
        add fault information to a queue of pending faults
        let the pager know that there is a pending fault
        wait until the fault has been handled by the pager
        terminate the process if necessary

    *********************/
    if(USLOSS_MmuGetCause() == P3_ACCESS_VIOLATION){
        // terminate proccess
        return P3_OUT_OF_SWAP;
    }
    else{
        rc = P1_Lock(lockId);
        assert(rc == P1_SUCCESS);
        fault *newFault = (fault *)malloc(sizeof(fault));
        newFault->next = faultQueueHead->next;
        // TODO: add fault information
        faultQueueHead->next = newFault;
        rc = P1_Signal(condId);
        assert(rc == P1_SUCCESS);
        rc = P1_Wait(condId);
        assert(rc == P1_SUCCESS);
        // TODO: terminate if needed
    }
}

static int 
Pager(void *arg)
{
    int pageSize, numPages, numFrames, mode;
    void *vmRegion, pmAddr;
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
    while(1){
        rc = P1_Wait(condId);
        assert(rc == P1_SUCCESS);
        // if process does not have page table
        if(tables[P1_GetPid()] == NULL){
            printf("Process does not have a page table\n");
            USLOSS_Abort();
        }
        rc = USLOSS_MmuGetConfig(&vmRegion, &pmAddr, &pageSize, &numPages, &numFrames, &mode);
        assert(rc == USLOSS_MMU_OK);
        // TODO: find page number
        rc = P3PageFaultResolve(P1_GetPid(), );
        if(rc == P3_OUT_OF_SWAP){

        }
        else{
            if(rc == P3_NOT_IMPLEMENTED){
                
            }
        }
    }
    return 0;
}

int
P3_VmInit(int unused, int pages, int frames, int pagers)
{
    int rc;
    int i;
    // check pager number
    if(pagers != P3_MAX_PAGERS){
        return P3_INVALID_NUM_PAGERS;
    }
    // zero P3_vmStats
    // NOTE: Values might not equal zero
    P3_VmStats->pages = pages;
    P3_VmStats->frames = frames;
    P3_VmStats->blocks = 0;
    P3_VmStats->freeFrames = 0;
    P3_VmStats->freeBlocks = 0
    P3_VmStats->faults = 0;
    P3_VmStats->newPages = 0;
    P3_VmStats->pageIns = 0;
    P3_VmStats->pageOuts = 0;
    P3_VmStats->replaced = 0;
    // initialize fault queue, lock, and condition variable
    faultQueueHead = (fault *)malloc(sizeof(fault));
    faultQueueHead->next = NULL;
    rc = P1_CreateLock("lock", &lockId);
    assert(rc == P1_SUCCESS);
    rc = P1_CreateCond("condition", lockId, &condId);
    assert(rc == P1_SUCCESS);
    // Set tables to NULL
    for(i = 0; i < P1_MAX_PROC; i++){
        tables[i] = NULL;
    }
    //TODO: figure out parameters for this function
    // NOTE: numMaps is not used for Page Tables
    USLOSS_MmuInit(0, pages, frames, USLOSS_MMU_MODE_PAGETABLE);
    // call P3FrameInit
    P3FrameInit(pages, frames);
    // call P3SwapInit
    P3SwapInit(pages, frames);
    // fork pager
    // TODO: might need fix parameters
    rc = P1_Fork("pager", Pager, void, P1_MAX_STACKSIZE, P3_PAGER_PRIORITY, P1_GetPid());
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
P3PageTableGet(int pid, USLOSS_PTE **table)
{
    if(pid >= P1_MAX_PROC){
        return P1_INVALID_PID;
    }
    *table = tables[pid];
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

