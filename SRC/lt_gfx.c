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


word LT_VRAM_Logical_Width;
// One map in ram stored at "LT_map"
word LT_map_width;
word LT_map_height;
word LT_map_ntiles;
word *LT_map_data;
byte *LT_map_collision;
// One tileset 
word LT_tileset_width;
word LT_tileset_height;
word LT_tileset_ntiles;
byte LT_tileset_palette[256*3];
byte *LT_tileset_data;
byte LT_EGA_Font_Palette[4] = {0,9,7,15};
unsigned char *LT_tile_tempdata; //Temp storage of non tiled data. and also sound samples
unsigned char *LT_tile_tempdata2; //Second half 
extern unsigned char *LT_sprite_data; // 
SPRITE LT_Loading_Animation; 
extern SPRITE *sprite;
extern word *sprite_id_table;
byte LT_Loaded_Image = 0; //If you load a 320x240 image (to the second page), the delete loading interrupt will paste it to first page
byte LT_PAL_TIMER = 0;
byte LT_EGA_FADE_STATE = 0;
word LT_FrameSkip = 0;
byte *LT_Filename; //Contains name of the images oppened, for LT_Header_BMP function 

void LT_Error(char *error, char *file);
void LT_Reset_Sprite_Stack();

extern word LT_Sprite_Size_Table_EGA_32[];
extern byte LT_SFX_MODE;
extern byte LT_Sprite_Stack;
extern byte LT_Sprite_Stack_Table[];
extern byte LT_Active_AI_Sprites[];
extern byte selected_AI_sprite;
word Compile_Bitmap(word logical_width, unsigned char *bitmap, unsigned char *output);
//GLOBAL VARIABLES

word FONT_VRAM = 0;
word TILE_VRAM = 0;
//Palette for fading
byte LT_Temp_palette[256*3];

//Palette cycling
byte LT_Cycle_paldata[32*3]; //2 palettes of 8 lours each (duplicated to simplify code) for cycling olours.
byte LT_Cycle_palframe = 0;
byte LT_Cycle_palcounter = 0;

//Parallax palette
byte LT_Parallax_Frame = 0;
byte LT_Parallax_paldata[3*128];

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
int vpage = 0;
int Enemies = 0;
void (*draw_map_column)(word, word, word, word);

extern void interrupt (*LT_old_time_handler)(void); 

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

void LT_VGA_Enable_4Planes(){
	asm mov dx,03c4h  	//dx = indexregister
	asm mov ax,0F02h	//INDEX = MASK MAP, 3d4
	asm out dx,ax 		//write all the bitplanes.
	asm mov dx,03ceh 	//dx = indexregister 3ce
	asm mov ax,0008h		
	asm out dx,ax
}

void LT_VGA_Return_4Planes(){
	asm mov dx,03ceh +1 //dx = indexregister
	asm mov ax,00ffh		
	asm out dx,ax
}

void LT_vsync(){
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

//Mode: 0 = image; 1 = tiles; 2 = sprite; 3 = window; 4/5 = animation/font 
void LT_Header_BMP(FILE *fp,int mode, int sprite_number){
	int index;
	word num_colors;
	byte get_pal = 1;
	byte first_color = 0;
	byte pal_colors = 0;
	word header;
	byte pixel_format = 0;
	SPRITE *s = &sprite[sprite_number];
	word _width;
	word _height;
	word *width;
	word *height;
	//Tiles or image
	if (mode < 2){width = &LT_tileset_width; height = &LT_tileset_height;}
	//Sprites
	if (mode == 2){width = &s->width; height = &s->height;}
	//Window //Animation //Font
	if (mode > 2){width = &_width; height = &_height;}
	
	//Read header
	fread(&header, sizeof(word), 1, fp);
	if (header != 0x4D42) LT_Error("Not a BMP file",LT_Filename);
	fseek(fp, 16, SEEK_CUR);
	fread(width, sizeof(word), 1, fp);
	fseek(fp, 2, SEEK_CUR);
	fread(height,sizeof(word), 1, fp);
	fseek(fp, 4, SEEK_CUR);
	fread(&pixel_format,sizeof(byte), 1, fp);
	fseek(fp, 17, SEEK_CUR);
	fread(&num_colors,sizeof(word), 1, fp);
	fseek(fp, 6, SEEK_CUR);

	if (num_colors==0)  num_colors=256;
	if (num_colors > 256) LT_Error("Image has more than 256 colors",LT_Filename);
	
	//Place colors in appropiate offset of VGA pallete
	//
	switch (mode){
		case 0://If reading image
			if (LT_VIDEO_MODE == 1) {
				if (LT_tileset_height != 200) LT_Error("Wrong size for VGA image, size must be 320x200: ",LT_Filename);
				if (LT_tileset_width != 320) LT_Error("Wrong size for VGA image, size must be 320x200: ",LT_Filename);
			}
			if (LT_VIDEO_MODE == 0){
				if (LT_tileset_height != 200) LT_Error("Wrong size for EGA image, size must be 320x200: ",LT_Filename);
				if (LT_tileset_width != 320) LT_Error("Wrong size for EGA image, size must be 320x200: ",LT_Filename);
			}
			if (pixel_format !=8) LT_Error("Wrong format for image, must be 8 bit per pixel: ",LT_Filename);
			pal_colors = 208;first_color = 0;
		break;
		case 1://If reading tileset
			if (pixel_format !=8) LT_Error("Wrong format for tileset, image must be 8 bit per pixel: ",LT_Filename);
			pal_colors = 208;first_color = 0;
		break;
		case 2://If reading sprites
			if (s->width & 1) LT_Error("Sprite width not even: ",LT_Filename);
			if (s->height & 1) LT_Error("Sprite height not even: ",LT_Filename);
			pal_colors = 32; first_color = 208;
		break;
		case 4://If reading Animation
		case 5://or font
			if (_height !=32) LT_Error("Wrong size for animation/font, image must be 128x32: ",LT_Filename);
			if (_width !=128) LT_Error("Wrong size for animation/font, image must be 128x32: ",LT_Filename);
			if (pixel_format !=4) LT_Error("Wrong format for animation/font image, must be 4 bit per pixel: ",LT_Filename);
			if (mode == 4){pal_colors = 4; first_color = 248;}
			else {pal_colors = 4; first_color = 252;}//If reading font
		break;
	}

	
	//Load Palette
	for(index=first_color;index<first_color+num_colors;index++){
		if (index-first_color == pal_colors) get_pal = 0;
		if (get_pal){
			LT_tileset_palette[(int)(index*3+2)] = fgetc(fp) >> 2;
			LT_tileset_palette[(int)(index*3+1)] = fgetc(fp) >> 2;
			LT_tileset_palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		} else {
			fgetc(fp);
			fgetc(fp);
			fgetc(fp);
		}
		fgetc(fp);
	}
}

//Frame Counter

void interrupt LT_Frame_Counter(void){
	asm CLI
	asm inc LT_FrameSkip
	
	asm STI
	
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
}


//32x32 animation for loading scene
void LT_Load_Animation(char *file){
	long index,offset;
	word data_offset = 0;
	word x,y;
	word i,j;
	word frame = 0;
	byte tileX;
	byte tileY;
	byte size = 32;
	FILE *fp;
    int code_size;
	
	LT_Filename = file;
	
	fp = fopen(file,"rb");
	if(!fp) LT_Error("Can't find ",file);
	LT_Header_BMP(fp,4,0);
	
	for(index=31*64;index>=0;index-=64){
		for(x=0;x<64;x++){
			unsigned char c = (byte)fgetc(fp);
			if (LT_VIDEO_MODE){
				LT_tile_tempdata[(index+x<<1)]   = ((c & 0xF0)>>4) + 248; //1111 0000c
				LT_tile_tempdata[(index+x<<1)+1] =  (c & 0x0F)     + 248; //0000 1111c Animation colors from 248 to 251
			} else {
				LT_tile_tempdata[(index+x<<1)]   = ((c & 0xF0)>>4);
				LT_tile_tempdata[(index+x<<1)+1] =  (c & 0x0F);
			}
		}
	}
	fclose(fp);
	
	index = 0; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=0;tileY<32;tileY+=size){
		for (tileX=0;tileX<128;tileX+=size){
			offset = (tileY<<7)+tileX;
			if (LT_VIDEO_MODE){
				LT_tile_tempdata2[index] = size;
				LT_tile_tempdata2[index+1] = size;
				index+=2;
			}
			for(x=0;x<size;x++){
				memcpy(&LT_tile_tempdata2[index],&LT_tile_tempdata[offset+(x<<7)],size);
				index+=size;
			}
		}
	}
	
	LT_Loading_Animation.nframes = 4;
	LT_Loading_Animation.code_size = 0;
	LT_Loading_Animation.ega_size = &LT_Sprite_Size_Table_EGA_32[0];
	//estimated size of code
	//fsize = (size * size * 7) / 2 + 25;
	for (frame = 0; frame < LT_Loading_Animation.nframes; frame++){		//
		if (LT_VIDEO_MODE){
			/*if ((LT_Loading_Animation.frames[frame].compiled_code = farcalloc(fsize,sizeof(unsigned char))) == NULL){
				LT_Error("Not enough RAM to load animation frames ",0);
			}*/
			//COMPILE SPRITE FRAME TO X86 MACHINE CODE
			//& Store the compiled data at it's final destination
			code_size = Compile_Bitmap(LT_VRAM_Logical_Width, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],&LT_sprite_data[data_offset]);
			LT_Loading_Animation.frames[frame].compiled_code = &LT_sprite_data[data_offset];
			data_offset += code_size;
			LT_Loading_Animation.code_size += code_size;
		} else {
			byte *pixels = &LT_tile_tempdata2[frame*size*size];
			LT_Loading_Animation.frames[frame].compiled_code = &LT_sprite_data[data_offset];
			offset = 0;
			for (y = 0; y < size; y++){//Read lines
				for (j = 0; j < 4; j++){
					byte cmask = 0x80;
					byte color = 0;
					byte mask = 0;
					for (x = 0; x < 8; x++){
						byte bit = pixels[offset++];//colors 1,2
						if (bit     ) mask += cmask;
						if (bit == 2) color += cmask;
						cmask = cmask >> 1;
					}
					LT_sprite_data[data_offset++] = color;
				}
			}
		}
	}	
	
	//IINIT SPRITE
	//sprite[sprite_number].bkg_data no bkg data
	LT_Loading_Animation.width = 32;
	LT_Loading_Animation.height = 32;
	LT_Loading_Animation.init = 0;
	LT_Loading_Animation.frame = 0;
	LT_Loading_Animation.baseframe = 0;
	LT_Loading_Animation.aframes = 4;
	LT_Loading_Animation.animate = 1;
	LT_Loading_Animation.anim_speed = 0;
	LT_Loading_Animation.anim_counter = 0;
}

