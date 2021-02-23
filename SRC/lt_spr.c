/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program.                     
	Remember to compile in the LARGE memory model!                        

	Please feel free to copy this source code.                            

	Used:	K1n9_Duk3's IMF Player - A simple IMF player for DOS
			Copyright (C) 2013-2016 K1n9_Duk3

			Based on the Apogee Sound System (ASS) and Wolfenstein 3-D (W3D)

			ASS is Copyright (C) 1994-1995 Apogee Software, Ltd.
			W3D is Copyright (C) 1992 Id Software, Inc.
	
##########################################################################*/

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
byte LT_AI_Stack = 0;
byte LT_AI_Stack_Table[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

byte Lt_AI_Sprite[] = {0,0,0,0,0,0,0,0};
byte selected_AI_sprite;
//NON SCROLLING WINDOW
extern byte LT_Window; 				//Displaces everything (16 pixels) in case yo use the 320x16 window in the game
unsigned char *LT_sprite_data; 
dword LT_sprite_data_offset = 0; 
void LT_Error(char *error, char *file);

//player movement modes
//MODE TOP = 0;
//MODE PLATFORM = 1;
//MODE PUZZLE = 2;
//MODE SIDESCROLL = 3;
byte LT_MODE = 0;

byte EGA_SPR_TEST[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int LT_player_jump_pos[] = {
	-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

//FUNCTIONS IN SPRC.ASM
//int x_compile_bitmap(word logical_screen_width, char *bitmap, char *output);       

void LT_scroll_follow(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	//LIMITS
	int x_limL = SCR_X + 120;
	int x_limR = SCR_X + 200;
	int y_limU = SCR_Y + 80;
	int y_limD = SCR_Y + 140;
	int wmap = LT_map_width<<4;
	int hmap = LT_map_height<<4;
	int x = abs(s->last_x - s->pos_x);
	int y = abs(s->last_y - (s->pos_y+LT_Window));
	int px = s->pos_x;
	int py = s->pos_y+LT_Window;
	//clamp limits
	if ((SCR_X > -1) && ((SCR_X+319)<wmap) && (SCR_Y > -1) && ((SCR_Y+239)<hmap)){
		if (py > y_limD) SCR_Y+=y;
		if (py < y_limU) SCR_Y-=y;
		if (px < x_limL) SCR_X-=x;
		if (px > x_limR) SCR_X+=x;
	}
	if (SCR_X < 0) SCR_X = 0; 
	if ((SCR_X+320) > wmap) SCR_X = wmap-320;
	if (SCR_Y < 0) SCR_Y = 0; 
	if ((SCR_Y+240) > hmap) SCR_Y = hmap-240; 
}


word ROL_AL = 0xc0d0;			//rol al
word SHORT_STORE_8 = 0x44c6;	//mov [si]+disp8,  imm8
word STORE_8 = 0x84c6;			//mov [si]+disp16, imm8
word SHORT_STORE_16 = 0x44c7;	//mov [si]+disp8,  imm16
word STORE_16 = 0x84c7;			//mov [si]+disp16, imm16
word ADC_SI_IMMED = 0xd683;		//adc si,imm8
byte OUT_AL = 0xee;				//out dx,al
byte RETURN = 0xcb;				//ret

int compile_bitmap(word logical_width, char *bitmap, char *output){
	word bwidth,input_size;
	word scanx = 0;
	word scany = 0;
	word outputx = 0;
	word outputy = 0;
	word column = 0;
	word set_column = 0;
	//word size = 0;
	
	asm{
	push ds

	lds si,[bitmap]     // 32-bit pointer to source bitmap

	les di,[output]     // 32-bit pointer to destination stream

	lodsb               // load width byte
	xor ah, ah          // convert to word
	mov [bwidth], ax    // save for future reference
	mov bl, al          // copy width byte to bl
	lodsb               // load height byte -- already a word since ah=0
	mul bl              // mult height word by width byte
	mov [input_size], ax//  to get pixel total 
	}
MainLoop:
	asm{
	mov bx, [scanx]     //position in original bitmap
	add bx, [scany]

	mov al, [si+bx]     //get pixel
	or  al, al          //skip empty pixels
	jnz NoAdvance
	jmp Advance
	}
NoAdvance:
	asm{
	mov dx, [set_column]
	cmp dx, [column]
	je SameColumn
	}
ColumnLoop:
	asm{
	mov word ptr es:[di],0c0d0h// emit code to move to new column
	add di,2
	mov word ptr es:[di],0d683h
	add di,2
	mov byte ptr es:[di],0
	inc di

	inc dx
	cmp dx, [column]
	jl ColumnLoop

	mov byte ptr es:[di],0eeh
	inc di						// emit code to set VGA mask for new column
	
	mov [set_column], dx
	}
SameColumn:
	asm{
	mov dx, [outputy]   // calculate output position
	add dx, [outputx]
	sub dx, 128

	add word ptr [scanx], 4
	mov cx, [scanx]     // within four pixels of right edge?
	cmp cx, [bwidth]
	jge OnePixel

	inc word ptr [outputx]
	mov ah, [si+bx+4]   // get second pixel
	or ah, ah
	jnz TwoPixels
	}
OnePixel:
	asm{
	cmp dx, 127         // can we use shorter form?
	jg OnePixLarge
	cmp dx, -128
	jl OnePixLarge
	mov word ptr es:[di],084c6h
	add di,2
	
	mov byte ptr es:[di],dl
	inc di				// 8-bit position in output
	
	jmp EmitOnePixel
	}
OnePixLarge:
	asm{
	mov word ptr es:[di],084c6h
	add di,2
	mov word ptr es:[di],dx
	add di,2 			//position in output
	}
EmitOnePixel:
	asm mov byte ptr es:[di],al
	asm inc di
	asm jmp short Advance
TwoPixels:
	asm{
	cmp dx, 127
	jg TwoPixLarge
	cmp dx, -128
	jl TwoPixLarge
	mov word ptr es:[di],044c7h
	add di,2
	mov byte ptr es:[di],dl
	inc di            	// 8-bit position in output
	jmp EmitTwoPixels
	}
TwoPixLarge:
	asm mov word ptr es:[di],084c7h
	asm add di,2
	asm mov word ptr es:[di],dx
	asm add di,2 		// position in output
EmitTwoPixels:
	asm mov word ptr es:[di],ax
	asm add di,2

Advance:
	asm{
	inc word ptr [outputx]
	mov ax, [scanx]
	add ax, 4
	cmp ax, [bwidth]
	jl AdvanceDone
	mov dx, [outputy]
	add dx, [logical_width]
	mov cx, [scany]
	add cx, [bwidth]
	cmp cx, [input_size]
	jl NoNewColumn
	inc word ptr [column]
	mov cx, [column]
	cmp cx, 4
	je Exit           // Column 4: there is no column 4.
	xor cx, cx          // scany and outputy are 0 again for
	mov dx, cx          // the new column
	}
NoNewColumn:
	asm mov [outputy], dx
	asm mov [scany], cx
	asm mov word ptr [outputx], 0
	asm mov ax,[column]
AdvanceDone:
	asm mov [scanx], ax
	asm jmp MainLoop

Exit:
	asm mov byte ptr es:[di],0cbh
	asm inc di
	asm mov ax,di
	asm sub ax,word ptr [output] // size of generated code
	
	//asm mov	word ptr size,ax
	
	asm pop ds
	//asm mov sp, bp
	//asm pop bp
	return ;

}

//load sprites with transparency (size = 8,16,32,64!)
void LT_Load_Sprite(char *file, int sprite_number, byte size){
	SPRITE *s = &sprite[sprite_number];
	long index,offset;
	word num_colors;
	word x,y;
	word i,j;
	word frame = 0;
	word fsize = 0;
	byte tileX;
	byte tileY;
	
	FILE *fp;

    int code_size;
	
	fp = fopen(file,"rb");
	
	if(!fp) LT_Error("Can't find ",file);
	
	fseek(fp, 2, SEEK_CUR);

	fseek(fp, 16, SEEK_CUR);
	fread(&s->width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&s->height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;

	for(index=0;index<num_colors;index++){
		s->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		x=fgetc(fp);
	}

	for(index=(s->height-1)*s->width;index>=0;index-=s->width){
		for(x=0;x<s->width;x++){
			LT_tile_tempdata[index+x]=(byte)fgetc(fp);
		}
	}
	fclose(fp);
	
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
		code_size = x_compile_bitmap(84, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],&LT_sprite_data[LT_sprite_data_offset]);
		s->frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
		LT_sprite_data_offset += code_size;
		s->code_size += code_size;
		if (LT_sprite_data_offset > 65536-8192) LT_Error("Not enough RAM to allocate frames ",file);
		//if (code_size == NULL) LT_Error("Not enough RAM to allocate frames ",file);
		//s->frames[frame].compiled_code = farrealloc(s->frames[frame].compiled_code, code_size);
		//Store the compiled data at it's final destination
	}
	
	//IINIT SPRITE
	//s->bkg_data set in init function (sys.c)
	s->width = size;
	s->height = size;
	s->init = 0;
	s->frame = 0;
	s->baseframe = 0;
	s->ground = 0;
	s->jump = 2;
	s->jump_frame = 0;
	s->climb = 0;
	s->animate = 0;
	s->anim_speed = 0;
	s->anim_counter = 0;
	s->speed_x = 0;
	s->speed_y = 0;
	s->s_x = 0;
	s->s_y = 0;
	s->fpos_x = 0;
	s->fpos_y = 0;
	s->s_delete = 0;
	s->misc = 0;
	s->mspeed_x = 1;
	s->mspeed_y = 1;
	s->size = s->height>>3;
	if (LT_VIDEO_MODE == 0){
		s->siz = s->size + 1;
		s->next_scanline = 42 - s->siz;
	}else{
		s->siz = (s->height>>2) + 1;
		s->next_scanline = 84 - s->siz;
	}
	s->get_item = 0;
	s->mode = 0;
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
	c->init = 0;
	c->frame = 0;
	c->baseframe = 0;
	c->ground = 0;
	c->jump = 2;
	c->jump_frame = 0;
	c->climb = 0;
	c->animate = 0;
	c->anim_speed = 0;
	c->anim_counter = 0;
	c->mspeed_x = 1;
	c->mspeed_y = 1;
	c->speed_x = 0;
	c->speed_y = 0;
	c->s_x = 0;
	c->s_y = 0;
	c->fpos_x = 0;
	c->fpos_y = 0;
	c->s_delete = 0;
	c->misc = 0;
	c->size = c->height>>3;
	c->siz = (c->height>>2) + 1;
	c->next_scanline = 84 - c->siz;
	c->get_item = 0;
	c->mode = 0;
}

void LT_Init_Sprite(int sprite_number,int x,int y){
	LT_Sprite_Stack_Table[LT_Sprite_Stack] = sprite_number;
	sprite[sprite_number].pos_x = x;
	sprite[sprite_number].pos_y = y;
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
void LT_Reset_AI_Stack(){
	int i;
	LT_AI_Stack = 0;
	for (i = 0; i<33; i++) LT_AI_Stack_Table[i] = 0;
}

void LT_Set_Sprite_Animation(int sprite_number, byte baseframe, byte frames, byte speed){
	SPRITE *s = &sprite[sprite_number];
	if (s->anim_counter > frames) s->anim_counter = 0;
	if (s->anim_speed > s->speed) s->anim_speed = 0;
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void (*LT_Delete_Sprite)(int);

void LT_Delete_Sprite_VGA(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	word bkg_data = s->bkg_data;
	word screen_offset0 = (s->last_y<<6)+(s->last_y<<4)+(s->last_y<<2)+(s->last_x>>2);
	word size = s->height>>3;
	word siz = (s->height>>2)+1;
	
	word next_scanline = 84 - siz;
	
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
	}

	asm pop si
	asm pop di
	asm pop ds	
	
	outport(GC_INDEX + 1, 0x0ff);
	
	s->init = 0;
}

void LT_Delete_Sprite_EGA(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	word bkg_data = s->bkg_data;
	word screen_offset0 = (s->last_y<<5)+(s->last_y<<3)+(s->last_y<<1)+(s->last_x>>3);
	word size = s->size;
	word siz = s->siz;
	word next_scanline = 42 - siz;
	
	///paste bkg chunk
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
		add 	di,bx
		dec 	ax
		jnz		bkg_scanline1
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

void Wait_Scanline(){
	asm mov		cx,128
	linecount:
	asm mov		dx,003dah
	WaitNotVsync:
	asm in      al,dx
	asm test    al,01h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,01h
	asm jz		WaitVsync
	asm	dec		cx
	asm jnz linecount
}

unsigned char imagebuff[12] = {
	0x55,//01010101
	0x33,//00110011
	0x0F,//00001111
	0x91,//10010001
	0x55,//01010101
	0x33,//00110011
	0x0F,//00001111
	0x91,//10010001
	0x55,//01010101
	0x33,//00110011
	0x0F,//00001111
	0x91,//10010001
};

void (*LT_Draw_Sprites)(void);

void LT_Draw_Sprites_EGA(){
	int sprite_number;
	char *bitmap = &imagebuff[0];
	//RESTORE SPRITE BKG
	for (sprite_number = LT_Sprite_Stack; sprite_number > -1; sprite_number--){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
	
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(y > SCR_Y)&&(y < (SCR_Y+224))){
		int lx = s->last_x;
		int ly = s->last_y;
		word bkg_data = s->bkg_data;
		word screen_offset0 = (ly<<5)+(ly<<3)+(ly<<1)+(lx>>3);
		word init = s->init;
		word size = s->size;
		word siz = s->siz;
		word next_scanline = s->next_scanline;
		
		// Wait 40 horizontal retraces 
		//Wait_Scanline();
		
		///restore destroyed bkg chunk in last frame
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
			
			cmp	byte ptr [init],1 //if (sprite.init == 1)
			jne	short rle_noinite
			
			mov 	ax,0A000h
			mov 	es,ax						
			mov		di,screen_offset0	//es:di destination vram				
			
			mov		ds,ax						
			mov		si,bkg_data			//ds:si source vram				
			mov 	ax,size
			mov		bx,next_scanline
			mov		dx,siz
		}
		bkg_scanlineg1:	
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
			jnz		bkg_scanlineg1
		}
		rle_noinite:
		s->init = 1;
		
		asm mov dx,GC_INDEX +1 //dx = indexregister
		asm mov ax,00ffh		
		asm out dx,ax 
		
		asm pop si
		asm pop di
		asm pop ds
		}
		
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}
	}
	
	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+224))){
			word bkg_data = s->bkg_data;
			word screen_offset1 = (y<<5)+(y<<3)+(y<<1)+(x>>3);
			word size = s->size;
			word siz = s->siz;
			//word sizz = 42 - size;
			word next_scanline = s->next_scanline;
			
			//animation
			if (s->animate == 1){
				s->frame = s->baseframe + s->anim_counter;
				if (s->anim_speed == s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->aframes) s->anim_counter = 0;
				}
				s->anim_speed++;
			}
			
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
			bkg_scanlineg2:	
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
				jnz		bkg_scanlineg2
				
				mov dx,GC_INDEX +1 //dx = indexregister
				mov ax,00ffh		
				out dx,ax 
			}
			

			//draw EGA sprite and destroy bkg
			asm{
				
				lds	si,[bitmap]
				mov	ax,0A000h
				mov	es,ax						
				mov	di,screen_offset1	//ES:DI destination vram
				//Test 16x16 sprite
				mov	cx,16	//16 scanlines
			}
				drawsprit:
				asm{
					//Draw ega byte, really all this to draw 8 pixels?
					mov	al,es:[di]			//load latches from VRAM destination es:di
					mov	al,01100110b		//setup bit mask
					mov	dx,03CEH+1       		
					out	dx,al				//al = mask from sprite data
					
					mov	dx,03C4H+1
					mov	al,01h
					out	dx,al				//select plane 0 DX = 03C4H+1
					movsb					//DS:SI to ES:DI, inc SI and DI.
					dec 	di
					mov	al,02h
					out	dx,al				//select plane 1
					movsb
					dec	di
					mov	al,04h
					out	dx,al				//select plane 2
					movsb
					dec	di
					mov	al,08h
					out	dx,al				//select plane 3
					movsb					//Inc SI, DI
					
					//Draw next ega byte
					mov	al,es:[di]			
					mov	al,01100110b
					mov	dx,03CEH+1       		
					out	dx,al
					
					mov	dx,03C4H+1
					mov	al,01h
					out	dx,al				
					movsb					
					dec 	di
					mov	al,02h
					out	dx,al				
					movsb
					dec	di
					mov	al,04h
					out	dx,al				
					movsb
					dec	di
					mov	al,08h
					out	dx,al				
					movsb
					
					//Draw next ega byte
					mov	al,es:[di]			
					mov	al,01100110b
					mov	dx,03CEH+1       		
					out	dx,al
					
					mov	dx,03C4H+1
					mov	al,01h
					out	dx,al				
					movsb					
					dec 	di
					mov	al,02h
					out	dx,al				
					movsb
					dec	di
					mov	al,04h
					out	dx,al				
					movsb
					dec	di
					mov	al,08h
					out	dx,al				
					movsb
					
					sub	si,12				//Go back to test array
					add di,42-3				//Next scanline
					
					loop	drawsprit		
				
				pop si
				pop di
				pop ds
			}
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {
			LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]);
			s->s_delete = 0;
			//LT_Unset_AI_Sprite(LT_Sprite_Stack_Table[sprite_number]);
		}
		if ((x < SCR_X-32)||(x > SCR_X+352)||(y < SCR_Y-32)||(y > SCR_Y+272)){
			memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
			LT_Sprite_Stack--;
		}
		
		s->last_x = x;
		s->last_y = y;
	}	
} 
	
