/*
 * Btrieve DOS Emulation common routines
 *
 * (C) leecher@dose.0wnz.at   05/2019
 *
 */

#ifdef WIN32
#define BTI_WIN_32
#endif
#include "BTITYPES.H"
#include "BTRCONST.H"

/* Define BOOL types, if not there... */
#define BTI_BOOL      BTI_INT
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Calling convention selection. BTI_API unfortunately doesn't serve us right for
 * dynamically loaded function pointers. */
#if defined(USING_REGISTERS)
#define BTI_CALL BTI_FAR BTI_PASCAL cdecl
#endif

#if !defined(USING_REGISTERS)  && !defined( BTI_OS2_32 ) && \
       !defined( BTI_WIN_32 )
#define BTI_CALL BTI_FAR BTI_PASCAL
#endif

#if defined( BTI_OS2_32 )
#if defined( BTI_SQL )
#define BTI_CALL APIENTRY16
#else
#define BTI_CALL APIENTRY
#endif
#endif

#if defined( BTI_WIN_32 )
#include <windef.h>
#define BTI_CALL BTI_FAR WINAPI
#endif

 /* Operation flags for input and output definition */
#define OP_READ_DATABUFFER	0x1
#define OP_READ_BUFLEN		0x2
#define OP_READ_KEYNUMBER	0x4
#define OP_READ_POSBLOCK	0x8
#define OP_READ_KEYBUFFER	0x10

#define OP_RETN_DATABUFFER	0x20
#define OP_RETN_BUFLEN		0x40
#define OP_RETN_KEYNUMBER	0x80
#define OP_RETN_POSBLOCK	0x100
#define OP_RETN_KEYBUFFER	0x200
#define OP_RETN_INVALID		0x400

#define OP_RETN_MASK		(OP_RETN_DATABUFFER|OP_RETN_BUFLEN|OP_RETN_KEYNUMBER|\
				             OP_RETN_POSBLOCK|OP_RETN_KEYBUFFER|OP_RETN_INVALID)


#define VARIABLE_ID      0x6176    /* id for variable length records - 'va' */
#define BTRV_CODE        7

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
typedef struct                     /* structure passed to Btrieve */
{
    BTI_VOID_PTR   DATA_BUF;                       /* callers data buffer */
    BTI_WORD       BUF_LEN;                      /* length of data buffer */
    BTI_VOID_PTR   POS_BLOCK;                      /* user position block */
    BTI_VOID_PTR   FCB;                                       /* disk FCB */
    BTI_WORD       FUNCTION;                        /* requested function */
    BTI_VOID_PTR   KEY_BUFFER;                       /* user's key buffer */
    BTI_BYTE       KEY_LENGTH;             /* length of user's key buffer */
    BTI_CHAR       KEY_NUMBER;            /* key of reference for request */
    BTI_SINT_PTR   STATUS;                                 /* status word */
    BTI_SINT       XFACE_ID;                       /* language identifier */
} XDATA;

/* Protected Mode switch parameter block */
typedef struct
{
    BTI_CHAR      sign[4];
    BTI_ULONG     flags;
    BTI_ULONG     functionCode;
    BTI_LONG      pmSwitchStatus;
    BTI_LONG      dataLength;
    BTI_VOID_PTR  dataPtr;
} pmswParmBlock;
#pragma pack()

typedef BTI_SINT (BTI_CALL *fpBTRCALL)(
    BTI_WORD     operation,
    BTI_VOID_PTR posBlock,
    BTI_VOID_PTR dataBuffer,
    BTI_ULONG_PTR dataLength,
    BTI_VOID_PTR keyBuffer,
    BTI_BYTE     keyLength,
    BTI_CHAR     keyNumber);

typedef BTI_SINT (BTI_CALL *fpBTRCALLID)(
    BTI_WORD       operation,
    BTI_VOID_PTR   posBlock,
    BTI_VOID_PTR   dataBuffer,
    BTI_ULONG_PTR   dataLength,
    BTI_VOID_PTR   keyBuffer,
    BTI_BYTE       keyLength,
    BTI_CHAR       keyNumber,
    BTI_BUFFER_PTR clientID);


extern fpBTRCALL BTRCALL;
extern fpBTRCALLID BTRCALLID;

BTI_WORD BtrQueryOps(BTI_WORD op);
BTI_WORD CopyBackParams(BTI_WORD opFlag, XDATA *pXdata, BTI_WORD btrRet, BTI_BUFFER_PTR posBlockPtr);
BTI_VOID_PTR BtrGetDataBuffer(BTI_WORD BUF_LEN);
void BtrFreeDataBuffer(BTI_VOID_PTR dataBufferLocal);
BTI_BOOL BTRInitialize();
void BTRUnload();

#ifdef __cplusplus
}
#endif
