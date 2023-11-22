#include "lt__eng.h"

//keyboard
int LT_Keys[256];
unsigned char keyhit;

extern void interrupt (*LT_old_key_handler)(void); 

//So easy, and so difficult to find samples of this...
void interrupt LT_Key_Handler(void){
	asm{
	cli
    in    al, 060h      
    mov   keyhit, al
    in    al, 061h       
    mov   bl, al
    or    al, 080h
    out   061h, al     
    mov   al, bl
    out   061h, al      
    mov   al, 020h       
    out   020h, al
    sti
    }
	if (keyhit & 0x80){ keyhit &= 0x7F; LT_Keys[keyhit] = 0;} //KEY_RELEASED;
    else LT_Keys[keyhit] = 1; //KEY_PRESSED;
}

void LT_install_key_handler(){
	int i;
	LT_old_key_handler();
	setvect(9,LT_Key_Handler);     //install new one
	for (i = 0; i != 256; i++) LT_Keys[i] = 0;
}

void LT_reset_key_handler(){
	setvect(9,LT_old_key_handler);     
}

void Clearkb(){
	asm mov ah,00ch
	asm mov al,0
	asm int 21h
}