void LT_Draw_Sprites_VGA(){
	int sprite_number;
	//RESTORE SPRITE BKG
	for (sprite_number = LT_Sprite_Stack; sprite_number > -1; sprite_number--){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
	
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(y > SCR_Y)&&(y < (SCR_Y+224))){
		int lx = s->last_x;
		int ly = s->last_y;
		word bkg_data = s->bkg_data;
		word screen_offset0 = (ly<<6)+(ly<<4)+(ly<<2)+(lx>>2);
		word init = s->init;
		word size = s->size;
		word siz = s->siz;
		word next_scanline = s->next_scanline;
		
		///restore destroyed bkg chunk in last frame
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
			
			cmp	byte ptr [init],1 //if (sprite.init == 1)
			jne	short rle_noinit
			
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
		}
		rle_noinit:
		s->init = 1;
		
		asm mov dx,GC_INDEX +1 //dx = indexregister
		asm mov ax,00ffh		
		asm out dx,ax 
		
		asm pop si
		asm pop di
		asm pop ds	
		}
		
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
			sb_play_sample(3,11025);
		}
	}
	
	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+224))){
			word bkg_data = s->bkg_data;
			word screen_offset1 = (y<<6)+(y<<4)+(y<<2)+(x>>2);
			word size = s->size;
			word siz = s->siz;
			word next_scanline = s->next_scanline;
			
			//animation
			if (s->animate == 1){
				s->frame = s->baseframe + s->anim_counter;
				if (s->anim_speed == s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->aframes) s->anim_counter = 0;
				}
				s->anim_speed++;
			}
			
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
			//draw sprite and destroy bkg
			run_compiled_sprite(x,y,s->frames[s->frame].compiled_code);
			
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {
			LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]);
			s->s_delete = 0;
			//LT_Unset_AI_Sprite(LT_Sprite_Stack_Table[sprite_number]);
		}
		if ((x < SCR_X-32)||(x > SCR_X+352)){
			memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
			LT_Sprite_Stack--;
		} else if ((y < SCR_Y-32)||(y > SCR_Y+272)){
			memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
			LT_Sprite_Stack--;
		}
		s->last_x = x;
		s->last_y = y;
	}
}


