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
unsigned char *LT_tile_tempdata; //Temp storage of non tiled data.
SPRITE LT_Loading_Animation; 

void LT_Error(char *error, char *file);

//GLOBAL VARIABLES

word FONT_VRAM = 0xC040; //0xC040
word TILE_VRAM = 0xC2C0; //0xC2C0

//Palette for fading
byte LT_Temp_palette[256*3];

//This points to video memory.
byte *VGA=(byte *)0xA0000000L; 

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 16;
int LT_current_x = 0;
int LT_last_x = 0;
int LT_current_y = 0;
int LT_last_y = 0;
int LT_map_offset = 0;
int LT_map_offset_Endless = 0;
int LT_follow = 0;

//NON SCROLLING WINDOW
byte LT_Window = 0; 				//Displaces everything (16 pixels) in case yo use the 320x16 window in the game

extern void interrupt (*LT_old_time_handler)(void); 

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

//32x32 animation for loading scene
void LT_Load_Animation(char *file, byte size){
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
	fread(&LT_Loading_Animation.width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_Loading_Animation.height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;

	for(index=0;index<num_colors;index++){
		LT_Loading_Animation.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_Loading_Animation.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_Loading_Animation.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		x=fgetc(fp);
	}

	for(index=(LT_Loading_Animation.height-1)*LT_Loading_Animation.width;index>=0;index-=LT_Loading_Animation.width){
		for(x=0;x<LT_Loading_Animation.width;x++){
			LT_tile_tempdata[index+x]=(byte)fgetc(fp);
		}
	}
	fclose(fp);
	
	index = 16384; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=0;tileY<LT_Loading_Animation.height;tileY+=size){
		for (tileX=0;tileX<LT_Loading_Animation.width;tileX+=size){
			offset = (tileY*LT_Loading_Animation.width)+tileX;
			LT_tile_tempdata[index] = size;
			LT_tile_tempdata[index+1] = size;
			index+=2;
			for(x=0;x<size;x++){
				memcpy(&LT_tile_tempdata[index],&LT_tile_tempdata[offset+(x*LT_Loading_Animation.width)],size);
				index+=size;
			}
		}
	}
	
	LT_Loading_Animation.nframes = (LT_Loading_Animation.width/size) * (LT_Loading_Animation.height/size);
	
	//calculate frames size
	if ((LT_Loading_Animation.frames = farcalloc(LT_Loading_Animation.nframes,sizeof(SPRITEFRAME))) == NULL){
		LT_Error("Not enough RAM to load animation\n",0);	
	}
	//estimated size of code
	fsize = (size * size * 7) / 2 + 25;
	for (frame = 0; frame < LT_Loading_Animation.nframes; frame++){		//
		if ((LT_Loading_Animation.frames[frame].compiled_code = farcalloc(fsize,sizeof(unsigned char))) == NULL){
			LT_Error("Not enough RAM to load animation frames ",0);
		}
		//COMPILE SPRITE FRAME TO X86 MACHINE CODE	
		//Store compiled code at &LT_tile_tempdata[16384]
		code_size = x_compile_bitmap(84, &LT_tile_tempdata[16384+(frame*2)+(frame*(size*size))],LT_Loading_Animation.frames[frame].compiled_code);
		//Store the compiled data at it's final destination
		LT_Loading_Animation.frames[frame].compiled_code = farrealloc(LT_Loading_Animation.frames[frame].compiled_code, code_size);
	}
	
	//IINIT SPRITE
	//sprite[sprite_number].bkg_data no bkg data
	LT_Loading_Animation.width = size;
	LT_Loading_Animation.height = size;
	LT_Loading_Animation.init = 0;
	LT_Loading_Animation.frame = 0;
	LT_Loading_Animation.baseframe = 0;
	LT_Loading_Animation.animate = 0;
	LT_Loading_Animation.anim_speed = 0;
	LT_Loading_Animation.anim_counter = 0;
}

void LT_Set_Animation(byte baseframe, byte frames, byte speed){
	LT_Loading_Animation.baseframe = baseframe;
	LT_Loading_Animation.aframes = frames;
	LT_Loading_Animation.speed = speed;
	LT_Loading_Animation.animate = 1;
}