void LT_Set_Animation(byte speed){
	LT_Loading_Animation.speed = speed;
}

void run_compiled_sprite(word XPos, word YPos, char *Sprite);

void interrupt LT_Loading(void){
	asm CLI
	{
		SPRITE *s = &LT_Loading_Animation;
		//animation
		if (s->anim_speed == s->speed){
			s->anim_speed = 0;
			s->frame++;
			if (s->frame == s->aframes) s->frame = 0;
		}
		s->anim_speed++;
	
		if (LT_VIDEO_MODE) run_compiled_sprite(s->pos_x,s->pos_y,s->frames[s->frame].compiled_code);
		else {
			//draw EGA sprite and destroy bkg
			char *bitmap = s->frames[s->frame].compiled_code;
			word screen_offset = (s->pos_y*LT_VRAM_Logical_Width)+(s->pos_x>>3);
			asm{
				push es
				push ds
				push di
				push si
				
				lds	si,[bitmap]
				mov	ax,0A000h
				mov	es,ax						
				mov	di,screen_offset	//ES:DI destination vram
	
				mov dx,0x03CE
				mov ax,0x0005		//write mode 0
				out dx,ax
				mov	dx,0x03CE	  //Set mask register
				mov ax,0xFF08
				out	dx,ax
				
				mov	ax,8			//scanlines
			}
			
			_drawsprit_32:
			asm{
				mov cx,4
				rep movsb
				add di,44-4	
				mov cx,4
				rep movsb
				add di,44-4	
				mov cx,4
				rep movsb
				add di,44-4	
				mov cx,4
				rep movsb
				add di,44-4				//Next scanline
				
				dec ax
				jnz	_drawsprit_32
			}

			asm{
				pop si
				pop di
				pop ds
				pop es
			
				mov dx,0x03CE
				mov ax,0x0205		//write mode 2
				out dx,ax
			}
		}
	}
	asm STI
	
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
}


void EGA_ClearScreen();
void LT_Set_Loading_Interrupt(){
	unsigned long spd = 1193182/30;
	
	LT_Stop_Music();
	LT_Fade_out();
	
	LT_Reset_Sprite_Stack();
	LT_Unload_Sprites();

	//Wait Vsync
	LT_vsync();
	//Reset scroll
	outport(0x03d4, 0x0D | (0 << 8));
	outport(0x03d4, 0x0C | (0 & 0xFF00));
	
	VGA_Scroll(0, 0);
	
	if (LT_VIDEO_MODE){ 
		VGA_ClearPalette();
		VGA_ClearScreen();//clear screen
	}
	else EGA_ClearScreen();
	memset(sprite_id_table,0,19*256*2);
	
	//change color 0, 1, 2 (black and white)
	LT_tileset_palette[0] = LT_Loading_Palette[0];
	LT_tileset_palette[1] = LT_Loading_Palette[1];
	LT_tileset_palette[2] = LT_Loading_Palette[1];

	//center loading animation
	LT_Loading_Animation.pos_x = 144;
	LT_Loading_Animation.pos_y = 104;
	
	asm CLI
	//set timer
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte
	//set interrupt
	setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
	
	asm STI

	LT_Draw_Text_Box(13,18,12,1,0,"LOADING DATA");
	
	//Wait Vsync
	LT_vsync();

	LT_Fade_in();
}

void LT_Delete_Loading_Interrupt(){
	unsigned long spd = 1193182/60;
	LT_Fade_out();
	asm CLI
	//set frame counter
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte
	setvect(0x1C, LT_old_time_handler);
	asm STI
	if (LT_Loaded_Image){
		int i;
		LT_VGA_Enable_4Planes();
		for (i = 0; i < LT_VRAM_Logical_Width*200;i++) VGA[i] = VGA[i + (304*LT_VRAM_Logical_Width)];
		LT_Loaded_Image = 0;
		LT_Fade_in();
	}
}

//Hardware scrolling 
void VGA_Scroll(word x, word y){
	SCR_X = x;
	SCR_Y = y;
}


void VGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&VGA[0],0,(352>>2)*240);
}
void EGA_ClearScreen(){
	asm mov dx,0x03CE
	asm mov ax,0x0005	//write mode 0
	asm out dx,ax
	memset(&VGA[0],0,(352>>2)*240);
}

