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

//Player
byte tile_number = 0;		//Tile a sprite is on
byte tilecol_number = 0;	//Collision Tile a sprite is on
byte tile_number_VR = 0;	//Tile vertical right
byte tile_number_VL = 0;	//Tile vertical left
byte tile_number_HU = 0;	//Tile horizontal up
byte tile_number_HD = 0;	//Tile horizontal down

byte LT_SpriteStack = 0;


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

void LT_scroll_follow(SPRITE *s){
	//LIMITS
	int y_limU = SCR_Y + 64;
	int y_limD = SCR_Y + 120;
	int x_limL = SCR_X + 80;
	int x_limR = SCR_X + 200;
	int wmap = LT_map.width<<4;
	int hmap = LT_map.height<<4;
	int x = abs(s->last_x - s->pos_x);
	int y = abs(s->last_y - s->pos_y);
	//clamp limits
	if ((SCR_X > -1) && ((SCR_X+303)<wmap) && (SCR_Y > -1) && ((SCR_Y+175)<hmap)){
		if (s->pos_y > y_limD) SCR_Y+=y;
		if (s->pos_y < y_limU) SCR_Y-=y;
		if (s->pos_x < x_limL) SCR_X-=x;
		if (s->pos_x > x_limR) SCR_X+=x;
	}
	if (SCR_X < 0) SCR_X = 0; 
	if ((SCR_X+304) > wmap) SCR_X = wmap-304;
	if (SCR_Y < 0) SCR_Y = 0; 
	if ((SCR_Y+176) > hmap) SCR_Y = hmap-176; 
}

