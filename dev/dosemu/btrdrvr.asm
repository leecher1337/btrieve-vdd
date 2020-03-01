;****************************************************************
;* BTRIEVE DOS DEVICE DRIVER                                    *
;*                                                              *
;* leecher@dose.0wnz.at                                 02/2020 *
;****************************************************************

; Assemble with:
; TASM BTRDRVR.ASM
; LINK BTRDRVR
; EXE2BIN BTRDRVR BTRDRVR.SYS
                                                                       
code     segment
                                                                        
	 assume cs:code,ds:code
                                                                        
         org 0                   ;Programm hat keinen PSP daher Anfang
                                 ;an der Offsetadresse 0

;== Constants ==========================================================
                                                                        
cmd      equ 2                   ;Offset of Command-field  in data block
status   equ 3                   ;Offset of Status-field   in data block
end_adr  equ 14                  ;Offset end-addr. of drvr in data block
b_adr    equ 14                  ;Offset of buffer address in data block
nr       equ 18                  ;Offset of number         in data block

nr_cmd   equ 16                  ;Number of commands that are supported
                                                                        
;-- Header of device driver --------------------------------------------
                                                                        
         dw -1,-1                ;Link to next driver
         dw 1000000000000000b    ;Attributes: character device
         dw offset strat         ;Pointer to strategy routine
         dw offset intr          ;Pointer to interrupt routine
         db "BTRDRV00"           ;Name of the device driver

;-- Init ---------------------------------------------------------------

; init routine goes here, because we have this 33h offset dependency
; for installation check, therefore no need to put routine at the end

init     proc near               ;Init-routine

         mov  ax, 257Bh          ;initialize interrupt vector
         mov  dx, 33h            ;We are required to live at offset 33h!
         int  21h
         mov  word ptr es:[di+end_adr],offset enddrv;set end-address of
         mov  es:[di+end_adr+2],cs                  ;the driver
         xor ax, ax              ;Everything ok

         ret

init     endp

strat    proc far                ;Strategy-Routine
                                                                        
         mov  cs:db_ptr,bx       ;Remember address of data block in
         mov  cs:db_ptr+2,es     ;variable db_ptr
                                                                        
         ret
                                                                        
strat    endp

;-- Dispatcher ----------------------------------------------------------

int 3                            ;Fill byte

; This HAS to be at offset 33h (installation check!) 
disp     proc far                ;Interrupt-Routine for 7B

         push ax                 ;Save registers on stack
         push cx

         mov  cx, 3              ;CX=3 for BTRIEVE driver
         mov  al, 7Bh            ;Function 7B of DOSEMU DOS_HELPER
         int  0e6h               ;Call DOSEMU DOS_HELPER INT

         pop  cx
         pop  ax                 ;Restore registers

         iret

disp     endp

;-- Data of device driver ----------------------------------------------

db_ptr   dw (?),(?)              ;Address of the provided data block

;-- Interrupt routine --------------------------------------------------

intr     proc far                ;Interrupt-Routine
         push ax                 ;Save registers on stack
         push bx
         push di
         push ds

         push cs                 
         pop  ds                 ;Data segment = Code segment

         les  di,dword ptr db_ptr;Address of data block to ES:DI
         mov  bl,es:[di+cmd]     ;Get command code
         cmp  bl, nr_cmd         ;Is it a valid call?
         ja   no_sup             ;No, not supported
         test bl,bl              ;>>Initialize driver<<
         jnz  dummy              ;Nope, just return 0
         push dx
         call init
         pop  dx
         jmp  retdrv
no_sup:  mov  ax,8003h           ;Command not recognized
         jmp  retdrv
dummy:   xor  ax, ax
retdrv:  les  di,dword ptr db_ptr;Address of data block to ES:DI
         or   ax,0100h           ;Set bit that we have finished
         mov  es:[di+status],ax  ;Save to status-field

         pop ds                  ;Restore registers
         pop di
         pop bx
         pop ax

         ret
enddrv:
intr     endp

;=======================================================================
                                                                        
code     ends
         end