byte p[4] = {0,2,4,6};
byte p1[8] = {0,1,2,3,4,5,6,7};
byte pix;

void PC_Speaker_SFX_Player();//I don't want to use interrupts, play pc speaker sfx here if playing == 1

//So I had to add a "double buffer" just to update the sprites without flicker
//Don't worry!! I only update 19 tiles and the sprites every frame, so it is even faster than before on 8088 and 8086.
//The bad news is, I had to reduce map size and tilecount, but it is stil cool!
//These values are added to scroll registers so it iterates between the "two pages"
word pageflip[] = {0,304};
word gfx_draw_page = 1;
//works smooth in dosbox, PCem, Real VGA.
//Choppy on SVGA or modern VGA compatibles
void LT_WaitVsync(){
	byte _ac;
	word x = SCR_X;
	word y = SCR_Y + pageflip[gfx_draw_page&1];
	
	//
	y*=LT_VRAM_Logical_Width;
	if (LT_VIDEO_MODE == 0) y += x>>3;
	if (LT_VIDEO_MODE == 1) y += x>>2;
	gfx_draw_page++;
	
	//change scroll registers: 
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	//LOW_ADDRESS 0x0D
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch	//HIGH_ADDRESS 0x0C;
	asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port

	//The smooth panning magic happens here
	//disable interrupts
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
	//_ac = inp(AC_INDEX);//Store the value of the controller
	if (LT_VIDEO_MODE == 0) pix = p1[SCR_X & 7]; //VGA
	else pix = p[SCR_X & 3]; //VGA
	
	//AC index 0x03c0
	asm mov dx,0x03C0
	asm mov al,0x33		//0x20 | 0x13 (palette normal operation | Pel panning reg)	
	asm out dx,al
	asm mov al,byte ptr pix
	asm out dx,al
	
	//Restore controller value
	//asm mov ax,word ptr _ac
	//asm out dx,ax
	
	LT_FrameSkip = 0;
	//enable interrupts
	asm sti
}

