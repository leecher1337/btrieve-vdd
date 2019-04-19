/*
 * Btrieve DOS Emulation driver WIN32 specific common routines
 *
 * (C) leecher@dose.0wnz.at   05/2019
 *
 */

#include "btrfnc.h"

 /* This helps us reducing the overhead of VirtualAlloc() */
static BTI_VOID_PTR dataBufferGlobal = NULL;
static BTI_BOOL dataBufferGlobalInUse = FALSE;
static CRITICAL_SECTION dataBufferGlobalCS;
static HMODULE hModule = NULL;

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
        dataBufferLocal = VirtualAlloc(NULL, BUF_LEN, MEM_COMMIT, PAGE_READWRITE);
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
        VirtualFree(dataBufferLocal, 0, MEM_RELEASE);
    LeaveCriticalSection(&dataBufferGlobalCS);
}

BTI_BOOL BTRInitialize()
{
    if (!(hModule = LoadLibrary("w3btrv7.dll")))
        return FALSE;
    if (!(dataBufferGlobal = VirtualAlloc(NULL, MAX_DATABUF_SIZE, MEM_COMMIT, PAGE_READWRITE)))
        return FALSE;
    InitializeCriticalSection(&dataBufferGlobalCS);
    BTRCALL = (fpBTRCALL)GetProcAddress(hModule, "BTRCALL");
    BTRCALLID = (fpBTRCALLID)GetProcAddress(hModule, "BTRCALLID");
    return BTRCALL && BTRCALLID;
}

void BTRUnload()
{
    if (dataBufferGlobal)
    {
        VirtualFree(dataBufferGlobal, 0, MEM_RELEASE);
        dataBufferGlobal = NULL;
    }
    DeleteCriticalSection(&dataBufferGlobalCS);
    if (hModule)
    {
        FreeLibrary(hModule);
        hModule = NULL;
    }
}

