/***********************
*  LITTLE GAME ENGINE  *
************************/

/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	Sprite loader and bliter from xlib
	
	This is a 16-bit program, Remember to compile in the LARGE memory model!                        
	
	All code is 8086 / 8088 compatible
	
	Please feel free to copy this source code.                            
	

	LT_MODE = player movement modes

	//MODE TOP = 0;
	//MODE PLATFORM = 1;
	//MODE PUZZLE = 2;
	//MODE SIDESCROLL = 3;
	
	//33 fixed sprites:
	//	8x8   (16 sprites: 0 - 15)	
	//	16x16 (12 sprites: 16 - 27)
	//	32x32 ( 4 sprites: 28 - 31)
	//	64x64 ( 1 sprite: 32)
	
##########################################################################*/

#include "lt__eng.h"

LT_Col LT_Player_Col;

int x,y = 0;
int i,j = 0;
int Scene = 0;
/*
void _x_set_start_addr(word x,word y){
	push si

	mov  si,[x]
	mov  ax,[_ScrnLogicalByteWidth]     ; Calculate Offset increment
	mov  cx,[y]                         ; for Y
	mul  cx
PageFlipEntry1:
	add  ax,[_Page0_Offs]               ; no - add page 0 offset
	jmp  short @@AddColumn

PageFlipEntry2:

	mov  [_PhysicalStartPixelX],si
	mov  [_PhysicalStartY],cx

@@AddColumn:
	mov  cx,si
	shr  cx,2
	mov  [_PhysicalStartByteX],cx
	add  ax,cx                          ; add the column offset for X
	mov  bh,al                          ; setup CRTC start addr regs and
						; values in word registers for
	mov  ch,ah                          ; fast word outs

StartAddrEntry:
	mov  bl,ADDR_LOW
	mov  cl,ADDR_HIGH
	and  si,0003h             ; select pel pan register value for the
	mov  ah,PelPanMask[si]    ; required x coordinate
	mov  al,PEL_PANNING+20h
	mov  si,ax

	cmp  [_VsyncHandlerActive],TRUE
	jne   @@NoVsyncHandler
; NEW STUFF
@@WaitLast:
	cmp   [_StartAddressFlag],0
	jne   @@WaitLast
	cli
	mov  [_WaitingStartLow],bx
	mov  [_WaitingStartHigh],cx
	mov  [_WaitingPelPan],si
	mov  [_StartAddressFlag],1
	sti
	jmp  short @@Return

@@NoVsyncHandler:
	mov  dx,INPUT_STATUS_0    ;Wait for trailing edge of Vsync pulse
@@WaitDE:
	in   al,dx
	test al,01h
	jnz  @@WaitDE            ;display enable is active low (0 = active)

	mov  dx,CRTC_INDEX
	mov  ax,bx
	cli
	out  dx,ax               ;start address low
	mov  ax,cx
	out  dx,ax               ;start address high
	sti

; Now wait for vertical sync, so the other page will be invisible when
; we start drawing to it.
	mov  dx,INPUT_STATUS_0    ;Wait for trailing edge of Vsync pulse
@@WaitVS:
	in   al,dx
	test al,08h
	jz @@WaitVS           ;display enable is active low (0 = active)


	mov  dx,AC_INDEX
	mov  ax,si                ; Point the attribute controller to pel pan
	cli
	out  dx,al                ; reg. Bit 5 also set to prevent blanking
	mov  al,ah
	out  dx,al                ; load new Pel Pan setting.
	sti

}

*/

void Load_Puzzle2(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/rick2m0.tmx");
	LT_Load_Tiles("GFX/rick2til.bmp");
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/enemy.bmp",17,16);
	LT_Clone_Sprite(18,17);
	LT_Clone_Sprite(19,17);
	LT_Clone_Sprite(20,17);
	LT_Clone_Sprite(21,17);
	LT_Clone_Sprite(22,17);
	LT_Clone_Sprite(23,17);
	
	LT_Load_Music("music/top_down.imf");
	
	LT_Delete_Loading_Interrupt();
	LT_SetWindow("GFX/window.bmp");
	
	LT_Set_Map(0,9);
	LT_MODE = 1; 
}