//Works on all compatible VGA cards from SVGA to 2020 GPUS.
//Choppy on real EGA/VGA
void LT_WaitVsync_SVGA(){
	word x = SCR_X;
	word y = SCR_Y;
	y = (y*LT_VRAM_Logical_Width) + (x>>2);	//(y*64)+(y*16)+(y*4) = y*84; + x/4
	
	asm mov		dx,INPUT_STATUS_0
	WaitDELoop:
	asm in      al,dx
	asm and     al,DE_MASK
	asm jnz     WaitDELoop

	//change scroll registers:  
	asm mov dx,003d4h //VGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	//LOW_ADDRESS 0x0D
	asm out dx,ax	//(y << 8) | 0x0D to VGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch	//HIGH_ADDRESS 0x0C;
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


void LT_Update(int sprite_follow, int sprite){
	LT_Update_AI_Sprites();

	if (sprite_follow) LT_scroll_follow(sprite);
	if (!LT_IMAGE_MODE) {
		//if (LT_ENDLESS_SIDESCROLL) LT_Endless_SideScroll_Map(0);
		//else LT_Scroll_Map();
		LT_Scroll_Map();
	}

	if (LT_SPRITE_MODE)LT_Draw_Sprites();
	else LT_Draw_Sprites_Fast();

	LT_WaitVsync();
}

// load_8x8 fonts to VRAM (64 characters)
void LT_Load_Font(char *file){
	word VGA_index = 0;
	word w = 0;
	int h = 0;
	word ty = 0;
	word jx = 0;
	word x = 0;
	word y = 0;
	word tileX = 0;
	word tileY = 0;
	byte plane = 0;
	dword offset = 0;
	FILE *fp;
	
	LT_Filename = file;
	
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	LT_Header_BMP(fp,5,0);

	//LOAD TO TEMP RAM
	x = 0; //data offset
	for (offset = 0; offset < 128*32/2; offset ++){
		unsigned char c = fgetc(fp); //it is a 4 bit BMP, every byte contains 2 pixels
		LT_tile_tempdata[x  ] = (((c & 0xF0)>>4)); //1111 0000
		LT_tile_tempdata[x+1] =  ((c & 0x0F)    ); //0000 1111
		x+=2;
	}
	fclose(fp);

	//COPY TO VRAM
	w = 16;
	h = 4;
	jx = 128+8;
	if (LT_VIDEO_MODE == 0){
		asm mov dx, 0x03CE
		asm mov ax, 0x0205	//write mode 2
		asm out dx, ax
		VGA_index = FONT_VRAM;	//VRAM FONT ADDRESS  //586*(336/4);
		asm CLI //disable interrupts so that loading animation does not interfere
		for (tileY = h; tileY > 0 ; tileY--){
			ty = (tileY<<3)-1;
			for (tileX = 0; tileX < w; tileX++){
				int i;
				offset = (ty*128) + (tileX<<3);
				for(i = 0; i < 8; i++){
					byte mask = 0x80;
					//Get an 8 pixel chunk, and convert to 4 bytes
					for(plane = 0; plane < 8; plane++){
						int pixel = VGA[VGA_index];	//Read latch
						pixel = LT_tile_tempdata[offset];
						asm mov	dx, 0x03CE  //Set mask register
						asm mov al, 0x08	
						asm mov ah, mask
						asm out dx, ax
						asm shr mask,1
						VGA[VGA_index] = LT_EGA_Font_Palette[pixel];
						offset++;
					}
					offset -= jx;
					VGA_index++;
				}
			}
		}
		asm STI //Re enable interrupts so that loading animation is played again
	}
	if (LT_VIDEO_MODE == 1){
		for (plane = 0; plane < 4; plane ++){
			asm CLI //disable interrupts so that loading animation does not interfere
			
			// select plane
			asm mov dx,03c4h
			asm mov ax,0F02h
			asm out dx,ax
			asm inc dx
			asm mov al,1
			asm mov cl,plane
			asm shl al,cl
			asm out	dx,al
			
			VGA_index = FONT_VRAM;	//VRAM FONT ADDRESS
			
			//SCAN ALL TILES
			for (tileY = h; tileY > 0 ; tileY--){
				ty = (tileY<<3)-1;
				for (tileX = 0; tileX < w; tileX++){
					offset = plane + (ty*128) + (tileX<<3);
					//LOAD TILE
					x=0;
					for(y = 0; y < 16; y++){
						VGA[VGA_index] = LT_tile_tempdata[offset] + 252; //Font color from 252 to 255
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


//Print 8x8 tiles, it is a bit slow on 8086, but it works for text boxes
//Text Box borders: # UL; $ UR; % DL; & DR; ( UP; ) DOWN; * LEFT; + RIGHT;
//Special latin characters: 
//	- : Spanish N with tilde;
//	< : Inverse question;
//	= : Inverse exclamation;
//	> : C cedille
//	[ \ ] ^ _ : AEIOU with tilde. Use double "\\" in strings to represent backslash
void (*LT_Print)(word,word,word,char*);
void LT_Print_VGA(word x, word y, word w, char *string){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset;
	byte datastring;
	word size = strlen(string);
	word i = 0;
	word line = 0;
	word lwidth = LT_VRAM_Logical_Width-2;
	word lwidth2 = LT_VRAM_Logical_Width*7;
	word line_jump = (LT_VRAM_Logical_Width*8) -24;
	y = (y<<3);
	screen_offset = (y<<6)+(y<<4)+(y<<3);	//(y*64)+(y*16)+(y*8) = y*88;
	
	//if (size > 40) size = 40;
	asm{
		push ds
		push di
		push si
		
		mov dx,SC_INDEX //dx = indexregister
		mov ax,0F02h	//INDEX = MASK MAP, 
		out dx,ax 		//write all the bitplanes.
		mov dx,GC_INDEX //dx = indexregister
		mov ax,0008h		
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
	if (datastring > 96) datastring -=32;
	asm{
		mov		dx,word ptr datastring
		sub		dx,32
	}
	asm{
		mov		si,FONT_ADDRESS;			//ds:si VRAM FONT TILE ADDRESS
		
		//go to desired tile
		mov		cl,4						//dx*16
		shl		dx,cl
		add		si,dx
		
		mov 	ax,0A000h
		mov 	es,ax						//es:di destination address	
		mov		bx,lwidth
		//UNWRAPPED COPY 8x8 TILE LOOP
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb				
		add 	di,bx
		movsb
		movsb								
		//END LOOP
		sub		di,lwidth2
		
	    mov 	ax,line
	    inc		ax
	    mov		line,ax
	    cmp		ax,w
	    jne		no_jump_line
		
		add		di,line_jump
		mov 	line,0
	}
	no_jump_line:
	i++;
	asm{
		pop 	bx
		dec		bx
		jnz		printloop3
		
		//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
}

void LT_Print_EGA(word x, word y, word w, char *string){
	word FONT_ADDRESS = FONT_VRAM;
	byte datastring;
	word size = strlen(string);
	byte i = 0;
	word line = 0;
	word screen_offset;
	word lwidth = LT_VRAM_Logical_Width-1;
	word lwidth2 = LT_VRAM_Logical_Width*7;
	word line_jump = (LT_VRAM_Logical_Width*8) -12;
	y = (y<<3);
	screen_offset = (y<<5)+(y<<3)+(y<<2); //y*32 + y*8 + y*4; y*44
	//if (size > 40) size = 40;
	asm{
		push ds
		push di
		push si
		
		mov dx,SC_INDEX //dx = indexregister
		mov ax,0F02h	//INDEX = MASK MAP, 
		out dx,ax 		//write all the bitplanes.
		mov dx,GC_INDEX //dx = indexregister
		mov ax,0008h		
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
	if (datastring > 96) datastring -=32;
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
		mov	bx,lwidth
		//UNWRAPPED COPY 8x8 TILE LOOP
		movsb			//COPY TILE (8 lines)
		add di,bx
		movsb
		add di,bx
		movsb
		add di,bx
		movsb
		add di,bx
		movsb
		add di,bx
		movsb
		add di,bx
		movsb
		add di,bx
		movsb			
		//END LOOP
		
		sub	di,lwidth2
		mov 	ax,line
	    inc		ax
	    mov		line,ax
	    cmp		ax,w
	    jne		no_jump_line
		
		add		di,line_jump
		mov 	line,0
	}
	no_jump_line:
	i++;
	asm{
		pop 	bx
		dec		bx
		jnz		printloop3
		
		//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}


//Print a three digit variable on the window, only 40 positions available 
void (*LT_Print_Variable)(byte,byte,word);

void LT_Print_Variable_VGA(byte x, byte y, word var){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset;
	word lwidth = LT_VRAM_Logical_Width-2;
	x += (SCR_X>>3);
	y += (SCR_Y>>3);
	screen_offset = (LT_VRAM_Logical_Width*(y<<3)) + (x<<1);
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

		mov		ax,2
		mov		bx,lwidth	
		
		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx //END LOOP
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
		
		mov		ax,2
		mov		bx,lwidth
		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx //END LOOP
		
		sub		di,ax //NEXT DIGIT POSITION (-
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

		mov		ax,2
		mov		bx,lwidth
		//UNWRAPPED COPY 8x8 TILE LOOP
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb				
		add 	di,bx
		mov 	cx,ax
		rep		movsb								
	}
		
	asm{//END	
		pop si
		pop di
		pop ds
	}

	outport(GC_INDEX + 1, 0x0ff);
}

void LT_Print_Variable_EGA(byte x, word var){
	word FONT_ADDRESS = FONT_VRAM;
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
		mov		ax,41
		movsb			//COPY TILE (8 lines)
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
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
		mov		ax,41
		movsb			//COPY TILE (8 lines)
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
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
		mov ax,41
		movsb			//COPY TILE (8 lines)
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
		movsb
		add di,ax
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


int LT_Text_Speak = 0;
int LT_Text_Speak_Pos = 0;
int LT_Text_Speak_End = 0;
int LT_Text_y2; //page 0 text
int LT_Text_y3;	//page 1 text
int LT_Text_size;
int LT_Text_i = 0; int LT_Text_j = 0;
int LT_Speak_Sound[16] = {30,31,32,33,34,35,30,20,0,30,0,4,20,0,0,0};
byte LT_Speak_Sound_AdLib[11] = {0x62,0x04,0x04,0x1E,0xF7,0xB1,0xF0,0xF0,0x00,0x00,0x03};
void LT_Draw_Text_Box(word x, word y, byte w, byte h, byte mode, char *string){
	int i;
	unsigned char up[40];
	unsigned char mid[40];
	unsigned char down[40];
	int y1;
	x += (SCR_X>>3);
	y += (SCR_Y>>3);
	y1 = y+38;	//page 1 box
	
	if (!LT_Text_Speak){
		LT_Text_y2 = y+1;
		LT_Text_y3 = y+1+38;
		LT_Text_size = strlen(string);
		up[0] = '#'; up[w+1] = '$'; up[w+2] = 0;
		mid[0] = '*'; mid[w+1] = '+'; mid[w+2] = 0;
		down[0] = '%'; down[w+1] = '&'; down[w+2] = 0;
		for (i = 1; i<w+1; i++){up[i] = '('; mid[i] = ' ';down[i] = ')';}
		//Page 0
		LT_Print(x,y,40,up);y++;
		for (i = 0; i<h; i++) {LT_Print(x,y,40,mid);y++;}
		LT_Print(x,y,40,down);
		//Page 1
		LT_Print(x,y1,40,up);y1++;
		for (i = 0; i<h; i++) {LT_Print(x,y1,40,mid);y1++;}
		LT_Print(x,y1,40,down);
		
		LT_Text_Speak_Pos = 0;
		LT_Text_i = 0; LT_Text_j = 0;
	}
	//Print text inside box
	if (!mode){
		LT_Print(x+1,LT_Text_y2,w,string);
		LT_Print(x+1,LT_Text_y3,w,string);
	} else { //Speaking mode, draw characters one by one
		LT_Text_Speak = 1;
		up[0] = string[LT_Text_Speak_Pos];
		up[1] = 0;
		if (up[0] == ' '){
			if (LT_SFX_MODE) LT_Play_AdLib_SFX(LT_Speak_Sound_AdLib,8,2,2);
			else LT_Play_PC_Speaker_SFX(LT_Speak_Sound);
		}
		LT_Print(x+1+LT_Text_i,LT_Text_y2,2,up);
		LT_Print(x+1+LT_Text_i,LT_Text_y3,2,up);
		
		//Update
		if (LT_Text_i < w-1) {LT_Text_i++;LT_Text_Speak_Pos++;}
		else {
			
			LT_Text_Speak_Pos++;
			LT_Text_i = 0; 
			if (LT_Text_j < h) {LT_Text_j++; LT_Text_y2++;LT_Text_y3++;}
		}
		if (LT_Text_Speak_Pos == LT_Text_size){
			LT_Text_Speak_Pos = 0;
			LT_Text_Speak = 0;
			LT_Text_Speak_End = 1;
		}
	}
	free(up);free(mid);free(down);
}

void LT_Delete_Text_Box(word x, word y, byte w, byte h){
	byte col = 0;
	byte ntile = 0;
	word xx; 
	word yy;
	byte oddw = 0;
	byte oddh = 0;
	byte tilesize;
	if (!LT_VIDEO_MODE) tilesize = 5;
	else tilesize = 6;
	x += (SCR_X>>3);
	y += (SCR_Y>>3);
	x = x>>1; y = y>>1;
	if (w&1) oddw = 1;
	if (h&1) oddh = 1;
	w = (w>>1) + 2 + oddw;
	h = (h>>1) + 2 + oddh;
	
	//Draw map tiles
	for (yy = y;yy<y+h;yy++){
		for (xx = x;xx<x+w;xx++){
			ntile = (LT_map_data[(yy<<8)+xx] - TILE_VRAM) >> tilesize;
			col = LT_map_collision[(yy<<8)+xx];
			LT_Edit_MapTile(xx,yy,ntile,col);
		}
	}
}

//Load and paste 320x240 image for complex images.
extern int LT_Load_Logo;
void LT_Load_Image(char *file){
	dword VGA_index = 0;
	int i;
	word h = 0;
	word x = 0;
	word y = 0;
	byte plane = 0;
	dword offset = 0;
	dword offset_Image = 0;
	FILE *fp;
	
	LT_Filename = file;
	
	//Open the same file with ".ega" extension for ega
	fp = fopen(file,"rb");
	if(!fp)LT_Error("Can't find ",file);
	LT_Header_BMP(fp,0,0);

	offset_Image = ftell(fp);
	
	//COPY TO VGA VRAM
	h = LT_tileset_height;
	fseek(fp,offset_Image,SEEK_SET);
	fread(&LT_tile_tempdata[0],sizeof(unsigned char),320*200, fp);
	
	if (LT_VIDEO_MODE == 0){
		asm CLI //disable interrupts so that loading animation does not interfere
		if (!LT_Load_Logo){
			VGA_index = (304*LT_VRAM_Logical_Width)+(LT_VRAM_Logical_Width*(h-1)); //Second page
		} else {
			VGA_index = 40*(32+224+32+32+224+32); //Store 320x200 image at third "page", leave 32 line spaces
		}
		offset = 0;
		
		asm mov dx, 0x03CE
		asm mov ax, 0x0205	//write mode 2
		asm out dx, ax
		
		//SCAN LINES 
		for (y = 0; y < 200; y++){
			for(x = 0; x < 40; x++){
				//Get an 8 pixel chunk, and convert to 4 bytes
				byte mask = 0x80;
				for(plane = 0; plane < 8; plane++){
					int pixel = VGA[VGA_index];
					pixel = LT_tile_tempdata[offset]; 
					asm mov	dx, 0x03CE  //Set mask register
					asm mov al, 0x08	
					asm mov ah, mask
					asm out dx, ax
					asm shr mask,1
					VGA[VGA_index] = pixel;
					offset++;
				}
				VGA_index++;
				//offset +=8;
			}
			if (!LT_Load_Logo) VGA_index-=(LT_VRAM_Logical_Width<<1)-4;
		}
		asm mov	dx, 0x03CE  //Reset mask register
		asm mov ax, 0x0F08	
		asm out dx, ax
		asm mov dx, 0x03CE
		asm mov ax, 0x0005	//write mode 0
		asm out dx, ax
		asm STI //Re enable interrupts so that loading animation is played again
	}
	if (LT_VIDEO_MODE == 1){
		for (plane = 0; plane < 4; plane ++){
			// select plane
			asm CLI //disable interrupts
			outp(SC_INDEX, MAP_MASK);          
			outp(SC_DATA, 1 << plane);
			if (!LT_Load_Logo){
				VGA_index = (304*LT_VRAM_Logical_Width)+(LT_VRAM_Logical_Width*(h-1)); //Second page
				//SCAN LINES 
				offset = plane;
				for (y = 0; y < 200; y++){
					for(x = 0; x < 80; x++){
						VGA[VGA_index] = LT_tile_tempdata[offset];
						VGA_index++;
						offset +=4;
					}
					VGA_index-=((LT_VRAM_Logical_Width<<1)-8);
				}
			}
			else {
				VGA_index = 80*(32+240+32+32+240+32); //Store 320x200 image at third "page", leave 32 line spaces
				memset(&VGA[VGA_index-(80*100)],LT_tile_tempdata[0],80*100);
				offset = plane;
				for (y = 0; y < 200; y++){
					for(x = 0; x < 80; x++){
						VGA[VGA_index] = LT_tile_tempdata[offset];
						VGA_index++;
						offset +=4;
					}
				}
			}
			asm STI //Re enable interrupts
		}
	}
	fclose(fp);
	LT_Loaded_Image = 1;
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
	byte plane = 0;
	dword offset = 0;
	FILE *fp;
	
	LT_Filename = file;
	
	//Open the same file with ".ega" extension for ega
	fp = fopen(file,"rb");
	
	if(!fp)LT_Error("Can't find ",file);
	
	LT_Header_BMP(fp,1,0);

	LT_tileset_ntiles = (LT_tileset_width>>4) * (LT_tileset_height>>4);

	//LOAD TO TEMP RAM
	fread(&LT_tile_tempdata[0],sizeof(unsigned char), LT_tileset_width*LT_tileset_height, fp);
	
	fclose(fp);
	//COPY TO VGA VRAM
	w = LT_tileset_width>>4;
	h = LT_tileset_height>>4;
	jx = LT_tileset_width+16; 
	if (LT_VIDEO_MODE == 0){
		
		asm mov dx, 0x03CE
		asm mov ax, 0x0205	//write mode 2
		asm out dx, ax
		VGA_index = TILE_VRAM;	//VRAM FONT ADDRESS  //586*(336/4);
		for (tileY = h; tileY > 0 ; tileY--){
			asm CLI //disable interrupts so that loading animation does not interfere
			ty = (tileY<<4)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = (ty*LT_tileset_width) + (tileX<<4);
				//LOAD TILE
				for(y = 0; y < 16; y++){//Get 2 chunks of 8 pixels per row
					int i,j,k;
					for (j = 0; j < 2; j++){
						byte mask = 0x80;
						//Get an 8 pixel chunk, and convert to 4 bytes
						for(plane = 0; plane < 8; plane++){
							int pixel = VGA[VGA_index];
							pixel = LT_tile_tempdata[offset];
							asm mov	dx, 0x03CE  //Set mask register
							asm mov al, 0x08	
							asm mov ah, mask
							asm out dx, ax
							asm shr mask,1
							VGA[VGA_index] = pixel;
							offset++;
						}
						
						VGA_index++;
						if (VGA_index > 0x8000) break;
					}
					offset -= jx;
				}
			}
			asm STI //Re enable interrupts so that loading animation is played again
		}
		
	}
	if (LT_VIDEO_MODE == 1){
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
						offset +=4;
						x++;
						if (x == 4){
							x = 0;
							offset -= jx;
						}
					}
					if (VGA_index > 0xFFFF) break; //Out of VRAM
				}
				if (VGA_index > 0xFFFF) break; //Out of VRAM
			}
			asm STI //Re enable interrupts so that loading animation is played again
		}
		
		//Populate palettes for colour cycling, colours 136-200
		LT_Cycle_palcounter = 0;
		LT_Cycle_palframe = 0;
		//Palette 1
		memcpy(&LT_Cycle_paldata[0],&LT_tileset_palette[200*3],8*3);
		memcpy(&LT_Cycle_paldata[8*3],&LT_tileset_palette[200*3],8*3);
		//Palette 2
		memcpy(&LT_Cycle_paldata[16*3],&LT_tileset_palette[200*3],8*3);
		memcpy(&LT_Cycle_paldata[24*3],&LT_tileset_palette[200*3],8*3);
		
		//Polulate palette for parallax, colours 176-239
		memcpy(&LT_Parallax_paldata[0],&LT_tileset_palette[136*3],64*3);
		memcpy(&LT_Parallax_paldata[64*3],&LT_tileset_palette[136*3],64*3);
	}
}

void LT_unload_tileset(){
	if (LT_tile_tempdata) {free(LT_tile_tempdata); LT_tile_tempdata = NULL;}
	if (LT_tile_tempdata2) {free(LT_tile_tempdata2); LT_tile_tempdata2 = NULL;}
}

//Load tiled TMX map in CSV format
//Be sure bkg layer map is the first to be exported, (before collision layer map)
void LT_Load_Map(char *file){ 
	FILE *f = fopen(file, "rb");
	word start_bkg_data = 0;
	word start_col_data = 0;
	word tile,tile1,tile2,tile3,tile4,tile5,tile6,tile7;
	word index = 0;
	word tilecount = 0; //Just to get the number of tiles to substract to collision tiles 
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
	if ((LT_map_width != 256) || (LT_map_height!=19)){
		LT_Error("Error, map must be 256x19",0);
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">

	//read tile array
	for (index = 0; index < LT_map_ntiles; index+=4){
		fscanf(f, "%i,%i,%i,%i,",&tile,&tile1,&tile2,&tile3);
		if (LT_VIDEO_MODE == 0){
			LT_map_data[index  ] = TILE_VRAM + ((tile -1)<<5); //Store actual tile addresses, to avoid calculations
			LT_map_data[index+1] = TILE_VRAM + ((tile1-1)<<5);
			LT_map_data[index+2] = TILE_VRAM + ((tile2-1)<<5);
			LT_map_data[index+3] = TILE_VRAM + ((tile3-1)<<5);
		}
		if (LT_VIDEO_MODE == 1){
			LT_map_data[index  ] = TILE_VRAM + ((tile -1)<<6); //Store actual tile addresses, to avoid calculations
			LT_map_data[index+1] = TILE_VRAM + ((tile1-1)<<6);
			LT_map_data[index+2] = TILE_VRAM + ((tile2-1)<<6);
			LT_map_data[index+3] = TILE_VRAM + ((tile3-1)<<6);
		}
		
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
	for (index = 0; index < LT_map_ntiles; index+=4){
		fscanf(f, "%d,%d,%d,%d,", &tile, &tile1, &tile2, &tile3);
		LT_map_collision[index] = tile -tilecount;
		LT_map_collision[index+1] = tile1 -tilecount;
		LT_map_collision[index+2] = tile2 -tilecount;
		LT_map_collision[index+3] = tile3 -tilecount;
	}
	fclose(f);
}

void LT_unload_map(){
	LT_map_width = 0;
	LT_map_height = 0;
	LT_map_ntiles = 0;
	if (LT_map_data) {free(LT_map_data); LT_map_data = NULL;}
	if (LT_map_collision) {free(LT_map_collision); LT_map_collision = NULL;}
}

extern int LT_wmap;
extern int LT_hmap;
extern byte LT_flipscroll;
int LT_Setting_Map = 0;
void LT_Set_Map(int x){
	int i = 0;
	int j = 0;
	byte tiles = 0;
	Enemies = 0;
	LT_wmap = LT_map_width<<4;
	LT_hmap = LT_map_height<<4;
	LT_map_offset = x;
	LT_map_offset_Endless = 20;
	SCR_X = x<<4;
	SCR_Y = 0;
	LT_last_x = LT_current_x;
	LT_Update(0,0);
	//draw map
	tiles = 19; 
	LT_VGA_Enable_4Planes();
	LT_flipscroll = 0;
	for (i = 0;i<352;i+=16) {draw_map_column(SCR_X+i,0,LT_map_offset+j,tiles);j++;}
	j = 0;
	LT_flipscroll = 1;
	LT_Setting_Map = 1;
	for (i = 0;i<352;i+=16) {draw_map_column(SCR_X+i,304,LT_map_offset+j,tiles);j++;}
	LT_flipscroll = 0;
	LT_Setting_Map = 0;
	LT_VGA_Return_4Planes();
	LT_Fade_in();
}

void LT_Sprite_Edit_Map(int sprite_number, byte ntile, byte col){
	SPRITE *s = &sprite[sprite_number];
	s->get_item = 1;
	s->ntile_item = ntile;
	s->col_item = col;
}

void (*LT_Edit_MapTile)(word,word,byte,byte);

void LT_Edit_MapTile_VGA(word x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM;
	word tile = (y<<8) + x;
	word y2;
	word ntile2 = TILE_VRAM + (ntile<<6);
	word screen_offset,screen_offset1;
	word lwidth = LT_VRAM_Logical_Width -4;
	x = x<<4;
	y = (y<<4);
	y2 = y + 304;
	screen_offset = (y<<6)+(y<<4)+(y<<3) + (x>>2);
	screen_offset1 = (y2<<6)+(y2<<4)+(y2<<3) + (x>>2);
	LT_map_collision[tile] = col;
	
	//asm cli
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
		
		mov		ax,word ptr [ntile2]		//go to desired tile
		mov		si,ax
		
		mov		di,screen_offset		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax
		mov		bx,lwidth
		//UNWRAPPED COPY 16x16 TILE LOOP
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx	
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		
		//Delete tile on the other page
		sub		si,4*16
		mov		di,screen_offset1		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax
		mov		bx,lwidth
		//UNWRAPPED COPY 16x16 TILE LOOP
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx	
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		movsb
		movsb 

		//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	//asm sti
	LT_map_data[tile] = TILE_ADDRESS + (ntile<<6);
}

void LT_Edit_MapTile_EGA(word x, word y, byte ntile, byte col){
	word TILE_ADDRESS = TILE_VRAM;
	word tile = (y<<8) + x;
	word y2;
	word ntile2 = TILE_VRAM + (ntile<<5);
	word screen_offset,screen_offset1;
	word lwidth = LT_VRAM_Logical_Width -2;
	x = x<<4;
	y = (y<<4);
	y2 = y + 304;
	screen_offset = (y<<5)+(y<<3)+(y<<2)+(x>>3);
	screen_offset1 = (y2<<5)+(y2<<3)+(y2<<2)+(x>>3);
	LT_map_collision[tile] = col;
	
	//asm cli
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
		
		mov		ax,word ptr [ntile2]		//go to desired tile
		mov		si,ax
		
		mov		di,screen_offset		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax
		mov		bx,lwidth
		//UNWRAPPED COPY 16x16 TILE LOOP
		movsb
		movsb
		add 	di,bx	
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb 
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		
		//Delete tile on the other page
		sub		si,2*16
		mov		di,screen_offset1		//es:di destination address							
		mov 	ax,0A000h
		mov 	es,ax
		mov		bx,lwidth
		//UNWRAPPED COPY 16x16 TILE LOOP
		movsb
		movsb
		add 	di,bx	
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		add 	di,bx
		movsb
		movsb

		//END
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}
	//asm sti
	LT_map_data[tile] = TILE_ADDRESS + (ntile<<5);
}

void LT_Set_AI_Sprite(byte sprite_number, byte mode, word x, word y, int sx, int sy, word id);
extern byte LT_flipscroll;
extern int LT_scroll_side;
extern int LT_number_of_ai;
void draw_map_column_vga(word x, word y, word map_offset, word ntiles){
	word screen_offset = (y<<6)+(y<<4)+(y<<3) + (x>>2);
	word *mapdata = LT_map_data;
	word lwidth = LT_VRAM_Logical_Width-4;
	int i = 0;
	word m_offset = map_offset;
	map_offset = map_offset<<1;
	asm{
		push ds
		push di
		push si
		
		mov 	ax,0A000h
		mov 	ds,ax

		les		bx,[mapdata]
		add		bx,map_offset
		mov		si,es:[bx]				//ds:si Tile data VRAM address
		
		mov		di,screen_offset		//es:di screen address	
		mov		cx,ntiles
	}
	//DRAW 19 TILES
	copy_vga_tile:
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		mov		bx,lwidth
		
		movsb				//COPY TILE (16 LINES) use movsb to enable hardware copy (4 pixels)
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx
		movsb
		movsb
		movsb
		movsb
		add 	di,bx//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax

		mov		ax,map_offset
		add		ax,512
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		si,es:[bx]	//ds:si Tile data VRAM address
		
		dec		cx
		JCXZ	end_vga_column
		jmp		copy_vga_tile
	}
	end_vga_column:
	asm{//END
	
		/*mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax */
		
		pop si
		pop di
		pop ds
	}
	//63.078
	if (LT_flipscroll==1){
		if (!LT_Setting_Map) m_offset -= 2560;
		if (LT_Sprite_Stack < 8){
			int j;
			int nsprite;
			for (i = 0; i <19;i++){
				int sprite0 = LT_map_collision[m_offset]; //sprite0-16 for sprite type
				switch (sprite0){
					case 16:
					for (j = 0; j < 3; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>4);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>4,i,LT_scroll_side,0,pos_id_table);//also increase sprite range
							LT_Sprite_Stack++;
							sprite_id_table[pos_id_table] = 1;
							break;
						}
					}
					break;
					case 17:
					for (j = 4; j < 7; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>4);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>4,i,LT_scroll_side,0,pos_id_table);
							LT_Sprite_Stack++;
							sprite_id_table[pos_id_table] = 1;
							break;
						}
					}
					break;
				}
				m_offset+= LT_map_width;
			}
		}
	}
}