void interrupt LT_Loading(void){
	SPRITE *s = &LT_Loading_Animation;
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

	x_run_compiled_sprite(s->pos_x,s->pos_y,s->frames[s->frame].compiled_code);
}

void LT_Set_Loading_Interrupt(){
	unsigned long spd = 1193182/30;
	int i;
	LT_Stop_Music();
	VGA_Fade_out();
	
	LT_WaitVsyncEnd();
	VGA_Scroll(0, 0);
	VGA_SplitScreen(0x0ffff); //disable split screen
	LT_ResetWindow();
	
	VGA_ClearPalette();
	VGA_ClearScreen();//clear screen
	
	//change color 0, 1, 2 (black and white)
	LT_Loading_Animation.palette[0] = LT_Loading_Palette[0];
	LT_Loading_Animation.palette[1] = LT_Loading_Palette[1];
	LT_Loading_Animation.palette[2] = LT_Loading_Palette[1];
	
	//center loading animation
	LT_Loading_Animation.pos_x = 144;
	LT_Loading_Animation.pos_y = 104;
	
	//set timer
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte
	//set interrupt
	setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
	
	LT_WaitVsyncStart();
	VGA_Fade_in(LT_Loading_Animation.palette);
}

void LT_Delete_Loading_Interrupt(){
	VGA_Fade_out();
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte
	setvect(0x1C, LT_old_time_handler);
}

//Hardware scrolling 
void VGA_Scroll(word x, word y){
	SCR_X = x;
	SCR_Y = y;
}

void VGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&VGA[0],0,(336>>2)*240);
}

byte p[5] = {0,2,4,6,8};
byte pix;

//wait for the VGA to stop drawing, and set scroll and Pel panning
void LT_WaitVsyncStart(){
	word x = SCR_X;
	word y = SCR_Y+LT_Window;
	
	y = (y<<6)+(y<<4)+(y<<2) + (x>>2);	//(y*64)+(y*16)+(y*4) = y*84;
	
	asm mov		dx,INPUT_STATUS_0
	WaitDELoop:
	asm in      al,dx
	asm and     al,DE_MASK
	asm jnz     WaitDELoop

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	outport(0x03d4, 0x0D | (y << 8));
	outport(0x03d4, 0x0C | (y & 0xff00));
	
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	//pixel panning value
	inportb(0x3DA); //Reset the VGA flip/flop
	pix = SCR_X & 3; 			
	outportb(AC_INDEX, AC_HPP);
	outportb(AC_INDEX, (byte) p[pix]);
}

//wait for the VGA to start drawing
void LT_WaitVsyncEnd(){
	asm mov		dx,INPUT_STATUS_0
	WaitVsync2:
	asm in      al,dx
	asm test    al,08h
	asm jz      WaitVsync2
	WaitNotVsync2:
	asm in      al,dx
	asm test    al,08h
	asm jnz     WaitNotVsync2
}

void VSYNC_HSYNC(int vpos_delay){
	int i;
	while(inportb(0x3DA)&8);    
	while(!(inportb(0x3DA)&8));				// Wait for vertical retrace 
 
	for(i=0;i<vpos_delay;i++){				// Wait for a number of horizontal retraces 
		while(inportb(0x3DA)&1);
		while(!(inportb(0x3DA)&1)); 		
	}
}

void VGA_SplitScreen(int line){
	line = line<<1;
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

void LT_SetWindow(char *file){
	word x,y = 0;
	word VGA_index = 0;
	word offset = 0;
	word num_colors = 0;
	byte plane = 0;
	dword index = 0;
	FILE *fp;
	
	LT_Window = 16;
	VGA_SplitScreen(14*16);
	
	fp = fopen(file,"rb");
	if(!fp) LT_Error("Can't find",file);
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
	}

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset.width*16, fp);
	
	for (plane = 0; plane < 4; plane ++){
		// select plane
		asm CLI //disable interrupts so that loading animation does not interfere
		
		outp(SC_INDEX, MAP_MASK);          
		outp(SC_DATA, 1 << plane);
		VGA_index = 0;	

		for (y = 15; y > 0; y--){
			offset = plane + (y*320);
			for (x = 0; x < 80; x++){
				VGA[VGA_index] = LT_tile_tempdata[offset];
				VGA_index++;
				offset +=4;
			}
			VGA_index+=4;
		}
		asm STI //Re enable interrupts so that loading animation is played again
	}
}

