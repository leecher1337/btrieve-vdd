/*
 * Btrieve DOS Emulation driver Linux specific common routines
 * NB: This is just a stub! If you have BTRIEVE libs for Linux
 * (if such a thing even exists), please fill code accordingly
 * No thread safety in this stub (i.e. via pthreads), you may need
 * to implement it where commented CriticalSection blocks are!
 *
 * (C) leecher@dose.0wnz.at   05/2019
 *
 */

#include "btrfnc.h"
#define _GNU_SOURCE
#include <dlfcn.h>

 /* This helps us reducing the overhead of VirtualAlloc() */
static BTI_VOID_PTR dataBufferGlobal = NULL;
static BTI_BOOL dataBufferGlobalInUse = FALSE;
//static CRITICAL_SECTION dataBufferGlobalCS;
static void *hModule = NULL;

BTI_VOID_PTR BtrGetDataBuffer(BTI_WORD BUF_LEN)
{
    BTI_VOID_PTR dataBufferLocal;

    //EnterCriticalSection(&dataBufferGlobalCS);
    if (!dataBufferGlobalInUse)
    {
        dataBufferLocal = dataBufferGlobal;
        dataBufferGlobalInUse = TRUE;
    }
    else
    {
        dataBufferLocal = malloc(BUF_LEN);
    }
    //LeaveCriticalSection(&dataBufferGlobalCS);
    return dataBufferLocal;
}

void BtrFreeDataBuffer(BTI_VOID_PTR dataBufferLocal)
{
    //EnterCriticalSection(&dataBufferGlobalCS);
    if (dataBufferLocal == dataBufferGlobal)
        dataBufferGlobalInUse = FALSE;
    else if (dataBufferLocal)
        free(dataBufferLocal);
    //LeaveCriticalSection(&dataBufferGlobalCS);
}

BTI_BOOL BTRInitialize()
{
    return FALSE;

    if (!(hModule = dlopen("lib_btrv7.so"))) // Don't know name of lib!
        return FALSE;
    if (!(dataBufferGlobal = malloc(MAX_DATABUF_SIZE)))
        return FALSE;
    //InitializeCriticalSection(&dataBufferGlobalCS);
    BTRCALL = (fpBTRCALL)dlsym(hModule, "BTRCALL");
    BTRCALLID = (fpBTRCALLID)dlsym(hModule, "BTRCALLID");
    return BTRCALL && BTRCALLID;
}

void BTRUnload()
{
    if (dataBufferGlobal)
    {
        free(dataBufferGlobal);
        dataBufferGlobal = NULL;
    }
    //DeleteCriticalSection(&dataBufferGlobalCS);
    if (hModule)
    {
        dlclose(hModule);
        hModule = NULL;
    }
}

