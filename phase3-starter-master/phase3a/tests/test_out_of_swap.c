/*
 * test_out_of_swap.c
 *
 * Tests what happens when P3PageFaultResolve returns P3_OUT_OF_SWAP.
 */
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <phase3.h>
#include <phase3Int.h>
#include <stdarg.h>
#include <unistd.h>

#include "tester.h"

#define PAGES 4        

static char *vmRegion;
static int  pageSize;
static int lock;
static int cond;
static int writerDone = FALSE;

#ifdef DEBUG
int debugging = 1;
#else
int debugging = 0;
#endif /* DEBUG */

static int passed = FALSE;

static void
Debug(char *fmt, ...)
{
    va_list ap;

    if (debugging) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}


static int
Writer(void *arg)
{
    int     rc;
    PID     pid;

    Sys_GetPid(&pid);
    Debug("Writer (%d) starting.\n", pid);
    for (int i = 0; i < PAGES; i++) {
        char *page = vmRegion + i * pageSize;
        Debug("Writing page %d.\n", i);
        for (int j = 0; j < pageSize; j++) {
            page[j] = i+1;
        }
    }

    // tell the reader we are done.
    rc = Sys_LockAcquire(lock);
    TEST_RC(rc, P1_SUCCESS);
    writerDone = TRUE;
    rc = Sys_CondSignal(cond);
    TEST_RC(rc, P1_SUCCESS);
    rc = Sys_LockRelease(lock);
    TEST_RC(rc, P1_SUCCESS);
    Debug("Writer done.\n", pid);
    return 0;
}
static int
Reader(void *arg)
{
    int     rc;
    PID     pid;

    Sys_GetPid(&pid);
    rc = Sys_LockAcquire(lock);
    TEST_RC(rc, P1_SUCCESS);
    while (!writerDone) {
        rc = Sys_CondWait(cond);
        TEST_RC(rc, P1_SUCCESS);
    }
    rc = Sys_LockRelease(lock);
    TEST_RC(rc, P1_SUCCESS);

    Debug("Reader (%d) starting.\n", pid);
    for (int i = 0; i < PAGES; i++) {
        char *page = vmRegion + i * pageSize;
        Debug("Reading page %d.\n", i);
        for (int j = 0; j < pageSize; j++) {
            TEST(page[j], i+1);
        }
    }
    Debug("Reader done.\n", pid);
    return 0;
}

int
P4_Startup(void *arg)
{
    int     i;
    int     rc;
    PID     pid;
    int     status;

    Debug("P4_Startup starting.\n");
    rc = Sys_VmInit(PAGES, PAGES, PAGES, 1, (void **) &vmRegion, &pageSize);
    TEST_RC(rc, P1_SUCCESS);

    rc = Sys_LockCreate("readylock", &lock);
    TEST_RC(rc, P1_SUCCESS);
    rc = Sys_CondCreate("readycond", lock, &cond);
    TEST_RC(rc, P1_SUCCESS);

    rc = Sys_Spawn("Reader", Reader, NULL, USLOSS_MIN_STACK * 2, 2, &pid);
    TEST_RC(rc, P1_SUCCESS);

    rc = Sys_Spawn("Writer", Writer, NULL, USLOSS_MIN_STACK * 2, 2, &pid);
    TEST_RC(rc, P1_SUCCESS);


    for (i = 0; i < 2; i++) {
        rc = Sys_Wait(&pid, &status);
        TEST_RC(rc, P1_SUCCESS);
        // writer should exit w/ P3_OUT_OF_SWAP
        TEST(status, P3_OUT_OF_SWAP);
        passed = TRUE;
        USLOSS_Halt(0);
    }
    USLOSS_Console("faults: %d\n", P3_vmStats.faults);
    TEST(P3_vmStats.faults, 2 * PAGES);
    TEST(P3_vmStats.newPages, 2 * PAGES);
    Sys_VmShutdown();
    passed = TRUE;
    Debug("P4_Startup done.\n");
    return 0;
}


void test_setup(int argc, char **argv) {
}

void test_cleanup(int argc, char **argv) {
    if (passed) {
        PASSED_FINISH();
    }

}

void finish(int argc, char **argv) {}

int P3PageFaultResolve(int pid, int page, int *frame) { return P3_OUT_OF_SWAP;}