void LT_ResetWindow(){
	LT_Window = 0;
}

// load_8x8 fonts to VRAM (31 characters)
void LT_Load_Font(char *file){
	word VGA_index = 0;
	word w = 0;
	word h = 0;
	word ty = 0;
	int jx = 0;
	word x = 0;
	word y = 0;
	word tileX = 0;
	word tileY = 0;
	word num_colors = 0;
	byte plane = 0;
	dword index = 0;
	dword offset = 0;
	FILE *fp;
	
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
	}

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset.width*LT_tileset.height, fp);
	
	//COPY TO VRAM
	w = LT_tileset.width>>3;
	h = LT_tileset.height>>3;
	jx = LT_tileset.width+8; 
	
	for (plane = 0; plane < 4; plane ++){
		// select plane
		asm CLI //disable interrupts so that loading animation does not interfere
		
		outp(SC_INDEX, MAP_MASK);          
		outp(SC_DATA, 1 << plane);
		VGA_index = FONT_VRAM;	//VRAM FONT ADDRESS  //586*(336/4);
		
		//SCAN ALL TILES
		for (tileY = h; tileY > 0 ; tileY--){
			ty = (tileY<<3)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = plane + (ty*LT_tileset.width) + (tileX<<3);
				//LOAD TILE
				x=0;
				for(y = 0; y < 16; y++){
					VGA[VGA_index] = LT_tile_tempdata[offset];
					VGA_index++;
					offset +=4;
					x++;
					if (x == 2){
						x = 0;
						offset -= jx;
					}
				}
			}
		}
		asm STI //Re enable interrupts so that loading animation is played again
	}
}

//Print a three digit variable on the window, only 40 positions available 
void LT_Print_Window_Variable(byte x, word var){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset = (84<<2) + (x<<1) + 6;

	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 

	asm{
		push ds
		push di
		push si
	}
	asm{//DIVIDE VAR BY 10, GET REMAINDER, DRAW DIGIT
		mov dx,0 
		mov ax,0
		mov	ax,[var]				
		mov bx,10
		div bx								//Divides var by 10 // dx = remainder // ax = var/10
		mov [var],ax
		
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,FONT_ADDRESS;					//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		mov		cl,4						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax

		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82 //END LOOP
	}
	asm{//DIVIDE VAR BY 10, GET REMAINDER, DRAW DIGIT
		mov dx,0 
		mov ax,0
		mov	ax,[var]				
		mov bx,10
		div bx								//Divides var by 10 // dx = remainder // ax = var/10
		mov [var],ax
		
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,FONT_ADDRESS;					//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		mov		cl,4						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset		//es:di destination address	
		sub		di,2
		mov 	ax,0A000h
		mov 	es,ax

		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82 //END LOOP
		
		sub		di,2 //NEXT DIGIT POSITION (-
	}
	asm{//DIVIDE VAR BY 10, GET REMAINDER, DRAW DIGIT
		mov dx,0 
		mov ax,0
		mov	ax,[var]				
		mov bx,10
		div bx								//Divides var by 10 // dx = remainder // ax = var/10
		mov [var],ax
		
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,FONT_ADDRESS;					//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		mov		cl,4						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset		//es:di destination address
		sub		di,4
		mov 	ax,0A000h
		mov 	es,ax

		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
		add 	di,82
		mov 	cx,2
		rep		movsb				
	}
		
	asm{//END	
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

// load_16x16 tiles to VRAM
void LT_Load_Tiles(char *file){
	dword VGA_index = 0;
	word w = 0;
	word h = 0;
	word ty = 0;
	int jx = 0;
	word x = 0;
	word y = 0;
	word tileX = 0;
	word tileY = 0;
	word num_colors = 0;
	byte plane = 0;
	dword index = 0;
	dword offset = 0;
	FILE *fp;
	
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	LT_tileset.ntiles = (LT_tileset.width>>4) * (LT_tileset.height>>4);

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset.width*LT_tileset.height, fp);
	
	//COPY TO VRAM
	w = LT_tileset.width>>4;
	h = LT_tileset.height>>4;
	jx = LT_tileset.width+16; 
	
	for (plane = 0; plane < 4; plane ++){
		// select plane
		asm CLI //disable interrupts so that loading animation does not interfere
		
		outp(SC_INDEX, MAP_MASK);          
		outp(SC_DATA, 1 << plane);
		VGA_index = TILE_VRAM;
		
		//SCAN ALL TILES
		for (tileY = h; tileY > 0 ; tileY--){
			ty = (tileY<<4)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = plane + (ty*LT_tileset.width) + (tileX<<4);
				//LOAD TILE
				x=0;
				for(y = 0; y < 64; y++){
					VGA[VGA_index] = LT_tile_tempdata[offset];
					VGA_index++;
					if (VGA_index > 0x10080) LT_Error("Too many tiles\n",0);
					offset +=4;
					x++;
					if (x == 4){
						x = 0;
						offset -= jx;
					}
				}
			}
		}
		asm STI //Re enable interrupts so that loading animation is played again
	}

}

