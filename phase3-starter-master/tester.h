#ifndef _TESTER_H_
#define _TESTER_H_

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <phase1.h>
#include <fcntl.h>

#ifndef PHASE1A
static char *states[] = {"Free", "Run", "Ready", "Quit", "Block", "Join"};
#endif

static char *errors[] = {
    "Success.",
    "Too many processes/contexts.",
    "Invalid stack.",
    "Invalid priority.",
    "Invalid tag.",
    "Process has no children.",
    "No children have quit.",
    "Too many locks.",
    "Too many condition variables.",
    "Name is NULL.",
    "Duplicate name.",
    "Invalid LID.",
    "Invalid VID.",
    "Lock not held by calling process.",
    "Processes are blocked on the lock or condition variable.",
    "Invalid PID.",
    "Invalid CID.",
    "Invalid state.",
    "Invalid device type.",
    "Invalid device unit.",
    "Wait aborted.",
    "Child has quit.",
    "Name is too long.",
    "Context is in-use.",
    "Lock is held.",
    "Invalid system call.",
    "Invalid number of seconds.",
    "Invalid first sector.",
    "Invalid number of sectors.",
    "Address is NULL.",
    "Process was not spawned.",
    "Invalid number of pages.",
    "Invalid number of frames.",
    "Invalid number of pagers.",
    "Already initialized.",
    "Already has a page table.",
    "Frame not mapped.",
    "Page not found.",
    "Out of swap.",
    "Not initialized.",
    "Out of pages.",
    "Invalid frame.",
    "Invalid page.",
    "Access violation.",
    "Not implemented."
};

static int numCodes = sizeof(errors) / sizeof(char *);

static char *
ErrorCodeToString(int code)
{
    code = -code;
    if ((code < 0) || (code >= numCodes)) {
        return "Invalid error code";
    } else {
        return errors[code];
    }
}

#ifndef PHASE1A

static void
DumpProcesses(void)
{
    int rc;
    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;

    USLOSS_Console("%10s %3s %8s %3s %4s %3s %3s %3s %s\n", "Name", "PID", "State", "Pri", "CPU", "LID", "VID", "Par", "Children");
    for (int i = 0; i < P1_MAXPROC; i++) {
        P1_ProcInfo info;
        memset(&info, '\0', sizeof(info));
        if (mode == 0) {
            // user mode
            rc = Sys_GetProcInfo(i, &info);
        } else {
            rc = P1_GetProcInfo(i, &info);
        }
        if ((rc == P1_SUCCESS) && (info.state != P1_STATE_FREE)) {
            USLOSS_Console("%10s %3d %8s %3d %4d %3d %3d %3d ", info.name, i, states[info.state], info.priority, info.cpu, info.lid, info.vid, info.parent);
            for (int j = 0; j < info.numChildren; j++) {
              USLOSS_Console("%d ", info.children[j]);
            }
            USLOSS_Console("\n");
        }
    }
}

#endif

static char *
MakeName(char *prefix, int suffix)
{
    static char name[P1_MAXNAME];
    snprintf(name, sizeof(name), "%s%d", prefix, suffix);
    return name;
}

static void
DeleteDisk(int unit)
{
    char path[30];
    snprintf(path, sizeof(path), "disk%d", unit);
    unlink(path);
}

static void
DeleteAllDisks(void)
{
    for (int i = 0; i < USLOSS_DISK_UNITS; i++) {
        DeleteDisk(i);
    }
}

static int
OpenDisk(int unit)
{
    char path[30];
    snprintf(path, sizeof(path), "disk%d", unit);
    return open(path, O_RDWR);
}

#define PASSED_MSG() { \
    USLOSS_Console("TEST PASSED.\n"); \
}

#define PASSED() { \
    PASSED_MSG(); \
    USLOSS_Halt(0); \
}

#define FAILED_MSG(val, expected) { \
    USLOSS_Console("%s:%d: %d != %d.\n", __FUNCTION__, __LINE__, (val), (expected)); \
    USLOSS_Console("TEST FAILED.\n"); \
}

#define FAILED(val, expected) { \
    FAILED_MSG((val), (expected)); \
    USLOSS_Halt(1); \
}

#define TEST_MSG(val, expected) { \
    if ((val) != (expected)) { \
        FAILED_MSG((val), (expected)); \
    } \
}

#define TEST_EQ(val, expected) { \
    if ((val) != (expected)) { \
        FAILED((val), (expected)); \
    } \
}

#define TEST_NE(val, expected) { \
    if ((val) == (expected)) { \
        FAILED((val), (expected)); \
    } \
}

// for backwards compatibility
#define TEST(val, expected) TEST_EQ((val), (expected))

#define TEST_RC(rc, expected) { \
    if ((rc) != (expected)) { \
        USLOSS_Console("%s:%d: Incorrect return code %d: %s\n", __FUNCTION__, __LINE__, (rc), \
                       ErrorCodeToString(rc)); \
        USLOSS_Halt(1); \
    } \
}



// use these in the finish function because you can't call Halt from finish

#define PASSED_FINISH() { \
    PASSED_MSG(); \
    return; \
}

#define FAILED_FINISH(val, expected) { \
    FAILED_MSG((val), (expected)); \
    return; \
}

#define TEST_FINISH(val, expected) { \
    if ((val) != (expected)) { \
        FAILED_MSG((val), (expected)); \
        return; \
    } \
}

#endif

