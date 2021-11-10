/*Compile And run sprites functions from XLIB - Mode X graphics library
; Written By Themie Gouthas                  
; Aeronautical Research Laboratory           
; Defence Science and Technology Organisation
; Australia                                  
;
; egg@dstos3.dsto.gov.au
; teg@bart.dsto.gov.au
*/

#include "lt__eng.h"

SPRITE *sprite;

//Player
byte tile_number = 0;		//Tile a sprite is on
byte tilecol_number = 0;	//Collision Tile a sprite is on
byte tilecol_number_down = 0;//Collision Tile down
byte tile_number_VR = 0;	//Tile vertical right
byte tile_number_VL = 0;	//Tile vertical left
byte tile_number_HU = 0;	//Tile horizontal up
byte tile_number_HD = 0;	//Tile horizontal down

byte LT_Sprite_Stack = 0;
byte LT_Sprite_Stack_Table[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte LT_AI_Enabled = 0;
byte LT_Active_AI_Sprites[] = {0,0,0,0,0,0,0};
byte Lt_AI_Sprite[] = {0,0,0,0,0,0,0,0};
byte LT_Sprite_Size_Table[] = {8,8,8,8,8,8,8,8,16,16,16,16,16,16,16,16,32,32,32,32,64};
int LT_Sprite_Animation_Player[32];
int LT_Sprite_Animation_AI_0[32];
int LT_Sprite_Animation_AI_1[32];
int LT_Sprite_Animation_AI_2[32];
int LT_Sprite_Animation_AI_3[32];

byte selected_AI_sprite;
int LT_number_of_ai = 0;
//NON SCROLLING WINDOW
extern byte LT_Window; 				//Displaces everything (16 pixels) in case yo use the 320x16 window in the game
unsigned char *LT_sprite_data; 
dword LT_sprite_data_offset = 16*1024; //just after loading animation; 
void LT_Error(char *error, char *file);
extern byte *LT_Filename;
void LT_VGA_Enable_4Planes();
void LT_VGA_Return_4Planes();

//player movement modes
//MODE TOP = 0;
//MODE PLATFORM = 1;
//MODE PUZZLE = 2;
//MODE SIDESCROLL = 3;
byte LT_MODE = 0;
int x_adjust = 160;

int LT_player_jump_pos[] = {
	-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

int LT_Spr_speed_float = 0;
int LT_Spr_speed[] = {
	//Add speeds for
	-2,-2,-3,-2,-2,-3,-2,-2,
	-2,-2,-2,-2,-1,-2,-2,-2,
	-2,-2,-2,-1,-1,-2,-2,-2,
	-2,-1,-2,-1,-2,-2,-2,-1,
	-2,-1,-2,-1,-2,-1,-2,-1,
	-2,-1,-1,-2,-1,-1,-2,-1,
	-2,-1,-1,-1,-2,-1,-1,-1,
	-2,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1, 0,-1,-1,-1,
	-1,-1,-1, 0, 0,-1,-1,-1,
	-1, 0,-1,-1,-1, 0,-1, 0,
	-1, 0,-1, 0,-1, 0,-1, 0,
	-1, 0, 0,-1, 0, 0, 1, 0,	
	-1, 0, 0, 0,-1, 0, 0, 0,
	-1, 0, 0, 0, 0, 0, 0, 0,
	
	0,0,0,0,0,0,0,0,	//8x16
	
	1,0,0,0,0,0,0,0,
	1,0,0,0,1,0,0,0,
	1,0,0,1,0,0,1,0,
	1,0,1,0,1,0,1,0,
	1,0,1,1,1,0,1,0,
	1,1,1,0,0,1,1,1,
	1,1,1,1,0,1,1,1,
	1,1,1,1,1,1,1,1,
	2,1,1,1,1,1,1,1,
	2,1,1,1,2,1,1,1,
	2,1,1,2,1,1,2,1,
	2,1,2,1,2,1,2,1,
	2,1,2,1,2,2,2,1,
	2,2,2,1,1,2,2,2,
	2,2,2,2,1,2,2,2,
	2,2,3,2,2,3,2,2,	//8x32
};

int LT_wmap = 0;
int LT_hmap = 0;//91.888
int LT_Scroll_Camera_float = 0;
int LT_Scroll_Camera_array[135] = {
	0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,
	8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

int LT_Scroll_Camera_speed[] = {
	0,0,0,0,0,0,0,0,
	1,0,0,0,1,0,0,0,
	1,0,1,0,1,0,1,0,
	1,1,1,0,1,1,1,0,
	1,1,1,1,1,1,1,1,
	2,1,1,1,2,1,1,1,
	2,1,2,1,2,1,2,1,
	2,2,2,1,2,2,2,1,
	2,2,2,2,2,2,2,2,
};

void LT_scroll_follow(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int x;
	int x1;
	int y = (s->pos_y-SCR_Y+LT_Window)-120;
	int y1 = abs(y);
	int speed_x = 0;
	int speed_y = 0;
	
	if (s->state == 3) x_adjust = 190; //FACING LEFT
	if (s->state == 4) x_adjust = 130; //FACING RIGHT
	//Show more screen in the direction the sprite is facing
	x = (s->pos_x-SCR_X) - x_adjust;
	x1 = abs(x);
	
	if ((SCR_X > -1) && ((SCR_X+319)<LT_wmap) && (SCR_Y > -1) && ((SCR_Y+239)<(LT_hmap+16))){
		if (LT_Scroll_Camera_float == 8) LT_Scroll_Camera_float = 0;
		speed_x = LT_Scroll_Camera_speed[(LT_Scroll_Camera_array[x1]<<3)+LT_Scroll_Camera_float];
		speed_y = LT_Scroll_Camera_speed[(LT_Scroll_Camera_array[y1]<<3)+LT_Scroll_Camera_float];
		if (x < 0) SCR_X-=speed_x;
		if (x > 0) SCR_X+=speed_x;
		if (y < 0) SCR_Y-=speed_y;
		if (y > 0) SCR_Y+=speed_y;
		
		LT_Scroll_Camera_float++;
	}

	if (SCR_X < 0) SCR_X = 0; 
	if ((SCR_X+320) > LT_wmap) SCR_X = LT_wmap-320;
	if (SCR_Y < 0) SCR_Y = 0; 
	if ((SCR_Y+240) > (LT_hmap+16)) SCR_Y = LT_hmap-240+16;
}

void LT_Header_BMP(FILE *fp,int mode, int sprite_number);

word Compile_Bitmap(word logical_width, unsigned char *bitmap, unsigned char *output){
	word bwidth,scanx,scany,outputx,outputy,column,set_column,input_size,code_size;

	asm push si
	asm push di
	asm push ds

	asm mov word ptr [scanx],0
	asm mov word ptr [scany],0
	asm mov word ptr [outputx],0
	asm mov word ptr [outputy],0
	asm mov word ptr [column],0
	asm mov word ptr [set_column],0

	asm lds si,[bitmap]     // 32-bit pointer to source bitmap

	asm les di,[output]     // 32-bit pointer to destination stream

	asm lodsb               // load width byte
	asm xor ah, ah          // convert to word
	asm mov [bwidth], ax    // save for future reference
	asm mov bl, al          // copy width byte to bl
	asm lodsb               // load height byte -- already a word since ah=0
	asm mul bl              // mult height word by width byte
	asm mov [input_size], ax//  to get pixel total 

_MainLoop:
	asm mov bx, [scanx]     // position in original bitmap
	asm add bx, [scany]

	asm mov al, [si+bx]     // get pixel
	asm or  al, al          // skip empty pixels
	asm jnz _NoAdvance
	asm jmp _Advance
_NoAdvance:

	asm mov dx, [set_column]
	asm cmp dx, [column]
	asm je _SameColumn
_ColumnLoop:
	asm mov word ptr es:[di],0c0d0h// emit code to move to new column
	asm add di,2
	asm mov word ptr es:[di],0d683h
	asm add di,2
	asm mov byte ptr es:[di],0
	asm inc di

	asm inc dx
	asm cmp dx, [column]
	asm jl _ColumnLoop

	asm mov byte ptr es:[di],0eeh
	asm inc di// emit code to set VGA mask for new column
	
	asm mov [set_column], dx
_SameColumn:
	asm mov dx, [outputy]   // calculate output position
	asm add dx, [outputx]
	asm sub dx, 128

	asm add word ptr [scanx], 4
	asm mov cx, [scanx]     // within four pixels of right edge?
	asm cmp cx, [bwidth]
	asm jge _OnePixel

	asm inc word ptr [outputx]
	asm mov ah, [si+bx+4]   // get second pixel
	asm or ah, ah
	asm jnz _TwoPixels
_OnePixel:
	asm cmp dx, 127         // can we use shorter form?
	asm jg _OnePixLarge
	asm cmp dx, -128
	asm jl _OnePixLarge
	asm mov word ptr es:[di],044c6h
	asm add di,2
	
	asm mov byte ptr es:[di],dl
	asm inc di// 8-bit position in output
	
	asm jmp _EmitOnePixel
_OnePixLarge:
	asm mov word ptr es:[di],084c6h
	asm add di,2
	asm mov word ptr es:[di],dx
	asm add di,2 //position in output
_EmitOnePixel:
	asm mov byte ptr es:[di],al
	asm inc di
	asm jmp short _Advance
_TwoPixels:
	asm cmp dx, 127
	asm jg _TwoPixLarge
	asm cmp dx, -128
	asm jl _TwoPixLarge
	asm mov word ptr es:[di],044c7h
	asm add di,2
	asm mov byte ptr es:[di],dl
	asm inc di            // 8-bit position in output
	asm jmp _EmitTwoPixels
_TwoPixLarge:
	asm mov word ptr es:[di],084c7h
	asm add di,2
	asm mov word ptr es:[di],dx
	asm add di,2 // position in output
_EmitTwoPixels:
	asm mov word ptr es:[di],ax
	asm add di,2

_Advance:
	asm inc word ptr [outputx]
	asm mov ax, [scanx]
	asm add ax, 4
	asm cmp ax, [bwidth]
	asm jl _AdvanceDone
	asm mov dx, [outputy]
	asm add dx, [logical_width]
	asm mov cx, [scany]
	asm add cx, [bwidth]
	asm cmp cx, [input_size]
	asm jl _NoNewColumn
	asm inc word ptr [column]
	asm mov cx, [column]
	asm cmp cx, 4
	asm je _Exit           // Column 4: there is no column 4.
	asm xor cx, cx          // scany and outputy are 0 again for
	asm mov dx, cx          // the new column
_NoNewColumn:
	asm mov [outputy], dx
	asm mov [scany], cx
	asm mov word ptr [outputx], 0
	asm mov ax,[column]
_AdvanceDone:
	asm mov [scanx], ax
	asm jmp _MainLoop

_Exit:
	asm mov byte ptr es:[di],0cbh
	asm inc di
	asm mov ax,di
	asm sub ax,word ptr [output] // size of generated code
	asm mov [code_size],ax
	asm pop ds
	asm pop di
	asm pop si
	
	return code_size;
}

//load sprites with transparency (size = 8,16,32,64!)
void LT_Load_Sprite(char *file, int sprite_number, byte *animations){
	SPRITE *s = &sprite[sprite_number];
	long index,offset;
	word num_colors;
	word x,y;
	word i,j;
	word frame = 0;
	byte tileX;
	byte tileY;
	byte size;
	FILE *fp;

    int code_size;
	
	fp = fopen(file,"rb");
	
	if(!fp) LT_Error("Can't find ",file);
	LT_Filename = file;
	LT_Header_BMP(fp,2,sprite_number);

	for(index=(s->height-1)*s->width;index>=0;index-=s->width){
		for(x=0;x<s->width;x++){
			unsigned char c = (byte)fgetc(fp);
			if (c) c+=208;//Sprite palette starts at 208, if 0 do not add 208, it is color 0 (transparent)
			LT_tile_tempdata[index+x]=c;
		}
	}
	fclose(fp);
	
	//Get size of sprite according to its number in the sprite array
	size = LT_Sprite_Size_Table[sprite_number];
	
	index = 0; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			LT_tile_tempdata2[index] = size;
			LT_tile_tempdata2[index+1] = size;
			index+=2;
			for(x=0;x<size;x++){
				memcpy(&LT_tile_tempdata2[index],&LT_tile_tempdata[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	
	s->nframes = (s->width/size) * (s->height/size);
	
	//Estimated size
	//fsize = (size * size * 7) / 2 + 25;
	s->code_size = 0;
	for (frame = 0; frame < s->nframes; frame++){
		/*if ((s->frames[frame].compiled_code = farcalloc(fsize,sizeof(unsigned char))) == NULL){
			LT_Error("Not enough RAM to allocate frames ",file);
		}*/
		//COMPILE SPRITE FRAME TO X86 MACHINE CODE
		//& Store the compiled data at it's final destination	
		code_size = Compile_Bitmap(84, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],&LT_sprite_data[LT_sprite_data_offset]);
		s->frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
		LT_sprite_data_offset += code_size;
		s->code_size += code_size;
		if (LT_sprite_data_offset > 65536-8192) LT_Error("Not enough RAM to allocate frames ",file);
		//if (code_size == NULL) LT_Error("Not enough RAM to allocate frames ",file);
		//s->frames[frame].compiled_code = farrealloc(s->frames[frame].compiled_code, code_size);
		//Store the compiled data at it's final destination
	}

	//IINIT SPRITE
	//Load animations
	memcpy(s->animations,animations,64);
	//Check animations so that game does not crash
	for (frame = 0; frame < 64;frame++){
		if (s->animations[frame] > s->nframes-1) s->animations[frame] = 0;
	}
	
	//s->bkg_data set in init function (sys.c)
	s->width = size;
	s->height = size;

	s->init = 0;
	s->frame = 0;
	s->baseframe = 65;
	s->ground = 0;
	s->jump = 2;
	s->jump_frame = 0;
	s->climb = 0;
	s->animate = 0;
	s->anim_speed = 0;
	s->speed = 4;
	s->anim_counter = 0;
	s->speed_x = 8*16;
	s->speed_y = 8*16;
	s->s_x = 0;
	s->s_y = 0;
	s->s_delete = 0;
	s->fixed_sprite_number = 0;
	s->mspeed_x = 1;
	s->mspeed_y = 1;
	s->size = s->height>>3;
	s->siz = (s->height>>2) + 1;
	s->next_scanline = 84 - s->siz;

	s->get_item = 0;
	s->mode = 0;
	s->ai_stack = 0;
}

void LT_Clone_Sprite(int sprite_number_c,int sprite_number){
	SPRITE *c = &sprite[sprite_number_c];
	SPRITE *s = &sprite[sprite_number];
	int j;
	c->nframes = s->nframes;
	c->width = s->width;
	c->height = s->height;
	for(j=0;j<c->nframes;j++) {
		c->frames[j].compiled_code = s->frames[j].compiled_code;
		c->frames[j] = s->frames[j];
	}
	
	//sprite[sprite_number_c].bkg_data  //allocated in lt_sys
	memcpy(c->animations,s->animations,32);
	c->init = 0;
	c->frame = 0;
	c->baseframe = 65;
	c->ground = 0;
	c->jump = 2;
	c->jump_frame = 0;
	c->climb = 0;
	c->animate = 0;
	c->anim_speed = 0;
	c->speed = 4;
	c->anim_counter = 0;
	c->mspeed_x = 1;
	c->mspeed_y = 1;
	c->speed_x = 8*16;
	c->speed_y = 8*26;
	c->s_x = 0;
	c->s_y = 0;
	c->s_delete = 0;
	c->fixed_sprite_number = 0;
	c->size = c->height>>3;
	c->siz = (c->height>>2) + 1;
	c->next_scanline = 84 - c->siz;
	c->get_item = 0;
	c->mode = 0;
	c->ai_stack = 0;
}

void LT_Init_Sprite(int sprite_number,int x,int y){
	LT_Sprite_Stack_Table[LT_Sprite_Stack] = sprite_number;
	sprite[sprite_number].pos_x = x;
	sprite[sprite_number].pos_y = y;
	sprite[sprite_number].ai_stack = LT_Sprite_Stack;
	sprite[sprite_number].init = 0;
	LT_Sprite_Stack++;
}

void LT_Add_Sprite(int sprite_number,word x, word y) {
	sprite[sprite_number].pos_x = x;
	sprite[sprite_number].pos_y = y;
}

void LT_Reset_Sprite_Stack(){
	int i;
	LT_Sprite_Stack = 0;
	for (i = 0; i<33; i++) LT_Sprite_Stack_Table[i] = 0;
	LT_AI_Sprite[0] = 0;
}

void LT_Set_Sprite_Animation(int sprite_number, byte anim){
	SPRITE *s = &sprite[sprite_number];
	if (s->baseframe != anim<<3){//If changed animation
		s->baseframe = anim<<3;
		s->anim_counter = anim<<3;
		s->animate = 1;
		if (s->baseframe > 64) s->baseframe = 0;
	}
}

void LT_Set_Sprite_Animation_Speed(int sprite_number, byte speed){
	SPRITE *s = &sprite[sprite_number];
	s->speed = speed;
}

void LT_Sprite_Stop_Animation(int sprite_number){
	if (sprite[sprite_number].animate == 1){
		sprite[sprite_number].animate = 0; 
		sprite[sprite_number].baseframe = 65;
	}
}

void (*LT_Delete_Sprite)(int);

void LT_Delete_Sprite_VGA(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	word bkg_data = s->bkg_data;
	word screen_offset0 = (s->last_y<<6)+(s->last_y<<4)+(s->last_y<<2)+(s->last_x>>2);
	word screen_offset1 = screen_offset0 + (304*84);
	word size = s->height>>3;
	word siz = (s->height>>2)+1;
	word next_scanline = 84 - siz;
	s->s_delete = 0;
	
	///paste bkg chunk
	outport(SC_INDEX, ((word)0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	asm{
		push ds
		push di
		push si
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0	//es:di destination vram				
		
		mov		ds,ax						
		mov		si,bkg_data			//ds:si source vram				
		mov 	ax,size
		mov		bx,next_scanline
		mov		dx,siz
	}
	bkg_scanline1:	
	asm{
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		dec 	ax
		jnz		bkg_scanline1

		//Delete second page
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset1	//es:di destination vram				
		
		mov		ds,ax						
		mov		si,bkg_data			//ds:si source vram				
		mov 	ax,size
		mov		bx,next_scanline
		mov		dx,siz
	}
	bkg_scanline2:	
	asm{
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		mov 	cx,dx
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,bx
		dec 	ax
		jnz		bkg_scanline2
	}

	asm pop si
	asm pop di
	asm pop ds	
	
	outport(GC_INDEX + 1, 0x0ff);
	
	s->init = 0;
}

byte CMask[] = {0x11,0x22,0x44,0x88};
void run_compiled_sprite(word XPos, word YPos, char *Sprite){
	asm{
		push si
		push ds
	
		mov ax, 84 //width
		mul word ptr [YPos] // height in bytes
		mov si, [XPos]
		mov bx, si
		mov	cl,2
		shr	si,cl
		add si,ax
		add si, 128         // (Xpos / 4) + 128 ==> starting pos
		and bx, 3
		mov ah,byte ptr CMask[bx]
		mov dx, 03c4h
		mov al, 02h
		out dx, ax
		inc dx              //ready to send out other masks as bytes
		mov al, ah
	
		mov bx, 0a000h
		mov ds, bx          // We do this so the compiled shape won't need  segment overrides.
	
		call dword ptr [Sprite] //the business end of the routine
		
		pop ds
		pop si
	}
}

extern word pageflip[];
extern word gfx_draw_page;
//Draw restoring bkg, slower
void (*LT_Draw_Sprites)(void);

extern unsigned char Enemies;
//If pos -2 (frame-2), then Clean bkg at pos-2 in draw page (for all sprites)
//If pos -1 (frame-1), pick bkg at pos -1 from the clean draw page.
//paste at current pos on draw page (if first paste, only do this)
void LT_Draw_Sprites_VGA(){
	int sprite_number;

	LT_VGA_Enable_4Planes();
	//RESTORE SPRITE BKG
	for (sprite_number = LT_Sprite_Stack; sprite_number > -1; sprite_number--){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		int lx = s->last_last_x;
		int ly = s->last_last_y+pageflip[gfx_draw_page&1];
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X+16)&&(x < (SCR_X+286))&&(y > (SCR_Y-8))&&(y < (SCR_Y+240))){
		if (s->init == 2){
		word bkg_data = s->bkg_data;
		word screen_offset0 = (ly<<6)+(ly<<4)+(ly<<2)+(lx>>2); //Paste at pos -2
		word size = s->size;
		word siz = s->siz;
		word next_scanline = s->next_scanline;
		///Clean pos 0 bkg
		asm{
		
			push ds
			push di
			push si
			
			mov dx,SC_INDEX //dx = indexregister
			mov ax,0F02h	//INDEX = MASK MAP, 
			out dx,ax 		//write all the bitplanes.
			mov dx,GC_INDEX //dx = indexregister
			mov ax,008h		
			out dx,ax 		
			
			mov 	ax,0A000h
			mov 	es,ax						
			mov		di,screen_offset0	//es:di destination vram				
			
			mov		ds,ax						
			mov		si,bkg_data			//ds:si source vram				
			mov 	ax,size
			mov		bx,next_scanline
			mov		dx,siz
		}
		bkg_scanline1:	
		asm{
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx
			mov 	cx,dx
			rep		movsb				// copy bytes from ds:si to es:di
			add 	di,bx				//632 -> 576
			dec 	ax
			jnz		bkg_scanline1
		}
		
		asm mov dx,GC_INDEX +1 //dx = indexregister
		asm mov ax,00ffh		
		asm out dx,ax 
		
		asm pop si
		asm pop di
		asm pop ds
		}
		}
	}
	
	LT_VGA_Return_4Planes();
	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window;
		int lx = s->last_x;
		int ly = s->last_y+pageflip[gfx_draw_page&1];
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X+16)&&(x < (SCR_X+286))&&(y > (SCR_Y-8))&&(y < (SCR_Y+240))){
			word bkg_data = s->bkg_data;
			word screen_offset1 = (ly<<6)+(ly<<4)+(ly<<2)+(lx>>2);//Get pos -1
			word size = s->size;
			word siz = s->siz;
			word next_scanline = s->next_scanline;
			
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			if (s->init){
			//Copy bkg chunk to a reserved VRAM part, before destroying it
			asm{
				push ds
				push di
				push si
				
				mov dx,SC_INDEX //dx = indexregister
				mov ax,0F02h	//INDEX = MASK MAP, 
				out dx,ax 		//write all the bitplanes.
				mov dx,GC_INDEX //dx = indexregister
				mov ax,008h		
				out dx,ax 
				
				mov 	ax,0A000h
				mov 	ds,ax						
				mov		si,screen_offset1	//ds:si source vram			
				
				mov 	es,ax						
				mov		di,bkg_data			//es:di destination
				
				mov		ax,size				//Scanlines
				mov		bx,next_scanline
				mov		dx,siz
			}
			bkg_scanline2:	
			asm{
				mov 	cx,dx				// copy width + 4 pixels
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				mov 	cx,dx				
				rep		movsb				// copy bytes from ds:si to es:di
				add 	si,bx
				dec 	ax
				jnz		bkg_scanline2	
				
				mov dx,GC_INDEX +1 //dx = indexregister
				mov ax,00ffh		
				out dx,ax 
				
				pop si
				pop di
				pop ds
			}
			s->last_last_x = s->last_x;
			s->last_last_y = s->last_y;
			}
			//draw sprite and destroy bkg
			run_compiled_sprite(x,y+pageflip[gfx_draw_page&1],s->frames[s->frame].compiled_code);
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
			if (s->init != 2) s->init++;
		}
		else if (s->s_delete == 1) {
			LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]);
		}
		if (sprite_number){
			if ((x < (SCR_X-16))||(x > SCR_X+321)){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 

				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
}

//Draw without restoring bkg, use over solid color backgrounds
void (*LT_Draw_Sprites_Fast)(void);

void LT_Draw_Sprites_Fast_VGA(){
	int sprite_number;

	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window+pageflip[gfx_draw_page&1];
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+224))){
			
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			
			//draw sprite and destroy bkg
			run_compiled_sprite(x,y,s->frames[s->frame].compiled_code);
			//run_compiled_sprite2(x,y);
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {
			LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]);
			s->s_delete = 0;
		}
		if (sprite_number){
			if ((x < SCR_X-16)||(x > SCR_X+336)){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				int i;
				for (i = 0; i < 7; i++){
					if (!LT_Active_AI_Sprites[i]){
						LT_Active_AI_Sprites[i] = spr; 
						i = 10;
					}
				}
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
}

void LT_Set_AI_Sprite(byte sprite_number, byte mode, word x, word y, int sx, int sy){
	//int i = 0;
	SPRITE *s = &sprite[sprite_number];
	s->pos_x = x<<4;
	s->pos_y = y<<4;
	s->mspeed_x = sx;
	s->mspeed_y = sy;
	if (s->mspeed_x!=0) {s->speed_x = 3; s->speed_y = 0;}
	if (s->mspeed_y!=0) {s->speed_x = 0; s->speed_y = 3;}
	s->mode = mode;
	s->ai_stack = LT_Sprite_Stack;
}

void LT_Set_AI_Sprites(byte first_type, byte second_type, byte mode_0, byte mode_1){
	int i;
	LT_AI_Enabled = 1;
	LT_AI_Sprite[0] = first_type;
	LT_AI_Sprite[3] = second_type;
	for (i = 0; i < 3; i++) {
		LT_Clone_Sprite(first_type+i,first_type);
		sprite[first_type+i].fixed_sprite_number = i;
		sprite[first_type+i].mode = mode_0;
		LT_Active_AI_Sprites[i] = first_type+i;
	}
	if (first_type != 17){
		for (i = 0; i < 4; i++) {
			LT_Clone_Sprite(first_type+3+i,second_type);
			sprite[first_type+3+i].fixed_sprite_number = i+3;
			sprite[first_type+3+i].mode = mode_1;
			LT_Active_AI_Sprites[i+3] = first_type+3+i;
		}
	}
	
	//for (i = first_ai+1; i < number_ai; i++) LT_Clone_Sprite(i,first_ai);
}

extern word TILE_VRAM;

byte LT_Player_Dir = 0;
LT_Sprite_State LT_move_player(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int tsize = 4;//16
	LT_Sprite_State LT_Collision;
	byte col_x = 0;
	byte col_y = 0;
	int x,y,platform_y;
	byte size = s->width;
	byte siz = s->width -1;
	byte si = s->width>>1;
	int last_dir;
	//GET TILE POS
	s->tile_x = (s->pos_x+si)>>tsize;
	s->tile_y = (s->pos_y+si)>>tsize;
	tile_number = (LT_map_data[( s->tile_y <<8) + s->tile_x]-TILE_VRAM)>>6;
	tilecol_number = LT_map_collision[(((s->pos_y+si)>>tsize) <<8) + ((s->pos_x+si)>>tsize)];
	tilecol_number_down = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + ((s->pos_x+si)>>tsize)];
	
	LT_Collision.move = 0;
	LT_Collision.ground = 0;
	s->state = 0;
	//PREDEFINED GAME TYPES
	switch (LT_MODE){
	case 0:{//TOP
		LT_Collision.ground = s->ground = 1;
		if (LT_Keys[LT_UP]){	//UP
			LT_Collision.move = 1;
			LT_Player_Dir = 0;
			col_y = 0;
			y = (s->pos_y-1)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if ((tile_number_VR == 1) || (tile_number_VR == 5)) col_y = tile_number_VR;
			if ((tile_number_VL == 1) || (tile_number_VL == 5)) col_y = tile_number_VL;
			if (col_y == 0) s->pos_y--;
		} else if (LT_Keys[LT_DOWN]){//DOWN
			LT_Collision.move = 2;
			LT_Player_Dir = 0;
			col_y = 0;
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if ((tile_number_VR == 1) || (tile_number_VR == 5)) col_y = tile_number_VR;
			if ((tile_number_VL == 1) || (tile_number_VL == 5)) col_y = tile_number_VL;
			if (col_y == 0) s->pos_y++;
		}
		
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Collision.move = 3;
			LT_Player_Dir = 1;
			s->state = 3;
			col_x = 0;
			x = (s->pos_x-1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];	
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x--;
		} else if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Collision.move = 4;
			LT_Player_Dir = 2;
			s->state = 4;
			col_x = 0;
			x = (s->pos_x+size)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x++;
		}
	break;}
	case 1:{//PLATFORM
		if ((s->ground == 1) && (LT_Keys[LT_JUMP])) {s->ground = 0; s->jump_frame = 0; s->jump = 1;}
		if (s->jump == 1){//JUMP
			col_y = 0;
			y = (s->pos_y-1)>>tsize;
			if (LT_player_jump_pos[s->jump_frame] < 0){
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
			} else {
				int platform_y = 1+(((s->pos_y+size)>>tsize)<<4);
				y = (s->pos_y+size)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1;  			
				if (tile_number_VR == 2) col_y = 1;  
				if (tile_number_VL == 2) col_y = 1;
				if (tile_number_VR == 3) col_y = 2;  
				if (tile_number_VL == 3) col_y = 2;
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				end_col1:
				if ((col_y == 1)&&(s->pos_y+size > platform_y)) col_y = 0;
			}
			if (col_y == 0){
				s->pos_y += LT_player_jump_pos[s->jump_frame];
				s->jump_frame++;
				if (s->jump_frame == 60) {s->jump_frame = 0; s->jump = 2;}
			}
			if (col_y > 0){
				s->jump_frame = 0; 
				s->jump = 2;
			}
		}
		if (s->jump == 2){//DOWN
			int platform_y = 1+(((s->pos_y+size)>>tsize)<<4);
			col_y = 0;
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (tile_number_VR == 3) col_y = 2;  
			if (tile_number_VL == 3) col_y = 2; 
			if (tile_number_VR == 5) col_y = 5;  
			if (tile_number_VL == 5) col_y = 5;
			if ((col_y == 1)&&(s->pos_y+size > platform_y)) col_y = 0;
			if (col_y == 0) {s->ground = 0; s->pos_y+=2;}
			if (col_y > 0) s->ground = 1;
		}
		
		if(LT_Keys[LT_UP]){//CLIMB UP
			LT_Player_Dir = 0;
			LT_Collision.move = 1;
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				s->ground = 1;
				s->jump = 3;
				col_y = 0;
				y = (s->pos_y-1)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				if (col_y == 0) s->pos_y--;
			}
		}else if(LT_Keys[LT_DOWN]){//CLIMB DOWN
			LT_Player_Dir = 0;
			LT_Collision.move = 2;
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				s->jump = 3;
				s->ground = 1;
				col_y = 0;
				y = (s->pos_y+size)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
				if (tile_number_VR == 1) col_y = 1;
				if (tile_number_VL == 1) col_y = 1;
				if (tile_number_VR == 2) col_y = 1; 
				if (tile_number_VL == 2) col_y = 1;
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				if (col_y == 0) s->pos_y++;
				else s->climb = 0;
			}
		}
		if (s->jump == 3){
			if ((tilecol_number != 3) && (tilecol_number != 4)){//DISABLE CLIMB
				s->climb = 0;
				s->jump = 2;
			}
		}
		
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Player_Dir = 1;
			LT_Collision.move = 3;
			s->state = 3;
			col_x = 0;
			x = (s->pos_x-1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];	
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x--;
		} else if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Player_Dir = 2;
			LT_Collision.move = 4;
			s->state = 4;
			col_x = 0;
			x = (s->pos_x+size)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x++;
		} 
		LT_Collision.ground = s->ground;
	
	break;}	
	case 2:{//TOP DOWN PHYSICS
		LT_Collision.ground = s->ground = 1;
		tile_number_VR = 0;
		tile_number_VL = 0;
		tile_number_HU = 0;
		tile_number_HD = 0;
		if (LT_Spr_speed_float == 4) LT_Spr_speed_float = 0;
		col_y = 0; col_x = 0;
		//UP
		if (LT_Keys[LT_UP]){ LT_Collision.move = 1; if (s->speed_y>8) s->speed_y-=8;}
		//DOWN
		else if (LT_Keys[LT_DOWN]){ LT_Collision.move = 2; if (s->speed_y < 8*32) s->speed_y+=8;}
		//LEFT
		if (LT_Keys[LT_LEFT]){ LT_Collision.move = 3;s->state = 3; if (s->speed_x>8) s->speed_x-=8;}
		//RIGHT
		else if (LT_Keys[LT_RIGHT]){ LT_Collision.move = 4;s->state = 4; if (s->speed_x <8*32) s->speed_x+=8;}
		
		//Tile physics
		if(LT_Spr_speed_float&1){
			switch (tilecol_number){
				case 0://Flat tile, friction
					if (s->speed_y < 128) s->speed_y+=4;
					if (s->speed_y > 128) s->speed_y-=4;
					if (s->speed_x < 128) s->speed_x+=4;
					if (s->speed_x > 128) s->speed_x-=4;
				break;
				//breakable block
				case 5:  break;
				//UP
				case 6: if (s->speed_y>16) s->speed_y-=16; break;
				//DOWN
				case 7: if (s->speed_y<8*30) s->speed_y+=16;break;
				//LEFT
				case 8: if (s->speed_x>16) s->speed_x-=16;break;
				//RIGHT
				case 9: if (s->speed_x<8*30) s->speed_x+=16;break;
			}
		}
		
		if (s->speed_x < 128){
			x = (s->pos_x)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if ((tile_number_HU==1) || (tile_number_HD==1)) s->speed_x = 8*21;
		} else if (s->speed_x > 128){
			x = (s->pos_x+size-1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if ((tile_number_HU==1) || (tile_number_HD==1)) s->speed_x = 8*11;
		}
		if (s->speed_y < 128){
			y = (s->pos_y-1)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if ((tile_number_VR==1) || (tile_number_VL==1)) s->speed_y = 8*21;
		} else if (s->speed_y > 128){
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if ((tile_number_VR==1) || (tile_number_VL==1)) s->speed_y = 8*11;
		}
		
		s->pos_x += (LT_Spr_speed[s->speed_x+LT_Spr_speed_float]);
		s->pos_y += (LT_Spr_speed[s->speed_y+LT_Spr_speed_float]);
		
		LT_Spr_speed_float++;
	break;}
	case 3:{//SIDESCROLL
		LT_Collision.ground = s->ground = 1;
		if (LT_Keys[LT_UP]){	//UP
			LT_Collision.move = 1;
			col_y = 0;
			y = (s->pos_y-1)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			LT_Collision.move = 2;
			col_y = 0;
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Collision.move = 3;
			col_x = 0;
			x = (s->pos_x-1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x-=2;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Collision.move = 4;
			col_x = 0;
			x = (s->pos_x+size)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
		col_x = 0;
		tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + ((s->pos_x+size)>>tsize)];
		tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + ((s->pos_x+size)>>tsize)];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x++;
		if (s->pos_x < SCR_X+3) s->pos_x = SCR_X+3;
	break;}
	};
	if (s->jump_frame == 1) LT_Collision.jump = 1;
	else LT_Collision.jump = 0;
	LT_Collision.tile_number = tile_number;
	LT_Collision.tilecol_number = tilecol_number;
	LT_Collision.col_x = col_x;
	LT_Collision.col_y = col_y;
	
	//Fixed animations
	if (LT_Collision.ground){
		switch (LT_Collision.move){
			case LT_SPR_UP: LT_Set_Sprite_Animation(sprite_number,0); break;
			case LT_SPR_DOWN: LT_Set_Sprite_Animation(sprite_number,1); break;
			case LT_SPR_LEFT: LT_Set_Sprite_Animation(sprite_number,2); break;
			case LT_SPR_RIGHT: LT_Set_Sprite_Animation(sprite_number,3); break;
			case LT_SPR_STOP:
				if (LT_Player_Dir == 1) s->frame = s->animations[16];
				if (LT_Player_Dir == 2) s->frame = s->animations[24];
				LT_Sprite_Stop_Animation(sprite_number);
			break;
		}
	} else {
		switch (LT_Player_Dir){
			case 1: LT_Set_Sprite_Animation(sprite_number,4); break;
			case 2: LT_Set_Sprite_Animation(sprite_number,5); break;
		}
	}
	
	return LT_Collision;
}