void LT_unload_tileset(){
	LT_tileset.width = NULL;
	LT_tileset.height = NULL;
	LT_tileset.ntiles = NULL;
	*LT_tileset.palette = NULL;
	free(LT_tile_tempdata); LT_tile_tempdata = NULL;
}

//Load tiled TMX map in CSV format
//Be sure bkg layer map is the first to be exported, (before collision layer map)
void LT_Load_Map(char *file){ 
	FILE *f = fopen(file, "rb");
	word start_bkg_data = 0;
	word start_col_data = 0;
	word tile = 0;
	word index = 0;
	byte tilecount = 0; //Just to get the number of tiles to substract to collision tiles 
	char line[128];
	char name[64]; //name of the layer in TILED

	if(!f) LT_Error("Can't find ",file);
	
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
	LT_current_y = ((SCR_Y+LT_Window)>>4)<<4;
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
	VGA_Scroll(SCR_X,SCR_Y);
	//draw map 
	for (i = 0;i<336;i+=16){draw_map_column(SCR_X+i,SCR_Y+LT_Window,LT_map_offset+j);j++;}	
	VGA_Fade_in(LT_tileset.palette);
}

void LT_Edit_MapTile(word x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM;
	word tile = (y * LT_map.width) + x;
	word screen_offset;
	x = x<<4;
	y = (y<<4)+LT_Window;
	screen_offset = (y<<6)+(y<<4)+(y<<2) + (x>>2);
	LT_map.collision[tile] = col;
	
	//LT_WaitVsyncEnd();
	
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 

	asm{
		push ds
		push di
		push si
		
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si source VRAM TILE ADDRESS  //590*(336/4);
		
		mov		al,byte ptr [ntile]		//go to desired tile
		mov		ah,0	
		mov		cl,6					//*64
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax

		//UNWRAPPED COPY 16x16 TILE LOOP
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80

		//END LOOP
		
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
	
	LT_map.data[tile] = ntile;
}

void draw_map_column(word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM;
	word screen_offset = (y<<6)+(y<<4)+(y<<2)+(x>>2);
	word width = LT_map.width;
	unsigned char *mapdata = LT_map.data;
	
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
	outport(GC_INDEX, 0x08); 
	
	asm{
		push ds
		push di
		push si
		
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address							
	}
	//UNWRAPPED LOOP
	//DRAW 16 TILES
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80	//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		mov 	cx,4		//COPY TILE (16 LINES)
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
	}

	asm{//END
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

void draw_map_row( word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM;
	unsigned char *mapdata = LT_map.data;
	word screen_offset = (y<<6)+(y<<4)+(y<<2) + (x>>2);
	
	outport(SC_INDEX, (0xff << 8) + MAP_MASK); //select all planes
    outport(GC_INDEX, 0x08); 
	
	asm{//SET ADDRESS
		push ds
		push di
		push si
			
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address								
	}
	//UNWRAPPED LOOPS
	//Copy 21 tiles
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,6
		shl		ax,cl
		add		si,ax
		
		sub		di,84*16
		add		di,4						//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		mov 	cx,4		//COPY 16 LINES
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
		add 	di,80
		mov 	cx,4
		rep		movsb				
	}
	
	asm{//END	
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

//update rows and colums
void LT_scroll_map(){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = ((SCR_Y+LT_Window)>>4)<<4;
	
	if (LT_current_x != LT_last_x) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_x < LT_last_x) 
			draw_map_column(LT_current_x,LT_current_y,LT_map_offset); 
		else  
			draw_map_column(LT_current_x+320,LT_current_y,LT_map_offset+20);
	}
	if (LT_current_y != LT_last_y) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_y < LT_last_y)
			draw_map_row(LT_current_x,LT_current_y,LT_map_offset);
		else 
			draw_map_row(LT_current_x,LT_current_y+240,LT_map_offset+(15*LT_map.width));
	}
	
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
}