//load RLE sprites with transparency (size = 8,16,32)
void LT_Load_Sprite(char *file,SPRITE *s, byte size){
	FILE *fp;
	long index,offset;
	word num_colors;
	byte x,y;
	word i,j;
	word frame = 0;
	byte tileX;
	byte tileY;
	
	word number_of_runs = 0;
	byte pixel = 0; //this is to know if a scan line is empty
	word e_sskip = 0;
	word e_bskip = 0;
	word foffset = 0; //offset in final data
	word frame_size;
	word run_sskip[256];
	byte run_bskip[256];
	byte run_startx[256];
	word run_length[256];
	byte current_byte;
	byte last_byte = 0;
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 

	fskip(fp,18);
	fread(&s->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&s->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
	}

	for(index=(s->height-1)*s->width;index>=0;index-=s->width)
		for(x=0;x<s->width;x++)
		LT_tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			for(x=0;x<size;x++){
				memcpy(&LT_sprite_tiledatatemp[index],&LT_tile_datatemp[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	s->nframes = (s->width/size) * (s->height/size);
	s->width = size;
	s->height = size;

	//calculate frames size
	s->rle_frames = farcalloc(s->nframes,sizeof(SPRITEFRAME));
	//CONVERT AND STORE EVERY FRAME IN RLE FORMAT
	for(j=0;j<s->nframes;j++){
		offset = 0;
		foffset = 0;
		frame = (size*size)*j;
		number_of_runs = 0;
		e_sskip = 0;
		e_bskip = 0;
		for(y = 0; y < size; y++) {
			last_byte = 0;
			pixel = 0;
			for(x = 0; x < size; x++) {
				//Get pixel value
				current_byte = LT_sprite_tiledatatemp[(size*y)+x+frame]; 
				//I found a Pixel, start a run
				if (last_byte == 0 && current_byte != 0){
					run_startx[number_of_runs] = x;
					run_sskip[number_of_runs] = e_sskip;
					run_bskip[number_of_runs] = e_bskip;
					e_sskip = 0;
					e_bskip = 0;
					pixel = 1;
				}
				//I found the end of a run
				if (last_byte != 0 && current_byte == 0){
					run_length[number_of_runs] = x - run_startx[number_of_runs];
					number_of_runs++;
					pixel = 2;
				}
				//If 0, skip
				if (current_byte == 0){e_sskip++;e_bskip++;}
				last_byte = current_byte;
			}
			//I didn't find the end of a run, and the scan line ended
			if (pixel == 1){
				run_length[number_of_runs] = x - run_startx[number_of_runs];
				number_of_runs++;
				e_bskip = 0;
			}
			//I didn't find any pixel, or the last part was empty, and the scan line ended. Do nothing
			//Next scan line
			e_sskip+=320-size;
		}
		
		//ALLOCATE SPACE FOR CURRENT FRAME
		frame_size = 2; //word "number of runs"
		for(i = 0; i < number_of_runs; i++) 
			frame_size+= (4 + run_length[i]); //word skip + word number of pixels + length
		s->rle_frames[j].rle_data = (byte*) farcalloc(frame_size,sizeof(byte));

		//copy RLE data to struct
		memcpy(&s->rle_frames[j].rle_data[0],&number_of_runs,2);
		foffset+=2;
		for(i = 0; i < number_of_runs; i++) {
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_sskip[i],2); //bytes to skip on vga screen
			foffset+=2;
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_length[i],2); //Number of bytes of pixel data that follow
			foffset+=2;
			offset+=run_bskip[i];
			memcpy(&s->rle_frames[j].rle_data[foffset],&LT_sprite_tiledatatemp[offset+frame],run_length[i]); //copy pixel data;
			foffset+=run_length[i];
			offset+=run_length[i];
		}
	}

	s->bkg_data = (byte *) farcalloc(size*size,sizeof(byte)); /*allocate memory for the 16x16 bkg chunk erased every time you draw the sprite*/
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
}

void LT_Clone_Sprite(SPRITE *c,SPRITE *s){
	int j;
	c->nframes = s->nframes;
	c->width = s->width;
	c->height = s->height;
	c->rle_frames = s->rle_frames;
	for(j=0;j<c->nframes;j++) c->rle_frames[j].rle_data = s->rle_frames[j].rle_data;
	c->bkg_data = (byte *) farcalloc(s->width*s->width,sizeof(byte)); /*allocate memory for the 16x16 bkg chunk erased every time you draw the sprite*/
	c->init = 0;
	c->frame = 0;
	c->baseframe = 0;
	c->ground = 0;
	c->jump = 2;
	c->jump_frame = 0;
	c->animate = 0;
	c->anim_speed = 0;
	c->anim_counter = 0;
	c->speed_x = 0;
	c->speed_y = 0;
	c->s_x = 0;
	c->s_y = 0;
	c->fpos_x = 0;
	c->fpos_y = 0;
	c->s_delete = 0;
	c->misc = 0;
}

void LT_Add_Sprite(SPRITE *s,word x, word y) {
	s->pos_x = x;
	s->pos_y = y;
	if (LT_SpriteStack == 2) LT_SpriteStack = 0;
	//Sprite[LT_SpriteStack].bkg_data
	LT_SpriteStack++;
}

void LT_Set_Sprite_Animation(SPRITE *s, byte baseframe, byte frames, byte speed){
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void LT_Delete_Sprite(SPRITE *s){
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset0 = (s->last_y<<8)+(s->last_y<<6)+s->last_x;
	word size = s->height;
	word siz = s->height>>1;
	word next_scanline = 320 - s->width;
	
	///Paste bkg chunk and delete sprite
	asm push ds
	asm push di
	asm push si
	asm{	
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0				
		
		lds		bx,[bkg_data]					
		lds		si,ds:[bx]						
		mov 	ax,size
	}
	delete:	
	asm{
		mov 	cx,siz
		rep		movsw				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		delete
	}
	asm pop si
	asm pop di
	asm pop ds
	
	s->init = 0;
}

void LT_Draw_Sprite(SPRITE *s){
	unsigned char *data;
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset1 = (s->pos_y<<8)+(s->pos_y<<6)+s->pos_x;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	
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
	
	data = (unsigned char *) &s->rle_frames[s->frame].rle_data; 
	//Copy bkg chunk before destroying it
	asm{
		push ds
		push di
		push si
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset1				
		
		les		bx,[bkg_data]					
		les		di,es:[bx]						
		mov 	ax,size
	}
	rle_2:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		dec 	ax
		jnz		rle_2	
	}
	
	//copy sprite and destroy bkg
	asm{

		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset1	//es:di = vga		
	
		lds		bx,[data]					
		lds		si,ds:[bx]			//ds:si = data		
	
		lodsw			//DS:SI -> AX (Number of runs)
		
		xchg dx,ax		//DX = Number of runs.
	}
	rle_3:				//DX = Number of runs
	asm{
		lodsw			//DS:SI -> AX. Number of bytes to ad to pointer DI.
		add 	di,ax	
		lodsw			//DS:SI -> AX. Number of bytes of pixel data that follow
		mov 	cx,ax	//CX = AX (counter)
		shr		cx,1	//counter / 2 because we copy words
		jc		rle_odd //if there was a left over, it was odd, go to rle_odd
		
		rep 	movsw	//copy pixel data from ds:si to es:di
		dec 	dx		//DX = Number of runs
		jnz 	rle_3
		jmp		rle_exit
	}
	rle_odd:
	asm{
		rep 	movsw	//copy pixel data from ds:si to es:di
		movsb			//Copy the last byte, because the line was odd
		dec 	dx		//DX = Number of runs	
		jnz 	rle_3
	}
	rle_exit:
	asm pop si
	asm pop di
	asm pop ds
	
	s->last_x = s->pos_x;
	s->last_y = s->pos_y;
}

void LT_Draw_Enemy(SPRITE *s){
  //DRAW THE ENEMY ONLY IF IT IS INSIDE THE ACTIVE MAP
  if ((s->pos_x > SCR_X)&&(s->pos_x < (SCR_X+288))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+160))){
	unsigned char *data;
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset1 = (s->pos_y<<8)+(s->pos_y<<6)+s->pos_x;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;	
 
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
	
	data = (unsigned char *) &s->rle_frames[s->frame].rle_data; 
	//Copy bkg chunk before destroying it
	asm{
		push ds
		push di
		push si
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset1				
		
		les		bx,[bkg_data]					
		les		di,es:[bx]						
		mov 	ax,size
	}
	rle_2:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		dec 	ax
		jnz		rle_2	
	}	
	//copy sprite and destroy bkg
	asm{

		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset1	//es:di = vga		
	
		lds		bx,[data]					
		lds		si,ds:[bx]			//ds:si = data		
	
		lodsw			//DS:SI -> AX (Number of runs)
		
		xchg dx,ax		//DX = Number of runs.
	}
	rle_3:				//DX = Number of runs
	asm{
		lodsw			//DS:SI -> AX. Number of bytes to ad to pointer DI.
		add 	di,ax	
		lodsw			//DS:SI -> AX. Number of bytes of pixel data that follow
		mov 	cx,ax	//CX = AX (counter)
		shr		cx,1	//counter / 2 because we copy words
		jc		rle_odd //if there was a left over, it was odd, go to rle_odd
		
		rep 	movsw	//copy pixel data from ds:si to es:di
		dec 	dx		//DX = Number of runs
		jnz 	rle_3
		jmp		rle_exit
	}
	rle_odd:
	asm{
		rep 	movsw	//copy pixel data from ds:si to es:di
		movsb			//Copy the last byte, because the line was odd
		dec 	dx		//DX = Number of runs	
		jnz 	rle_3
	}
	rle_exit:
	asm pop si
	asm pop di
	asm pop ds
	
	s->last_x = s->pos_x;
	s->last_y = s->pos_y;
	
	s->s_delete = 1;
  }
  else if (s->s_delete == 1) {LT_Delete_Sprite(s); s->s_delete = 0;}
	s->last_x = s->pos_x;
	s->last_y = s->pos_y;
}

