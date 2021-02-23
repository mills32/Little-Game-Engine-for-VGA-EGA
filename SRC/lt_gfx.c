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

// One map in ram stored at "LT_map"
word LT_map_width;
word LT_map_height;
word LT_map_ntiles;
byte *LT_map_data;
byte *LT_map_collision;
// One tileset 
word LT_tileset_width;
word LT_tileset_height;
word LT_tileset_ntiles;
byte LT_tileset_palette[256*3];
byte *LT_tileset_data;
unsigned char *LT_tile_tempdata; //Temp storage of non tiled data. and also sound samples
unsigned char *LT_tile_tempdata2; //Second half 
extern unsigned char *LT_sprite_data; // 
extern dword LT_sprite_data_offset;
SPRITE LT_Loading_Animation; 

void LT_Error(char *error, char *file);
void LT_Reset_Sprite_Stack();
void LT_Reset_AI_Stack();

extern byte LT_Sprite_Stack;
extern byte LT_Sprite_Stack_Table[];
extern byte LT_AI_Stack;
extern byte LT_AI_Stack_Table[];
extern byte selected_AI_sprite;

//GLOBAL VARIABLES

word FONT_VRAM = 0xC040; //0xC040
word TILE_VRAM = 0xC440; //0xC2C0
word FONT_VRAM_EGA = 0x5C00;
word TILE_VRAM_EGA = 0x5E00;
//Palette for fading
byte LT_Temp_palette[256*3];

//This points to video memory.
byte *VGA=(byte *)0xA0000000L; 
byte *EGA=(byte *)0xA0000000L;

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
int vpage = 0;
int Enemies = 0;
//NON SCROLLING WINDOW
byte LT_Window = 0; 				//Displaces everything (16 pixels) in case yo use the 320x16 window in the game

extern void interrupt (*LT_old_time_handler)(void); 

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

void vsync(){
	asm mov		dx,INPUT_STATUS_0
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	
	//while( inp( INPUT_STATUS_0 ) & 0x08 );
	//while( !(inp( INPUT_STATUS_0 ) & 0x08 ) );
}

// load_16x16 tiles to VRAM
void LT_Load_Animation_EGA(char *file){
	dword EGA_index = 0;
	word w = 0;
	word h = 0;
	word ty = 0;
	int jx = 0;
	//word x = 0;
	word y = 0;
	word tileX = 0;
	word tileY = 0;
	word num_colors = 0;
	//byte plane = 0;
	dword index = 0;
	dword offset = 0;
	FILE *fp;
	
	//Open the same file with ".ega" extension for ega
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset_width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset_height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset_palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset_width*LT_tileset_height, fp);
	
	fclose(fp);
	

	//COPY TO EGA VRAM
	w = LT_tileset_width>>4;
	h = LT_tileset_height>>4;
	jx = LT_tileset_width+16;
	
	EGA_index = 0x0000;
	asm CLI //disable interrupts so that loading animation does not interfere
	//SCAN ALL TILES
	for (tileY = h; tileY > 0 ; tileY--){
		ty = (tileY<<4)-1;
		for (tileX = 0; tileX < w; tileX++){
			offset = (ty*LT_tileset_width) + (tileX<<4);
			//LOAD TILE
			for(y = 0; y < 16; y++){//Get 2 chunks of 8 pixels per row
				int i,j,k;
				for (j = 0; j < 2; j++){
					//Get an 8 pixel chunk, and convert to 4 bytes
					byte pl0 = 0;
					byte pl1 = 0;
					byte pl2 = 0;
					byte pl3 = 0;
					for(i = 0; i < 8; i++){
						//i defines the inverted shift (the pixel to activate)
						int pixel = LT_tile_tempdata[offset];
						k = 7-i;
						//Store bitplanes in pl0,pl1,pl2,pl3
						switch (pixel){
							case 0: pl0 |= 0 << k; break;	// black, bit 0 in byte-plane 0
							case 1:	pl0 |= 1 << k; break;	// dark blue, bit 1 in b-plane 0
							case 2: pl1 |= 1 << k; break;	// dark green, bit 1 in b-plane 1
							case 3: pl2 |= 1 << k; break;	// dark red, bit 1 in b-plane 2
							case 4:	pl0 |= 1 << k; pl1 |= 1 << k; break;// dark cyan, bit 1 in b-planes 0,1
							case 5:	pl1 |= 1 << k; pl2 |= 1 << k; break;// dark yellow, bit 1 in b-plane 1,2
							case 6:	pl0 |= 1 << k; pl2 |= 1 << k; break;// dark magenta, bit 1 in b-plane 0,2	
							case 7:	pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; break;// light gray, bit 1 in b-plane 0,1,2
							//now the same but activating the intensity plane (3)
							case 8: pl0 |= 0 << k; pl3 |= 1 << k; break;// dark gray, bit 0 in b-plane 0; bit 1 in b-plane 3
							case 9:	pl0 |= 1 << k; pl3 |= 1 << k; break;// light blue, bit 1 in b-plane 0,3
							case 10: pl1 |= 1 << k; pl3 |= 1 << k; break;// light green, bit 1 in b-plane 1,3	
							case 11: pl2 |= 1 << k; pl3 |= 1 << k; break;// light red, bit 1 in b-plane 2,3
							case 12: pl0 |= 1 << k; pl1 |= 1 << k; pl3 |= 1 << k; break;// light cyan, bit 1 in b-plane 0,1,3
							case 13: pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light yellow, bit 1 in b-plane 1,2,3
							case 14: pl0 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light magenta, bit 1 in b-plane 0,2,3
							case 15: pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// white, all metro lines
						}
						offset++;
					}
					
					//Paste pl1,pl2,pl3,pl4 to EGA address
					
					asm mov dx, 03C4h
					asm mov ax, 0102h	//Plane 0		
					asm out dx, ax
					EGA[EGA_index] = pl0;
	
					asm mov ax, 0202h 	//Plane 1
					asm out dx, ax
					EGA[EGA_index] = pl1;
			
					asm mov ax, 0402h 	//Plane 2
					asm out dx, ax
					EGA[EGA_index] = pl2;
	
					asm mov ax, 0802h 	//Plane 3
					asm out dx, ax
					EGA[EGA_index] = pl3;
					
					EGA_index++;
					
					if (EGA_index > 0x7680) LT_Error("Too many tiles\n",0);
				}
				
				offset -= jx;
			}
		}
	}
	asm STI //Re enable interrupts so that loading animation is played again
}

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
		LT_tileset_palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		x=fgetc(fp);
	}

	for(index=(LT_Loading_Animation.height-1)*LT_Loading_Animation.width;index>=0;index-=LT_Loading_Animation.width){
		for(x=0;x<LT_Loading_Animation.width;x++){
			LT_tile_tempdata[index+x]=(byte)fgetc(fp);
		}
	}
	fclose(fp);
	
	index = 0; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=0;tileY<LT_Loading_Animation.height;tileY+=size){
		for (tileX=0;tileX<LT_Loading_Animation.width;tileX+=size){
			offset = (tileY*LT_Loading_Animation.width)+tileX;
			LT_tile_tempdata2[index] = size;
			LT_tile_tempdata2[index+1] = size;
			index+=2;
			for(x=0;x<size;x++){
				memcpy(&LT_tile_tempdata2[index],&LT_tile_tempdata[offset+(x*LT_Loading_Animation.width)],size);
				index+=size;
			}
		}
	}
	
	LT_Loading_Animation.nframes = (LT_Loading_Animation.width/size) * (LT_Loading_Animation.height/size);
	LT_Loading_Animation.code_size = 0;
	//estimated size of code
	//fsize = (size * size * 7) / 2 + 25;
	for (frame = 0; frame < LT_Loading_Animation.nframes; frame++){		//
		/*if ((LT_Loading_Animation.frames[frame].compiled_code = farcalloc(fsize,sizeof(unsigned char))) == NULL){
			LT_Error("Not enough RAM to load animation frames ",0);
		}*/
		//COMPILE SPRITE FRAME TO X86 MACHINE CODE
		//& Store the compiled data at it's final destination
		code_size = x_compile_bitmap(84, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],&LT_sprite_data[LT_sprite_data_offset]);
		LT_Loading_Animation.frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
		LT_sprite_data_offset += code_size;
		LT_Loading_Animation.code_size += code_size;
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

