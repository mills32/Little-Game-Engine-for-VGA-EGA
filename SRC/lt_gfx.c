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

//Predefined structs
/*******************
OK let's do this console style, (of course You can do whatever you want).
At any moment there will only be:
You'll have to unload a music/map/tilset before loading another.
********************/

MAP LT_map;			// One map in ram stored at "LT_map"
TILE LT_tileset;	// One tileset in ram stored at "LT_tileset"
ANIMATION LT_Loading_Animation;// Animation for loading screen
unsigned char *LT_tile_datatemp; //Temp storage of non tiled data.
unsigned char *LT_sprite_tiledatatemp; //Temp storage of tiled sprites to be converted to RLE
SPRITE lt_gpnumber0; // sprites for printing
//GLOBAL VARIABLES

//Palette for fading
byte LT_Temp_palette[256*3];

//This points to video memory.
byte *MCGA=(byte *)0xA0000000L; 

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 0;
int LT_current_x = 0;
int LT_last_x = 0;
int LT_current_y = 0;
int LT_last_y = 0;
int LT_map_offset = 0;
int LT_map_offset_Endless = 0;
int LT_follow = 0;

extern void interrupt (*LT_old_time_handler)(void); 

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

void interrupt LT_Loading(void){
	LT_Draw_Animation(&LT_Loading_Animation);
}

void LT_Set_Loading_Interrupt(){
	unsigned long spd = 1193182/50;
	int i;
	lt_gpnumber0.init = 0;
	MCGA_Fade_out();
	LT_Stop_Music();
	MCGA_SplitScreen(0x0ffff); //disable split screen
	MCGA_ClearScreen();//clear screen
	MCGA_ClearPalette();
	
	//change color 0, 1, 2 (black and white)
	LT_Loading_Animation.palette[0]= LT_Loading_Palette[0];
	LT_Loading_Animation.palette[1]= LT_Loading_Palette[1];
	LT_Loading_Animation.palette[2]= LT_Loading_Palette[1];
	
	//Print "Loading", without deleting the animation.
	//Every character fills a 8x8 cell in the 64Kb VRAM, so:
	gotoxy(17,15);
	printf("LOADING\r");

	//center loading animation
	LT_Loading_Animation.pos_x = 138;
	LT_Loading_Animation.pos_y = 72;
	//set timer
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte*/
	//set interrupt
	setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
	MCGA_Fade_in(LT_Loading_Animation.palette);
}

void LT_Delete_Loading_Interrupt(){
	MCGA_Fade_out();
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, LT_old_time_handler);
}

//Hardware scrolling 
void MCGA_Scroll(word x, word y){
	byte p[8] = {0,2,4,6};
	byte pix = x & 3; 	//pixel panning value
	x=x>>2;				//x/4
	y=(y<<6)+(y<<4);	//(y*64)+(y*16) = y*80;

	//MCGA_WaitVBL
	while ((inp(0x03da) & 0x08));
	//change pixel panning register 
	inp(0x3da);
	outp(0x3c0, 0x33);
	outp(0x3c0, p[pix]);
	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	outport(0x03d4, 0x0C | (x+y & 0xff00));
	outport(0x03d4, 0x0D | (x+y << 8));
	while (!(inp(0x03da) & 0x08));

	//-3,+5,+1,+1,
	//-3,+5,+1,+1
}

void MCGA_WaitVBL(){ //This does not work well outside MCGA_Scroll
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

void MCGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&MCGA[0],0,(320*200)/4);
	SCR_X = 0;
	SCR_Y = 0;
	MCGA_Scroll(SCR_X,SCR_Y);
}

