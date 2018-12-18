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
byte tile_number_VR = 0;	//Tile vertical right
byte tile_number_VL = 0;	//Tile vertical left
byte tile_number_HU = 0;	//Tile horizontal up
byte tile_number_HD = 0;	//Tile horizontal down

byte LT_SpriteStack = 0;

//NON SCROLLING WINDOW
extern byte LT_Window; 				//Displaces everything (16 pixels) in case yo use the 320x16 window in the game

void LT_Error(char *error, char *file);

//player movement modes
//MODE TOP = 0;
//MODE PLATFORM = 1;
//MODE PUZZLE = 2;
//MODE SIDESCROLL = 3;
byte LT_MODE = 0;

int LT_player_jump_pos[] = {
	-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

//FUNCTIONS IN SPRC.ASM
int x_compile_bitmap(word logical_screen_width, char *bitmap, char *output);       
void x_run_compiled_sprite(word XPos, word YPos, char *CompiledBitmap);

void LT_scroll_follow(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	//LIMITS
	int x_limL = SCR_X + 80;
	int x_limR = SCR_X + 240;
	int y_limU = SCR_Y + 80;
	int y_limD = SCR_Y + 140;
	int wmap = LT_map.width<<4;
	int hmap = LT_map.height<<4;
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
	
	index = 16384; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			LT_tile_tempdata[index] = size;
			LT_tile_tempdata[index+1] = size;
			index+=2;
			for(x=0;x<size;x++){
				memcpy(&LT_tile_tempdata[index],&LT_tile_tempdata[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	
	s->nframes = (s->width/size) * (s->height/size);
	
	//calculate frames size
	if ((s->frames = farcalloc(s->nframes,sizeof(SPRITEFRAME))) == NULL) 
		LT_Error("Error loading ",file);
	
	//Estimated size
	fsize = (size * size * 7) / 2 + 25;
	for (frame = 0; frame < s->nframes; frame++){
		if ((s->frames[frame].compiled_code = farcalloc(fsize,sizeof(unsigned char))) == NULL){
			LT_Error("Not enough RAM to allocate frames ",file);
		}
		//COMPILE SPRITE FRAME TO X86 MACHINE CODE		
		code_size = x_compile_bitmap(84, &LT_tile_tempdata[16384+(frame*2)+(frame*(size*size))],s->frames[frame].compiled_code);
		s->frames[frame].compiled_code = farrealloc(s->frames[frame].compiled_code, code_size);
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
	s->siz = (s->height>>2) + 1;
	s->next_scanline = 84 - s->siz;
}

void LT_Clone_Sprite(int sprite_number_c,int sprite_number){
	SPRITE *c = &sprite[sprite_number_c];
	SPRITE *s = &sprite[sprite_number];
	int j;
	c->nframes = s->nframes;
	c->width = s->width;
	c->height = s->height;
	c->frames = s->frames;
	for(j=0;j<c->nframes;j++) c->frames[j].compiled_code = s->frames[j].compiled_code;
	//sprite[sprite_number_c].bkg_data  //allocated in lt_sys
	c->init = 0;
	c->frame = 0;
	c->baseframe = 0;
	c->ground = 0;
	c->jump = 2;
	c->jump_frame = 0;
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
}

void LT_Add_Sprite(int sprite_number,word x, word y) {
	sprite[sprite_number].pos_x = x;
	sprite[sprite_number].pos_y = y;
	if (LT_SpriteStack == 2) LT_SpriteStack = 0;
	//Sprite[LT_SpriteStack].bkg_data
	LT_SpriteStack++;
}

void LT_Set_Sprite_Animation(int sprite_number, byte baseframe, byte frames, byte speed){
	SPRITE *s = &sprite[sprite_number];
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void LT_Delete_Sprite(int sprite_number){
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
	}
	bkg_scanline1:	
	asm{
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		bkg_scanline1
	}

	asm pop si
	asm pop di
	asm pop ds	
	
	outport(GC_INDEX + 1, 0x0ff);
	
	s->init = 0;
}

void LT_Draw_Sprite(int sprite_number){//mierda
	SPRITE *s = &sprite[sprite_number];
	word py = s->pos_y+LT_Window;
	word bkg_data = s->bkg_data;
	word screen_offset0 = (py<<6)+(py<<4)+(py<<2)+(s->pos_x>>2);
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
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	asm{
		push ds
		push di
		push si
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset0	//ds:si source vram			
		
		mov 	es,ax						
		mov		di,bkg_data			//es:di destination
		
		mov		ax,size				//Scanlines
	}
	bkg_scanline2:	
	asm{
		mov 	cx,siz				// copy width + 4 pixels
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		dec 	ax
		jnz		bkg_scanline2	
		
		pop si
		pop di
		pop ds
	}
	outport(GC_INDEX + 1, 0x0ff);
	
	//draw sprite and destroy bkg
	x_run_compiled_sprite(s->pos_x,py,s->frames[s->frame].compiled_code);
	
	s->last_x = s->pos_x;
	s->last_y = py;
}

void LT_Draw_Enemy(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int x = s->pos_x;
	int y = s->pos_y+LT_Window;
  //DRAW THE ENEMY ONLY IF IT IS INSIDE THE ACTIVE MAP
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
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	asm{
		push ds
		push di
		push si
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset1	//ds:si source vram			
		
		mov 	es,ax						
		mov		di,bkg_data			//es:di destination
		
		mov		ax,size				//Scanlines
	}
	bkg_scanline2:	
	asm{
		mov 	cx,siz				// copy width + 4 pixels
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		mov 	cx,siz				
		rep		movsb				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		dec 	ax
		jnz		bkg_scanline2	
		
		pop si
		pop di
		pop ds
	}
	outport(GC_INDEX + 1, 0x0ff);
	//draw sprite and destroy bkg
	x_run_compiled_sprite(x,y,s->frames[s->frame].compiled_code);
	
	s->last_x = x;
	s->last_y = y;
	s->s_delete = 1;
  }
  else if (s->s_delete == 1) {LT_Delete_Sprite(sprite_number); s->s_delete = 0;}
	s->last_x = x;
	s->last_y = y;
}

void LT_Set_Enemy(int sprite_number, word x, word y, int sx, int sy){
	SPRITE *s = &sprite[sprite_number];
	s->pos_x = x<<4;
	s->pos_y = y<<4;
	s->speed_x = sx;
	s->speed_y = sy;
}

void LT_Restore_Sprite_BKG(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	word bkg_data = s->bkg_data;
	word screen_offset0 = (s->last_y<<6)+(s->last_y<<4)+(s->last_y<<2)+(s->last_x>>2);
	word init = s->init;
	word size = s->size;
	word siz = s->siz;
	word next_scanline = s->next_scanline;
	
	///restore destroyed bkg chunk in last frame
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	asm{
		push ds
		push di
		push si
		
		cmp	byte ptr [init],1 //if (sprite[sprite_number].init != 1)
		jne	short rle_noinit
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0	//es:di destination vram				
		
		mov		ds,ax						
		mov		si,bkg_data			//ds:si source vram				
		mov 	ax,size
	}
	bkg_scanline1:	
	asm{
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		bkg_scanline1
	}
	rle_noinit:
	s->init = 1;

	asm pop si
	asm pop di
	asm pop ds	
	
	outport(GC_INDEX + 1, 0x0ff);
}

void LT_Restore_Enemy_BKG(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int x = s->pos_x;
	int y = s->pos_y;
  
  //DRAW THE ENEMY ONLY IF IT IS INSIDE THE ACTIVE MAP
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
	outport(SC_INDEX, ((word)0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	asm{
		push ds
		push di
		push si
		
		cmp	byte ptr [init],1 //if (sprite[sprite_number].init == 1)
		jne	short rle_noinit
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0	//es:di destination vram				
		
		mov		ds,ax						
		mov		si,bkg_data			//ds:si source vram				
		mov 	ax,size
	}
	bkg_scanline1:	
	asm{
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		mov 	cx,siz
		rep		movsb				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		bkg_scanline1
	}
	rle_noinit:
	s->init = 1;
	
	asm pop si
	asm pop di
	asm pop ds	
	
	outport(GC_INDEX + 1, 0x0ff);
  }
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
	tile_number = LT_map.data[(((s->pos_y+si)>>4) * LT_map.width) + ((s->pos_x+si)>>4)];
	tilecol_number = LT_map.collision[(((s->pos_y+si)>>4) * LT_map.width) + ((s->pos_x+si)>>4)];
	
	//PREDEFINED GAME TYPES
	switch (LT_MODE){
	case 0:{//TOP
		if (LT_Keys[LT_UP]){	//UP
			col_y = 0;
			y = (s->pos_y-1)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			col_y = 0;
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x--;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
	break;}
	case 1:{//PLATFORM
		if ((s->ground == 1) && (LT_Keys[LT_JUMP])) {s->ground = 0; s->jump_frame = 0; s->jump = 1;}
		if (s->jump == 1){//JUMP
				col_y = 0;
				y = (s->pos_y-1)>>4;
				if (LT_player_jump_pos[s->jump_frame] < 0){
				tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
			} else {
				platform_y = 1+(((s->pos_y+size)>>4)<<4);
				y = (s->pos_y+size)>>4;
				tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
				tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 			
				if ((tile_number_VR == 2)&&(s->pos_y+size < platform_y)) col_y = 1;
				if ((tile_number_VL == 2)&&(s->pos_y+size < platform_y)) col_y = 1;
			}
			if (col_y == 0){
				s->pos_y += LT_player_jump_pos[s->jump_frame];
				s->jump_frame++;
				if (s->jump_frame == 76) {s->jump_frame = 0; s->jump = 2;}
			}
			if (col_y == 1){
				s->jump_frame = 0; 
				s->jump = 2;
			}
		}
		if (s->jump == 2){//DOWN
			col_y = 0;
			platform_y = 1+(((s->pos_y+size)>>4)<<4);
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if ((tile_number_VR == 2)&&(s->pos_y+size < platform_y)) col_y = 1;
			if ((tile_number_VL == 2)&&(s->pos_y+size < platform_y)) col_y = 1;
			
			if (col_y == 0) {s->ground = 0; s->pos_y+=2;}
			if (col_y == 1) s->ground = 1;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x--;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		} 
	break;}	
	case 2:{//PHYSICS 2 
		
		if (s->speed_y < 0){	//UP
			col_y = 0;
			y = (s->pos_y+2)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz-3)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + ((s->pos_x+4)>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 1) {s->speed_y *= -1;s->speed_y-=48;}
		}
		if (s->speed_y > 0){	//DOWN
			col_y = 0;
			y = (s->pos_y+size-2)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz-3)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + ((s->pos_x+4)>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 1) {s->speed_y *= -1;s->speed_y+=48;}
		}
		if (s->speed_x < 0){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+4)>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz-4)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 1) {s->speed_x *= -1;s->speed_x-=48;}
		}
		if (s->speed_x > 0){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+4>>4)) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz-4)>>4) * LT_map.width) + x];
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
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			col_y = 0;
			y = (s->pos_y+size)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x-=2;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
		col_x = 0;
		tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
		tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
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
		tile_number_VR = LT_map.collision[( y * LT_map.width) + x];
		tile_number_VL = LT_map.collision[( y * LT_map.width) + ((s->pos_x)>>4)];
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
		tile_number_VR = LT_map.collision[( y * LT_map.width) + x];
		tile_number_VL = LT_map.collision[( y * LT_map.width) + ((s->pos_x)>>4)];
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
		tile_number_HU = LT_map.collision[(((s->pos_y)>>4) * LT_map.width) + x];
		tile_number_HD = LT_map.collision[( y * LT_map.width) + x];	
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
		tile_number_HU = LT_map.collision[(((s->pos_y)>>4) * LT_map.width) + x];
		tile_number_HD = LT_map.collision[( y * LT_map.width) + x];
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