//This method is faster and gets rid of sprite flicker, but it comes at a cost.
//It does not restore bkg, so sprite transparent parts must have solid colours(do not use palette 
//colour 0), And the sprite can only move on top of solid colour backgrounds.
//Using this function, an 8088 can draw more sprites without flickering, and an 8086 can use Huge
//64x64 sprites
void LT_Draw_Sprites_Fast(){
	int sprite_number;
	
	if (LT_VIDEO_MODE == 0){
	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+224))){
			word screen_offset1 = (y<<5)+(y<<3)+(y<<1)+(x>>3);
			//animation
			if (s->animate == 1){
				s->frame = s->baseframe + s->anim_counter;
				if (s->anim_speed == s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->aframes) s->anim_counter = 0;
				}
				s->anim_speed++;
			}
			
			//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
			if (s->get_item == 1){
				LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
				s->get_item = 0;
			}
			//draw sprite and destroy bkg
			asm{// source ds:si
				mov 	ax,0A000h
				mov 	es,ax
				mov		di,screen_offset1 //es:di screen address
		
				mov		byte ptr es:[di],00000000b
				mov		byte ptr es:[di+1],00000000b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00111111b
				mov		byte ptr es:[di+1],11111100b
				add		di,42
				mov		byte ptr es:[di],00000000b
				mov		byte ptr es:[di+1],00000000b
				add		di,42
				mov		byte ptr es:[di],00000000b
				mov		byte ptr es:[di+1],00000000b
				add		di,42
			}
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]); s->s_delete = 0;}
		s->last_x = x;
		s->last_y = y;
	}
	}
	else
	{	
	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+LT_Window;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X)&&(x < (SCR_X+304))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+224))){
			//animation
			if (s->animate == 1){
				s->frame = s->baseframe + s->anim_counter;
				if (s->anim_speed == s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->aframes) s->anim_counter = 0;
				}
				s->anim_speed++;
			}
			
			//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
			if (s->get_item == 1){
				LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
				s->get_item = 0;
			}
			//draw sprite and destroy bkg
			run_compiled_sprite(x,y,s->frames[s->frame].compiled_code);
			
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {LT_Delete_Sprite(LT_Sprite_Stack_Table[sprite_number]); s->s_delete = 0;}
		s->last_x = x;
		s->last_y = y;
	}
	}
}