void MCGA_SplitScreen(int line){
	MCGA_WaitVBL();
    asm{
	push    ax
    push    cx
    push    dx
	//Set the split screen scan line.
    cli		// make sure all the registers get set at once
	}
	word_out(CRTC_INDEX, LINE_COMPARE, line); // set bits 7-0 of the split screen scan line

	asm{
    mov    ah,byte ptr [line+1]
    and    ah,1
    mov    cl,4
    shl    ah,cl                      // move bit 8 of the split split screen scan line into position for the Overflow reg
    mov    al,OVERFLOW

	//The Split Screen, Overflow, and Line Compare registers all contain part of the split screen start scan line on the VGA
    out    dx,al                      //set CRTC Index reg to point to Overflow
    inc    dx                         //point to CRTC Data reg
    in     al,dx                      //get the current Overflow reg setting
    and    al,not 10h                 //turn off split screen bit 8
    or     al,ah                      //insert the new split screen bit 8 (works in any mode)
    out    dx,al                      //set the new split screen bit 8
    dec    dx                         //point to CRTC Index reg
    mov    ah,byte ptr [line+1]
    and    ah,2
    mov    cl,3
    ror    ah,cl                      // move bit 9 of the split split screen scan line into position for the Maximum Scan Line register
    mov    al,MAX_SCAN_LINE
    out    dx,al                      //set CRTC Index reg to point to Maximum Scan Line
    inc    dx                         //point to CRTC Data reg
    in     al,dx                      //get the current Maximum Scan Line setting
    and    al,not 40h                 //turn off split screen bit 9
    or     al,ah                      //insert the new split screen bit 9 (works in any mode)
    out    dx,al                      //set the new split screen bit 9

	//Turn on split screen pal pen suppression, so the split screen
	//won't be subject to pel panning as is the non split screen portion.

	mov  dx,INPUT_STATUS_0
	in   al,dx                  	//Reset the AC Index/Data toggle to index state
	mov  al,AC_MODE_CONTROL+20h 	//Bit 5 set to prevent screen blanking
	mov  dx,AC_INDEX				//Point AC to Index/Data register
	out  dx,al
	inc  dx							//Point to AC Data reg (for reads only)
	in   al,dx						//Get the current AC Mode Control reg
	or   al,20h						//Enable split scrn Pel panning suppress.
	dec  dx							//Point to AC Index/Data reg (for writes only)
	out  dx,al						//Write the new AC Mode Control setting with split screen pel panning suppression turned on	
	
    sti
    pop    dx
    pop    cx
    pop    ax
    }
}

void fskip(FILE *fp, word num_bytes){
	int i;
	for (i=0; i<num_bytes; i++) fgetc(fp);
}

void LT_Load_BKG(char *file){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	for(index=(LT_tileset.height-1)*LT_tileset.width;index>=0;index-=LT_tileset.width)
		for(x=0;x<LT_tileset.width;x++)
			LT_tileset.tdata[index+x]=(byte)fgetc(fp);
	fclose(fp);
}

void LT_Draw_BKG(){
	dword bitmap_offset = 0;
	dword screen_offset = 0;
	int j;
	MCGA_Fade_in(LT_tileset.palette);
	for(j=0;j<174;j++){
		memcpy(&MCGA[screen_offset],&LT_tileset.tdata[bitmap_offset],LT_tileset.width);
		bitmap_offset+=LT_tileset.width;
		screen_offset+=320;
	}
}