void draw_map_column_ega(word x, word y, word map_offset, word ntiles){
	word screen_offset = (y<<5)+(y<<3)+(y<<2)+(x>>3);
	word *mapdata = LT_map_data;
	word lwidth = LT_VRAM_Logical_Width-2;
	int i = 0;
	word m_offset = map_offset;
	map_offset = map_offset<<1;
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

		les		bx,[mapdata]
		add		bx,map_offset
		mov		si,es:[bx]				//ds:si Tile data VRAM address
		
		mov		di,screen_offset		//es:di screen address	
		mov		cx,ntiles
	}
	//UNWRAPPED LOOP
	//DRAW 16 TILES
	blit_ega_tile:
	asm{//COPY TILE
		mov 	ax,0A000h
		mov 	es,ax		//es:di screen address
		mov bx,lwidth
		movsb			//COPY TILE (16 lines)
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb
		movsb
		add di,bx
		movsb
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add di,bx
		movsb	
		movsb
		add 	di,bx//END COPY TILE
		
		//SET ADDRESS FOR NEXT TILE
		mov 	ax,0A000h
		mov 	ds,ax

		mov		ax,map_offset
		add		ax,512		//
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		si,es:[bx]	//ds:si Tile data VRAM address
		
		dec		cx
		JCXZ	end_vga_column
		jmp		blit_ega_tile
	}
	end_vga_column:
	asm{//END
	
		mov dx,GC_INDEX +1 //dx = indexregister
		mov ax,00ffh		
		out dx,ax 
		
		pop si
		pop di
		pop ds
	}

	if (LT_flipscroll==1){
		if (!LT_Setting_Map) m_offset -= 2560;
		if (LT_Sprite_Stack < 8){
			int j;
			int nsprite;
			for (i = 0; i <19;i++){
				int sprite0 = LT_map_collision[m_offset]; //sprite0-16 for sprite type
				switch (sprite0){
					case 16:
					for (j = 0; j < 3; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>4);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>4,i,LT_scroll_side,0,pos_id_table);//also increase sprite range
							LT_Sprite_Stack++;
							sprite_id_table[pos_id_table] = 1;
							break;
						}
					}
					break;
					case 17:
					for (j = 4; j < 7; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>4);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>4,i,LT_scroll_side,0,pos_id_table);
							LT_Sprite_Stack++;
							sprite_id_table[pos_id_table] = 1;
							break;
						}
					}
					break;
				}
				m_offset+= LT_map_width;
			}
		}
	}
}