void run_compiled_sprite(word XPos, word YPos, char *Sprite);
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

	run_compiled_sprite(s->pos_x,s->pos_y,s->frames[s->frame].compiled_code);
}

void LT_Set_Loading_Interrupt(){
	unsigned long spd = 1193182/30;
	
	LT_Stop_Music();
	LT_Fade_out();
	
	LT_Reset_Sprite_Stack();
	LT_Reset_AI_Stack();
	
	//Wait Vsync
	vsync();
	//Reset scroll
	outport(0x03d4, 0x0D | (0 << 8));
	outport(0x03d4, 0x0C | (0 & 0xFF00));
	
	VGA_Scroll(0, 0);
	VGA_SplitScreen(0x0ffff); //disable split screen
	LT_ResetWindow();
	
	VGA_ClearPalette();
	VGA_ClearScreen();//clear screen
	
	//change color 0, 1, 2 (black and white)
	LT_tileset_palette[0] = LT_Loading_Palette[0];
	LT_tileset_palette[1] = LT_Loading_Palette[1];
	LT_tileset_palette[2] = LT_Loading_Palette[1];
	
	//center loading animation
	LT_Loading_Animation.pos_x = 144;
	LT_Loading_Animation.pos_y = 104;
	
	//set timer
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte
	//set interrupt
	setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
	
	//Wait Vsync
	vsync();	
	LT_Fade_in();
}

