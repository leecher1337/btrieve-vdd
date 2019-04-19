/*
 * Btrieve DOS Emulation module for DOSBox 
 *
 * (C) leecher@dose.0wnz.at   05/2019
 *
 */

#include "regs.h"
#include "callback.h"
#include "dos_inc.h"

#include "btrfnc.h"

static Bitu INT7B_Handler(void)
{
    XDATA xdata;
    pmswParmBlock *newParms;
    BTI_BYTE keyBufferLocal[MAX_KEY_SIZE + 1];
    BTI_BYTE keyBuffer[MAX_KEY_SIZE + 1];
    BTI_BYTE posBlock[POS_BLOCK_SIZE];
    BTI_BYTE clientID[16];
    BTI_WORD posbufLen;
    PhysPt xdataPtr;
    BTI_VOID_PTR clientIdPtr = NULL;
    BTI_ULONG dataLen32;
    BTI_VOID_PTR dataBufferLocal = NULL;
    BTI_WORD op_flags;
    BTI_SINT stat = B_NO_ERROR;
    BTI_CHAR ckeynum;
    BTI_BYTE keyLength;

    xdataPtr = SegPhys(ds) + reg_dx;
    MEM_BlockRead(xdataPtr, &xdata, sizeof(xdata));
    newParms = (pmswParmBlock *)&xdata;
    if (*((BTI_LONG_PTR) &(newParms->sign)) == *((BTI_LONG_PTR) "PMSW")
        && newParms->functionCode == BTRV_CODE
        && newParms->dataLength == sizeof(pmswParmBlock))
    {
        xdataPtr = Real2Phys((RealPt)newParms->dataPtr);
        clientIdPtr = clientID;
        MEM_BlockRead(Real2Phys((RealPt)newParms->dataPtr + 4), clientID, sizeof(clientID));
        MEM_BlockRead(xdataPtr, &xdata, sizeof(xdata));
    }
    op_flags = BtrQueryOps(xdata.FUNCTION);
    keyLength = xdata.KEY_LENGTH;
    ckeynum = xdata.KEY_NUMBER;
    dataLen32 = xdata.BUF_LEN;
    if (!(op_flags & (OP_RETN_DATABUFFER | OP_READ_DATABUFFER)) || !xdata.DATA_BUF)
    {
        if (dataLen32)
            dataLen32 = 0;
    }
    else
    {
        if (!(dataBufferLocal = BtrGetDataBuffer(xdata.BUF_LEN)))
        {
            SETFLAGBIT(CF, 1);
            return CBRET_NONE;
        }
        if (xdata.BUF_LEN)
            MEM_BlockRead(Real2Phys((RealPt)xdata.DATA_BUF), dataBufferLocal, xdata.BUF_LEN);
    }
    if (xdata.FCB && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
        MEM_BlockRead(Real2Phys((RealPt)xdata.FCB), posBlock, 38);
    if (xdata.POS_BLOCK && (op_flags & (OP_RETN_POSBLOCK | OP_READ_POSBLOCK)))
    {
        MEM_BlockRead(Real2Phys((RealPt)xdata.POS_BLOCK), &posBlock[38], sizeof(posBlock) - 38);
        posbufLen = *(BTI_WORD*)&posBlock[38];
    }
    if ((xdata.FUNCTION == B_INSERT || xdata.FUNCTION == B_UPDATE ||
        xdata.FUNCTION == B_EXT_INSERT) && ckeynum == MAX_KEY_SIZE)
        op_flags &= ~OP_READ_BUFLEN;
    if (xdata.KEY_BUFFER)
    {
        if (xdata.FUNCTION != B_OPEN && xdata.FUNCTION != B_CREATE &&
            xdata.FUNCTION != 50 + B_CREATE)
        {
            if ((op_flags & (OP_RETN_KEYBUFFER | OP_READ_KEYBUFFER)))
            {
                MEM_BlockRead(Real2Phys((RealPt)xdata.KEY_BUFFER), keyBuffer, keyLength);
                memcpy(keyBufferLocal, keyBuffer, keyLength);
            }
        }
        else
        {
            BTI_BYTE len = (keyLength == MAX_KEY_SIZE || !keyLength) ? MAX_KEY_SIZE : keyLength + 1;
            MEM_BlockRead(Real2Phys((RealPt)xdata.KEY_BUFFER), keyBuffer, len);
        }
    }
    if (op_flags & OP_RETN_INVALID)
    {
        stat = B_INVALID_FUNCTION;
    }
    else
    {
        if (clientIdPtr)
            stat = BTRCALLID(
                xdata.FUNCTION,
                posBlock,
                dataBufferLocal,
                &dataLen32,
                keyBuffer,
                keyLength,
                ckeynum,
                clientID);
        else
            stat = BTRCALL(
                xdata.FUNCTION,
                posBlock,
                dataBufferLocal,
                &dataLen32,
                keyBuffer,
                keyLength,
                ckeynum);
    }
    op_flags = CopyBackParams(op_flags, &xdata, stat, (BTI_BUFFER_PTR)&posbufLen);
    if (op_flags & OP_RETN_KEYBUFFER)
    {
        register BTI_BYTE i;

        for (i = 0; i < keyLength; i++)
            if (keyBufferLocal[i] != keyBuffer[i])
                mem_writeb(Real2Phys((RealPt)xdata.KEY_BUFFER + i), keyBuffer[i]);
    }
    if (op_flags & OP_RETN_DATABUFFER)
        MEM_BlockWrite(Real2Phys((RealPt)xdata.DATA_BUF), dataBufferLocal, dataLen32);
    if (op_flags & OP_RETN_POSBLOCK)
    {
        MEM_BlockWrite(Real2Phys((RealPt)xdata.FCB), posBlock, 38);
        MEM_BlockWrite(Real2Phys((RealPt)xdata.POS_BLOCK), &posBlock[38], sizeof(posBlock) - 38);
    }
    if (op_flags & OP_RETN_KEYNUMBER)
        xdata.KEY_NUMBER = ckeynum;
    mem_writew(Real2Phys((RealPt)xdata.STATUS), stat);
    BtrFreeDataBuffer(dataBufferLocal);
    if (op_flags & OP_RETN_BUFLEN)
        xdata.BUF_LEN = (BTI_WORD)dataLen32;
    MEM_BlockWrite(xdataPtr, &xdata, sizeof(xdata));
    SETFLAGBIT(CF, 0);
    return CBRET_NONE;
}

static CALLBACK_HandlerObject callback_btrieve;
static BTI_BOOL BTR_Inited = FALSE;

void BTRIEVE_OnReset(Section*)
{
    Bit16u BTRSeg;
    
    if (BTR_Inited || !BTRInitialize()) return;

    /* offset 0x33 is used as an installation check according to
     * Ralf Brown's interrupt list, so we need to allocate an extra segment and
     * write our callback code there like a normal DOS driver
     */
    BTRSeg = DOS_GetMemory(4, "BTRIEVE");
    callback_btrieve.Install(&INT7B_Handler, CB_IRET, PhysMake(BTRSeg, 0x33), "BTRIEVE Int 7B");


    RealSetVec(0x7B, RealMake(BTRSeg, 0x33));
    BTR_Inited = TRUE;
}

void BTRIEVE_ShutDown(Section*)
{
    callback_btrieve.Uninstall();
    BTRUnload();
}

void BTRIEVE_Init(void)
{
    LOG(LOG_MISC, LOG_DEBUG)("Initializing BTRIEVE handler");
    AddExitFunction(AddExitFunctionFuncPair(BTRIEVE_ShutDown), true);
    AddVMEventFunction(VM_EVENT_DOS_INIT_CONFIG_SYS_DONE, AddVMEventFunctionFuncPair(BTRIEVE_OnReset));
}
