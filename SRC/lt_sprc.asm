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
	
	PUBLIC  _x_compile_bitmap            

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


; opcodes emitted by _x_compile_sprite
ROL_AL          equ 0c0d0h              ; rol al
SHORT_STORE_8   equ 044c6h              ; mov [si]+disp8,  imm8
STORE_8         equ 084c6h              ; mov [si]+disp16, imm8
SHORT_STORE_16  equ 044c7h              ; mov [si]+disp8,  imm16
STORE_16        equ 084c7h              ; mov [si]+disp16, imm16
ADC_SI_IMMED    equ 0d683h              ; adc si,imm8
OUT_AL          equ 0eeh                ; out dx,al
RETURN          equ 0cbh                ; ret


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
	mov word ptr es:[di],0c0d0h; emit code to move to new column
	add di,2
	mov word ptr es:[di],0d683h
	add di,2
	mov byte ptr es:[di],0
	inc di

	inc dx
	cmp dx, [column]
	jl @@ColumnLoop

	mov byte ptr es:[di],0eeh
	inc di; emit code to set VGA mask for new column
	
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
	mov word ptr es:[di],044c6h
	add di,2
	
	mov byte ptr es:[di],dl
	inc di; 8-bit position in output
	
	jmp @@EmitOnePixel
@@OnePixLarge:
	mov word ptr es:[di],084c6h
	add di,2
	mov word ptr es:[di],dx
	add di,2 ;position in output
@@EmitOnePixel:
	mov byte ptr es:[di],al
	inc di
	jmp short @@Advance
@@TwoPixels:
	cmp dx, 127
	jg @@TwoPixLarge
	cmp dx, -128
	jl @@TwoPixLarge
	mov word ptr es:[di],044c7h
	add di,2
	mov byte ptr es:[di],dl
	inc di            ; 8-bit position in output
	jmp @@EmitTwoPixels
@@TwoPixLarge:
	mov word ptr es:[di],084c7h
	add di,2
	mov word ptr es:[di],dx
	add di,2 ; position in output
@@EmitTwoPixels:
	mov word ptr es:[di],ax
	add di,2

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
	mov byte ptr es:[di],0cbh
	inc di
	mov ax,di
	sub ax,word ptr [output] ; size of generated code

	pop ds
	pop di
	pop si
	mov sp, bp
	pop bp

	ret
_x_compile_bitmap endp

end