void LT_Set_AI_Sprite(byte sprite_number, byte mode, word x, word y, int sx, int sy){
	//int i = 0;
	SPRITE *s = &sprite[sprite_number];
	s->pos_x = x<<4;
	s->pos_y = y<<4;
	s->speed_x = sx;
	s->speed_y = sy;
	s->mode = mode;
	
	//LT_AI_Stack_Table[0] = sprite_number;
	//LT_AI_Stack = 1;
	/*while (i != 16){
		if (LT_AI_Stack_Table[i] == 0){
			LT_AI_Stack_Table[LT_AI_Stack] = sprite_number;
			LT_AI_Stack++;
			i= 16;
		}
		if (i<16)i++;
	}*/
}
extern int Enemies;
void LT_Unset_AI_Sprite(byte sprite_number){
	//SPRITE *s = &sprite[sprite_number];
	memcpy(&LT_AI_Stack_Table[sprite_number],&LT_AI_Stack_Table[sprite_number+1],8);
	LT_AI_Stack--;
	//Enemies--;
	/*while (i != 16){
		if (LT_AI_Stack_Table[i] == 0){
			LT_AI_Stack_Table[LT_AI_Stack] = sprite_number;
			LT_AI_Stack++;
			i= 16;
		}
		if (i<16)i++;
	}*/
}

