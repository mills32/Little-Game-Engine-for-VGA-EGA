;-----------------------------------------------------------------------
; MODULE XCBITMAP
; This module was written by Matthew MacKenzie
; matm@eng.umd.edu
;
; Compiled bitmap  functions all MODE X 256 Color resolutions
;
; Compile with Tasm.
; C callable.
;
; ****** XLIB - Mode X graphics library                ****************
; ******                                               ****************
; ****** Written By Themie Gouthas                     ****************
; ****** Aeronautical Research Laboratory              ****************
; ****** Defence Science and Technology Organisation   ****************
; ****** Australia                                     ****************
;
; egg@dstos3.dsto.gov.au
; teg@bart.dsto.gov.au
;-----------------------------------------------------------------------

LOCALS

.model large


SC_INDEX        equ  03c4h   ;Sequence Controller Index
MAP_MASK      equ    02h     ;index in SC of Map Mask register

	global _ScrnPhysicalByteWidth   :word
	global _ScrnPhysicalPixelWidth  :word
	global _ScrnPhysicalHeight      :word

	global _ScrnLogicalByteWidth    :word
	global _ScrnLogicalPixelWidth   :word
	global _ScrnLogicalHeight       :word
	
	PUBLIC  _x_compile_bitmap            
	PUBLIC  _x_run_compiled_sprite

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _x_compile_bitmap
;
; Compile a linear bitmap to generate machine code to plot it using planar bitmaps as in module XPBIITMAP
;
; C near-callable as:
; int x_compile_bitmap  (WORD logical_screen_width, char far * bitmap, char far * output);
;
; The logical width is in bytes rather than pixels.
;
; All four main registers are totaled.
;
;  The source linear bitmaps have the following structure:
;
;  BYTE 0                 The bitmap width in pixels  range 1..255
;  BYTE 1                 The bitmap height in rows   range 1..255
;  BYTE 2..n              The width*height bytes of the bitmap in
;                         cloumn row order
;
; The returned value is the size of the compiled bitmap, in bytes.


; accessory macros to save typing (what else?)
Emitb macro arg
	mov byte ptr es:[di],&arg&
	inc di
	endm

Emitw macro arg
	mov word ptr es:[di],&arg&
	add di,2
	endm

; opcodes emitted by _x_compile_sprite
ROL_AL          equ 0c0d0h              ; rol al
SHORT_STORE_8   equ 044c6h              ; mov [si]+disp8,  imm8
STORE_8         equ 084c6h              ; mov [si]+disp16, imm8
SHORT_STORE_16  equ 044c7h              ; mov [si]+disp8,  imm16
STORE_16        equ 084c7h              ; mov [si]+disp16, imm16
ADC_SI_IMMED    equ 0d683h              ; adc si,imm8
OUT_AL          equ 0eeh                ; out dx,al
RETURN          equ 0cbh                ; ret


.data

align 2
ColumnMask      db      011h,022h,044h,088h


.code

	align   2
_x_compile_bitmap proc	far
ARG   logical_width:word,bitmap:dword,output:dword
LOCAL bwidth,scanx,scany,outputx,outputy,column,set_column,input_size:word=LocalStk
	push bp
	mov  bp, sp         ; caller's stack frame
	sub  sp,LocalStk    ; local space
	push si
	push di
	push ds

	mov word ptr [scanx],0
	mov word ptr [scany],0
	mov word ptr [outputx],0
	mov word ptr [outputy],0
	mov word ptr [column],0
	mov word ptr [set_column],0

	lds si,[bitmap]     ; 32-bit pointer to source bitmap

	les di,[output]     ; 32-bit pointer to destination stream

	lodsb               ; load width byte
	xor ah, ah          ; convert to word
	mov [bwidth], ax    ; save for future reference
	mov bl, al          ; copy width byte to bl
	lodsb               ; load height byte -- already a word since ah=0
	mul bl              ; mult height word by width byte
	mov [input_size], ax;  to get pixel total 

@@MainLoop:
	mov bx, [scanx]     ; position in original bitmap
	add bx, [scany]

	mov al, [si+bx]     ; get pixel
	or  al, al          ; skip empty pixels
	jnz @@NoAdvance
	jmp @@Advance