//Only 19 tiles are updated if we hit a new column
//we update the column at other page in the next frame
byte LT_flipscroll = 0;
int LT_scroll_side = -1;
void LT_Scroll_Map(void){
	LT_current_x = ((SCR_X+8)>>4)<<4;
	
	LT_VGA_Enable_4Planes();
	
	//Second frame
	if (LT_flipscroll == 5){
		if (LT_scroll_side == 1){
			if (LT_last_x) draw_map_column(LT_last_x-16,304,LT_map_offset-1,10); 
		} else 
			draw_map_column(LT_last_x+320,304,LT_map_offset+21,10);
	}
	
	//Third frame
	if (LT_flipscroll == 3){
		LT_map_offset += 2560;
		if (LT_scroll_side == 1){
			if (LT_last_x) draw_map_column(LT_last_x-16,160,LT_map_offset-1,9); 
		} else 
			draw_map_column(LT_last_x+320,160,LT_map_offset+21,9);
	}
	//Final frame
	if (LT_flipscroll == 1){
		if (LT_scroll_side == 1){
			if (LT_last_x) draw_map_column(LT_last_x-16,160+304,LT_map_offset-1,9); 
		} else 
			draw_map_column(LT_last_x+320,160+304,LT_map_offset+21,9);
	}
	if (LT_flipscroll) LT_flipscroll --;
	//Init drawing: First frame
	if (LT_current_x != LT_last_x){
		LT_map_offset = SCR_X>>4;
		if (LT_current_x < LT_last_x){
			LT_scroll_side = 1;
			if (LT_current_x) draw_map_column(LT_current_x-16,0,LT_map_offset-1,10); 
		} else { 
			LT_scroll_side = -1;
			draw_map_column(LT_current_x+320,0,LT_map_offset+21,10);
		}
		LT_flipscroll = 7;
	}
	
	LT_VGA_Return_4Planes();
	
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
}