void LT_Set_Enemy(SPRITE *s, word x, word y, int sx, int sy){
	s->pos_x = x<<4;
	s->pos_y = y<<4;
	s->speed_x = sx;
	s->speed_y = sy;
}

void LT_Restore_Sprite_BKG(SPRITE *s){
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset0 = (s->last_y<<8)+(s->last_y<<6)+s->last_x;
	word init = s->init;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	
	///restore destroyed bkg chunk in last frame
	asm{
		push ds
		push di
		push si
		
		cmp	byte ptr [init],1 //if (s->init == 1)
		jne	short rle_noinit
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0				
		
		lds		bx,[bkg_data]					
		lds		si,ds:[bx]						
		mov 	ax,size
	}
	rle_1:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		rle_1
	}
	rle_noinit:
	s->init = 1;
		
	asm pop si
	asm pop di
	asm pop ds	
}

void LT_Restore_Enemy_BKG(SPRITE *s){
  //DRAW THE ENEMY ONLY IF IT IS INSIDE THE ACTIVE MAP
  if ((s->pos_x > SCR_X)&&(s->pos_x < (SCR_X+288))&&(s->pos_y > SCR_Y)&&(s->pos_y < (SCR_Y+160))){
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset0 = (s->last_y<<8)+(s->last_y<<6)+s->last_x;
	word init = s->init;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	
	///restore destroyed bkg chunk in last frame
	asm{
		push ds
		push di
		push si
		
		cmp	byte ptr [init],1 //if (s->init == 1)
		jne	short rle_noinit
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0				
		
		lds		bx,[bkg_data]					
		lds		si,ds:[bx]						
		mov 	ax,size
	}
	rle_1:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		rle_1
	}
	rle_noinit:
	s->init = 1;
		
	asm pop si
	asm pop di
	asm pop ds
  }
}