//load static animations without transparency
void LT_Load_Animation(char *file,ANIMATION *s, byte size){
	FILE *fp;
	long index,offset;
	word num_colors;
	byte x,y;
	word i,j;
	byte tileX;
	byte tileY;
	
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
		s->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}
	
	if ((s->data = farcalloc(s->height*s->width,sizeof(byte))) == NULL){
		printf("Error allocating animation data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);		
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
				memcpy(&s->data[index],&LT_tile_datatemp[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	
	s->nframes = (s->width>>4) * (s->height>>4);
	s->width = size;
	s->height = size;
	s->frame = 0;
	s->baseframe = 0;
	s->anim_speed = 0;
	s->anim_counter = 0;
}

void LT_Set_Animation(ANIMATION *s, byte baseframe, byte frames, byte speed){
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void LT_Draw_Animation(ANIMATION *s){
	word screen_offset = (s->pos_y<<8)+(s->pos_y<<6)+s->pos_x;
	word size = s->width;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	unsigned char *data;
	byte frame;
	
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
	
	data = (unsigned char *) &s->data;
	frame = s->frame;
	asm{
		push ds
		push di
		push si
		
		lds		bx,dword ptr[data]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		ax,word ptr [frame]
		mov		cl,10
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address							
		mov 	ax,0A000h
		mov 	es,ax
		mov 	ax,size
	}
	copy_tile_a:	
	asm{
		mov 	cx,size2
		rep		movsw				
		add 	di,next_scanline
		dec 	ax
		jnz		copy_tile_a
		
		pop si
		pop di
		pop ds
	}	
}

void LT_Unload_Animation(ANIMATION *s){
	free(s->data); s->data = NULL;
}

/* load_16x16 tiles */
void load_tiles(char *file){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	for(index=(LT_tileset.height-1)*LT_tileset.width;index>=0;index-=LT_tileset.width)
		for(x=0;x<LT_tileset.width;x++)
			LT_tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<LT_tileset.height;tileY+=16){
		for (tileX=0;tileX<LT_tileset.width;tileX+=16){
			offset = (tileY*LT_tileset.width)+tileX;
			for(x=0;x<16;x++){
				memcpy(&LT_tileset.tdata[index],&LT_tile_datatemp[offset+(x*LT_tileset.width)],16);
				index+=16;
			}
		}
	}
	
	LT_tileset.ntiles = (LT_tileset.width>>4) * (LT_tileset.height>>4);
}

void LT_unload_tileset(){
	LT_tileset.width = NULL;
	LT_tileset.height = NULL;
	LT_tileset.ntiles = NULL;
	*LT_tileset.palette = NULL;
	free(LT_tileset.tdata); LT_tileset.tdata = NULL;
}

//Load tiled TMX map in CSV format
//Be sure bkg layer map is the first to be exported, (before collision layer map)
void load_map(char *file){ 
	FILE *f = fopen(file, "rb");
	word start_bkg_data = 0;
	word start_col_data = 0;
	word tile = 0;
	word index = 0;
	byte tilecount = 0; //Just to get the number of tiles to substract to collision tiles 
	char line[128];
	char name[64]; //name of the layer in TILED

	if(!f){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}
	//read file
	fseek(f, 0, SEEK_SET);			
	while(start_bkg_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 't')){ // get tilecount
			sscanf(line," <tileset firstgid=\"%i[^\"]\" name=\"%24[^\"]\" tilewidth=\"%i[^\"]\" tileheight=\"%i[^\"]\" tilecount=\"%i[^\"]\"",&tilecount,&tilecount,&tilecount,&tilecount,&tilecount);
		}
		if((line[1] == '<')&&(line[2] == 'l')){// get map dimensions
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&LT_map.width,&LT_map.height);
			start_bkg_data = 1;
		}
	}
	LT_map.ntiles = LT_map.width*LT_map.height;
	if (LT_map.ntiles > 32767){
		printf("Error, map is bigger than 32 kb");
		sleep(2);
		LT_ExitDOS();
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">

	//read tile array
	for (index = 0; index < LT_map.ntiles; index++){
		fscanf(f, "%i,", &tile);
		LT_map.data[index] = tile-1;
	}
	//skip 
	while(start_col_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 'l')){
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&LT_map.width,&LT_map.height);
			start_col_data = 1;
		}
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">
	//read collision array
	for (index = 0; index < LT_map.ntiles; index++){
		fscanf(f, "%d,", &tile);
		LT_map.collision[index] = tile -tilecount;
	}
	fclose(f);
}

void LT_unload_map(){
	LT_map.width = NULL;
	LT_map.height = NULL;
	LT_map.ntiles = NULL;
	free(LT_map.data); LT_map.data = NULL;
	free(LT_map.collision); LT_map.collision = NULL;
}