void LT_Update_AI_Sprites(){
	int i = 0;
	int motion = 0;
	if (LT_AI_Enabled){
	//for (sprite_number = LT_Sprite_Stack; sprite_number > 1; sprite_number--){
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		int px = s->pos_x;
		int py = s->pos_y;
		//CALCULATE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((px > SCR_X)&&(px < (SCR_X+320))&&(py > 16)&&(py < 16*18)){	
			byte col_x = 0;
			byte col_y = 0;
			int x,y;
			int sx = s->mspeed_x;
			int sy = s->mspeed_y;
			byte siz = s->width+1;
			byte si = s->width>>1;
			
			switch (s->mode){
			case 0:{ //WALKS ON TOP VIEW
				y = (py+si>>4);
				switch (sx){
					case -1:	//LEFT
					motion = 3;
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[( y <<8) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->speed_x = 0; s->speed_y = 3; s->mspeed_x = 0;s->mspeed_y = -1;}
					break;
					case 1:		//RIGHT
					motion = 4;
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[( y <<8) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->speed_x = 0; s->speed_y = 3; s->mspeed_x = 0; s->mspeed_y = 1;}
					break;
				}
				x = (px+si)>>4;
				switch (sy){
					case -1:	//UP
					motion = 1;
					col_y = 0;
					y = (py-1)>>4;
					tile_number_VR = LT_map_collision[( y <<8) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->speed_x = 3; s->speed_y = 0;s->mspeed_y = 0; s->mspeed_x = 1;}
					break;
					case 1:		//DOWN
					motion = 2;
					col_y = 0;
					y = (py+siz)>>4;
					tile_number_VR = LT_map_collision[( y <<8) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->speed_x = 3; s->speed_y = 0;s->mspeed_y = 0; s->mspeed_x = -1;}
					break;
				}
			break;}
			case 1:{ //ONLY WALKS ON PLATFORMS UNTILL IT REACHES EDGES OR SOLID TILES
				switch (sx){
					case -1:	//LEFT
					motion = 3;
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[(((py+si)>>4) <<8) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) <<8) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1;  //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
					case 1:		//RIGHT
					motion = 4;
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[(((py+si>>4)) <<8) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) <<8) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1; //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
				}
			break;}
			case 3:{ //Only flies to the left
				s->speed_x = 1;
				s->mspeed_x = -1;
			break;}
			}
			col_x = 0;
			col_y = 0;
			
			switch (motion){
				case 1: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],0); break;
				case 2: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],1); break;
				case 3: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],2); break;
				case 4: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],3); break;
			}
			//SPEED CONTROL TO MAKE ENEMIES TO MOVE SLOWER THAN +=1
			if (s->speed_x != 0){
				if (s->s_x == s->speed_x) {
					s->s_x = 0;
					s->pos_x += s->mspeed_x;
				}
				s->s_x ++;
			}
			if (s->speed_y != 0){
				if (s->s_y == s->speed_y) {
					s->s_y = 0;
					s->pos_y += s->mspeed_y;
				}
				s->s_y ++;
			}
		}
	}
	}
}

int LT_Player_Col_Enemy(){
	unsigned char i = 0;
	int col = 0;
	int ex = 0;
	int ey = 0;
	SPRITE *p = &sprite[LT_Sprite_Stack_Table[0]];
	int px = p->pos_x;
	int py = p->pos_y;
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		ex = s->pos_x;
		ey = s->pos_y;
		if ((abs(px-ex)<16)&&(abs(py-ey)<16)) col = 1;
	}
	return col;
}

void LT_Unload_Sprites(){
	int i = 0;
	for (i=0;i<20;i++){
		SPRITE *s = &sprite[i];
		s->init = 0;
		s->code_size = 0;
	}
	LT_AI_Enabled = 0;
	LT_sprite_data_offset = 16*1024; //just after loading animation
	//LT_Delete_Sprite(sprite_number);
	/*for (i=0;i<s->nframes;i++){
		farfree(s->frames[i].compiled_code);
		s->frames[i].compiled_code = NULL;
	}*/
}	


void (*LT_Draw_Sprites)(void);