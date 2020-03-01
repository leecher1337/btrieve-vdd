/* 
 * (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING.DOSEMU in the DOSEMU distribution
 */

/*
 * This is file plugin_config.h for use within the src/plugin/<name>/
 *
 * It should contain public prototypes for the hook routines
 * (init, close, inte6, ioselect and poll)
 * and config.h - like configuration statements
 *
 */

#define DOS_HELPER_BTRIEVE          0x7B

#ifndef __ASSEMBLER__
extern int BTRInitialize(void);
extern void BTRUnload(void);
extern int BTRIEVE_int7b(void);
#endif