void LT_Set_AI_Sprites(byte first_ai, byte number_ai){
	int i;
	LT_AI_Sprite[0] = first_ai;
	for (i = first_ai+1; i < (first_ai+1)+number_ai; i++) LT_Clone_Sprite(i,first_ai);
	//for (i = first_ai+1; i < number_ai; i++) LT_Clone_Sprite(i,first_ai);
}

LT_Col LT_move_player(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	LT_Col LT_Collision;
	byte col_x = 0;
	byte col_y = 0;
	int x,y,platform_y;
	byte size = s->width;
	byte siz = s->width -1;
	byte si = s->width>>1;
	
	//GET TILE POS
	s->tile_x = (s->pos_x+si)>>4;
	s->tile_y = (s->pos_y+si)>>4;
	tile_number = LT_map_data[( s->tile_y * LT_map_width) + s->tile_x];
	tilecol_number = LT_map_collision[(((s->pos_y+si)>>4) * LT_map_width) + ((s->pos_x+si)>>4)];
	tilecol_number_down = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + ((s->pos_x+si)>>4)];
	
	//PREDEFINED GAME TYPES
	switch (LT_MODE){
	case 0:{//TOP
		if (LT_Keys[LT_UP]){	//UP
			col_y = 0;
			y = (s->pos_y-1)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			col_y = 0;
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x--;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
	break;}
	case 1:{//PLATFORM
		if ((s->ground == 1) && (LT_Keys[LT_JUMP])) {s->ground = 0; s->jump_frame = 0; s->jump = 1;sb_play_sample(4,11025);}
		if (s->jump == 1){//JUMP
			col_y = 0;
			y = (s->pos_y-1)>>4;
			if (LT_player_jump_pos[s->jump_frame] < 0){
				tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
			} else {
				int platform_y = 1+(((s->pos_y+size)>>4)<<4);
				y = (s->pos_y+size)>>4;
				tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1;  			
				if (tile_number_VR == 2) col_y = 1;  
				if (tile_number_VL == 2) col_y = 1;
				if (tile_number_VR == 3) col_y = 2;  
				if (tile_number_VL == 3) col_y = 2;  
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
			int platform_y = 1+(((s->pos_y+size)>>4)<<4);
			col_y = 0;
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (tile_number_VR == 3) col_y = 2;  
			if (tile_number_VL == 3) col_y = 2;  
			if ((col_y == 1)&&(s->pos_y+size > platform_y)) col_y = 0;
			if (col_y == 0) {s->ground = 0; s->pos_y+=2;}
			if (col_y > 0) s->ground = 1;
		}
		
		if(LT_Keys[LT_UP]){//CLIMB UP
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				s->ground = 1;
				s->jump = 3;
				col_y = 0;
				y = (s->pos_y-1)>>4;
				tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
				if (col_y == 0) s->pos_y--;
			}
		}
		if(LT_Keys[LT_DOWN]){//CLIMB DOWN
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				s->jump = 3;
				s->ground = 1;
				col_y = 0;
				y = (s->pos_y+size)>>4;
				tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1;
				if (tile_number_VL == 1) col_y = 1;
				if (tile_number_VR == 2) col_y = 1; 
				if (tile_number_VL == 2) col_y = 1;
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
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x--;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		} 
	
	break;}	
	case 2:{//PHYSICS 2 
		
		if (s->speed_y < 0){	//UP
			col_y = 0;
			y = (s->pos_y+2)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz-3)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + ((s->pos_x+4)>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 1) {s->speed_y *= -1;s->speed_y-=48;}
		}
		if (s->speed_y > 0){	//DOWN
			col_y = 0;
			y = (s->pos_y+size-2)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz-3)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + ((s->pos_x+4)>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 1) {s->speed_y *= -1;s->speed_y+=48;}
		}
		if (s->speed_x < 0){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map_collision[(((s->pos_y+4)>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-4)>>4) * LT_map_width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 1) {s->speed_x *= -1;s->speed_x-=48;}
		}
		if (s->speed_x > 0){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map_collision[(((s->pos_y+4>>4)) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-4)>>4) * LT_map_width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 1) {s->speed_x *= -1;s->speed_x+=48;}
		}
		if (tilecol_number == 0){ //FRICTION
			if (s->speed_x > 0) s->speed_x-=2;
			if (s->speed_x < 0) s->speed_x+=2;
			if (s->speed_y > 0) s->speed_y-=2;
			if (s->speed_y < 0) s->speed_y+=2;
			if ((s->speed_x < 2)&&(s->speed_y < 2)){
				s->state = 0; //Stopped
			}
		}
		col_x = 0;
		col_y = 0;
		//FORCE UP
		if (tilecol_number == 6)s->speed_y-=16;
		//FORCE DOWN
		if (tilecol_number == 7)s->speed_y+=16;
		//FORCE LEFT
		if (tilecol_number == 8)s->speed_x-=16;
		//FORCE RIGHT
		if (tilecol_number == 9)s->speed_x+=16;

		if (s->speed_x > 400) s->speed_x = 400;
		if (s->speed_x < -400) s->speed_x = -400;
		if (s->speed_y > 400) s->speed_y = 400;
		if (s->speed_y < -400) s->speed_y = -400;
		
		s->fpos_x += (float)s->speed_x/100;
		s->fpos_y += (float)s->speed_y/100;
		
		s->pos_x = s->fpos_x;
		s->pos_y = s->fpos_y;
	break;}
	case 3:{//SIDESCROLL
		if (LT_Keys[LT_UP]){	//UP
			col_y = 0;
			y = (s->pos_y-1)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			col_y = 0;
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map_collision[( y * LT_map_width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map_collision[( y * LT_map_width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x-=2;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
		col_x = 0;
		tile_number_HU = LT_map_collision[((s->pos_y>>4) * LT_map_width) + ((s->pos_x+size)>>4)];
		tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>4) * LT_map_width) + ((s->pos_x+size)>>4)];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x++;
		if (s->pos_x < SCR_X+3) s->pos_x = SCR_X+3;
	break;}
	};
	LT_Collision.tile_number = tile_number;
	LT_Collision.tilecol_number = tilecol_number;
	LT_Collision.col_x = col_x;
	LT_Collision.col_y = col_y;
	
	return LT_Collision;
}

