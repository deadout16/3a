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

int pageSize;
int procTerminate;
int lockId;
int condId;
int pagerCondId;
int initCalled;
int termPager;

//TODO: What goes into the fault struct?
typedef struct fault{
    int page;
    int pid;
    struct fault *next;
} fault;

fault *faultQueueHead;
USLOSS_PTE *tables[P1_MAXPROC];

// type is type of interrupt, arg is offset
static void
FaultHandler(int type, void *arg)
{
    int rc;
    fault *newFault;
    /*******************
    if it's an access fault (USLOSS_MmuGetCause)
        terminate faulting process w/ P3_ACCESS_VIOLATION
    else
        add fault information to a queue of pending faults
        let the pager know that there is a pending fault
        wait until the fault has been handled by the pager
        terminate the process if necessary
    *********************/
   // NOTE call P2_Terminate according to post @638
    if(USLOSS_MmuGetCause() == USLOSS_MMU_ACCESS){
        // terminate proccess
        P2_Terminate(P3_ACCESS_VIOLATION);
    }
    else{

        //printf("else\n");
        rc = P1_Lock(lockId);
        //printf("rc = %d\n",rc);
        assert(rc == P1_SUCCESS);
        newFault = (fault *)malloc(sizeof(fault));
        newFault->next = faultQueueHead->next;
        newFault->pid = P1_GetPid();
        newFault->page = (int) arg / pageSize;
        faultQueueHead->next = newFault;
        printf("procId = %d\n",newFault->pid);
        printf("page = %d\n",newFault->page);

        rc = P1_Signal(pagerCondId);
        assert(rc == P1_SUCCESS);
        rc = P1_Wait(condId);
        assert(rc == P1_SUCCESS);
        // TODO: terminate if needed
        if(procTerminate == 1){
            procTerminate = 0;
            rc = P2_Terminate(P3_OUT_OF_SWAP);
            printf("rc = %d\n",rc);
        }
        P3_vmStats.faults ++;
        P3_vmStats.newPages ++;

        rc = P1_Unlock(lockId);
        assert(rc == P1_SUCCESS);
    }
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
    fault *currFault;
    fault *tailFault;
    //USLOSS_PTE currPTE;
    int currPage;
    int currPid;
    int frame;
    int rc;
    

    while(1){
        rc = P1_Lock(lockId);
        assert(rc == P1_SUCCESS);
        rc = P1_Wait(pagerCondId);
        if(termPager == 1){
            printf("Done.\n");
            rc = P1_Unlock(lockId);
        assert(rc == P1_SUCCESS);
            break;
        }
        // get fault from queue
        currFault = faultQueueHead;
        while(currFault->next != NULL){
            printf("curr=%d\n",currFault->pid);
            tailFault = currFault;
            currFault = currFault->next;
        }
        printf("butthole---------------------------------->\n");
        currPid = currFault->pid;
        currPage = currFault->page;
        //currPTE = tables[currPid][currPage];
        // if process does not have page table
        if(tables[currPid] == NULL){
            printf("Process does not have a page table\n");
            USLOSS_Abort(0);    // NOTE: passed in 0
        }
        rc = P3PageFaultResolve(currPid, currPage, &frame);
        if(rc == P3_OUT_OF_SWAP){
            printf("Out of swap\n");
            // TODO: send P3_OUT_OF_SWAP back to handler to terminate
            procTerminate = 1;
        }
        else{
            if(rc == P3_NOT_IMPLEMENTED){
                //printf("not implemented\n");
                frame = currPage;    // NOTE: was page but page didnt exist
            }
            tables[currPid][currPage].frame = frame;
            tables[currPid][currPage].incore = 1;

           
            //printf("freeFrames = %d\n",P3_vmStats.freeFrames);
        }
        // remove from queue
        tailFault->next = NULL;
        printf("tail=%d\n",tailFault->pid);
        tailFault = NULL;
        printf("fram: %d incore: %d\n", tables[currPid][currPage].frame,
            tables[currPid][currPage].incore );

        rc = P1_Signal(condId);
        assert(rc == P1_SUCCESS);

        rc = P1_Unlock(lockId);
        assert(rc == P1_SUCCESS);
    }

    return 0;
}