void LT_Set_Map(int x, int y){
	//UNDER CONSTRUCTION
	int i = 0;
	int j = 0;
	
	LT_map_offset = (LT_map.width*y)+x;
	LT_map_offset_Endless = 20;
	SCR_X = x<<4;
	SCR_Y = y<<4;
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
	MCGA_Scroll(SCR_X,SCR_Y);
	//draw map 
	for (i = 0;i<320;i+=16){draw_map_column(SCR_X+i,SCR_Y,LT_map_offset+j);j++;}	
	MCGA_Fade_in(LT_tileset.palette);
}

void LT_Edit_MapTile(byte x, byte y, byte ntile, byte col){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	word tile = (y * LT_map.width) + x;
	word screen_offset = ((y<<8)+(y<<6)) + x<<4;
	LT_map.collision[tile] = col;
	asm{
		push ds
		push di
		push si
		
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		ax,word ptr [ntile]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address							
		mov 	ax,0A000h
		mov 	es,ax

		//UNWRAPPED COPY 16x16 TILE LOOP
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		//END LOOP
		
		pop si
		pop di
		pop ds
	}
	LT_map.data[tile] = ntile;
}

void draw_map_column(word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	unsigned char *mapdata = LT_map.data;
	word tile_offset = 0;
	word width = LT_map.width;
	word screen_offset = (y<<8)+(y<<6)+x;
	asm{
		push ds
		push di
		push si	
		
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		word ptr [tile_offset],ds
		mov		word ptr [tile_offset+2],si

		mov		ax,map_offset
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		mov		di,screen_offset		//es:di screen address							
	}
	//UNWRAPPED LOOPS, Just to give some more cycles for the 8088.	
	//COPY 12 tiles 
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax

		mov 	cx,8		//COPY TILE (16 LINES)
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16	//END COPY TILE
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
	}
	
	asm{//END	
		pop si
		pop di
		pop ds
	}
}

void draw_map_row( word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	unsigned char *mapdata = LT_map.data;
	word tile_offset = 0;
	//word width = map.width;
	word screen_offset = (y<<8)+(y<<6)+x;
	asm{//SET ADDRESS
		push ds
		push di
		push si
			
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		word ptr [tile_offset],ds
		mov		word ptr [tile_offset+2],si

		mov		ax,map_offset
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		mov		di,screen_offset		//es:di screen address							
	}
	
	//UNWRAPPED LOOPS
	//Copy 20 tiles
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,8		//COPY 16 LINES
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		
		//SET ADDRESS FOR NEXT TILE
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
	}
	
	asm{//END	
		pop si
		pop di
		pop ds
	}
}

//update rows and colums
void LT_scroll_map(){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	
	if (LT_current_x != LT_last_x) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_x < LT_last_x) 
			draw_map_column(LT_current_x,LT_current_y,LT_map_offset); 
		else  
			draw_map_column(LT_current_x+304,LT_current_y,LT_map_offset+19);
	}
	if (LT_current_y != LT_last_y) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_y < LT_last_y)
			draw_map_row(LT_current_x,LT_current_y,LT_map_offset);
		else 
			draw_map_row(LT_current_x,LT_current_y+176,LT_map_offset+(11*LT_map.width));
	}
	
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
}

//Endless Side Scrolling Map
void LT_Endless_SideScroll_Map(int y){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	if (LT_current_x > LT_last_x) { 
		draw_map_column(LT_current_x+304,LT_current_y,(LT_map_offset_Endless%LT_map.width)+(y*LT_map.width));
		LT_map_offset_Endless++;
	}
	LT_last_x = LT_current_x;
}

void LT_load_font(char *file){
	LT_Load_Sprite(file,&lt_gpnumber0,8);
	farfree(lt_gpnumber0.bkg_data);
	lt_gpnumber0.bkg_data = NULL;
	lt_gpnumber0.bkg_data = (byte *) farcalloc(24*8,sizeof(byte));
}

