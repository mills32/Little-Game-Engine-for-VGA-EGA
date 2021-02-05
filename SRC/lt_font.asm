; LOADFONT 1.02 - loads a font on a VGA.
; Copyright (c) 1999 Stephen Kitt
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
; To contact the author, email steve@tardis.ed.ac.uk

LOCALS

.model large

.data

align 2

fullstop db '.';
crlf     db 00Dh, 00Ah, '$'
errormsg db 'Error: $'
nofont   db 'no font file specified$'
novga    db 'no VGA present$'
nomem    db 'not enough memory (18KB required)$'
doserr01 db 'invalid function number$'
doserr02 db 'file not found$'
doserr03 db 'path not found$'
doserr04 db 'too many open files$'
doserr05 db 'access denied$'
doserr0C db 'access code invalid$'
doserr56 db 'invalid password$'
doserrun db 'unknown DOS error$'
doserrls db 001h, 002h, 003h, 004h, 005h, 00Ch, 056h
doserrs  equ 007h
doserrpt dw doserr01, doserr02, doserr03, doserr04, doserr05, doserr0C, doserr56


handle   dw   00000h              ; Font file handle

fontedit db 'fontedit 1.0 file'

;cafecom                      ; Signature combined with message
fcafecom db 'CAFE!exec'           ; This needs modified before printing

;pcmagcom                           ; Signature combined with message
fpcmag   db 'PC Magazine'

_length   dw 1                  ; Font file length
font     dw 1                   ; Pointer to font data
height   db 1                   ; Cell height
columns  db 1                   ; Screen columns
lines    db 1                   ; Screen lines
file     db 16384 DUP(?)        ; File
filen	db 'PC Magazine'

.code

align 2

_wait macro arg
    jmp %%end
    nop
%%end:
endm

_x_load_font proc	far
ARG filename:word
        ;cmp sp, lfend+256        ; Check available memory
        ;jl memok
        ;mov ax, word ptr [nomem]
		;mov dx, ax
        ;jmp error
memok:
		mov ax, 01A00h           ; Check for VGA
         int 010h
         cmp al, 01Ah
         jne vgaerr
         cmp bl, 7
         jl vgaerr
         cmp bl, 8
         jg vgaerr
         ; Following ten lines based on code by Tylisha C. Andersen, published
         ; in Tennie Remmel's Programming Tips & Tricks issue 7
         
        
         mov ah,03Dh
		 mov al,000h			;0 read only
		 mov dx,
         mov [handle], ax         ; Store the file handle
         mov bx, ax
         mov ah, 03Fh            ; Read file
         mov cx, 16384            ; Up to 16KB
         mov ax, word ptr file
		 mov dx,ax
         int 021h
         jc doserr
         mov [_length], ax         ; Store the file's length
         call decode              ; Determine the file format
enddecode:
         jmp setfont

vgaerr:   
		mov dx, word ptr[novga]            ; No VGA present
         jmp error

ncl:      
		mov dx, word ptr[nofont]          ; No font file specified
error:
		push dx
         mov dx, word ptr[errormsg]
         mov ah, 009h
         int 021h
         pop dx
         int 021h
         mov dx, word ptr[fullstop]
         int 021h
         jmp _end

doserr:   
		mov di, word ptr[doserrls]
         mov cx, doserrs
         repnz scasb
         jcxz dosun
         sub di, word ptr[doserrls+1]
         mov bp, di
         shl bp, 1
         mov dx, [bp+doserrpt]
         jmp error
dosun:
		mov dx, word ptr[doserrun]
         jmp error

; Set font.
setfont:
    ; Set VGA up for font modification
         mov dx, 003C4h
         mov ax, 00402h           ; Write enable display memory plane 2
         out dx, ax
         wait
         mov ax, 00704h           ; Sequential access to all text mode memory
         out dx, ax
         wait
         mov dl, 0CEh             ; DX = 0x03CE
         mov ah, 004h             ; AX = 0x0404
         out dx, ax
         wait
         mov ax, 00005h
         out dx, ax
         wait
         mov al, 006h             ; AX = 0x0006
         out dx, ax
    ; Load the font
         xor cx, cx
         mov cl, [height]
         mov si, [font]
         mov ax, 0A000h
         mov es, ax
         xor di, di               ; ES:DI points to 0xA000:0x0000
         mov dx, 00100h           ; 256 characters
         mov bx, 32
         sub bx, cx               ; BX stores the difference
         push cx                  ; Remember cell height