void Run_Puzzle2(){
	int map_chunk = 0;
	int score = 0;
	Scene = 0;
	
	sprite[16].pos_x = 2*16;
	sprite[16].pos_y = 21*16;

	while(Scene == 0){
		LT_WaitVsyncEnd();

		LT_scroll_follow(16);
		
		LT_Restore_Sprite_BKG(16);
		LT_Draw_Sprite(16);

		LT_scroll_map();
		
		//In this mode sprite is controlled using L R and Jump
		LT_Player_Col = LT_move_player(16);

		//set player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,6,6,4);
		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		if (LT_Player_Col.tilecol_number == 11){// ? change map chunk
			map_chunk++;
			if (map_chunk == 1) {
				LT_Load_Map("GFX/rick2m1.tmx");
				LT_Set_Map(0,12);
				sprite[16].pos_x = 5*16;
				sprite[16].pos_y = 24*16;
			}
			if (map_chunk == 2){
				LT_Load_Map("GFX/rick2m2.tmx");
				LT_Set_Map(12,12);
				sprite[16].pos_x = 27*16;
				sprite[16].pos_y = 25*16;
			}
			if (map_chunk == 3){
				LT_Load_Map("GFX/rick2m3.tmx");
				LT_Set_Map(26,0);
				sprite[16].pos_x = 33*16;
				sprite[16].pos_y = 0*16;
			}
		}
		do_play_music();
		LT_WaitVsyncStart();
	}
	LT_Unload_Sprite(16); //manually free sprites
}

void Load_Hocus(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/hpmap.tmx");
	LT_Load_Tiles("GFX/hptil.bmp");
	LT_Load_Sprite("GFX/hplayer.bmp",16,16);
	LT_Load_Sprite("GFX/enemy.bmp",17,16);
	LT_Clone_Sprite(18,17);
	LT_Clone_Sprite(19,17);
	LT_Clone_Sprite(20,17);
	LT_Clone_Sprite(21,17);
	LT_Clone_Sprite(22,17);
	LT_Clone_Sprite(23,17);
	
	LT_Load_Music("music/hocus.imf");
	
	LT_Delete_Loading_Interrupt();
	LT_SetWindow("GFX/window.bmp");
	
	LT_Set_Map(0,0);
	LT_MODE = 1; 	
}

void Run_Hocus(){
	int map_chunk = 0;
	int score = 0;
	Scene = 0;
	
	sprite[16].pos_x = 2*16;
	sprite[16].pos_y = 2*16;

	while(Scene == 0){
		LT_WaitVsyncEnd();

		LT_scroll_follow(16);
		
		LT_Restore_Sprite_BKG(16);
		
		//EDIT MAP: GET DIAMONDS
		if ((LT_Player_Col.tile_number == 20) || (LT_Player_Col.tile_number == 21)){
			LT_Edit_MapTile(sprite[16].tile_x,sprite[16].tile_y, 4, 0);
		}
		
		LT_Draw_Sprite(16);

		LT_scroll_map();
		
		//In this mode sprite is controlled using L R and Jump
		LT_Player_Col = LT_move_player(16);
		
		LT_Print_Window_Variable(32,LT_Player_Col.tile_number);
		
		//set player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,1,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,1,1,4);
		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		do_play_music();
		LT_WaitVsyncStart();
	}
	LT_Unload_Sprite(16); //manually free sprites	
}

void main(){

	system("cls");

	LT_Init_GUS(12);
    LT_init();
	
	//You can use a custom loading animation for the Loading_Interrupt
	LT_Load_Animation("GFX/loading.bmp",32);
	LT_Set_Animation(0,4,2);
	
	LT_Load_Font("GFX/font.bmp");
	
	Load_Hocus();
	Run_Hocus();
	
	LT_ExitDOS();
	
}