void LT_gprint(byte var, word x, word y){
	word fx = SCR_X+x;
	word fy = SCR_Y+y;
	word screen_offset0 = (lt_gpnumber0.last_y<<8)+(lt_gpnumber0.last_y<<6)+lt_gpnumber0.last_x;
	word screen_offset1 = (fy<<8)+(fy<<6)+fx;
	word init = lt_gpnumber0.init;
	unsigned char *bkg_data = (unsigned char *) &lt_gpnumber0.bkg_data;
	unsigned char *data;
	byte d = 0;
	
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
	
		mov 	cx,12
		rep		movsw				// copy bytes from ds:si to es:di
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw				
		add 	di,320 - 24
		mov 	cx,12
		rep		movsw
	}
	rle_noinit:
	lt_gpnumber0.init = 1;	
	//Copy bkg chunk before destroying it
	asm{
	
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset1				
		
		les		bx,[bkg_data]					
		les		di,es:[bx]						
	
		mov 	cx,12
		rep		movsw				// copy bytes from ds:si to es:di
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		add 	si,320 - 24
		mov 	cx,12
		rep		movsw
		
		pop si
		pop di
		pop ds
	}
	
	screen_offset1+=16;
	data = (unsigned char *) &lt_gpnumber0.rle_frames[var % 10].rle_data;
	while (d < 3){//DIGITS
		//copy sprite and destroy bkg
		asm{
			push ds
			push di
			push si	
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
		
		d++;
		var /= 10;
		data = (unsigned char *) &lt_gpnumber0.rle_frames[var % 10].rle_data;
		screen_offset1-=8;
	}
	
	lt_gpnumber0.last_x = fx;
	lt_gpnumber0.last_y = fy;
}
	
void LT_unload_font(){
	LT_unload_sprite(&lt_gpnumber0);
}

void set_palette(unsigned char *palette){
	int i;
	outp(0x03c8,0); 
	for(i=0;i<256*3;i++) outp(0x03c9,palette[i]);
}

void MCGA_ClearPalette(){
	int i;
	outp(0x03c8,0); 
	for(i=0;i<256*3;i++) outp(0x03c9,0x00);
}

void MCGA_Fade_in(unsigned char *palette){
	int i,j;
	byte c;
	//All colours black
	outp(0x03c8,0);
	for(i=0;i<256;i++){ outp(0x03c9,0x00); outp(0x03c9,0x00); outp(0x03c9,0x00);}
	
	for(j=0;j<256*3;j++) LT_Temp_palette[j] = 0x00;
	
	i = 0;
	//Fade in
	while (i < 64){
		outp(0x03c8,0);
		for(j=0;j<256*3;j++){
			if (LT_Temp_palette[j] < palette[j]) LT_Temp_palette[j]++;
			outp(0x03c9,LT_Temp_palette[j]);
		}
		while ((inp(0x03da) & 0x08));
		while (!(inp(0x03da) & 0x08));
		i ++;
	}
}

void MCGA_Fade_out(){
	int i,j;
	i = 0;
	//Fade to black
	outp(0x03c8,0);
	while (i < 64){
		for(j=0;j<256*3;j++){
			if (LT_Temp_palette[j] > 0) LT_Temp_palette[j]--;
			outp(0x03c9,LT_Temp_palette[j]);
		}
		while ((inp(0x03da) & 0x08));
		while (!(inp(0x03da) & 0x08));
		i ++;
	}
}

//init Cycle struct  
void cycle_init(COLORCYCLE *cycle,unsigned char *palette){
	cycle->frame = 0;
	cycle->counter = 0;
	cycle->palette = palette;
}

//Cycle palette  
void cycle_palette(COLORCYCLE *cycle, byte speed){
	int i;
	if (cycle->frame == 24) cycle->frame = 0;
	outp(0x03c8,1); //start at position 1 of palette index (colour 1)
	for(i=0;i!=24;i++) outp(0x03c9,cycle->palette[i+cycle->frame]);
	if (cycle->counter == speed){cycle->counter = 0; cycle->frame+=3;}
	cycle->counter++;
}