//Endless Side Scrolling Map
void LT_Endless_SideScroll_Map(int y){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	LT_VGA_Enable_4Planes();
	if (LT_current_x > LT_last_x) { 
		draw_map_column(LT_current_x+320,LT_current_y,((LT_map_offset_Endless+20)%LT_map_width)+(y<<8),19);
		LT_map_offset_Endless++;
	}
	LT_VGA_Return_4Planes();
	LT_last_x = LT_current_x;
}

void set_palette(unsigned char *palette){
	int i;
	outp(0x03c8,0); 
	for(i=0;i<256*3;i++) outp(0x03c9,palette[i]);
}

void VGA_ClearPalette(){
	int i;
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
	LT_vsync();
}

void (*LT_Fade_in)(void);
void (*LT_Fade_out)(void);

void LT_Fade_in_VGA(){
	int i,j;
	unsigned char *pal = LT_Temp_palette;
	memset(LT_Temp_palette,256*3,0x00);//All colours black
	i = 0;
	
	//Fade in
	asm mov	dx,003c8h
	asm mov al,0
	asm out	dx,al
	while (i < 14){//SVGA FAILED with 15
		asm lds si,pal		//Get palette address in ds:si
		asm les di,pal		//Get palette address in es:di
		asm mov dx,003c9h //Palete register
		asm mov cx,256*3
		asm mov bx,0
		fade_in_loop:
			asm LODSB //Load byte from DS:SI into AL, then advance SI
			asm cmp	al,byte ptr LT_tileset_palette[bx]
			asm jae	pal_is_greater
			asm add al,4
			pal_is_greater:
			asm STOSB //Store byte in AL to ES:DI, then advance DI
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
	set_palette(LT_tileset_palette);
}

void LT_Fade_out_VGA(){
	int i,j;
	unsigned char *pal = LT_Temp_palette;
	
	i = 0;
	//asm push ds
	//asm push di
	//asm push si
	
	//Fade to black
	asm mov	dx,003c8h
	asm mov ax,0
	asm out	dx,al
	while (i < 15){
		asm lds si,pal		//Get palette address in ds:si
		asm les di,pal		//Get palette address in es:di
		asm mov dx,003c9h //Palete register
		asm mov cx,256*3
		asm mov bx,0
		fade_out_loop:
			asm LODSB //Load byte from DS:SI into AL, then advance SI
			asm cmp al,0
			asm jz	pal_is_zero
			asm sub al,4
			pal_is_zero:
			asm STOSB //Store byte in AL to ES:DI, then advance DI
			asm out dx,al
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
	
	//asm pop ds
	//asm pop di
	//asm pop si
	if (LT_Sprite_Stack)
		for (i = 1; i != LT_Sprite_Stack; i++) LT_Delete_Sprite(LT_Sprite_Stack_Table[i]);
}

//Cycle palette 
void LT_Cycle_palette(byte palnum, byte speed){
	int i;
	byte pn = 200;
	const unsigned char *datapal;
	if (palnum) {palnum = 16*3; pn = 200;}//Palette 2
	datapal = &LT_Cycle_paldata[LT_Cycle_palframe+palnum];
	
	asm lds si,datapal
	asm mov	dx,003c8h
	asm mov al,pn
	asm out	dx,al //start at position 200 of palette index
	asm mov cx,4
	asm inc dx  //Palete register 003c9h
	cycleloop:
		asm LODSB //Load byte from DS:SI into AL, then advance SI
		asm out dx,al
		asm LODSB 
		asm out dx,al
		asm LODSB 
		asm out dx,al
		
		asm LODSB
		asm out dx,al
		asm LODSB 
		asm out dx,al
		asm LODSB 
		asm out dx,al

		asm loop cycleloop
	
	if (LT_Cycle_palcounter == speed){
		LT_Cycle_palcounter = 0;
		LT_Cycle_palframe+=3;
	}
	LT_Cycle_palcounter++;
	if (LT_Cycle_palframe == 24) LT_Cycle_palframe = 0;
}

//Parallax pallete
void LT_Parallax(){
	int i;
	const unsigned char *datapal;
	int BKG_X = 64 - ((SCR_X>>1)&63); //Get scroll x/2, convert it to a number between 0-64 and invert it.
	LT_Parallax_Frame = BKG_X + BKG_X + BKG_X; //x3, every colour is made of 3 bytes in the array 
	datapal = &LT_Parallax_paldata[LT_Parallax_Frame];
	
	asm lds si,datapal
	asm mov	dx,003c8h
	asm mov al,136
	asm out	dx,al //start at position 136 of palette index
	asm mov cx,32
	asm inc dx  //Palete register 003c9h
	cycleloop:
		asm LODSB //Load byte from DS:SI into AL, then advance SI
		asm out dx,al
		asm LODSB 
		asm out dx,al
		asm LODSB 
		asm out dx,al
		
		asm LODSB //Load byte from DS:SI into AL, then advance SI
		asm out dx,al
		asm LODSB 
		asm out dx,al
		asm LODSB 
		asm out dx,al
		
		asm loop cycleloop
}


//EGA INDEX NUMBERS:
//-------------------
// 0			BLACK
// 1			DARK BLUE
// 2			DARK GREEN
// 3			DARK CYAN
// 4			DARK RED
// 5			DARK MAGENTA
// 6			DARK YELLOW = BROWN
// 7 & 15		LIGHT GRAY, colors 8 to 14 are not used (BLACK)
// 16			DARK GRAY
// 17			LIGHT BLUE
// 18			LIGHT GREEN
// 19			LIGHT CYAN
// 20			LIGHT RED
// 21			LIGHT MAGENTA
// 22			LIGHT YELLOW
// 23			WHITE
byte LT_EGA_FADE[] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,//Full
	0,1,8,3,8,5,4,7,8,9, 2,11, 4,13, 6, 7,
	0,0,1,8,0,1,4,8,0,1, 2, 3, 4, 5, 6, 7,
	0,0,0,8,0,1,8,8,0,1, 8, 8, 8, 1, 4, 8,
	0,0,0,0,0,0,0,0,0,0, 1, 8, 0, 1, 4, 8,
	0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0,//0.00
};

//Nicer colors of EGA palette. Requires special EGA monitor.
//It will work on any VGA card / monitor
byte LT_EGA_64[] = {
	0x00,0x08,0x18,0x11,0x24,0x28,0x14,0x07,0x38,0x19,0x12,0x2B,0x34,0x2D,0x2E,0x3F
};

void LT_64COL_EGA(){
	byte j = 0;
	byte col= 0;
	for (j= 0; j<16;j++){
		col = LT_EGA_64[j];
		inportb(0x03DA);
		outportb(0x03C0,j);	//Index (0-15)
		outportb(0x03C0,col);//Color
		outportb(0x03C0,0x20);
	}
}

void LT_Fade_in_EGA(){
	int i = 0;
	int j = 0;
	int col= 0;
	word fade = 16*5;
	if (LT_EGA_FADE_STATE == 1) return;
	while (i < 6){
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			if (col > 7) col+=8;
			inportb(0x03DA);
			outportb(0x03C0,j);	//Index (0-15)
			outportb(0x03C0,col);//Color (0-23)
			outportb(0x03C0,0x20);
		}
		fade-=16;
		i++;
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 1;
	//LT_64COL_EGA();
}

void LT_Fade_out_EGA(){
	byte x = 0;
	byte i = 0;
	byte j = 0;
	byte col= 0;
	word fade = 0;
	if (LT_EGA_FADE_STATE == 0) return;
	for (x= 0; x<5;x++){
		fade+=16;
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			if (col > 7) col+=8;
			inportb(0x03DA);
			outportb(0x03C0,j);	//Index (0-15)
			outportb(0x03C0,col);//Color
			outportb(0x03C0,0x20);
		}
		
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 0;
	if (LT_Sprite_Stack)
		for (i = 1; i != LT_Sprite_Stack; i++) LT_Delete_Sprite(LT_Sprite_Stack_Table[i]);
}


void LT_Cycle_Palette_EGA(byte *p,byte *c,byte speed){
	int x;
	byte col;
	for (x= 0; x<4;x++){
		col = c[(LT_Cycle_palframe+x)&3];
		if (col > 7) col+=8;
		inportb(0x03DA);
		outportb(0x03C0,p[x]);	//Index (0-15)
		outportb(0x03C0,col);	//Color
		outportb(0x03C0,0x20);
	}
	if (LT_Cycle_palcounter == speed){
		LT_Cycle_palcounter = 0;
		LT_Cycle_palframe++;
		if (LT_Cycle_palframe == 4) LT_Cycle_palframe = 0;
	}
	LT_Cycle_palcounter++;
}