void LT_Enemy_walker(int sprite_number, byte mode){
	SPRITE *s = &sprite[sprite_number];
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

	switch (mode){
	  case 0:{ //WALKS ON TOP VIEW
		y = (py+si>>4);
		switch (sx){
			case -1:	//LEFT
			col_x = 0;
			x = (px-1)>>4;
			tile_number_HU = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (col_x == 1) s->mspeed_x *= -1;
			break;
			case 1:		//RIGHT
			col_x = 0;
			x = (px+siz)>>4;
			tile_number_HU = LT_map.collision[( y * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (col_x == 1) s->mspeed_x *= -1;
			break;
		}
		x = (px+si)>>4;
		switch (sy){
			case -1:	//UP
			col_y = 0;
			y = (py-1)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_VR == 1) col_y = 1;
			if (col_y == 1) s->mspeed_y *= -1;
			break;
			case 1:		//DOWN
			col_y = 0;
			y = (py+siz)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_VR == 1) col_y = 1;
			if (col_y == 1) s->mspeed_y *= -1;
			break;
		}
	  break;}
	  case 1:{ //ONLY WALKS ON PLATFORMS UNTILL IT REACHES EDGES  OR SOLID TILES
		switch (sx){
			case -1:	//LEFT
			col_x = 0;
			x = (px-1)>>4;
			tile_number_HU = LT_map.collision[(((py+si)>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((py+siz)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 10) col_x = 1;  //Platform edge
			if (col_x == 1) s->mspeed_x *= -1;
			break;
			case 1:		//RIGHT
			col_x = 0;
			x = (px+siz)>>4;
			tile_number_HU = LT_map.collision[(((py+si>>4)) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((py+siz)>>4) * LT_map.width) + x];
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

void LT_unload_sprite(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	int i;
	s->init = 0;
	LT_Delete_Sprite(sprite_number);
	for (i=0;i<s->nframes;i++){
		farfree(s->frames[i].compiled_code);
		s->frames[i].compiled_code = NULL;
	}
	farfree(s->frames); s->frames = NULL;
}	