LT_Col LT_Bounce_Ball(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	LT_Col LT_Collision;
	word col_x = 0;
	word col_y = 0;
	word size = s->width;
	word siz = s->width -1;
	int x,y;
	if (s->speed_y < 0){	//UP
		col_y = 0;
		x = (s->pos_x+siz)>>4;
		y = (s->pos_y-1)>>4;
		tile_number_VR = LT_map_collision[( y * LT_map_width) + x];
		tile_number_VL = LT_map_collision[( y * LT_map_width) + ((s->pos_x)>>4)];
		if (tile_number_VR == 1) col_y = 1;
		if (tile_number_VL == 1) col_y = 1;
		if (tile_number_VR == 5) {col_y = 1; LT_Collision.col_y = 5; LT_Edit_MapTile( x, y, 0, 0);}
		if (tile_number_VL == 5) {col_y = 1; LT_Collision.col_y = 5; LT_Edit_MapTile((s->pos_x)>>4, y, 0, 0);}
		if (col_y == 1) s->speed_y *= -1;
	}
	if (s->speed_y > 0){	//DOWN
		col_y = 0;
		x = (s->pos_x+siz)>>4;
		y = (s->pos_y+size)>>4;
		tile_number_VR = LT_map_collision[( y * LT_map_width) + x];
		tile_number_VL = LT_map_collision[( y * LT_map_width) + ((s->pos_x)>>4)];
		if (tile_number_VR == 1) col_y = 1;
		if (tile_number_VL == 1) col_y = 1;
		if (tile_number_VR == 5) {col_y = 1; LT_Collision.col_y = 5; LT_Edit_MapTile( x, y, 0, 0);}
		if (tile_number_VL == 5) {col_y = 1; LT_Collision.col_y = 5; LT_Edit_MapTile((s->pos_x)>>4,y, 0, 0);}
		if (col_y == 1) s->speed_y *= -1;
	}
	if (s->speed_x < 0){	//LEFT
		col_x = 0;
		x = (s->pos_x-1)>>4;
		y = (s->pos_y+siz)>>4;
		tile_number_HU = LT_map_collision[(((s->pos_y)>>4) * LT_map_width) + x];
		tile_number_HD = LT_map_collision[( y * LT_map_width) + x];	
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (tile_number_HU == 5) {col_x = 1; LT_Collision.col_x = 5; LT_Edit_MapTile( x, (s->pos_y)>>4, 0, 0);}
		if (tile_number_HD == 5) {col_x = 1; LT_Collision.col_x = 5; LT_Edit_MapTile( x, y, 0, 0);}
		if (col_x == 1) s->speed_x *= -1;
	}
	if (s->speed_x > 0){	//RIGHT
		col_x = 0;
		x = (s->pos_x+size)>>4;
		y = (s->pos_y+siz)>>4;
		tile_number_HU = LT_map_collision[(((s->pos_y)>>4) * LT_map_width) + x];
		tile_number_HD = LT_map_collision[( y * LT_map_width) + x];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (tile_number_HU == 5) {col_x = 1; LT_Collision.col_x = 5; LT_Edit_MapTile( x, (s->pos_y)>>4, 0, 0);}
		if (tile_number_HD == 5) {col_x = 1; LT_Collision.col_x = 5; LT_Edit_MapTile( x, y, 0, 0);}
		if (col_x == 1) s->speed_x *= -1;
	}
	col_x = 0;
	col_y = 0;
	
	s->pos_x += s->speed_x;
	s->pos_y += s->speed_y;
	
	return LT_Collision;
}