void LT_Delete_Loading_Interrupt(){
	LT_Fade_out();
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

byte p[4] = {0,2,4,6};
byte pix;

void (*LT_WaitVsync)(void);
//works smooth in dosbox, PCem, Real EGA/VGA.
//Choppy on SVGA or modern VGA compatibles
void LT_WaitVsync0_EGA(){
	byte _ac;
	word x = SCR_X;
	word y = SCR_Y+LT_Window;
	
	//This is as optimized as it can get wen compiled to asm (I checked)
	y = (y<<5)+(y<<3)+(y<<1)+(x>>3);//y*42 + (x>>3);
	
	//change scroll registers: LOW_ADDRESS 0x0D; HIGH_ADDRESS 0x0C;
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port

	//The smooth panning magic happens here
	//disable interrupts because we are doing crazy things
	asm cli
	
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	asm mov		dx,INPUT_STATUS_0 //Read input status, to Reset the VGA flip/flop
	_ac = inp(AC_INDEX);//Store the value of the controller
	pix = SCR_X & 7;//EGA

	//AC index 0x03c0
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,byte ptr pix
	asm out dx,al
	
	//Restore controller value
	asm mov ax,word ptr _ac
	asm out dx,ax
	
	//enable interrupts because we finished doing crazy things
	asm sti
}

void LT_WaitVsync0_VGA(){
	byte _ac;
	word x = SCR_X;
	word y = SCR_Y+LT_Window;
	
	//This is as optimized as it can get wen compiled to asm (I checked)
	y = (y<<6)+(y<<4)+(y<<2) + (x>>2);	//(y*64)+(y*16)+(y*4) = y*84; + x/4
	
	//change scroll registers: LOW_ADDRESS 0x0D; HIGH_ADDRESS 0x0C;
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port

	//The smooth panning magic happens here
	//disable interrupts because we are doing crazy things
	asm cli
	
	//Wait Vsync
	asm mov		dx,INPUT_STATUS_0
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	asm mov		dx,INPUT_STATUS_0 //Read input status, to Reset the VGA flip/flop
	_ac = inp(AC_INDEX);//Store the value of the controller
	pix = p[SCR_X & 3]; //VGA

	//AC index 0x03c0
	asm mov dx,003c0h
	asm mov ax,033h
	asm out dx,ax
	asm mov al,byte ptr pix
	asm out dx,al
	
	//Restore controller value
	asm mov ax,word ptr _ac
	asm out dx,ax
	
	//enable interrupts because we finished doing crazy things
	asm sti
}

//Works on all compatible VGA cards from SVGA to 2020 GPUS.
//Choppy on real EGA/VGA
void LT_WaitVsync1_EGA(){
	word x = SCR_X;
	word y = SCR_Y+LT_Window;
	y = (y<<5)+(y<<3)+(y<<1)+(x>>3);//y*42 + (x>>3);
	
	asm mov		dx,INPUT_STATUS_0
	WaitDELoop:
	asm in      al,dx
	asm and     al,DE_MASK
	asm jnz     WaitDELoop

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
	
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
	pix = SCR_X & 7;//EGA			
	outportb(AC_INDEX, AC_HPP);
	outportb(AC_INDEX, (byte) p[pix]);

}

void LT_WaitVsync1_VGA(){
	word x = SCR_X;
	word y = SCR_Y+LT_Window;
	y = (y<<6)+(y<<4)+(y<<2) + (x>>2);	//(y*64)+(y*16)+(y*4) = y*84;
	
	asm mov		dx,INPUT_STATUS_0
	WaitDELoop:
	asm in      al,dx
	asm and     al,DE_MASK
	asm jnz     WaitDELoop

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
	
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
	
	mov		dx,CRTC_INDEX
	mov		ax,line
	mov    	cl,8
	shl		ax,cl
	add		ax,LINE_COMPARE
	out    	dx,ax		// set bits 7-0 of the split screen scan line

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

	
	//Turn on split screen pel panning suppression, so the split screen
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

//Reduced split screen function, just to move the position in 320x240 mode
void LT_MoveWindow(int line){
    asm{
	push    ax
    push    cx
    push    dx
    cli
	
	mov		dx,CRTC_INDEX
	mov		ax,line
	mov    	cl,9
	shl		ax,cl
	add		ax,LINE_COMPARE
	out    	dx,ax

    mov    ah,byte ptr [line+1]
    and    ah,1
    mov    cl,4
    shl    ah,cl
    mov    al,OVERFLOW

	out    dx,al                      
    inc    dx                         
    in     al,dx                      
    and    al,not 10h                 
    or     al,ah                      
    out    dx,al                      

    sti
    pop    dx
    pop    cx
    pop    ax
    }
	line = 0;
}

void LT_SetWindow(char *file){
	int i;
	word x,y = 0;
	word VGA_index = 0;
	word EGA_index = 0;
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
	fread(&LT_tileset_width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset_height,sizeof(word), 1, fp);
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
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset_width*16, fp);
	
	if (LT_VIDEO_MODE ==0){
		asm CLI //disable interrupts so that loading animation does not interfere
		EGA_index = 0;	
		for (y = 15; y > 0; y--){
			offset = y*320;
			for (x = 0; x < 320; x+=8){
				//Get an 8 pixel chunk, and convert to 4 bytes
				byte pl0 = 0;
				byte pl1 = 0;
				byte pl2 = 0;
				byte pl3 = 0;
				for(i = 0; i < 8; i++){
					//i defines the inverted shift (the pixel to activate)
					int pixel = LT_tile_tempdata[offset];
					int k = 7-i;
					//Store bitplanes in pl0,pl1,pl2,pl3
					switch (pixel){
						case 0: pl0 |= 0 << k; break;	// black, bit 0 in byte-plane 0
						case 1:	pl0 |= 1 << k; break;	// dark blue, bit 1 in b-plane 0
						case 2: pl1 |= 1 << k; break;	// dark green, bit 1 in b-plane 1
						case 3: pl2 |= 1 << k; break;	// dark red, bit 1 in b-plane 2
						case 4:	pl0 |= 1 << k; pl1 |= 1 << k; break;// dark cyan, bit 1 in b-planes 0,1
						case 5:	pl1 |= 1 << k; pl2 |= 1 << k; break;// dark yellow, bit 1 in b-plane 1,2
						case 6:	pl0 |= 1 << k; pl2 |= 1 << k; break;// dark magenta, bit 1 in b-plane 0,2	
						case 7:	pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; break;// light gray, bit 1 in b-plane 0,1,2
						//now the same but activating the intensity plane (3)
						case 8: pl0 |= 0 << k; pl3 |= 1 << k; break;// dark gray, bit 0 in b-plane 0; bit 1 in b-plane 3
						case 9:	pl0 |= 1 << k; pl3 |= 1 << k; break;// light blue, bit 1 in b-plane 0,3
						case 10: pl1 |= 1 << k; pl3 |= 1 << k; break;// light green, bit 1 in b-plane 1,3	
						case 11: pl2 |= 1 << k; pl3 |= 1 << k; break;// light red, bit 1 in b-plane 2,3
						case 12: pl0 |= 1 << k; pl1 |= 1 << k; pl3 |= 1 << k; break;// light cyan, bit 1 in b-plane 0,1,3
						case 13: pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light yellow, bit 1 in b-plane 1,2,3
						case 14: pl0 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light magenta, bit 1 in b-plane 0,2,3
						case 15: pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// white, all metro lines
					}
					offset++;
				}
				
				//Paste pl1,pl2,pl3,pl4 to EGA address
				
				asm mov dx, 03C4h
				asm mov ax, 0102h	//Plane 0		
				asm out dx, ax
				EGA[EGA_index] = pl0;
		
				asm mov ax, 0202h 	//Plane 1
				asm out dx, ax
				EGA[EGA_index] = pl1;
				
				asm mov ax, 0402h 	//Plane 2
				asm out dx, ax
				EGA[EGA_index] = pl2;
	
				asm mov ax, 0802h 	//Plane 3
				asm out dx, ax
				EGA[EGA_index] = pl3;
				
				EGA_index++;
				
				//if (EGA_index > 0x7680) LT_Error("Too many tiles\n",0);
			}
			EGA_index+=2;
		}
		asm STI //Re enable interrupts so that loading animation is played again
	}else {
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
}

void LT_ResetWindow(){
	LT_Window = 0;
}

// load_8x8 fonts to VRAM (64 characters)
void LT_Load_Font(char *file){
	word VGA_index = 0;
	word EGA_index = 0;
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
	fread(&LT_tileset_width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset_height,sizeof(word), 1, fp);
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
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset_width*LT_tileset_height, fp);
	fclose(fp);
	//COPY TO VRAM
	if (LT_tileset_width != 128) LT_Error("Font file must be 128 pixels wide ",file);
	w = 16;//LT_tileset_width>>3;
	h = 4;//LT_tileset_height>>3;
	jx = LT_tileset_width+8; 
	
	if (LT_VIDEO_MODE == 0){
		//COPY TO FONT VRAM
		EGA_index = FONT_VRAM_EGA;
		asm CLI //disable interrupts so that loading animation does not interfere
		//SCAN ALL TILES
		for (tileY = h; tileY > 0 ; tileY--){
			ty = (tileY<<3)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = (ty*LT_tileset_width) + (tileX<<3);
				//LOAD TILE
				for(y = 0; y < 8; y++){//Get 1 chunk of 8 pixels per row
					int i,j,k;
					//Get an 8 pixel chunk, and convert to 4 bytes
					byte pl0 = 0;
					byte pl1 = 0;
					byte pl2 = 0;
					byte pl3 = 0;
					for(i = 0; i < 8; i++){
						//i defines the inverted shift (the pixel to activate)
						int pixel = LT_tile_tempdata[offset];
						k = 7-i;
						//Store bitplanes in pl0,pl1,pl2,pl3
						switch (pixel){
							case 0: pl0 |= 0 << k; break;	// black, bit 0 in byte-plane 0
							case 1:	pl0 |= 1 << k; break;	// dark blue, bit 1 in b-plane 0
							case 2: pl1 |= 1 << k; break;	// dark green, bit 1 in b-plane 1
							case 3: pl2 |= 1 << k; break;	// dark red, bit 1 in b-plane 2
							case 4:	pl0 |= 1 << k; pl1 |= 1 << k; break;// dark cyan, bit 1 in b-planes 0,1
							case 5:	pl1 |= 1 << k; pl2 |= 1 << k; break;// dark yellow, bit 1 in b-plane 1,2
							case 6:	pl0 |= 1 << k; pl2 |= 1 << k; break;// dark magenta, bit 1 in b-plane 0,2	
							case 7:	pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; break;// light gray, bit 1 in b-plane 0,1,2
							//now the same but activating the intensity plane (3)
							case 8: pl0 |= 0 << k; pl3 |= 1 << k; break;// dark gray, bit 0 in b-plane 0; bit 1 in b-plane 3
							case 9:	pl0 |= 1 << k; pl3 |= 1 << k; break;// light blue, bit 1 in b-plane 0,3
							case 10: pl1 |= 1 << k; pl3 |= 1 << k; break;// light green, bit 1 in b-plane 1,3	
							case 11: pl2 |= 1 << k; pl3 |= 1 << k; break;// light red, bit 1 in b-plane 2,3
							case 12: pl0 |= 1 << k; pl1 |= 1 << k; pl3 |= 1 << k; break;// light cyan, bit 1 in b-plane 0,1,3
							case 13: pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light yellow, bit 1 in b-plane 1,2,3
							case 14: pl0 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light magenta, bit 1 in b-plane 0,2,3
							case 15: pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// white, all metro lines
						}
						offset++;
					}
					
					//Paste pl1,pl2,pl3,pl4 to EGA address
					
					asm mov dx, 03C4h
					asm mov ax, 0102h	//Plane 0		
					asm out dx, ax
					EGA[EGA_index] = pl0;
		
					asm mov ax, 0202h 	//Plane 1
					asm out dx, ax
					EGA[EGA_index] = pl1;
				
					asm mov ax, 0402h 	//Plane 2
					asm out dx, ax
					EGA[EGA_index] = pl2;
	
					asm mov ax, 0802h 	//Plane 3
					asm out dx, ax
					EGA[EGA_index] = pl3;
					
					EGA_index++;
					
					if (EGA_index > 0x7680) LT_Error("Too many tiles\n",0);
					offset -= jx;
				}
			}
		}
		asm STI //Re enable interrupts so that loading animation is played again
	} else {
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
				offset = plane + (ty*LT_tileset_width) + (tileX<<3);
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

}

//Print a three digit variable on the window, only 40 positions available 
void (*LT_Print_Window_Variable)(byte,word);

void LT_Print_Window_Variable_VGA(byte x, word var){
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
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		add		dx,16
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
		add		dx,16
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
		add		dx,16
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

void LT_Print_Window_Variable_EGA(byte x, word var){
	word FONT_ADDRESS = FONT_VRAM_EGA;
	word screen_offset = (42<<2) + (x) + 3;
	
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
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		add		dx,16
		mov		cl,3						//dx*8
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset			//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (8 lines)
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
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
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		add		dx,16
		mov		cl,3						//dx*8
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset			//es:di destination address	
		sub		di,1
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (8 lines)
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
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
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS = 586*(336/4);
		
		//go to desired tile
		add		dx,16
		mov		cl,3						//dx*8
		shl		dx,cl
		add		si,dx
		
		mov		di,screen_offset			//es:di destination address	
		sub		di,2
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (8 lines)
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
	}

	asm{//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

//Print 8x8 tiles, it is a bit slow on 8086, but it works for text boxes
//Text Box borders: # UL; $ UR; % DL; & DR; ( UP; ) DOWN; * LEFT; + RIGHT;
//Special latin characters: 
//	- : Spanish N with tilde;
//	< : Inverse question;
//	= : Inverse exclamation;
//	> : C cedille
//	[ \ ] ^ _ : AEIOU with tilde. Use double "\\" in strings to represent backslash
void (*LT_Print)(word,word,char*,byte);

void LT_Print_VGA(word x, word y, char *string, byte win){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset;
	byte datastring;
	word size = strlen(string);
	byte i = 0;
	if (!win) y += 240-44;//LT_Window;
	else y = (y<<3);
	screen_offset = (y<<6)+(y<<4)+(y<<2);
	if (size > 40) size = 40;
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
		//
		mov	di,screen_offset
		mov ax,x
		shl	ax,1
		add di,ax
		mov ax,0A000h
		mov ds,ax
		mov bx,size
	}
	printloop3:
	asm push bx
	datastring = string[i];
	asm{
		mov		dx,word ptr datastring
		sub		dx,32
		
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS
		
		//go to desired tile
		mov		cl,4						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov 	ax,0A000h
		mov 	es,ax						//es:di destination address	

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
		//END LOOP
		sub		di,588
	}
	i++;
	asm pop 	bx
	asm dec		bx
	asm jnz		printloop3
		
	asm{//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
}

void LT_Print_EGA(word x, word y, char *string, byte win){
	word FONT_ADDRESS = FONT_VRAM_EGA;
	byte datastring;
	word size = strlen(string);
	byte i = 0;
	word screen_offset;
	if (!win) y += 240-44;//LT_Window;
	y = (y<<3);
	screen_offset = (y<<5)+(y<<3)+(y<<1);
	if (size > 40) size = 40;
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
		//
		mov	di,screen_offset
		mov ax,x
		add di,ax
		mov ax,0A000h
		mov ds,ax
		mov bx,size
	}
	printloop3:
	asm push bx
	datastring = string[i];
	asm{
		mov		dx,word ptr datastring
		sub		dx,32
		
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS
		
		//go to desired tile
		mov		cl,3						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov 	ax,0A000h
		mov 	es,ax						//es:di destination address	

		//UNWRAPPED COPY 8x8 TILE LOOP
		movsb			//COPY TILE (8 lines)
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb
		add di,41
		movsb			
		add di,41 //END LOOP
		
		sub	di,335
	}
		i++;
	asm pop 	bx
	asm dec		bx
	asm jnz		printloop3
		
	asm{//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

void LT_Draw_Text_Box(byte x, byte y, byte w, byte h, byte win){
	int i;
	unsigned char up[40];
	unsigned char mid[40];
	unsigned char down[40];

	up[0] = '#'; up[w+1] = '$'; up[w+2] = 0;
	mid[0] = '*'; mid[w+1] = '+'; mid[w+2] = 0;
	down[0] = '%'; down[w+1] = '&'; down[w+2] = 0;
	for (i = 1; i<w+1; i++){up[i] = '('; mid[i] = ' ';down[i] = ')';}
	LT_Print(x,y,up,win);y++;
	for (i = 0; i<h+1; i++) {LT_Print(x,y,mid,win);y++;}
	LT_Print(x,y,down,win);
	
	free(up);free(mid);free(down);
}

void LT_Delete_Text_Box_VGA(byte x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM;
	word tile = (y * LT_map_width) + x;
	word screen_offset;
	x = x<<4;
	y = (y<<4)+LT_Window;
	screen_offset = (y<<6)+(y<<4)+(y<<2) + (x>>2);
	LT_map_collision[tile] = col;
	
	//LT_WaitVsyncEnd();
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
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	
	LT_map_data[tile] = ntile;
}

//Load and paste 320x480 image for complex images.
//It uses all map VRAM, do not use loading animations.
void LT_Load_Image(char *file){
	dword VGA_index = 0;
	dword EGA_index = 0;
	int i;
	word h = 0;
	word lines = 0;
	word last_chunk_lines = 0;
	byte chunks = 0;
	word offset_chunk = 0;
	word x = 0;
	word y = 0;
	word num_colors = 0;
	byte plane = 0;
	dword index = 0;
	dword offset = 0;
	dword offset_Image = 0;
	FILE *fp;
	
	//Open the same file with ".ega" extension for ega
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset_width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset_height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset_palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}
	offset_Image = ftell(fp);
	chunks = LT_tileset_height/200;
	if (LT_tileset_height%200) chunks++;
	if (LT_VIDEO_MODE == 0){
		//COPY TO EGA VRAM
		asm CLI //disable interrupts so that loading animation does not interfere
		//SCAN ALL TILES
		//COPY TO VGA VRAM
		h = LT_tileset_height;
		lines = LT_tileset_height;
		fseek(fp,offset_Image,SEEK_SET);
		for (i = 0; i < chunks; i++){
			if (lines < 200) last_chunk_lines = 200-lines;
			fread(&LT_tile_tempdata[0],sizeof(unsigned char),320*(200-last_chunk_lines), fp);
			EGA_index = (42*(h-1-offset_chunk));
			offset = 0;
			//SCAN LINES 
			for (y = 0; y < 200 - last_chunk_lines; y++){
				for(x = 0; x < 40; x++){
					//Get an 8 pixel chunk, and convert to 4 bytes
					byte pl0 = 0;
					byte pl1 = 0;
					byte pl2 = 0;
					byte pl3 = 0;
					for(plane = 0; plane < 8; plane++){
						//plane defines the inverted shift (the pixel to activate)
						int pixel = LT_tile_tempdata[offset];
						byte k = 7-plane;
						//Store bitplanes in pl0,pl1,pl2,pl3
						switch (pixel){
							case 0: pl0 |= 0 << k; break;	// black, bit 0 in byte-plane 0
							case 1:	pl0 |= 1 << k; break;	// dark blue, bit 1 in b-plane 0
							case 2: pl1 |= 1 << k; break;	// dark green, bit 1 in b-plane 1
							case 3: pl2 |= 1 << k; break;	// dark red, bit 1 in b-plane 2
							case 4:	pl0 |= 1 << k; pl1 |= 1 << k; break;// dark cyan, bit 1 in b-planes 0,1
							case 5:	pl1 |= 1 << k; pl2 |= 1 << k; break;// dark yellow, bit 1 in b-plane 1,2
							case 6:	pl0 |= 1 << k; pl2 |= 1 << k; break;// dark magenta, bit 1 in b-plane 0,2	
							case 7:	pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; break;// light gray, bit 1 in b-plane 0,1,2
							//now the same but activating the intensity plane (3)
							case 8: pl0 |= 0 << k; pl3 |= 1 << k; break;// dark gray, bit 0 in b-plane 0; bit 1 in b-plane 3
							case 9:	pl0 |= 1 << k; pl3 |= 1 << k; break;// light blue, bit 1 in b-plane 0,3
							case 10: pl1 |= 1 << k; pl3 |= 1 << k; break;// light green, bit 1 in b-plane 1,3	
							case 11: pl2 |= 1 << k; pl3 |= 1 << k; break;// light red, bit 1 in b-plane 2,3
							case 12: pl0 |= 1 << k; pl1 |= 1 << k; pl3 |= 1 << k; break;// light cyan, bit 1 in b-plane 0,1,3
							case 13: pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light yellow, bit 1 in b-plane 1,2,3
							case 14: pl0 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light magenta, bit 1 in b-plane 0,2,3
							case 15: pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// white, all metro lines
						}
						offset++;
					}
					
					//Paste pl1,pl2,pl3,pl4 to EGA address
					asm mov dx, 03C4h
					asm mov ax, 0102h	//Plane 0		
					asm out dx, ax
					EGA[EGA_index] = pl0;
		
					asm mov ax, 0202h 	//Plane 1
					asm out dx, ax
					EGA[EGA_index] = pl1;
				
					asm mov ax, 0402h 	//Plane 2
					asm out dx, ax
					EGA[EGA_index] = pl2;
	
					asm mov ax, 0802h 	//Plane 3
					asm out dx, ax
					EGA[EGA_index] = pl3;
						
					EGA_index++;
					//offset +=8;
				}
				EGA_index-=82;
			}
			asm STI //Re enable interrupts
			lines -= 200;
			offset_chunk+= 200;
		}
		asm STI //Re enable interrupts so that loading animation is played again
	} else {
		//COPY TO VGA VRAM
		h = LT_tileset_height;
		lines = LT_tileset_height;
		fseek(fp,offset_Image,SEEK_SET);
		for (i = 0; i < chunks; i++){
			if (lines < 200) last_chunk_lines = 200-lines;
			fread(&LT_tile_tempdata[0],sizeof(unsigned char),320*(200-last_chunk_lines), fp);
			for (plane = 0; plane < 4; plane ++){
				// select plane
				asm CLI //disable interrupts
				outp(SC_INDEX, MAP_MASK);          
				outp(SC_DATA, 1 << plane);
				VGA_index = (84*(h-1-offset_chunk));
				//SCAN LINES 
				offset = plane;
				for (y = 0; y < 200 - last_chunk_lines; y++){
					for(x = 0; x < 80; x++){
						VGA[VGA_index] = LT_tile_tempdata[offset];
						VGA_index++;
						offset +=4;
					}
					VGA_index-=164;
				}
				asm STI //Re enable interrupts
			}
			lines -= 200;
			offset_chunk+= 200;
		}
	}
	fclose(fp);
}

// load_16x16 tiles to VRAM
void LT_Load_Tiles(char *file){
	dword VGA_index = 0;
	dword EGA_index = 0;
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
	
	//Open the same file with ".ega" extension for ega
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	fgetc(fp);
	fgetc(fp);

	fseek(fp, 16, SEEK_CUR);
	fread(&LT_tileset_width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(&LT_tileset_height,sizeof(word), 1, fp);
	fseek(fp, 22, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset_palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset_palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	LT_tileset_ntiles = (LT_tileset_width>>4) * (LT_tileset_height>>4);

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset_width*LT_tileset_height, fp);
	
	fclose(fp);
	
	if (LT_VIDEO_MODE == 0){
		//COPY TO EGA VRAM
		w = LT_tileset_width>>4;
		h = LT_tileset_height>>4;
		jx = LT_tileset_width+16;
		
		EGA_index = TILE_VRAM_EGA;
		asm CLI //disable interrupts so that loading animation does not interfere
		//SCAN ALL TILES
		for (tileY = h; tileY > 0 ; tileY--){
			ty = (tileY<<4)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = (ty*LT_tileset_width) + (tileX<<4);
				//LOAD TILE
				for(y = 0; y < 16; y++){//Get 2 chunks of 8 pixels per row
					int i,j,k;
					for (j = 0; j < 2; j++){
						//Get an 8 pixel chunk, and convert to 4 bytes
						byte pl0 = 0;
						byte pl1 = 0;
						byte pl2 = 0;
						byte pl3 = 0;
						for(i = 0; i < 8; i++){
							//i defines the inverted shift (the pixel to activate)
							int pixel = LT_tile_tempdata[offset];
							k = 7-i;
							//Store bitplanes in pl0,pl1,pl2,pl3
							switch (pixel){
								case 0: pl0 |= 0 << k; break;	// black, bit 0 in byte-plane 0
								case 1:	pl0 |= 1 << k; break;	// dark blue, bit 1 in b-plane 0
								case 2: pl1 |= 1 << k; break;	// dark green, bit 1 in b-plane 1
								case 3: pl2 |= 1 << k; break;	// dark red, bit 1 in b-plane 2
								case 4:	pl0 |= 1 << k; pl1 |= 1 << k; break;// dark cyan, bit 1 in b-planes 0,1
								case 5:	pl1 |= 1 << k; pl2 |= 1 << k; break;// dark yellow, bit 1 in b-plane 1,2
								case 6:	pl0 |= 1 << k; pl2 |= 1 << k; break;// dark magenta, bit 1 in b-plane 0,2	
								case 7:	pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; break;// light gray, bit 1 in b-plane 0,1,2
								//now the same but activating the intensity plane (3)
								case 8: pl0 |= 0 << k; pl3 |= 1 << k; break;// dark gray, bit 0 in b-plane 0; bit 1 in b-plane 3
								case 9:	pl0 |= 1 << k; pl3 |= 1 << k; break;// light blue, bit 1 in b-plane 0,3
								case 10: pl1 |= 1 << k; pl3 |= 1 << k; break;// light green, bit 1 in b-plane 1,3	
								case 11: pl2 |= 1 << k; pl3 |= 1 << k; break;// light red, bit 1 in b-plane 2,3
								case 12: pl0 |= 1 << k; pl1 |= 1 << k; pl3 |= 1 << k; break;// light cyan, bit 1 in b-plane 0,1,3
								case 13: pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light yellow, bit 1 in b-plane 1,2,3
								case 14: pl0 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// light magenta, bit 1 in b-plane 0,2,3
								case 15: pl0 |= 1 << k; pl1 |= 1 << k; pl2 |= 1 << k; pl3 |= 1 << k; break;// white, all metro lines
							}
							offset++;
						}
						
						//Paste pl1,pl2,pl3,pl4 to EGA address
						
						asm mov dx, 03C4h
						asm mov ax, 0102h	//Plane 0		
						asm out dx, ax
						EGA[EGA_index] = pl0;
		
						asm mov ax, 0202h 	//Plane 1
						asm out dx, ax
						EGA[EGA_index] = pl1;
				
						asm mov ax, 0402h 	//Plane 2
						asm out dx, ax
						EGA[EGA_index] = pl2;
	
						asm mov ax, 0802h 	//Plane 3
						asm out dx, ax
						EGA[EGA_index] = pl3;
						
						EGA_index++;
						
						if (EGA_index > 0x7680) LT_Error("Too many tiles\n",0);
					}
					
					offset -= jx;
				}
			}
		}
		asm STI //Re enable interrupts so that loading animation is played again
	} else {
		//COPY TO VGA VRAM
		w = LT_tileset_width>>4;
		h = LT_tileset_height>>4;
		jx = LT_tileset_width+16; 
		
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
					offset = plane + (ty*LT_tileset_width) + (tileX<<4);
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

}

void LT_unload_tileset(){
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
		//<layer id="5" name="Ground" width="40" height="30">
		if((line[1] == '<')&&(line[2] == 'l')){// get map dimensions
			sscanf(line," <layer id=\"%i\" name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&LT_map_width,&name,&LT_map_width,&LT_map_height);
			start_bkg_data = 1;
		}
	}
	LT_map_ntiles = LT_map_width*LT_map_height;
	if (LT_map_ntiles > 256*32){
		printf("Error, map is bigger than 32 kb");
		sleep(2);
		LT_ExitDOS();
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">

	//read tile array
	for (index = 0; index < LT_map_ntiles; index++){
		fscanf(f, "%i,", &tile);
		LT_map_data[index] = tile-1;
	}
	//skip 
	while(start_col_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 'l')){
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&LT_map_width,&LT_map_height);
			start_col_data = 1;
		}
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">
	//read collision array
	for (index = 0; index < LT_map_ntiles; index++){
		fscanf(f, "%d,", &tile);
		LT_map_collision[index] = tile -tilecount;
	}
	fclose(f);
}

void LT_unload_map(){
	LT_map_width = 0;
	LT_map_height = 0;
	LT_map_ntiles = 0;
	//free(LT_map_data); LT_map_data = NULL;
	//free(LT_map_collision); LT_map_collision = NULL;
}

void LT_Set_Map(int x, int y){
	//UNDER CONSTRUCTION
	int i = 0;
	int j = 0;
	Enemies = 0;
	LT_map_offset = (LT_map_width*y)+x;
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
	LT_Fade_in();
}

void LT_Get_Item(int sprite_number, byte ntile, byte col){
	SPRITE *s = &sprite[sprite_number];
	s->get_item = 1;
	s->ntile_item = ntile;
	s->col_item = col;
}

void (*LT_Edit_MapTile)(word,word,byte,byte);

void LT_Edit_MapTile_VGA(word x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM;
	word tile = (y * LT_map_width) + x;
	word screen_offset;
	x = x<<4;
	y = (y<<4)+LT_Window;
	screen_offset = (y<<6)+(y<<4)+(y<<2) + (x>>2);
	LT_map_collision[tile] = col;
	
	//LT_WaitVsyncEnd();
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
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	
	LT_map_data[tile] = ntile;
}

void LT_Edit_MapTile_EGA(word x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM_EGA;
	word tile = (y * LT_map_width) + x;
	word screen_offset;
	x = x<<4;
	y = (y<<4)+LT_Window;
	screen_offset = (y<<5)+(y<<3)+(y<<1)+(x>>3);

	LT_map_collision[tile] = col;
	
	//LT_WaitVsyncEnd();
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
		mov		si,TILE_ADDRESS				//ds:si source VRAM TILE ADDRESS  //590*(336/4);
		
		mov		al,byte ptr [ntile]		//go to desired tile
		mov		ah,0	
		mov		cl,5					//*32
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax

		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE

		//END LOOP
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	
	LT_map_data[tile] = ntile;
}

void (*draw_map_column)(word,word,word);

void draw_map_column_vga(word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM;
	word screen_offset = (y<<6)+(y<<4)+(y<<2)+(x>>2);
	word width = LT_map_width;
	unsigned char *mapdata = LT_map_data;
	int i = 0;
	word m_offset = map_offset;

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
	
	asm{//END
	
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	//LT_Sprite_Stack = 2;
	for (i = 0; i <16;i++){
		int sprite = LT_map_collision[m_offset];
		if ((sprite > 15) && (sprite < 32)) {
			LT_Sprite_Stack_Table[LT_Sprite_Stack] = LT_AI_Sprite[0] + (Enemies&7);
			LT_Set_AI_Sprite(LT_AI_Sprite[0] + (Enemies&7),sprite-16,x>>4,(y>>4)+i-1,3,0);
			LT_Sprite_Stack++;
			Enemies++;
		}
		//if (Enemies == 7) Enemies = 0;
		//if (LT_Sprite_Stack == 8) LT_Sprite_Stack = 1;
		
		m_offset+= LT_map_width;
	}
}

void draw_map_column_ega(word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM_EGA;
	word screen_offset = (y<<5)+(y<<3)+(y<<1)+(x>>3);
	word width = LT_map_width;
	unsigned char *mapdata = LT_map_data;
	int i = 0;
	word m_offset = map_offset;

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
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address	
	}
	//UNWRAPPED LOOP
	//DRAW 16 TILES
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		add di,40
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		shl		ax,cl
		add		si,ax
	}

	asm{//END
	
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}

	for (i = 0; i <16;i++){
		int sprite = LT_map_collision[m_offset];
		if ((sprite > 15) && (sprite < 32)) {
			if (Enemies != 0) {
				LT_Clone_Sprite(selected_AI_sprite+Enemies,selected_AI_sprite);
				LT_Set_AI_Sprite(selected_AI_sprite+Enemies,1,x>>4,(y>>4)+i-1,2,0);
				Enemies++;
			}else{
				selected_AI_sprite = LT_AI_Sprite[sprite-16];
				LT_Set_AI_Sprite(selected_AI_sprite,1,x>>4,(y>>4)+i-1,2,0);
				Enemies++;
			}
		}
		m_offset+= LT_map_width;
	}
}

void (*draw_map_row)(word,word,word);

void draw_map_row_vga( word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM;
	unsigned char *mapdata = LT_map_data;
	word screen_offset = (y<<6)+(y<<4)+(y<<2) + (x>>2);
	int i = 0;
	word m_offset = map_offset;
	
	asm{//SET ADDRESS
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
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
	
		pop si
		pop di
		pop ds
	}
	for (i = 0; i <20;i++){
		int sprite = LT_map_collision[m_offset];
		if ((sprite > 15) && (sprite < 32)) {
			LT_Sprite_Stack_Table[LT_Sprite_Stack] = LT_AI_Sprite[0] + (Enemies&7);
			LT_Set_AI_Sprite(LT_AI_Sprite[0] + (Enemies&7),sprite-16,x>>4,(y>>4)+i-1,3,0);
			LT_Sprite_Stack++;
			Enemies++;
		}
		//if (Enemies == 7) Enemies = 0;
		//if (LT_Sprite_Stack == 8) LT_Sprite_Stack = 1;
		
		m_offset+= LT_map_width;
	}
}

void draw_map_row_ega( word x, word y, word map_offset){
	word TILE_ADDRESS = TILE_VRAM_EGA;
	unsigned char *mapdata = LT_map_data;
	word screen_offset = (y<<5)+(y<<3)+(y<<1)+(x>>3);
	
	asm{//SET ADDRESS
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
		mov		si,TILE_ADDRESS				//ds:si Tile data VRAM address = FIXED VRAM AT scan line 590; 

		les		bx,[mapdata]
		add		bx,map_offset
		mov		al,byte ptr es:[bx]
		mov		ah,0
		mov		cl,5
		
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address								
	}
	//UNWRAPPED LOOPS
	//Copy 21 tiles
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//COPY TILE	
		mov 	ax,0A000h
		mov 	es,ax
		
		movsb			//COPY TILE (16 lines)
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb
		add di,40
		movsb	
		movsb	//END COPY TILE
		
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
		mov		cl,5
		shl		ax,cl
		add		si,ax

		sub		di,42*15					//next horizontal tile position
	}
	asm{//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
	
		pop si
		pop di
		pop ds
	}
}

//update rows and colums
void LT_scroll_map(){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = ((SCR_Y+LT_Window)>>4)<<4;
	if (LT_current_x != LT_last_x) {
		LT_map_offset = ((SCR_Y>>4)*LT_map_width)+(SCR_X>>4);
		if (LT_current_x < LT_last_x) 
			draw_map_column(LT_current_x,LT_current_y,LT_map_offset); 
		else  
			draw_map_column(LT_current_x+320,LT_current_y,LT_map_offset+20);
	}
	if (LT_current_y != LT_last_y) {
		LT_map_offset = ((SCR_Y>>4)*LT_map_width)+(SCR_X>>4);
		if (LT_current_y < LT_last_y)
			draw_map_row(LT_current_x,LT_current_y,LT_map_offset);
		else 
			draw_map_row(LT_current_x,LT_current_y+240,LT_map_offset+(15*LT_map_width));
	}		
	
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
}

//Endless Side Scrolling Map
void LT_Endless_SideScroll_Map(int y){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = ((SCR_Y+LT_Window)>>4)<<4;
	
	if (LT_current_x > LT_last_x) { 
		draw_map_column(LT_current_x+320,LT_current_y,((LT_map_offset_Endless+20)%LT_map_width)+(y*LT_map_width));
		LT_map_offset_Endless++;
	}
	
	LT_last_x = LT_current_x;
}

void set_palette(unsigned char *palette){
	//outp(0x03c8,0);
	asm{
		push si
		push di
		push ds
		
		mov dx,003c8h
		mov ax,0	
		out dx,ax
		
		les		bx,[palette]
		mov 	cx,64
		mov 	dx,003c9h //Palete register
	}
	//for(i=0;i<256*3;i+=12){
	pal_loop:
	asm{
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		mov ax,word ptr es:[bx]
		out dx,ax
		add bx,1
		
		loop pal_loop
		
		pop si
		pop di
		pop ds
		
	}

	vsync();
}

void VGA_ClearPalette(){
	int i;
	if (LT_VIDEO_MODE == 1){
	asm {
		mov	dx,003c8h
		mov al,0
		out	dx,al
		mov cx,64
		
		mov al,0
		mov dx,003c9h
	}
	ploop:
	asm{
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		out dx,al
		
		loop ploop
	}
	vsync();
	}
}

void (*LT_Fade_in)(void);
void (*LT_Fade_out)(void);

void LT_Fade_in_VGA(){
	int i,j;
	memset(LT_Temp_palette,256*3,0x00);//All colours black
	i = 0;
	//Fade in
	asm mov	dx,003c8h
	asm mov al,0
	asm out	dx,al
	while (i < 14){//SVGA FAILED with 15
		asm mov dx,003c9h //Palete register
		asm mov cx,256*3
		asm mov bx,0
		fade_in_loop:
			asm mov al,byte ptr LT_Temp_palette[bx]
			asm cmp	al,byte ptr LT_tileset_palette[bx]
			asm jae	pal_is_greater
			asm mov al,byte ptr LT_Temp_palette[bx]
			asm add al,4
			asm mov byte ptr LT_Temp_palette[bx],al
			pal_is_greater:
			
			asm	mov al,byte ptr LT_Temp_palette[bx]
			asm out dx,al
			
			asm inc bx
			asm loop fade_in_loop
		
		i ++;
		asm mov		dx,INPUT_STATUS_0
		WaitNotVsync:
		asm in      al,dx
		asm test    al,08h
		asm jnz		WaitNotVsync
		WaitVsync:
		asm in      al,dx
		asm test    al,08h
		asm jz		WaitVsync
	}
}

void LT_Fade_out_VGA(){
	int i,j;
	i = 0;
	//Fade to black
	asm mov	dx,003c8h
	asm mov al,0
	asm out	dx,al
	while (i < 15){
		asm mov dx,003c9h //Palete register
		asm mov cx,256*3
		asm mov bx,0
		fade_out_loop:
			asm mov al,byte ptr LT_Temp_palette[bx]
			asm cmp	byte ptr LT_Temp_palette[bx],0
			asm je	pal_is_zero
			asm mov al,byte ptr LT_Temp_palette[bx]
			asm sub al,4
			asm mov byte ptr LT_Temp_palette[bx],al
			pal_is_zero:
			asm	mov al,byte ptr LT_Temp_palette[bx]
			asm out dx,al
			
			asm inc bx
			asm loop fade_out_loop
		i ++;
		//Wait Vsync
		asm mov		dx,INPUT_STATUS_0
		WaitNotVsync:
		asm in      al,dx
		asm test    al,08h
		asm jnz		WaitNotVsync
		WaitVsync:
		asm in      al,dx
		asm test    al,08h
		asm jz		WaitVsync
	}
}

void LT_Fade_in_EGA(){
	// Colors have weird arrangament if VGA.
	sleep(1);
	asm{
		mov   dx,03dah
		in    al,dx
		mov   dx,03c0h
		mov   al,0		//Color index
		out   dx,al
		mov   al,0		//Color number 
		out   dx,al
		mov   al,20h
		out   dx,al 
	}
	sleep(1);
}

void LT_Fade_out_EGA(){
	int i,j;
	i = 0;
	sleep(1);
}

//init Cycle struct  
void cycle_init(COLORCYCLE *cycle,const unsigned char *palette){
	cycle->frame = 0;
	cycle->counter = 0;
	cycle->palette = (const unsigned char*)palette;
}


//Cycle palette 
void cycle_palette(COLORCYCLE *cycle, byte speed){
	int i;
	const unsigned char *datapal = &cycle->palette[cycle->frame];
	
	if (LT_VIDEO_MODE == 1){
		asm lds bx,datapal
		asm mov	dx,003c8h
		asm mov al,1
		asm out	dx,al //start at position 1 of palette index (colour 1)
		asm mov cx,8
		asm inc dx  //Palete register 003c9h
		cycleloop:
			asm	mov al,byte ptr ds:[bx]
			asm out dx,al
			asm	mov al,byte ptr ds:[bx+1]
			asm out dx,al
			asm	mov al,byte ptr ds:[bx+2]
			asm out dx,al
			asm add bx,3
			asm loop cycleloop
	
		if (cycle->counter == speed){
			cycle->counter = 0;
			cycle->frame+=3;
		}
		cycle->counter++;
		if (cycle->frame == 24) cycle->frame = 0;
	}
}


//Custom palette for EGA, ONLY works on VGA cars using EGA mode.
//EGA mode is twice as fast to update compared to VGA mode X. 
//(every EGA VRAM byte contains 8 pixels)
//So I plan to make the engine only in EGA mode but with custom palettes, 
//to help poor 8088 and 8086's.
byte testframe = 0;
byte testspeed = 0;
void VGA_EGAMODE_CustomPalette(unsigned char *palette){
	//unsigned char i = 0;
	
	const unsigned char *datapal = &palette[testframe];
	
	asm lds bx,datapal
	asm mov cx,24
	asm mov al,0
	asm mov	dx,003c8h
	asm out	dx,al	//start at position 1 of palette index (colour 1)
	asm inc dx		//Palete register 003c9h
	
	cycleloop:
	
		asm	mov al,byte ptr ds:[bx]
		asm out dx,al
		asm	mov al,byte ptr ds:[bx+1]
		asm out dx,al
		asm	mov al,byte ptr ds:[bx+2]
		asm out dx,al
		asm add bx,3
		
		asm loop cycleloop
	
	/*
	asm lds bx,datapal
	asm add bx,3*8
	asm mov	dx,003c8h
	asm mov al,16
	asm out	dx,al	//start at position 1 of palette index (colour 1)
	asm mov cx,8
	asm inc dx		//Palete register 003c9h
	cycleloop1:
		asm	mov al,byte ptr ds:[bx]
		asm out dx,al
		asm	mov al,byte ptr ds:[bx+1]
		asm out dx,al
		asm	mov al,byte ptr ds:[bx+2]
		asm out dx,al
		asm add bx,3
		asm loop cycleloop1*/
	/*
	testspeed++; 
	if (testspeed == 60) {
		testspeed = 0;
		testframe+=3;
		if (testframe == 24) testframe = 0;
	}*/
}