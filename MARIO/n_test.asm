DGROUP         	GROUP    _DATA, _BSS

_DATA          	SEGMENT WORD PUBLIC 'DATA'
   
_DATA          	ENDS

_BSS           	SEGMENT   WORD PUBLIC 'BSS'
				;EXTRN  _test_width:WORD
_BSS           	ENDS

_TEXT          	SEGMENT BYTE PUBLIC 'CODE'
				ASSUME CS:_TEXT,DS:DGROUP,SS:DGROUP
				
				PUBLIC _asm_draw_bkg 
				PUBLIC _asm_draw_sprite       
				

_asm_draw_bkg	proc	far ;working
	push bp
	mov	bp,sp
	push ds
	push di
    push si
	
	;screen_offset = (y<<8)+(y<<6)+x;
	mov		bx,[bp+12]			
	shl		bx,8
	mov		dx,[bp+12]
	shl		dx,6
	add		bx,dx
	add		bx,[bp+10]
	mov 	ax,0A000h
	mov 	es,ax						
	mov		di,bx				
	
	;sprite bkg data
	lds	bx,[bp+6]
	lds	si,ds:[bx]				
	
	mov 	ax,16
copy_line1:	
	mov 	cx,8
	rep		movsw				; copy 16 bytes from ds:si to es:di
	add 	di,320-16
	dec 	ax
	jnz		copy_line1
	;----------------------------------End
	
	pop si
	pop di
	pop ds
	pop	bp
	ret	
_asm_draw_bkg	endp
	
_asm_draw_sprite	proc	far ;working
	push bp
	mov	bp,sp
	push ds
	push di
    push si
	
	;screen_offset 
	mov		bx,[bp+10]			
	mov 	ax,0A000h
	mov 	es,ax						
	mov		di,bx				
	
	lds		bx,[bp+6]					
	lds		si,ds:[bx]			
	mov		ax,[bp+12]
	shl		ax,8				
	add		si,ax				
	
	mov 	ax,16
copy_line2:	
	mov 	cx,8
	rep		movsw				; copy 16 bytes from ds:si to es:di
	add 	di,320-16
	dec 	ax
	jnz		copy_line2
	;----------------------------------End
	
	pop si
	pop di
	pop ds
	mov	sp,bp
	pop	bp
	ret	
_asm_draw_sprite	endp


_TEXT          ENDS
               END
