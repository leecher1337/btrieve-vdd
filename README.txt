Pervasive BTRIEVE Virtual Device Driver for DOS Virtual machines (=emulators)

(c) leecher@dose.0wnz.at 2019

What is it?
-----------
This is a virtual device driver to connect INT 7B BTRIEVE service functions 
to any of the following DOS Virtual machines:

 * NTVDM  (Not needed, you get the driver from Pervasive)
 * DOXBOX
 * VDOS

Currently it is only for Windows builds, but adapting it to other environments
that support BTRIEVE shouldn't be too hard.

Motivation
----------
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
wise.

File structure
--------------
Common modules
	BTITYPES.H	- platform-independent data types used by Btrieve
	BTRCONST.H	- Btrieve Constants 
	BTRFNC.H	- Header file for common functions
	BTRFNC.C	- Common functions (platform independent)
	Platform dependent implementation of common modules (select ONE!)
		BTRFNC_WIN32.C	- Windows implementation of platform specific initializers
		BTRFNC_LINUX.C	- Just a stub for Linux, please implement it, if needed!
Emulator specific modules (entry points for emulator, select ONE!)
	MOD_NTVDM.C	- VDD for NTVDM
	MOD_DOSBOX.CPP	- Device driver for DOSBOX
	MOD_VDOS.CPP	- Device driver for VDOS

Compiling
---------
In your makefile:
 * Add BTRFNC.C
 * Depending on platform (WIN32, LINUX, ...), add platform dependent 
   BTRFNC_*.c file
 * Depending on used DOS virtual machine, add the appropriate MOD_*.*

Then according to the appropriate target machine:

NTVDM:
You normally don't want to use this version at all, as there is an official
driver for it anyway. But this driver may have the advantage that it uses a
global scratchpad buffer and doesn't VirtualAlloc/VirtualFree a memory page
on every single Btrieve-call, which generates quite some overhead.
It only works in conjunction with the BTRDRVR.SYS from Pervasive!
Compile as a VDD (link with /entry:"VDDInitialize", required ntvdm.lib is in 
NTDDK). 

DOSBOX:
in gui\sdlmain.cpp in function main, add 
	BTRIEVE_Init();
somewhere after AUTOEXEC_Init();

VDOS:
In vdos.cpp in function vDos_Init(), add
	BTRIEVE_Init();
right before SHELL_Init();

Limitations
-----------
Currently DOS Multiplexer Interrupt calls to BTRIEVE (as documented in 
Ralf Brown's interrupt list) weren't implemented.
Is anybody missing them? I guess not.

Updates
-------
https://github.com/leecher1337/btrieve-vdd

Author
------
mail: leecher@dose.0wnz.at
https://github.com/leecher1337/

License
-------
This Application and Sourcecode may be distributed freely.
The Sourcecode is Licensed in Terms of the GNU GPL v2.
See: http://www.gnu.org/licenses/gpl.html

Have a nice DOS...
