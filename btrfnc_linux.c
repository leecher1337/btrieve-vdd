/*
 * Btrieve DOS Emulation driver Linux specific common routines
 * NB: This is just a stub! If you have BTRIEVE libs for Linux
 * (if such a thing even exists), please fill code accordingly
 *
 * (C) leecher@dose.0wnz.at   05/2019
 *
 */

#include "btrfnc.h"
#define _GNU_SOURCE
#include <dlfcn.h>

// If you do not want to use locking (maybe not needed?), set this:
//#define NOLOCKS

#ifndef NOLOCKS
#include <pthread.h>
#define InitializeCriticalSection(l)   pthread_mutex_init(l, NULL)
#define DeleteCriticalSection(l)       pthread_mutex_destroy(l)
#define EnterCriticalSection(l)        pthread_mutex_lock(l)
#define LeaveCriticalSection(l)        pthread_mutex_unlock(l)
static pthread_mutex_t dataBufferGlobalCS;
#else
#define InitializeCriticalSection(l)
#define DeleteCriticalSection(l)
#define EnterCriticalSection(l)
#define LeaveCriticalSection(l)
#endif

 /* This helps us reducing the overhead of VirtualAlloc() */
static BTI_VOID_PTR dataBufferGlobal = NULL;
static BTI_BOOL dataBufferGlobalInUse = FALSE;
static void *hModule = NULL;

BTI_VOID_PTR BtrGetDataBuffer(BTI_WORD BUF_LEN)
{
    BTI_VOID_PTR dataBufferLocal;

    EnterCriticalSection(&dataBufferGlobalCS);
    if (!dataBufferGlobalInUse)
    {
        dataBufferLocal = dataBufferGlobal;
        dataBufferGlobalInUse = TRUE;
    }
    else
    {
        dataBufferLocal = malloc(BUF_LEN);
    }
    LeaveCriticalSection(&dataBufferGlobalCS);
    return dataBufferLocal;
}

void BtrFreeDataBuffer(BTI_VOID_PTR dataBufferLocal)
{
    EnterCriticalSection(&dataBufferGlobalCS);
    if (dataBufferLocal == dataBufferGlobal)
        dataBufferGlobalInUse = FALSE;
    else if (dataBufferLocal)
        free(dataBufferLocal);
    LeaveCriticalSection(&dataBufferGlobalCS);
}

BTI_BOOL BTRInitialize(void)
{
    if (!(hModule = dlopen("libbtrvif.so", RTLD_NOW | RTLD_DEEPBIND)) && 
        !(hModule = dlopen("libpsqlmif.so", RTLD_NOW | RTLD_DEEPBIND)) &&
        !(hModule = dlopen("/usr/local/psql/lib/libbtrvif.so", RTLD_NOW | RTLD_DEEPBIND)))
        return FALSE;
    if (!(dataBufferGlobal = malloc(MAX_DATABUF_SIZE)))
        return FALSE;
    InitializeCriticalSection(&dataBufferGlobalCS);
    BTRCALL = (fpBTRCALL)dlsym(hModule, "BTRCALL");
    BTRCALLID = (fpBTRCALLID)dlsym(hModule, "BTRCALLID");
    return BTRCALL && BTRCALLID;
}

void BTRUnload(void)
{
    if (dataBufferGlobal)
    {
        free(dataBufferGlobal);
        dataBufferGlobal = NULL;
    }
    DeleteCriticalSection(&dataBufferGlobalCS);
    if (hModule)
    {
        dlclose(hModule);
        hModule = NULL;
    }
}