LT_Col LT_move_player(SPRITE *s){
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
			if (col_y == 1) s->speed_y *= -1;
		}
		if (s->speed_y > 0){	//DOWN
			col_y = 0;
			y = (s->pos_y+size-2)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + ((s->pos_x+siz-3)>>4)];
			tile_number_VL = LT_map.collision[( y * LT_map.width) + ((s->pos_x+4)>>4)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 1) s->speed_y *= -1;
		}
		if (s->speed_x < 0){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+4)>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz-4)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 1) s->speed_x *= -1;
		}
		if (s->speed_x > 0){	//RIGHT
			col_x = 0;
			x = (s->pos_x+size)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+4>>4)) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz-4)>>4) * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 1) s->speed_x *= -1;
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
		if (tilecol_number == 6)s->speed_y-=9;
		//FORCE DOWN
		if (tilecol_number == 7)s->speed_y+=9;
		//FORCE LEFT
		if (tilecol_number == 8)s->speed_x-=9;
		//FORCE RIGHT
		if (tilecol_number == 9)s->speed_x+=9;

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

LT_Col LT_Bounce_Ball(SPRITE *s){
	LT_Col LT_Collision;
	byte col_x = 0;
	byte col_y = 0;
	byte size = s->width;
	byte siz = s->width -1;
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
		y = (s->pos_y+size+1)>>4;
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

void LT_Enemy_walker(SPRITE *s, byte mode){
	byte col_x = 0;
	byte col_y = 0;
	int x,y;
	byte siz = s->width+1;
	byte si = s->width>>1;

	if (mode == 0){ //WALKS ON TOP VIEW
		y = (s->pos_y+si>>4);
		if (s->speed_x < 0){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (col_x == 1) s->speed_x *= -1;
		}
		if (s->speed_x > 0){	//RIGHT
			col_x = 0;
			x = (s->pos_x+siz)>>4;
			tile_number_HU = LT_map.collision[( y * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (col_x == 1) s->speed_x *= -1;
		}
		x = (s->pos_x+si)>>4;
		if (s->speed_y < 0){	//UP
			col_y = 0;
			y = (s->pos_y-1)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_VR == 1) col_y = 1;
			if (col_y == 1) s->speed_y *= 1;
		}
		if (s->speed_y > 0){	//DOWN
			col_y = 0;
			y = (s->pos_y+siz)>>4;
			tile_number_VR = LT_map.collision[( y * LT_map.width) + x];	
			if (tile_number_VR == 1) col_y = 1;
			if (col_y == 1) s->speed_y *= -1;
		}
	}	
	if (mode == 1){ //ONLY WALKS ON PLATFORMS UNTILL IT REACHES EDGES  OR SOLID TILES
		if (s->speed_x < 0){	//LEFT
			col_x = 0;
			x = (s->pos_x-1)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+si)>>4) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 10) col_x = 1;  //Platform edge
			if (col_x == 1) s->speed_x *= -1;
		}
		if (s->speed_x > 0){	//RIGHT
			col_x = 0;
			x = (s->pos_x+siz)>>4;
			tile_number_HU = LT_map.collision[(((s->pos_y+si>>4)) * LT_map.width) + x];
			tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 10) col_x = 1; //Platform edge
			if (col_x == 1) s->speed_x *= -1;
		}
	}
	col_x = 0;
	col_y = 0;
	
	//SPEED CONTROL TO MAKE ENEMIES TO MOVE SLOWER THAN +=1
	if (s->s_x == 2) s->s_x = 0;
	if (s->s_y == 2) s->s_y = 0;
	s->pos_x += s->speed_x*s->s_x;
	s->pos_y += s->speed_y*s->s_y;
	
	s->s_x ++;
	s->s_y ++; 
	return;
}

void LT_unload_sprite(SPRITE *s){
	int i;
	s->init = 0;
	LT_Delete_Sprite(s);
	farfree(s->bkg_data); s->bkg_data = NULL;
	for (i=0;i<s->nframes;i++){
		farfree(s->rle_frames[i].rle_data);
		s->rle_frames[i].rle_data = NULL;
	}
	farfree(s->rle_frames); s->rle_frames = NULL;
	s = NULL;
}	