void LT_Update_AI_Sprites(){
	int i = 0;
	//for (sprite_number = LT_Sprite_Stack; sprite_number > 1; sprite_number--){
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		int px = s->pos_x;
		int py = s->pos_y;
		//CALCULATE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((px > SCR_X)&&(px < (SCR_X+304))&&(py > SCR_Y)&&(py < (SCR_Y+224))){	
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
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[( y * LT_map_width) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->mspeed_x = 0;s->speed_y=4;s->mspeed_y = -1;}
					break;
					case 1:		//RIGHT
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[( y * LT_map_width) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->mspeed_x = 0;s->speed_y=4; s->mspeed_y = 1;}
					break;
				}
				x = (px+si)>>4;
				switch (sy){
					case -1:	//UP
					col_y = 0;
					y = (py-1)>>4;
					tile_number_VR = LT_map_collision[( y * LT_map_width) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->mspeed_y = 0; s->speed_x=4;s->mspeed_x = 1;}
					break;
					case 1:		//DOWN
					col_y = 0;
					y = (py+siz)>>4;
					tile_number_VR = LT_map_collision[( y * LT_map_width) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->mspeed_y = 0; s->speed_x=4; s->mspeed_x = -1;}
					break;
				}
			break;}
			case 1:{ //ONLY WALKS ON PLATFORMS UNTILL IT REACHES EDGES OR SOLID TILES
				switch (sx){
					case -1:	//LEFT
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[(((py+si)>>4) * LT_map_width) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) * LT_map_width) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1;  //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
					case 1:		//RIGHT
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[(((py+si>>4)) * LT_map_width) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) * LT_map_width) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1; //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
				}
			break;}
			}
			col_x = 0;
			col_y = 0;
			
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

int LT_Player_Col_Enemy(){
	int i = 0;
	int col = 0;
	SPRITE *p = &sprite[LT_Sprite_Stack_Table[0]];
	int px = p->pos_x;
	int py = p->pos_y;
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		int ex = s->pos_x;
		int ey = s->pos_y;
		//CALCULATE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((ex > SCR_X)&&(ex < (SCR_X+304))&&(ey > SCR_Y)&&(ey < (SCR_Y+224))){	
			if ((abs(px-ex)<16)&&(abs(py-ey)<16)) col = 1;
			else col = 0;
		}
	}
	return col;
}


void LT_unload_sprite(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int i;
	s->init = 0;
	LT_sprite_data_offset -= s->code_size;
	s->code_size = 0;
	//LT_Delete_Sprite(sprite_number);
	/*for (i=0;i<s->nframes;i++){
		farfree(s->frames[i].compiled_code);
		s->frames[i].compiled_code = NULL;
	}*/
}	