@@NoAdvance:

	mov dx, [set_column]
	cmp dx, [column]
	je @@SameColumn
@@ColumnLoop:
	Emitw ROL_AL        ; emit code to move to new column
	Emitw ADC_SI_IMMED
	Emitb 0

	inc dx
	cmp dx, [column]
	jl @@ColumnLoop

	Emitb OUT_AL        ; emit code to set VGA mask for new column
	mov [set_column], dx
@@SameColumn:
	mov dx, [outputy]   ; calculate output position
	add dx, [outputx]
	sub dx, 128

	add word ptr [scanx], 4
	mov cx, [scanx]     ; within four pixels of right edge?
	cmp cx, [bwidth]
	jge @@OnePixel

	inc word ptr [outputx]
	mov ah, [si+bx+4]   ; get second pixel
	or ah, ah
	jnz @@TwoPixels
@@OnePixel:
	cmp dx, 127         ; can we use shorter form?
	jg @@OnePixLarge
	cmp dx, -128
	jl @@OnePixLarge
	Emitw SHORT_STORE_8
	Emitb dl            ; 8-bit position in output
	jmp @@EmitOnePixel
@@OnePixLarge:
	Emitw STORE_8
	Emitw dx            ; position in output
@@EmitOnePixel:
	Emitb al
	jmp short @@Advance
@@TwoPixels:
	cmp dx, 127
	jg @@TwoPixLarge
	cmp dx, -128
	jl @@TwoPixLarge
	Emitw SHORT_STORE_16
	Emitb dl            ; 8-bit position in output
	jmp @@EmitTwoPixels
@@TwoPixLarge:
	Emitw STORE_16
	Emitw dx            ; position in output
@@EmitTwoPixels:
	Emitw ax

@@Advance:
	inc word ptr [outputx]
	mov ax, [scanx]
	add ax, 4
	cmp ax, [bwidth]
	jl @@AdvanceDone
	mov dx, [outputy]
	add dx, [logical_width]
	mov cx, [scany]
	add cx, [bwidth]
	cmp cx, [input_size]
	jl @@NoNewColumn
	inc word ptr [column]
	mov cx, [column]
	cmp cx, 4
	je @@Exit           ; Column 4: there is no column 4.
	xor cx, cx          ; scany and outputy are 0 again for
	mov dx, cx          ; the new column
@@NoNewColumn:
	mov [outputy], dx
	mov [scany], cx
	mov word ptr [outputx], 0
	mov ax,[column]
@@AdvanceDone:
	mov [scanx], ax
	jmp @@MainLoop

@@Exit:
	Emitb RETURN
	mov ax,di
	sub ax,word ptr [output] ; size of generated code

	pop ds
	pop di
	pop si
	mov sp, bp
	pop bp

	ret
_x_compile_bitmap endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _x_put_cbitmap
;
; Displays a compiled bitmap generated by x_compile_bitmap at given
; coordinates, on a given screen page.
;
; C near-callable as:
; void x_put_cbitmap (int XPos, int YPos,
;                     unsigned int PageOffset, char far * Sprite);
; ax, bx, cx, and dx are squashed like insignificant insects.

	align   2
_x_run_compiled_sprite proc	far
ARG XPos:word,YPos:word,Sprite:dword

	push bp
	mov bp, sp
	push si
	push ds

	mov ax, 84 ; width
	mul word ptr [YPos] ; height in bytes
	mov si, [XPos]
	mov bx, si
	sar si, 2           ; width in bytes
	add si, ax
	add si, 128         ;   (Xpos / 4) + page base + 128 ==> starting pos

	and bx, 3
	mov ah, ColumnMask[bx]

	mov dx, SC_INDEX
	mov al, MAP_MASK
	out dx, ax
	inc dx              ; ready to send out other masks as bytes
	mov al, ah

	mov bx, 0a000h
	mov ds, bx          ; We do this so the compiled shape won't need  segment overrides.

	call dword ptr [Sprite] ; the business end of the routine

	pop ds
	pop si
	pop bp

	ret
_x_run_compiled_sprite endp

end