_loop:   
		pop cx
         push cx
         rep movsb                ; No assumptions on cell height
         add di, bx
         dec dx
         jnz _loop
         pop cx
    ; Restore video settings
         mov dx, 003C4h
         mov ax, 00302h
         out dx, ax
         wait
         mov al, 004h
         out dx, ax
         wait
         mov dl, 0CEh
         xor ah, ah
         out dx, ax
         wait
         mov ax, 01005h
         out dx, ax
         wait
         mov ax, 00E06h
         out dx, ax

; Store new line information if necessary.
setbios:
		mov ax, 040h
         mov es, ax
         mov cl, [es:00084h]      ; Old screen lines
         inc cl
         mov [lines], cl
         mov cl, [es:0004Ah]      ; Old screen columns
         mov [columns], cl
         mov bx, [es:00085h]      ; Old character height
         cmp bl, [height]
         je _end                  ; If same as new, leave untouched
         mov dx, 003D4h          ; Get vertical display end
         mov al, 012h             ; i.e. scan lines in screen
         out dx, al
         inc dx
         in al, dx
         mov bl, al               ; Main part in BL
         dec dx
         mov al, 007h
         out dx, al
         inc dx
         in al, dx
         mov bh, al               ; Supplementary part in BH
         xor ax, ax
         mov al, bl
         test bh, 002h
         jz cont1
         add ah, 1
cont1:   test bh, 040h
         jz cont2
         add ah, 2
cont2:   inc ax                   ; Scan lines in AX
         mov bl, [height]         ; New character height
         div bl
         mov cl, al               ; New screen rows
         mov [lines], cl
         dec cl                   ; Adjust for BIOS storage
         mov [es:00084h], cl      ; Update BIOS memory locations
         mov [es:00085h], bx
         dec bx                   ; Adjust for VGA storage
         mov dx, 003D4h           ; Get current value for 0x03D4/0x09
         mov al, 009h
         out dx, al
         inc dx
         in al, dx
         and al, 0E0h             ; Clear low five bits
         or al, bl                ; Set new character height
         dec dx                   ; Store...
         push ax
         mov al, 009h
         out dx, al
         pop ax
         inc dx
         out dx, al
_end:
   
		mov bx, [handle]         ; Quit, closing files if necessary
         or bx, bx
         jz nofiles
         mov ah, 03Eh
         int 021h
nofiles: 
		ret
_x_load_font endp


; Decode a loaded file's format
; This routine should remain separate, since multiple ret's are smaller than
; multiple jmp's.
decode:
		cmp word ptr [file], 055AAh
         je cafe
         cld
         mov di, word ptr [fcafecom]
         mov si, word ptr [file + 3]
         mov cx, 9
         repz cmpsb
         jcxz cafecom
         mov di, word ptr [fontedit]
         mov si, word ptr [file]
         mov cx, 17
         repz cmpsb
         jcxz _fontedit
         mov di, word ptr [fpcmag]
         mov si, word ptr [file + 10]
         mov cx, 11
         repz cmpsb
         jcxz pcmagcom
         jmp raw
cafe:
		mov ax, word ptr file + 8 ; CAFE font file
		mov word [font], ax ; CAFE font file
         mov ax, word ptr file + 4
         mov [height], al

         ret
cafecom:
		mov ax, word ptr file + 14      ; CAFE executable
         add ax, word ptr file
         mov [font], ax
         mov al, file + 12
         mov [height], al

         ret
_fontedit:                         ; FONTEDIT 1.0 font file
		mov ax, word ptr file + 18
         mov word [font], ax
         mov ax, [_length]
         sub ax, 18

         jmp _height
pcmagcom:                       ; PC Magazine FONTEDIT executable
		mov ax, word ptr file + 99
         mov word [font], ax
         mov al, file + 50
         mov [height], al

         ret
raw:     
		mov ax, word ptr file
		mov word [font], ax    ; Raw font file
         mov ax, [_length]

_height:  mov [height], ah         ; Length divided by 256...
        jmp enddecode


end