int
P3_VmInit(int unused, int pages, int frames, int pagers)
{
    int numPages, numFrames, mode;
    void *vmRegion;
    void *pmAddr;
    int rc;
    int i;
    int pid;
    // check pager number
    if(pagers > P3_MAX_PAGERS){
        return P3_INVALID_NUM_PAGERS;
    }
    // zero P3_vmStats
    // TODO: fix values
    P3_vmStats.pages = pages;
    P3_vmStats.frames = frames;
    P3_vmStats.blocks = 0;
    P3_vmStats.freeFrames = frames;
    P3_vmStats.freeBlocks = 0;
    P3_vmStats.faults = 0;
    P3_vmStats.newPages = 0;
    P3_vmStats.pageIns = 0;
    P3_vmStats.pageOuts = 0;
    P3_vmStats.replaced = 0;

    // initialize fault queue, lock, and condition variable
    faultQueueHead = (fault *)malloc(sizeof(fault));
    faultQueueHead->next = NULL;
    faultQueueHead->pid = -1;
    faultQueueHead->page = -1;
    rc = P1_LockCreate("lock", &lockId);
    assert(rc == P1_SUCCESS);
    rc = P1_CondCreate("condition", lockId, &condId);
    assert(rc == P1_SUCCESS);
    rc = P1_CondCreate("pagerCondition", lockId, &pagerCondId);
    assert(rc == P1_SUCCESS);
    // Set tables to NULL
    for(i = 0; i < P1_MAXPROC; i++){
        tables[i] = NULL;
    }
    rc = USLOSS_MmuInit(unused, pages, frames, USLOSS_MMU_MODE_PAGETABLE);
    assert(rc == USLOSS_MMU_OK);

    // call P3FrameInit
    rc = P3FrameInit(pages, frames);
    assert(rc == P1_SUCCESS);
    // call P3SwapInit
    rc = P3SwapInit(pages, frames);
    assert(rc == P1_SUCCESS);

    rc = USLOSS_MmuGetConfig(&vmRegion, &pmAddr, &pageSize, &numPages, &numFrames, &mode);
    assert(rc == USLOSS_MMU_OK);
    // fork pager
    // TODO: might need fix parameters (changed STACK param @627)
    initCalled = 1;
    rc = P1_Fork("pager", Pager, NULL,2*USLOSS_MIN_STACK, P3_PAGER_PRIORITY, &pid);
    assert(rc == P1_SUCCESS);
    
    USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;
    return P1_SUCCESS;
}

// NOTE: moved print inside (instruction say does nothing unless initcalled)
// shut down process edited. @631
void
P3_VmShutdown(void)
{
    // cause pager to quit and waits for it to do so
    if(initCalled == 1){
        int rc;
        rc = P1_Lock(lockId);
        assert(rc == P1_SUCCESS);
        rc = USLOSS_MmuDone();
        P3_PrintStats(&P3_vmStats);
        termPager = 1;
        rc = P1_Signal(pagerCondId);
        assert(rc == P1_SUCCESS);
        rc = P1_Unlock(lockId);
        assert(rc == P1_SUCCESS);
    }
}

USLOSS_PTE *
P3_AllocatePageTable(int pid)
{
    // NOTE: init these (num pages was not declared)
    int numPages, numFrames, mode;
    void *vmRegion;
    void *pmAddr;
    int rc;
    int i;

    USLOSS_PTE  *table = NULL;
    if(initCalled == 1){
        // NOTE: Error USLOSS_MMU_ERR_OFF (mmu is not enabled)
        rc = USLOSS_MmuGetConfig(&vmRegion, &pmAddr, &pageSize, &numPages, &numFrames, &mode);

        assert(rc == USLOSS_MMU_OK);
        table = (USLOSS_PTE *) malloc(sizeof(USLOSS_PTE) * numPages);
        tables[pid] = table;
        for(i = 0; i < numPages; i++){
            table[i].incore = 0;
            table[i].read = 1;
            table[i].write = 1;
            table[i].frame = 0;
        }
    }
    return table;
}

// NOTE: removed null from return. No return value
void
P3_FreePageTable(int pid)
{
    // NOTE: added inits and getter

    int rc;



    USLOSS_PTE  *table = tables[pid];
    if(initCalled != 1){
        return;
    }
    rc = P3FrameFreeAll(pid);
    assert(rc == P1_SUCCESS);
    rc = P3SwapFreeAll(pid);
    assert(rc == P1_SUCCESS);
    // NOTE: removed loop (throwing error on table[i])
    free(table);
    tables[pid] = NULL;
}

int
P3PageTableGet(int pid, USLOSS_PTE **table)
{
    if(pid >= P1_MAXPROC){
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