//Endless Side Scrolling Map
void LT_Endless_SideScroll_Map(int y){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = ((SCR_Y+LT_Window)>>4)<<4;
	if (LT_current_x > LT_last_x) { 
		draw_map_column(LT_current_x+320,LT_current_y,((LT_map_offset_Endless+20)%LT_map.width)+(y*LT_map.width));
		LT_map_offset_Endless++;
	}
	LT_last_x = LT_current_x;
}

void set_palette(unsigned char *palette){
	int i = 0;
	outp(0x03c8,0); 
	LT_WaitVsyncEnd();
	for(i=0;i<256*3;i+=12){
		outp(0x03c9,palette[i]);outp(0x03c9,palette[i+1]);outp(0x03c9,palette[i+2]);
		outp(0x03c9,palette[i+3]);outp(0x03c9,palette[i+4]);outp(0x03c9,palette[i+5]);
		outp(0x03c9,palette[i+6]);outp(0x03c9,palette[i+7]);outp(0x03c9,palette[i+8]);
		outp(0x03c9,palette[i+9]);outp(0x03c9,palette[i+10]);outp(0x03c9,palette[i+11]);
	}
	LT_WaitVsyncStart();
}

void VGA_ClearPalette(){
	int i;
	LT_WaitVsyncEnd();
	outp(0x03c8,0); 
	for(i=0;i<256*3;i+=12){
		outp(0x03c9,0);outp(0x03c9,0);outp(0x03c9,0);
		outp(0x03c9,0);outp(0x03c9,0);outp(0x03c9,0);
		outp(0x03c9,0);outp(0x03c9,0);outp(0x03c9,0);
		outp(0x03c9,0);outp(0x03c9,0);outp(0x03c9,0);
	}
	LT_WaitVsyncStart();
}

void VGA_Fade_in(unsigned char *palette){
	int i,j;
	byte c;
	//All colours black
	VGA_ClearPalette();
	LT_WaitVsyncStart();
	for(j=0;j<256*3;j++) LT_Temp_palette[j] = 0x00;
	i = 0;
	//Fade in
	while (i < 15){
		LT_WaitVsyncEnd();
		outp(0x03c8,0);
		for(j=0;j<256*3;j++){
			if (LT_Temp_palette[j] < palette[j]) LT_Temp_palette[j]+=4;
			outp(0x03c9,LT_Temp_palette[j]);
		}
		i ++;
		LT_WaitVsyncStart();
	}
}

void VGA_Fade_out(){
	int i,j;
	i = 0;
	//Fade to black
	outp(0x03c8,0);
	while (i < 15){
		LT_WaitVsyncEnd();
		for(j=0;j<256*3;j++){
			if (LT_Temp_palette[j] > 0) LT_Temp_palette[j]-=4;
			outp(0x03c9,LT_Temp_palette[j]);
		}
		while ((inp(0x03da) & 0x08));
		while (!(inp(0x03da) & 0x08));
		i ++;
		LT_WaitVsyncStart();
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
