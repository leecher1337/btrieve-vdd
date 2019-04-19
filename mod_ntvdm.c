/*
 * Btrieve DOS Emulation VDD for NTVDM
 *
 * leecher@dose.0wnz.at   05/2019
 *
 */
#include <vddsvc.h>
#include "../btrfnc.h"

#pragma comment (lib, "ntvdm.lib")

__declspec(dllexport) BOOL __cdecl
VDDInitialize(
    HANDLE   hVdd,
    DWORD    dwReason,
    LPVOID   lpReserved)
{
    return TRUE;
}

__declspec(dllexport) VOID __cdecl
VDDRegisterInit(
    VOID
    )
{
    setCF(0);
}

__declspec(dllexport) VOID __cdecl
VDDDispatch()
{
    XDATA *xdataPtr;
    pmswParmBlock *newParms;
    BTI_ULONG_PTR twoPointersPtr;
    BTI_BYTE keyBufferLocal[MAX_KEY_SIZE+1];
    BTI_BYTE keyBuffer[MAX_KEY_SIZE+1];
    BTI_BYTE posBlock[POS_BLOCK_SIZE];
    BTI_BYTE clientID[16];
    BTI_VOID_PTR clientIdPtr = NULL;
    BTI_VOID_PTR dataBufferPtr = NULL;
    BTI_VOID_PTR FCBPtr = NULL;
    BTI_VOID_PTR posBlockPtr = NULL;
    BTI_ULONG dataLen32;
    BTI_SINT_PTR statusPtr;
    BTI_VOID_PTR dataBufferLocal = NULL;
    BTI_BUFFER_PTR keyBufferPtr = NULL;
    BTI_WORD op_flags;
    BTI_SINT stat = B_NO_ERROR;
    BTI_CHAR ckeynum;
    BTI_BYTE keyLength;

    if (getCX() != 3)
    {
        setCF(1);
        return;
    }
    xdataPtr = (XDATA *)GetVDMPointer((getDS() << 16) | getDX(), sizeof(XDATA), 0);
    newParms = (pmswParmBlock *)xdataPtr;
    if (*((BTI_LONG_PTR) &(newParms->sign)) == *((BTI_LONG_PTR) "PMSW")
        && newParms->functionCode == BTRV_CODE
        && newParms->dataLength == sizeof(pmswParmBlock))
    {
        twoPointersPtr = (BTI_ULONG_PTR)GetVDMPointer((ULONG)newParms->dataPtr, 2 * sizeof(twoPointersPtr), 0);
        xdataPtr = (XDATA *)GetVDMPointer((ULONG)twoPointersPtr[0], sizeof(XDATA), 0);
        clientIdPtr = GetVDMPointer((ULONG)twoPointersPtr[1], sizeof(clientID), 0);
        memcpy(clientID, clientIdPtr, sizeof(clientID));
    }
    op_flags = BtrQueryOps(xdataPtr->FUNCTION);
    keyLength = xdataPtr->KEY_LENGTH;
    ckeynum = xdataPtr->KEY_NUMBER;
    dataLen32 = xdataPtr->BUF_LEN;
    if (!(op_flags & (OP_RETN_DATABUFFER | OP_READ_DATABUFFER)) || !xdataPtr->DATA_BUF)
    {
        if (dataLen32)
            dataLen32 = 0;
    }
    else
    {
        dataBufferPtr = GetVDMPointer((ULONG)xdataPtr->DATA_BUF, xdataPtr->BUF_LEN, 0);
        if (!(dataBufferLocal = BtrGetDataBuffer(xdataPtr->BUF_LEN)))
        {
            setCF(1);
            return;
        }
        if (xdataPtr->BUF_LEN)
            memcpy(dataBufferLocal, dataBufferPtr, xdataPtr->BUF_LEN);
    }
    if (xdataPtr->FCB && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
    {
        FCBPtr = GetVDMPointer((ULONG)xdataPtr->FCB, 38, 0);
        memcpy(posBlock, FCBPtr, 38);
    }
    if (xdataPtr->POS_BLOCK && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
    {
        posBlockPtr = GetVDMPointer((ULONG)xdataPtr->POS_BLOCK, sizeof(posBlock) - 38, 0);
        memcpy(&posBlock[38], posBlockPtr, sizeof(posBlock) - 38);
    }
    if ((xdataPtr->FUNCTION == B_INSERT ||  xdataPtr->FUNCTION == B_UPDATE ||
        xdataPtr->FUNCTION == B_EXT_INSERT) && ckeynum == MAX_KEY_SIZE)
        op_flags &= ~OP_READ_BUFLEN;
    if (xdataPtr->KEY_BUFFER)
    {
        if (xdataPtr->FUNCTION != B_OPEN && xdataPtr->FUNCTION != B_CREATE &&
            xdataPtr->FUNCTION != 50 + B_CREATE)
        {
            if ((op_flags & (OP_RETN_KEYBUFFER | OP_READ_KEYBUFFER)))
            {
                keyBufferPtr = (PBYTE)GetVDMPointer((ULONG)xdataPtr->KEY_BUFFER, keyLength, 0);
                memcpy(keyBuffer, keyBufferPtr, keyLength);
                memcpy(keyBufferLocal, keyBuffer, keyLength);
            }
        }
        else
        {
            BTI_BYTE len = (keyLength == MAX_KEY_SIZE || !keyLength) ? MAX_KEY_SIZE : keyLength + 1;
            keyBufferPtr = GetVDMPointer((ULONG)xdataPtr->KEY_BUFFER, len, 0);
            memcpy(keyBuffer, keyBufferPtr, len);
        }
    }
    statusPtr = (BTI_SINT_PTR)GetVDMPointer((ULONG)xdataPtr->STATUS, 4, 0);
    if (op_flags & OP_RETN_INVALID)
    {
        stat = B_INVALID_FUNCTION;
    }
    else
    {
        if (clientIdPtr)
            stat = BTRCALLID(
                xdataPtr->FUNCTION,
                posBlock,
                dataBufferLocal,
                &dataLen32,
                keyBuffer,
                keyLength,
                ckeynum,
                clientID);
        else
            stat = BTRCALL(
                xdataPtr->FUNCTION,
                posBlock,
                dataBufferLocal,
                &dataLen32,
                keyBuffer,
                keyLength,
                ckeynum);
    }
    op_flags = CopyBackParams(op_flags, xdataPtr, stat, posBlockPtr);
    if (op_flags & OP_RETN_KEYBUFFER)
    {
        register BTI_BYTE i;

        for (i=0; i<keyLength; i++)
            if (keyBufferLocal[i] != keyBuffer[i])
                keyBufferPtr[i] = keyBuffer[i];
    }
    if (op_flags & OP_RETN_DATABUFFER)
        memcpy(dataBufferPtr, dataBufferLocal, dataLen32);
    if (op_flags & OP_RETN_POSBLOCK)
    {
        memcpy(FCBPtr, posBlock, 38);
        memcpy(posBlockPtr, &posBlock[38], sizeof(posBlock) - 38);
    }
    if (op_flags & OP_RETN_KEYNUMBER)
        xdataPtr->KEY_NUMBER = ckeynum;
    *statusPtr = stat;
    BtrFreeDataBuffer(dataBufferLocal);
    if (op_flags & OP_RETN_BUFLEN)
        xdataPtr->BUF_LEN = (BTI_WORD)dataLen32;
    setCF(0);
}
