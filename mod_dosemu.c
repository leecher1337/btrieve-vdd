/*
 * Btrieve DOS Emulation VDD for DOSMEU
 *
 * leecher@dose.0wnz.at   02/2020
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "emu.h"
#include "cpu.h"
#include "memory.h"
#include "doshelpers.h"
#include "dos2linux.h"
#include "btrfnc.h"
#include "../../dosext/mfs/lfn.h"

/* If you need path translation between linux and DOS, you need to
 * set this: (May be useful for local engine and not useful for server engine
 * on windows host..?)
 * You can turn this setting on via desemu config value:
 * btrvdd_transpath=1
 */
int btrvdd_transpath = 0;

extern int build_posix_path(char *dest, const char *src, int allowwildcards); //lfs.c

int BTRIEVE_int7b(void)
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
    int bTransKeybuf = FALSE;

    if (_CX != 3 || !BTRCALL)
    {
        CARRY;
        return 1;
    }
    xdataPtr = (XDATA *)SEGOFF2LINEAR(_DS, _DX);
    newParms = (pmswParmBlock *)xdataPtr;
    if (*((BTI_LONG_PTR) &(newParms->sign)) == *((BTI_LONG_PTR) "PMSW")
        && newParms->functionCode == BTRV_CODE
        && newParms->dataLength == sizeof(pmswParmBlock))
    {
        twoPointersPtr = rFAR_PTR(BTI_ULONG_PTR, (Bit32u)newParms->dataPtr);
        xdataPtr = rFAR_PTR(XDATA *,(Bit32u)twoPointersPtr[0]);
        clientIdPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)twoPointersPtr[1]);
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
        dataBufferPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)xdataPtr->DATA_BUF);
        if (!(dataBufferLocal = BtrGetDataBuffer(xdataPtr->BUF_LEN)))
        {
            CARRY;
            return 1;
        }
        if (xdataPtr->BUF_LEN)
            memcpy(dataBufferLocal, dataBufferPtr, xdataPtr->BUF_LEN);
    }
    if (xdataPtr->FCB && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
    {
        FCBPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)xdataPtr->FCB);
        memcpy(posBlock, FCBPtr, 38);
    }
    if (xdataPtr->POS_BLOCK && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
    {
        posBlockPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)xdataPtr->POS_BLOCK);
        memcpy(&posBlock[38], posBlockPtr, sizeof(posBlock) - 38);
    }
    if ((xdataPtr->FUNCTION == B_INSERT ||  xdataPtr->FUNCTION == B_UPDATE ||
        xdataPtr->FUNCTION == B_EXT_INSERT) && ckeynum == MAX_KEY_SIZE)
        op_flags &= ~OP_READ_BUFLEN;
    if (xdataPtr->KEY_BUFFER)
    {
        char fullname[PATH_MAX];

        if (xdataPtr->FUNCTION != B_OPEN && xdataPtr->FUNCTION != B_CREATE &&
            xdataPtr->FUNCTION != 50 + B_CREATE)
        {
            if ((op_flags & (OP_RETN_KEYBUFFER | OP_READ_KEYBUFFER)))
            {
                keyBufferPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)xdataPtr->KEY_BUFFER);
                memcpy(keyBuffer, keyBufferPtr, keyLength);
                memcpy(keyBufferLocal, keyBuffer, keyLength);
            }
            bTransKeybuf = btrvdd_transpath && xdataPtr->FUNCTION == B_SET_DIR;
        }
        else
        {
            BTI_BYTE len = (keyLength == MAX_KEY_SIZE || !keyLength) ? MAX_KEY_SIZE : keyLength + 1;
            keyBufferPtr = rFAR_PTR(BTI_VOID_PTR,(Bit32u)xdataPtr->KEY_BUFFER);
            memcpy(keyBuffer, keyBufferPtr, len);
            bTransKeybuf = btrvdd_transpath;
        }

#ifdef TRANSL_PATH
        if (bTransKeybuf)
        {
           // Map DOS-directory to physical directory
            build_posix_path(fullname, (const char *)keyBuffer, 0);
            keyLength = snprintf((char*)keyBuffer, sizeof(keyBuffer), "%s", fullname)+1;
            memcpy(keyBufferLocal, keyBuffer, keyLength);
        }
#endif
    }
    statusPtr = rFAR_PTR(BTI_SINT_PTR,(Bit32u)xdataPtr->STATUS);
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

#ifdef TRANSL_PATH
        if (bTransKeybuf && memcmp(keyBufferLocal, keyBuffer, keyLength))
        {
            char fullname[PATH_MAX];
            int drive;

            drive = find_drive((char*)keyBuffer);
            if (drive<0) drive=-drive;
            make_unmake_dos_mangled_path(fullname, (char*)keyBuffer, drive, 0);
            strcpy(keyBuffer, fullname);
        }
#endif
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
    NOCARRY;
    return 1;
}

