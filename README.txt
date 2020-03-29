Pervasive BTRIEVE Virtual Device Driver for DOS Virtual machines (=emulators)

(c) leecher@dose.0wnz.at 2019

===============================================================================
What is it?
===============================================================================
This is a virtual device driver to connect INT 7B BTRIEVE service functions 
to any of the following DOS Virtual machines:

 * NTVDM  (Not needed, you get the driver from Pervasive)
 * DOXBOX (Currently only tested on Windows Dosbox builds)
 * VDOS
 * DOSEMU
 * DOSEMU2

===============================================================================
Motivation
===============================================================================
The motivation behind the project was to use a Btrieve based DOS application
on a 64bit machine, because Microsoft only supports 64bit on Terminal servers
in 2019 and external users are required to be able to access the DOS 
application. NTVDMx64 did the job OK, but due to its slow CCPU emulation, it 
was much slower than V86 mode based original NTVDM on 32bit Windows.

So the next step was to evaluate which way to go. There are several DOS 
emulators out there, like the ones listed above, but none of them had BTRIEVE
support, the VDD by Pervasive was only supplied for the classic NTVDM
(so it worked on NTVDMx64, but as said, performance wasn't good enough).
Thus, to get proper evaluation results, BTRIEVE support had to be implemented.

If you want to know which emulator won: vDOS was the clear winner performance-
wise on Windows.
Overall, the best performance can be accomplished with DOSEMU on Linux in V86
mode. For a guide on how to setup it, refer to:
https://github.com/leecher1337/ntvdmx64/issues/49

===============================================================================
File structure
===============================================================================
Common modules
	BTITYPES.H	- platform-independent data types used by Btrieve
	BTRCONST.H	- Btrieve Constants 
	BTRFNC.H	- Header file for common functions
	BTRFNC.C	- Common functions (platform independent)
	Platform dependent implementation of common modules (select ONE!)
		BTRFNC_WIN32.C	- Windows implementation of platform functions
		BTRFNC_LINUX.C	- Linux   implementation of platform functions
Emulator specific modules (entry points for emulator, select ONE!)
	MOD_NTVDM.C	- VDD for NTVDM
	MOD_DOSBOX.CPP	- Device driver for DOSBOX
	MOD_VDOS.CPP	- Device driver for VDOS
	MOD_DOSEMU.C	- VDD for DOSEMU

DEV/	DOS device drivers required for specific emulator
	DOSEMU
		BTRDRVR.ASM	- Source of DOS driver for DOSEMU
		BTRDRVR.SYS	- Compiled  DOS driver for DOSEMU
	NTVDM
		Just use BTRDRVR from Pervasive

CONFIG/	Configuration files for DOSEMU plugin 

===============================================================================
Compiling
===============================================================================
DOSEMU:
-------
In Dosemu, this works as a plugin:
 * Create directory src/plugin/btrieve-vdd and copy the contents of this
   directory to it
 * The following step can be done automatically with:

     sed -i 's/static int build_posix_path/int build_posix_path/g; s/static int truename/int truename/g' src/dosext/mfs/lfn.c

   or manually:
   Edit src/dosext/mfs/lfn.c and:
   * Remove static keyword from function build_posix_path
     so that it reads:

       int build_posix_path(char *dest, const char *src, int allowwildcards)

   * Remove static keyword from function truename so that it reads:

       int truename(char *dest, const char *src, int allowwildcards)

 * ./default-configure
 * make
 * Copy DEV/dosemu/btrdrvr.sys to your Dosemu DOS directory
   ~/.dosemu/drives/d/dosemu/
 * Load the driver on startup by adding the following line to your
   Dosemu config.sys  (~/.dosemu/drives/c/config.sys):

     devicehigh=d:\dosemu\btrdrvr.sys

 * If you want you DOS path to be translated to a LOCAL unix path, add

     $_btrv_maplocalpath = (on)

   to your ~/.dosemurc
 * If you want a special translation for DOS paths (i.e. when accessing
   a BTRIEVE server), you can use the following command in ~/.dosemurc:

     btrv_mappath(LocalDOSpath, DestinationPath)

   For example:

     btrv_mappath ("C:\data", "\\server\data\")

   If you open C:\data\MYTBL.DAT, it will be translated to 
   \\server\data\MYTBL.DAT


DOSEMU2
-------
Dosemu2 unfortunately lacks the convenient plugin interface from DOSEMU. 
Therefore this is more complicated:

 * Create directory src/plugin/btrieve-vdd and copy the contents of this
   directory to it
 * Edit Makefile of this plugin and change:

    mod_dosemu.c       to   mod_dosemu2.c
    dev/dosemu/%.sys   to   dev/dosemu2/%.sys

 * Edit src/dosext/mfs/lfn.c and:
   * Remove static keyword from function truename so that it reads:

       int truename(char *dest, const char *src, int allowwildcards)

   Note that using it this way is not recommended by dosemu2 maintainers, but
   works at the time of writing. If you want to get the plugin approved as part
   of dosemu2, see dosemu2.c and implement the commented function stubs in 
   there.

 * Due to lacking plugin support, you have to add start and exit functions.
   * In src/emu.c in "int main" add:

     BTRInitialize();

   after timer_interrupt(); call.

   * In src/emu.c in "void __leavedos" add:

     BTRUnload();

   before keyb_close();

   * In src/include/doshelpers.h add:

     #define DOS_HELPER_BTRIEVE          0x7B

   * In src/base/async/int.c in function "int dos_helper" add the following
     lines to the end of the switch() statement:

      case DOS_HELPER_BTRIEVE:
        if (!BTRIEVE_int7b()) return 0;
        break;

 * ./default-configure --enable-plugin=btrieve-vdd
 * make
 * Copy DEV/dosemu2/btrdrvr.sys to your Dosemu DOS directory
   ~/.dosemu/drives/d/dosemu/
 * Load the driver on startup by adding the following line to your
   Dosemu config.sys  (~/.dosemu/drives/c/config.sys):

     devicehigh=d:\dosemu\btrdrvr.sys

   In dosemu2, there is no configuration file support available anymore.
   You need to append the parameters to the commandline of the btrdrvr.sys 
   driver statement.
   The following options are available:

   * If you want you DOS path to be translated to a LOCAL unix path, add

       maplocalpath=1 

     to the devicehigh= line above, i.e.:

       devicehigh=d:\dosemu\btrdrvr.sys maplocalpath=1

   * If you want a special translation for DOS paths (i.e. when accessing
     a BTRIEVE server), you have to write a fiel with mapping inside DOS
     accessible path, i.e. 

       edit c:\map.txt

    The format of the file is LocalDOSpath=DestinationPath, one entry per
    line, i.e.:

      C:\data=\\server\data\

     If you open C:\data\MYTBL.DAT, it will be translated to 
     \\server\data\MYTBL.DAT

    After creating the map file, you have to point the driver to it by
    adding

      mappathfile=[Dos path to file]

    i.e.:

      devicehigh=d:\dosemu\btrdrvr.sys mappathfile=c:\map.txt



All other emulators:
--------------------
In your makefile:
 * Add BTRFNC.C
 * Depending on platform (WIN32, LINUX, ...), add platform dependent 
   BTRFNC_*.c file
 * Depending on used DOS virtual machine, add the appropriate MOD_*.*

Then according to the appropriate target machine:

NTVDM:
------
You normally don't want to use this version at all, as there is an official
driver for it anyway. But this driver may have the advantage that it uses a
global scratchpad buffer and doesn't VirtualAlloc/VirtualFree a memory page
on every single Btrieve-call, which generates quite some overhead.
It only works in conjunction with the BTRDRVR.SYS from Pervasive!
Compile as a VDD (link with /entry:"VDDInitialize", required ntvdm.lib is in 
NTDDK). 

DOSBOX:
-------
in gui\sdlmain.cpp in function main, add 
	BTRIEVE_Init();
somewhere after AUTOEXEC_Init();

VDOS:
-----
In vdos.cpp in function vDos_Init(), add
	BTRIEVE_Init();
right before SHELL_Init();

===============================================================================
Limitations
===============================================================================
Currently DOS Multiplexer Interrupt calls to BTRIEVE (as documented in 
Ralf Brown's interrupt list) weren't implemented.
Is anybody missing them? I guess not.

===============================================================================
Updates
===============================================================================
https://github.com/leecher1337/btrieve-vdd

===============================================================================
Author
===============================================================================
mail: leecher@dose.0wnz.at
https://github.com/leecher1337/

===============================================================================
License
===============================================================================
This Application and Sourcecode may be distributed freely.
The Sourcecode is Licensed in Terms of the GNU GPL v2.
See: http://www.gnu.org/licenses/gpl.html

Have a nice DOS...
