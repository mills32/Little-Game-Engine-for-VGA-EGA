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
#define INPUT_STATUS_0		0x03da
#define SC_INDEX            0x03c4    // VGA sequence controller
#define SC_DATA             0x03c5

#define GC_INDEX            0x03ce    // VGA graphics controller
#define GC_DATA             0x03cf

#define MAP_MASK            0x02
#define ALL_PLANES          0xff02
 

extern byte LT_DETECTED_CARD;
word LT_VRAM_Logical_Width;
// One map in ram stored at "LT_map"
word LT_map_width;
word LT_map_height;
word LT_map_ntiles;
word far *LT_map_data;
byte far *LT_map_collision;
// One tileset 
word LT_tileset_width;
word LT_tileset_height;
word LT_tileset_ntiles;
byte LT_tileset_palette[256*3];
byte far *LT_tileset_data;
byte LT_EGA_Font_Palette[4] = {0,9,7,15};
unsigned char far *LT_tile_tempdata; //Temp storage of non tiled data. and also sound samples
unsigned char far *LT_tile_tempdata2; //Second half 
unsigned char far *LT_CGA_TGA_FONT;
extern unsigned char *LT_sprite_data; // 
SPRITE LT_Loading_Animation; 
extern SPRITE far *sprite;
extern byte far *sprite_id_table;
byte LT_Loaded_Image = 0; //If you load a 320x240 image (to the second page), the delete loading interrupt will paste it to first page
byte LT_PAL_TIMER = 0;
byte LT_EGA_FADE_STATE = 0;
byte LT_EGA_TEXT_TRANSLUCENT = 0;
word LT_FrameSkip = 0;
byte *LT_Filename; //Contains name of the images oppened, for LT_Load_BMP function 

void LT_Error(char *error, char *file);
void LT_Reset_Sprite_Stack();
void DAT_Seek(FILE *fp, char *dat_string);

extern word LT_Sprite_Size_Table_EGA_32[];
extern byte LT_SFX_MODE;
extern byte LT_VIDEO_MODE;
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
byte *CGA=(byte *)0xB8000000L;
byte *VGA=(byte *)0xA0000000L; 

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 0;
int TGA_SCR_X = 0;
int TGA_SCR_Y = 0;
int TANDY_SCROLL_X = 0;
int TANDY_SCROLL_Y = 0;
int LT_C64 = 0;
int LT_current_x = 0;
int LT_last_x = 0;
int LT_current_y = 0;
int LT_last_y = 0;
int LT_map_offset = 0;
int LT_map_offset_y = 0;
int LT_map_offset_Endless = 0;
int LT_follow = 0;
byte LT_var = 0;//To disable fade in/out
int vpage = 0;
int Enemies = 0;
void (*draw_map_column)(word, word, word, word);
word LT_offset_vram_TXT;
void LT_Draw_Sprites_TGA();
extern void interrupt (*LT_old_time_handler)(void); 
void interrupt (far * LT_getvect(byte intr))();
void LT_setvect(byte intr, void interrupt (far *func)());
void LT_TGA_MapPage(byte);
byte tga_page = 0;

void DAT_Seek2(word fp,char *dat_string); 
void LT_fread(word file_handle,word bytes,void *buffer);
unsigned long LT_fseek(word file_handle,unsigned long bytes,byte origin);

word _strlen(char *str){
	word len = 0;
	while(1){if (str[len] == 0) break;len++;}
	return len;
}

void _memcpy(void *dest, void *src, word number){
	asm push ds;asm push si;asm push es;asm push di;
	
	asm lds	si,src
	asm les	di,dest
	asm mov cx,number
	asm rep movsb //ds:si to es:di
	
	asm pop di;asm pop es;asm pop si;asm pop ds;
}


void Check_Graphics_Card(){
	byte reg = 0;
	asm mov ax,0x1A00
	asm mov bl,0x32
	asm int 0x10; asm mov reg,al;
	if (reg == 0x1A) {_printf("Card detected: VGA\n\r$"); LT_DETECTED_CARD = 1;LT_VIDEO_MODE = 1;}
	else {
		asm mov ah,0x12
		asm mov bx,0x1010
		asm int 0x10; asm mov reg,bh;
		if (reg == 0) {
			_printf("Card detected: EGA\n\r$");
			LT_DETECTED_CARD = 0;
		} else {
			asm mov ah,0x0F
			asm mov bl,0x00
			asm int 0x10; asm mov reg,al;
			if (reg == 0x07) {
				_printf("Card detected: Hercules\n\r$");
				LT_Exit();
			} else {_printf("Card detected: CGA or TANDY\n\r$");LT_DETECTED_CARD = 2;}
		}
	}
	
	Clearkb();
}

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

void LT_VGA_Enable_4Planes(){
	if (LT_VIDEO_MODE <2){
	word planes = 0x0F02;
	if ((LT_VIDEO_MODE == 0) && (LT_EGA_TEXT_TRANSLUCENT)) planes = 0x0702;
	
	asm mov dx,03c4h  	//dx = indexregister
	asm mov ax,planes	//INDEX = MASK MAP, 3d4
	asm out dx,ax 		//write all the bitplanes.
	asm mov dx,03ceh 	//dx = indexregister 3ce
	asm mov ax,0008h		
	asm out dx,ax
	
	}
}

void LT_VGA_Return_4Planes(){
	if (LT_VIDEO_MODE <2){
	asm mov dx,03ceh +1 //dx = indexregister
	asm mov ax,00ffh		
	asm out dx,ax
	if ((LT_VIDEO_MODE == 0)&& (LT_EGA_TEXT_TRANSLUCENT)){
		asm mov dx,03C4h	//dx = indexregister
		asm mov ax,0F02h	//INDEX = MASK MAP, 
		asm out dx,ax 		//write plane 1.
	}
	}
}

void LT_vsync(){
	//Try to catch Vertical retrace
	asm mov	dx,0x3DA; asm mov bl,0x08;
	WaitNotVsync: asm in al,dx; asm test al,bl; asm jnz WaitNotVsync;
	WaitVsync:    asm in al,dx; asm test al,bl; asm jz WaitVsync;
	//while( inp( INPUT_STATUS_0 ) & 0x08 );
	//while( !(inp( INPUT_STATUS_0 ) & 0x08 ) );
}

void LT_ClearScreen(){
	if (LT_VIDEO_MODE == 0){
		asm mov dx,0x03CE
		asm mov ax,0x0005	//write mode 0
		asm out dx,ax
		asm mov	dx,0x03CE	  //Set mask register
		asm mov ax,0xFF08
		asm out	dx,ax
		LT_memset(VGA,0x00,(352>>2)*240);
	}
	if (LT_VIDEO_MODE == 1){
		VGA_ClearPalette();
		//clear screen
		asm mov dx,0x03C4
		asm mov ax,0xFF02
		asm out dx,ax
		LT_memset(VGA,0x00,(352>>2)*240);
	}
	if (LT_VIDEO_MODE == 2);
	if (LT_VIDEO_MODE == 3){
		LT_TGA_MapPage(1);LT_memset(CGA,0x00,32*1024);
		LT_TGA_MapPage(0);LT_memset(CGA,0x00,32*1024);
	}
	
}


// Uncompress BMP image (either RLE8 or RLE4)
// FROM GIMP: removed most multiplications, all divisions, and delta
void decompress_RLE_BMP(word fp, unsigned char bpp, int width, int height,byte palette){
	int xpos = 0; int ypos = 0;
	int i,j,n;
	int total_bytes_read = 0;
	byte i_max = 0;
	byte _bpp = 0;
	byte count_val[2] = {0,0};//Get mode Get data
	
	if (bpp == 8) _bpp = 3;
	if (bpp == 4) _bpp = 2;
    while (ypos < height && xpos <= width){
		int y_offset = ypos * width;
		LT_fread(fp,2,count_val);
		// Count + Color - record 
		if (count_val[0] != 0){                 
            // encoded mode run - count == run_length; val == pixel data
			if (count_val[1]) (count_val[1]+=palette);
            for (j = 0; ( j < count_val[0]) && (xpos < width);){
				for (i = 1;((i <= (8 >> _bpp)) && (xpos < width) && ( j < count_val[0]));i++, xpos++, j++){
                    LT_tile_tempdata[y_offset + xpos] = (count_val[1] & (((1<<bpp)-1) << (8 - (i << _bpp)))) >> (8 - (i << _bpp));
				}
			}
        }
		// uncompressed record
		if ((count_val[0] == 0) && (count_val[1] > 2)){ 
			n = count_val[1];
			total_bytes_read = 0;
			for (j = 0; j < n; j += (8 >> _bpp)){
                // read the next byte in the record 
				byte c; byte d = 0;
				LT_fread(fp,1,&c);
				if (c) d = palette; else d = 0;
				c+=d;
                total_bytes_read++;
				// read all pixels from that byte
				i_max = 8 >> _bpp;
				if (n - j < i_max) i_max = n - j;
                i = 1;
				while ((i <= i_max) && (xpos < width)){
                    LT_tile_tempdata[y_offset + xpos] = (c >> (8-(i<<_bpp))) & ((1<<bpp)-1);
                    i++; xpos++;
                }
            }
			// absolute mode runs are padded to 16-bit alignment
            if (total_bytes_read & 1) LT_fseek(fp,1,1);
        }
		// Line end
		if ((count_val[0] == 0) && (count_val[1] == 0)){ypos++;xpos = 0;}
		// Bitmap end
		if ((count_val[0] == 0) && (count_val[1] == 1)) break;
		// Deltarecord. I did not find any BMP using this
        //if ((count == 0) && (val == 2)){
		//	count = fgetc(fp);val = fgetc(fp);xpos += count; ypos += val;
        //}
    }
}

byte LT_mode_sprite = 0;
byte LT_pixel_format = 0;
//Compatible with all 4/8 bpp BMP files
//Mode: 0 = image; 1 = tiles; 2 = sprite; 3 = window; 4/5 = animation/font 
void LT_Load_BMP(char *file, char *dat_string, int mode, int sprite_number){
	int index;
	int read_pal = 1;
	word x;
	word _offset = 0;
	byte b = 0;
	byte d = 0;
	word num_colors;
	byte get_pal = 1;
	byte first_color = 0;
	byte pal_colors = 0;
	byte RLE = 0;
	word DIB_HEADER_SIZE = 0;
	word header;
	SPRITE *s = &sprite[sprite_number];
	word _width;
	word _height;
	word *width;
	word *height;
	
	word fp = LT_fopen(file,0);
	if(!fp) LT_Error("Can't find $",file);
	if (dat_string) {DAT_Seek2(fp,dat_string);LT_Filename = dat_string;}
	else LT_Filename = file;
	
	LT_mode_sprite = 0;
	LT_pixel_format = 0;
	//Tiles or image
	if (mode < 2){width = &LT_tileset_width; height = &LT_tileset_height;}
	//Sprites
	if (mode == 2){width = &s->width; height = &s->height;}
	//Window //Animation //Font
	if (mode > 2){width = &_width; height = &_height;}
	
	//Read header
	LT_fread(fp,2,&header);
	if (header != 0x4D42) LT_Error("Not a BMP file $",LT_Filename);
	LT_fseek(fp, 12,SEEK_CUR);
	LT_fread(fp, 2,&DIB_HEADER_SIZE );
	LT_fseek(fp, 2, SEEK_CUR);
	LT_fread(fp, 2, width);
	LT_fseek(fp, 2, SEEK_CUR);
	LT_fread(fp, 2, height);
	LT_fseek(fp, 4, SEEK_CUR);
	LT_fread(fp, 1, &LT_pixel_format);
	LT_fseek(fp, 1, SEEK_CUR);
	LT_fread(fp, 1, &RLE);	//0 none, 1 = 8 bit, 2 = 4 bit
	LT_fseek(fp, 15,SEEK_CUR);
	LT_fread(fp, 2, &num_colors);
	//Advance to palette data
	//Skip "color space information" (if it is there) added by modern apps like GIMP
	DIB_HEADER_SIZE -=34;
	LT_fseek(fp,DIB_HEADER_SIZE, SEEK_CUR);

	if (num_colors==0)  num_colors=256;
	if (num_colors > 256) LT_Error("Image has more than 256 colors $",LT_Filename);
	
	//Place colors in appropiate offset of VGA palette
	switch (mode){
		case 0://If reading image
			//if (LT_tileset_height != 200) LT_Error("Wrong size for image, size must be 320x200: ",LT_Filename);
			//if (LT_tileset_width != 320) LT_Error("Wrong size for image, size must be 320x200: ",LT_Filename);
			pal_colors = 208;first_color = 0;
		break;
		case 1://If reading tileset
			pal_colors = 208;first_color = 0;
		break;
		case 2://If reading sprites
			if (s->width & 1) LT_Error("Sprite width not even: $",LT_Filename);
			if (s->height & 1) LT_Error("Sprite height not even: $",LT_Filename);
			pal_colors = 32; first_color = 208;
			LT_tileset_width = s->width; LT_tileset_height = s->height;
			LT_mode_sprite = 1;
		break;
		case 4://If reading Animation
		case 5://or font
			if (LT_pixel_format !=4)LT_Error("Wrong format for loading animation, must be 4 bit per pixel: $",LT_Filename);
			if (_height !=32) LT_Error("Wrong size for animation/font, image must be 128x32: $",LT_Filename);
			if (_width !=128) LT_Error("Wrong size for animation/font, image must be 128x32: $",LT_Filename);
			LT_tileset_width = _width; LT_tileset_height = _height;
			if (mode == 4){pal_colors = 4; first_color = 248;}
			else {pal_colors = 4; first_color = 252;}//If reading font
			LT_mode_sprite = 2;
		break;
	}
	if (mode == 4) LT_mode_sprite=2;
	if (mode == 5) LT_mode_sprite=3;

	if (LT_mode_sprite == 1){
		if ((LT_pixel_format !=4)&&(LT_VIDEO_MODE == 3))LT_Error("TANDY sprite must be 4 bit per pixel: $",LT_Filename);
		if ((LT_pixel_format !=4)&&(LT_VIDEO_MODE == 0))LT_Error("EGA sprite must be 4 bit per pixel: $",LT_Filename);
		if ((LT_pixel_format !=8)&&(LT_VIDEO_MODE == 1))LT_Error("VGA sprite must be 8 bit per pixel: $",LT_Filename);
	}

	if ((LT_DETECTED_CARD == 1)&& (LT_VIDEO_MODE ==0)){
		if (mode > 1) read_pal = 0;
		else {first_color = 0; pal_colors = 16; read_pal = 1;}
	}
	//Load Palette
	for(index=first_color;index<first_color+num_colors;index++){
		if (index-first_color == pal_colors) get_pal = 0;
		if (get_pal && read_pal){
			byte col[3];
			LT_fread(fp,3,col);
			LT_tileset_palette[(int)(index*3+2)] = col[0] >> 2;
			LT_tileset_palette[(int)(index*3+1)] = col[1] >> 2;
			LT_tileset_palette[(int)(index*3+0)] = col[2] >> 2;
		} else {
			LT_fseek(fp,3,1);
		}
		LT_fseek(fp,1,1);
	}
	//
	//EGA INDEX NUMBERS:
	// 0-7/15-23
	if ((LT_VIDEO_MODE == 0) && (LT_DETECTED_CARD == 1)){
		_memcpy(&LT_tileset_palette[16*3],&LT_tileset_palette[8*3],8*3);;
	}

	//PROCESS IMAGE
	if (LT_VIDEO_MODE == 1){
		if (LT_mode_sprite==1) b=208;
		if (LT_mode_sprite==2) b=248;
		if (LT_mode_sprite==3) b=252; //Font color from 252 to 255
	}
	if (LT_pixel_format == 4){
		if (RLE){
			word w = width[0];word h = height[0];
			decompress_RLE_BMP(fp,LT_pixel_format,w,h,b);
		} else {
			int pixels = (LT_tileset_width>>1)*LT_tileset_height;
			LT_fread(fp,pixels,LT_tile_tempdata2);
			for (x = 0; x < pixels; x ++){
				unsigned char c = LT_tile_tempdata2[x];//it is a 4 bit BMP, every byte contains 2 pixels
				LT_tile_tempdata[_offset++] = (c >>4) +b;
				LT_tile_tempdata[_offset++] = (c & 0x0F)+b;
			}
		}
	}
	if (LT_pixel_format == 8){
		if (RLE){
			word w = width[0];word h = height[0];
			decompress_RLE_BMP(fp,LT_pixel_format,w,h,b);
		} else {
			for (x = 0; x < LT_tileset_width*LT_tileset_height; x ++){
				unsigned char c = 0;
				LT_fread(fp,1,&c);
				if (c) d = b; else d = 0;
				LT_tile_tempdata[_offset] = c + d;
				_offset++;
			}
		}
	}
	if (LT_pixel_format == 1){
		
	}
	
	LT_fclose(fp);
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
extern unsigned char *LT_sprite_data;
void LT_Load_Animation(char *file, char *dat_string){
	long index,offset;
	word data_offset = 0;
	word x,y;
	word i,j;
	word frame = 0;
	byte tileX = 0;
	byte size = 32;
    int code_size;
	LT_Filename = file;
	
	LT_Load_BMP(file,dat_string,4,0);
	
	index = 0; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileX=0;tileX<128;tileX+=size){
		offset = (128*31)+tileX;
		if (LT_VIDEO_MODE == 1){
			LT_tile_tempdata2[index] = size;
			LT_tile_tempdata2[index+1] = size;
			index+=2;
		}
		for(x=0;x<32;x++){
			_memcpy(&LT_tile_tempdata2[index],&LT_tile_tempdata[offset-(x<<7)],size);
			index+=size;
		}
	}

	LT_Loading_Animation.nframes = 4;
	LT_Loading_Animation.code_size = 0;
	LT_Loading_Animation.ega_size = &LT_Sprite_Size_Table_EGA_32[0];

	if (LT_VIDEO_MODE <2){
		for (frame = 0; frame < LT_Loading_Animation.nframes; frame++){		//
			if (LT_VIDEO_MODE == 1){
				code_size = Compile_Bitmap(LT_VRAM_Logical_Width, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],&LT_sprite_data[data_offset]);
				LT_Loading_Animation.frames[frame].compiled_code = &LT_sprite_data[data_offset];
				data_offset += code_size;
				LT_Loading_Animation.code_size += code_size;
			} 
			if (LT_VIDEO_MODE == 0){
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
	}
	if (LT_VIDEO_MODE == 3){
		offset = 0;index = 0;
		for (offset = 0; offset < 4096; offset+=2){
			LT_sprite_data[index++] = (LT_tile_tempdata2[offset]<<4)+LT_tile_tempdata2[offset+1];
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
void run_compiled_tga_sprite(word XPos, word YPos, char *Sprite);
void interrupt LT_Loading(void){
	asm CLI
	{	
		int i;
		SPRITE *s = &LT_Loading_Animation;
		//animation
		if (s->anim_speed == s->speed){
			s->anim_speed = 0;
			s->frame++;
			if (s->frame == s->aframes) s->frame = 0;
		}
		s->anim_speed++;
	
		if (LT_VIDEO_MODE == 1) run_compiled_sprite(s->pos_x,s->pos_y,s->frames[s->frame].compiled_code);
		if (LT_VIDEO_MODE == 0){
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
		if (LT_VIDEO_MODE == 3) {
			int i;
			word offset = s->frame<<9;
			word offset1 = (160*26)+72;
			tga_page = 0;
			LT_TGA_MapPage(0);
			for(i = 0; i < 8;i++){
				_memcpy(&CGA[offset1],&LT_sprite_data[offset],16);offset1+=8192;offset+=16;
				_memcpy(&CGA[offset1],&LT_sprite_data[offset],16);offset1+=8192;offset+=16;
				_memcpy(&CGA[offset1],&LT_sprite_data[offset],16);offset1+=8192;offset+=16;
				_memcpy(&CGA[offset1],&LT_sprite_data[offset],16);offset1-=((8192*3)-160);offset+=16;
			}
			LT_TGA_MapPage(1);
		}
	}
	asm STI
	
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
}

void LT_Start_Loading(){//77.144
	LT_Cycle_palcounter = 0;
	LT_Stop_Music();
	LT_Fade_out();
	LT_Disable_Speaker();
	LT_Reset_Sprite_Stack();
	LT_Unload_Sprites();
	//Wait Vsync
	LT_vsync();
	//Reset scroll
	asm mov dx,0x03D4
	asm mov ax,0x000D
	asm out dx,ax
	asm mov ax,0x000C
	asm out dx,ax
	
	SCR_X = 0; SCR_Y = 0;
	TGA_SCR_X = 0;TGA_SCR_Y = 0;
	TANDY_SCROLL_X = 0;TANDY_SCROLL_Y = 0;
	
	LT_ClearScreen();
	
	LT_memset(sprite_id_table,0,19*256);
	
	//change color 0, 1, 2 (black and white)
	if (!LT_var){
	LT_tileset_palette[0] = LT_Loading_Palette[0];
	LT_tileset_palette[1] = LT_Loading_Palette[1];
	LT_tileset_palette[2] = LT_Loading_Palette[1];
	}
	//center loading animation
	LT_Loading_Animation.pos_x = 144;
	LT_Loading_Animation.pos_y = 104;
	
	asm CLI
	//set timer
	asm mov al,0x36
	asm out 0x43,al
	asm mov al,92
	asm out 0x40,al	//lo-byte
	asm mov al,155
	asm out 0x40,al	//hi-byte
	//set interrupt
	LT_setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
	
	asm STI
	
	//Wait Vsync
	LT_vsync();
	LT_Keys[LT_ESC] = 0;LT_Keys[LT_ENTER] = 0;LT_Keys[LT_ACTION] = 0; LT_Keys[LT_JUMP] = 0;
	LT_Fade_in();
}

void LT_End_Loading(){
	//unsigned long spd = 1193182/60;
	LT_Fade_out();
	LT_Keys[LT_ESC] = 0;LT_Keys[LT_ENTER] = 0;LT_Keys[LT_ACTION] = 0; LT_Keys[LT_JUMP] = 0;
	asm CLI
	//set frame counter
	asm mov al,0x36
	asm out 0x43,al
	asm mov al,0xFF
	asm out 0x40,al
	asm out 0x40,al
	LT_setvect(0x1C, LT_old_time_handler);
	asm STI
	
	if (LT_Loaded_Image){
		int i;
		if (LT_VIDEO_MODE<2){
			LT_VGA_Enable_4Planes();
			for (i = 0; i < LT_VRAM_Logical_Width*200;i++) VGA[i] = VGA[i + (304*LT_VRAM_Logical_Width)];
			LT_Loaded_Image = 0;
		}
		if (LT_VIDEO_MODE == 3){
			word offset = 0;
			for (i = 0; i < 8;i++){
				LT_TGA_MapPage(1);
				_memcpy(LT_map_collision,&CGA[offset],4096);
				LT_TGA_MapPage(0);
				_memcpy(&CGA[offset],LT_map_collision,4096);
				offset+=4096;
			}
			LT_Loaded_Image = 0;
		}
		LT_Fade_in();
	}
	LT_VGA_Return_4Planes();
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

//////////////////////
//Hardware scrolling// 
//////////////////////
void (*LT_WaitVsync)();

//This is smooth in dosbox, PCem, 86Box, original EGA/VGA many VGA compatibles on very slow systems.
//This is optimized to be as fast as it can be for the poor 8088
void LT_WaitVsync_VGA(){
	word x = SCR_X;
	word y = SCR_Y + pageflip[gfx_draw_page&1];

	y*=LT_VRAM_Logical_Width;
	if (LT_VIDEO_MODE == 0) {y += x>>3; pix = p1[SCR_X & 7];} //EGA
	if (LT_VIDEO_MODE == 1) {y += x>>2; pix =  p[SCR_X & 3];} //VGA
	gfx_draw_page++;
	
	//change scroll registers: 
	asm {
		cli
		mov	cl,8; mov dx,003d4h; //VGA PORT
		mov ax,y; shl ax,cl; or ax,00Dh; out dx,ax; //(y << 8) | 0x0D; to VGA port
		mov ax,y; and ax,0FF00h; or ax,00Ch; out dx,ax;	//(y & 0xFF00) | 0x0C; to VGA port
		sti
	}
	
	//Try to catch Vertical retrace
	asm mov	dx,0x3DA; asm mov bl,0x08;
	WaitNotVsync: asm in al,dx; asm test al,bl; asm jnz WaitNotVsync;
	WaitVsync:    asm in al,dx; asm test al,bl; asm jz WaitVsync;
	
	//The smooth panning happens here
	asm {
		cli
		mov dx,0x03C0;
		mov al,0x33; out dx,al;	//0x20 | 0x13 (palette normal operation | Pel panning reg)	
		mov al,byte ptr pix; out dx,al;
		sti
	}
	
	//I don't think this is necessary
	//asm mov		dx,INPUT_STATUS_0 //Read input status, to Reset the VGA flip/flop
	//_ac = inp(AC_INDEX);//Store the value of the controller
	//AC index 0x03c0
	
	//Restore controller value
	//asm mov ax,word ptr _ac
	//asm out dx,ax
	//enable interrupts
}

//Alternative functions taken from keen70Hz, should be compatible with most cards
static unsigned char MaxHblankLength = 1;
static unsigned char benchmark_hblank_length(){
	unsigned char hblank_length;
	asm cli
	asm mov dx, 3DAh

wait_blank_end:  // if we are in hblank or vblank, wait for it to end
	asm in al, dx  // Read 3DAh - Status Register
	asm test al, 1 // Bit 0: Display Blank
	asm jnz wait_blank_end

wait_active_end: // wait for the end of the active scanline
	asm in al, dx  // Read 3DAh again
	asm test al, 1
	asm jz wait_active_end

	// We are now right at the start of a blank period, either hblank or vblank
	// Calculate how many cycles this blank lasts.
	asm mov cx, 0    // store in cl: seen_status, ch: hblank_len
calc_blank_length: // measure how many I/O port read cmds we can do in blank
	asm in al, dx    // Read port.
	asm or cl, al    // Track in cl if this blank period contained vsync bit
	asm inc ch       // Accumulate count of I/Os performed
	asm test al, 1   // Still in blank period?
	asm jnz calc_blank_length

	asm test cl, 8   // Blank period is now over. Check if it included vsync
	asm jnz wait_blank_end // If so, restart all from scratch. We wanted hblank.
  asm mov hblank_length, ch
  asm sti
  return hblank_length;
}

void CalibrateMaxHblankLength(){
	int i;
	MaxHblankLength = 1;
	// Benchmark: What is the maximum amount of ISA/PCI/AGP port I/Os that can be
	// performed within a single hblank period?
	// I.e. find what is the max length on the bus that a Hblank can take.
	for(i = 0; i < 1000; ++i)
	{
		int length = benchmark_hblank_length();
		if (length > MaxHblankLength) MaxHblankLength = length;
	}
	// Add just a tiny bit more over the count that we got.
	if (MaxHblankLength <= 253) MaxHblankLength += 2;
	//printf("MaxHblankLength: %d\n", (int)MaxHblankLength);
}

void LT_WaitVsync_VGA_Compatible(){
	word x = SCR_X;
	word y = SCR_Y + pageflip[gfx_draw_page&1];
	int i;
	
	//
	y*=LT_VRAM_Logical_Width;
	if (LT_VIDEO_MODE == 0) {y += x>>3; pix = p1[SCR_X & 7];} //EGA
	if (LT_VIDEO_MODE == 1) {y += x>>2; pix =  p[SCR_X & 3];} //VGA
	gfx_draw_page++;

	asm mov dx, 3DAh

	wait_for_active_picture:// Wait until we are in visible picture area.
	asm in al,dx			// Read 3DAh - Status Register
	asm test al,1			// Bit 0: set if we are in Display Blank.
	asm jnz wait_for_active_picture

	// We are now in visible picture area (so can't be in vsync, or right headed
	// into it)
	asm mov cx,0
	wait_for_hblank:
		asm cli // Enter time critical stage: estimating the length of a blank.
		asm mov cl, MaxHblankLength // Reset wait counter to default

	loop_hblank_length_times:
		asm in al,dx	// Read status port
		asm test al,1	// Are we in display blank?
		asm jnz in_blank// If 1, then we are in blank
		// Else 0, we are still in visible picture area. Enable interrupts and restart the wait.
		asm sti			
		asm jmp wait_for_hblank 

		in_blank:		// We are in blank, but have we slipped over to vsync?
		asm test al,8	// Test if in vsync?
		asm jz in_blank_not_vsync // If 0, we are not in vsync

		asm sti			// Else 1, we are in vsync, so we blew it. Restart the wait.
		asm jmp wait_for_active_picture

		in_blank_not_vsync: // We are in blank, either hblank or vblank
		// Decrement search counter
		asm loop loop_hblank_length_times // And loop back if we still need to.

	// If we get here, we have entered a display blank period that is longer than
	// a hblank interval, so we conclude we must have just now entered a vblank.
	// (but we aren't yet at start of vsync)
	// Interrupts are disabled at this point, so we can safely update
	// Display Start (DS) and Horizontal Shift Count (HS) registers so all
	// adapters will latch it properly, with all their varying quirky behaviors.
	// (Pedantically, it is tiny bit better better to write DS register before HS,
	// because IBM EGA and VGA latch the DS register before the HS register)

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
	
	//AC index 0x03c0
	asm mov dx,0x03C0
	asm mov al,0x33		//0x20 | 0x13 (palette normal operation | Pel panning reg)	
	asm out dx,al
	asm mov al,byte ptr pix
	asm out dx,al

	//enable interrupts
	asm sti
}


//Generic function that may work on many "SVGA" cards on very fast systems
//Assumes a very fast PC which will catch the vsync at the very beginning,
//and the CPU will update all registers at once, no need to optimize in asm.
//Tested on toshiba satellite laptop
void LT_WaitVsync_VGA_crappy(){//77.128
	word x = SCR_X;
	word y = SCR_Y + pageflip[gfx_draw_page&1];
	int i;
	
	y*=LT_VRAM_Logical_Width;
	if (LT_VIDEO_MODE == 0) {y += x>>3; pix = p1[SCR_X & 7];} //EGA
	if (LT_VIDEO_MODE == 1) {y += x>>2; pix =  p[SCR_X & 3];} //VGA
	gfx_draw_page++;

	//Try to catch Vertical retrace
	asm mov	dx,0x3DA; asm mov bl,0x08;
	WaitNotVsync: asm in al,dx; asm test al,bl; asm jnz WaitNotVsync;
	WaitVsync:    asm in al,dx; asm test al,bl; asm jz WaitVsync;
	
	//change scroll registers: 
	asm {
		cli
		mov	cl,8; mov dx,003d4h; //VGA PORT
		mov ax,y; shl ax,cl; or ax,00Dh; out dx,ax; //(y << 8) | 0x0D; to VGA port
		mov ax,y; and ax,0FF00h; or ax,00Ch; out dx,ax;	//(y & 0xFF00) | 0x0C; to VGA port
		
		mov dx,0x03C0
		mov al,0x33		//0x20 | 0x13 (palette normal operation | Pel panning reg)	
		out dx,al
		mov al,byte ptr pix
		out dx,al
		sti
	}	
}


byte panning = 0;
byte static_screen = 0;

//CRT Page
//Bits 0-2 select the 16K page showed by the video. In 32K modes, bit 0 is ignored.
//Processor Page
//bits 3-5 combined with the CPU address to select the
//32K segment of memory accessed at B8000. If an odd page number is selected
//(1,3,5, etc.) the window is reduced to 16K.
//Bits 6-7 mode 320x240 = 11
//
//Tandy graphics will use the last 128K of RAM for the 8 pages (16k each)
//We should use the last 4 pages (2 32K pages) for owr double buffer
byte page_show[] = {0xC4,0xC6};//show Page 4 / show page 6 (+ mode 320x200)
byte page_write[] = {0x20,0x30};//write Page 4 / write page 6

void LT_TGA_MapPage(byte wpage){
	byte val = (page_write[wpage&1] | page_show[tga_page&1]);
	asm mov dx,03DFh
	asm mov al,val		//Select write page
	asm out dx,al
}

void LT_WaitVsync_TGA(){
	word x = TGA_SCR_X;
	word y = TGA_SCR_Y;
	byte val;
	tga_page++;
	val = page_show[tga_page&1];
	
	y = y>>2;//even
	y = (y<<6)+(y<<4)+(x>>2); //(y*64)+(y*16) = y*80; + x/4

	//Wait Vsync
	asm mov		dx,03DAh
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //TGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to TGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to TGA port
	
	asm mov dx,03DFh
	asm mov al,val		//Select show page
	asm out dx,al
}

void LT_Update_Scroll_TGA(){
	word x = TGA_SCR_X;
	word y = TGA_SCR_Y;
	y = y>>2;//even
	y = (y<<6)+(y<<4)+(x>>2); //(y*64)+(y*16) = y*80; + x/4

	//Wait Vsync
	asm mov		dx,03DAh
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	//outport(0x03d4, 0x0D | (y << 8));
	//outport(0x03d4, 0x0C | (y & 0xff00));
	asm mov dx,003d4h //TGA PORT
	asm mov	cl,8
	asm mov ax,y
	asm	shl ax,cl
	asm	or	ax,00Dh	
	asm out dx,ax	//(y << 8) | 0x0D to TGA port
	asm mov ax,y
	asm	and ax,0FF00h
	asm	or	ax,00Ch
	asm out dx,ax	//(y & 0xFF00) | 0x0C to TGA port
}


//Updates the graphics and sprites
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
	LT_Read_Joystick();
}

void LT_Wait(byte seconds){
	seconds*=60;
	while (seconds){seconds--;LT_Update(0,0);}
}

// load_8x8 fonts to VRAM (64 characters)
void LT_Load_Font(char *file, char *dat_string){
	word VGA_index = 0;
	byte TGA_CONVERT[] = {0,9,7,15};
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
	
	LT_Filename = file;
	//LT_Load_BMP2("IMAGES.DAT","LOGO_EGA.bmp",0,0);
	LT_Load_BMP(file,dat_string,5,0);

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
	
	if (LT_VIDEO_MODE == 3){
		VGA_index = 0;
		for (tileY = h; tileY > 0 ; tileY--){
			asm CLI //disable interrupts so that loading animation does not interfere
			ty = (tileY<<3)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = (ty*128) + (tileX<<3);
				//LOAD TILE
				for(y = 0; y < 8; y++){
					int j;
					for (j = 0; j < 4; j++){
						byte pl0 = TGA_CONVERT[LT_tile_tempdata[offset++]];
						byte pl1 = TGA_CONVERT[LT_tile_tempdata[offset++]];
						byte pixel = (pl0<<4) | pl1;
						LT_CGA_TGA_FONT[VGA_index] = pixel;
						VGA_index++;
					}
					offset -= jx;
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
	word size = _strlen(string);
	word i = 0;
	word line = 0;
	word lwidth = LT_VRAM_Logical_Width-2;
	word lwidth2 = LT_VRAM_Logical_Width*7;
	word line_jump = (LT_VRAM_Logical_Width*8) - (w<<1);
	y = (y<<3);
	screen_offset = (y<<6)+(y<<4)+(y<<3);	//(y*64)+(y*16)+(y*8) = y*88;

	//if (size > 40) size = 40;
	asm{
		push ds
		push di
		push si
		
		//mov dx,SC_INDEX //dx = indexregister
		//mov ax,0F02h	//INDEX = MASK MAP, 
		//out dx,ax 		//write all the bitplanes.
		//mov dx,GC_INDEX //dx = indexregister
		//mov ax,0008h		
		//out dx,ax 
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
		
	    inc		line
	    mov 	ax,line
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
		//mov dx,GC_INDEX +1 //dx = indexregister
		//mov ax,00ffh		
		//out dx,ax 
		
		pop si
		pop di
		pop ds
	}
}

void LT_Print_EGA(word x, word y, word w, char *string){
	word FONT_ADDRESS = FONT_VRAM;
	byte datastring;
	word size = _strlen(string);
	byte i = 0;
	word line = 0;
	word screen_offset;
	word lwidth = LT_VRAM_Logical_Width-1;
	word lwidth2 = LT_VRAM_Logical_Width*7;
	word line_jump = (LT_VRAM_Logical_Width*8) -w;
	//word planes = 0x0F02;
	//if (LT_EGA_TEXT_TRANSLUCENT) planes = 0x0702;
	y = (y<<3);
	screen_offset = (y<<5)+(y<<3)+(y<<2); //y*32 + y*8 + y*4; y*44
	//if (size > 40) size = 40;
	asm{
		push ds
		push di
		push si
		
		//mov dx,SC_INDEX //dx = indexregister
		//mov ax,planes	//INDEX = MASK MAP, 
		//out dx,ax 		//write bitplanes.
		//mov dx,GC_INDEX //dx = indexregister
		//mov ax,0008h		
		//out dx,ax 
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
		
		inc		line
	    mov		ax,line
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
		//mov dx,GC_INDEX +1 //dx = indexregister
		//mov ax,00ffh		
		//out dx,ax 
		
		pop si
		pop di
		pop ds
	}
}

void LT_Print_TGA(word x, word y, word w, char *string){
	byte *tiles = LT_CGA_TGA_FONT;
	byte datastring;
	word size = _strlen(string);
	byte i = 0;
	word line = 0;
	word screen_offset;
	word lwidth = 8192-4;
	word lwidth2 = (8192*3)+160;
	word line_jump = (160*2) -(w<<2);

	x = ((x<<3)>>2);
	y = 160*2*y;
	screen_offset = y+x;
	//if (size > 40) size = 40;
	asm{
		push ds
		push di
		push si
		
		mov	di,screen_offset
		mov ax,x
		add di,ax
		mov bx,size
	}
	printloop3:
	asm push bx
	datastring = string[i];
	if (datastring > 96) datastring -=32;
	asm{
		mov		ah,0
		mov		al,datastring
		sub		al,32
		
		lds		si,tiles;			//ds:si FONT TILE ADDRESS
		
		//go to desired tile
		mov		cl,5						//dx*32
		shl		ax,cl
		add		si,ax
		
		mov 	ax,0B800h
		mov 	es,ax						//es:di destination address	
		mov	bx,lwidth
		//UNWRAPPED COPY 8x8 TILE LOOP
		and		di,8191
		movsw
		movsw
		add		di,bx
		movsw
		movsw
		add		di,bx
		movsw
		movsw
		add		di,bx
		movsw
		movsw
		
		sub		di,(8192*3)-156
		and		di,8191
		movsw
		movsw
		add		di,bx
		movsw
		movsw
		add		di,bx
		movsw
		movsw
		add		di,bx
		movsw
		movsw			
		//END LOOP
		
		sub	di,lwidth2
		
		inc		line
	    mov		ax,line
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
		
		pop si
		pop di
		pop ds
	}
	
}


//Print a three digit variable on the window
void (*LT_Print_Variable)(byte,byte,word);

void LT_Print_Variable_VGA(byte x, byte y, word var){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset;
	word lwidth = LT_VRAM_Logical_Width-2;
	x += (SCR_X>>3);
	y += (SCR_Y>>3);
	screen_offset = (LT_VRAM_Logical_Width*(y<<3)) + (x<<1);
	LT_VGA_Enable_4Planes();

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

	LT_VGA_Return_4Planes();
}

void LT_Print_Variable_EGA(byte x, word var){
	word FONT_ADDRESS = FONT_VRAM;
	word screen_offset = (42<<2) + (x) + 3;
	LT_VGA_Enable_4Planes();
	asm{
		push ds
		push di
		push si

		//DIVIDE VAR BY 10, GET REMAINDER, DRAW DIGIT
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

	LT_VGA_Return_4Planes();
}


int LT_deleting_text = 0;
int LT_Text_Speak = 0;
int LT_Text_Speak_Pos = 0;
int LT_Text_Speak_End = 0;
int LT_Text_y2; //page 0 text
int LT_Text_y3;	//page 1 text
int LT_Text_size;
int LT_Text_i = 0; int LT_Text_j = 0;
byte LT_Speak_Sound[16] = {30,31,32,33,34,35,30,20,0,30,0,4,20,0,0,0};
byte LT_Speak_Sound_AdLib[11] = {0x62,0x04,0x04,0x1E,0xF7,0xB1,0xF0,0xF0,0x00,0x00,0x03};
unsigned char up[40]; unsigned char mid[40]; unsigned char down[40];
void draw_text_box(word x, word y, byte w, byte h, byte mode, char *string){
	int i;
	int y1;
	if (LT_VIDEO_MODE == 3){
		x += (TGA_SCR_X>>3);
		y += (TGA_SCR_Y>>3);
	} 
	if (LT_VIDEO_MODE < 2){
		x += (SCR_X>>3);
		y += (SCR_Y>>3);
	} 
	y1 = y+38;	//page 1 box
	if (!LT_Text_Speak){//
		int size = 0;
		if (_strlen(string) > (w*h)) size = w*h;
		else size = _strlen(string);
		LT_Text_size = size;
		LT_Text_y2 = y+1;
		LT_Text_y3 = y+1+38;
		if (LT_VIDEO_MODE == 3) {LT_Text_y3 = y+1;}
		up[0] = '#'; up[w+1] = '$'; up[w+2] = 0;
		mid[0] = '*'; mid[w+1] = '+'; mid[w+2] = 0;
		down[0] = '%'; down[w+1] = '&'; down[w+2] = 0;
		for (i = 1; i<w+1; i++){up[i] = '('; mid[i] = ' ';down[i] = ')';}
		//Page 0
		if (LT_VIDEO_MODE == 3) {y1 = y; LT_TGA_MapPage(0);}
		LT_Print(x,y,w+2,up);y++;
		for (i = 0; i<h; i++) {LT_Print(x,y,w+2,mid);y++;}
		LT_Print(x,y,w+2,down);
		if (LT_VIDEO_MODE == 3) LT_TGA_MapPage(1);
		if (!LT_Loaded_Image){//Page 1
			LT_Print(x,y1,w+2,up);y1++;
			for (i = 0; i<h; i++) {LT_Print(x,y1,w+2,mid);y1++;}
			LT_Print(x,y1,w+2,down);
		}
		LT_Text_Speak_Pos = 0;
		LT_Text_i = 0; LT_Text_j = 0;
	}
	//Print text inside box
	if (!mode){
		if (LT_VIDEO_MODE == 3) LT_TGA_MapPage(0);
		LT_Print(x+1,LT_Text_y2,w,string);
		if (LT_VIDEO_MODE == 3) LT_TGA_MapPage(1);
		if (!LT_Loaded_Image) LT_Print(x+1,LT_Text_y3,w,string);
	} else { //Speaking mode, draw characters one by one
		LT_Text_Speak = 1;
		up[0] = string[LT_Text_Speak_Pos];
		up[1] = 0;
		if (up[0] == ' '){
			if (LT_SFX_MODE) LT_Play_AdLib_SFX(LT_Speak_Sound_AdLib,8,2,2);
			else LT_Play_PC_Speaker_SFX(LT_Speak_Sound);
		}
		if (LT_VIDEO_MODE == 3) LT_TGA_MapPage(0);
		LT_Print(x+1+LT_Text_i,LT_Text_y2,2,up);
		if (LT_VIDEO_MODE == 3) LT_TGA_MapPage(1);
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
}

void delete_text_box(word x, word y, byte w, byte h){
	byte ntile = 0;
	word xx; 
	word yy;
	byte oddw = 0;
	byte oddh = 0;
	byte tilesize;
	if (LT_VIDEO_MODE==0) tilesize = 5;
	else tilesize = 6;
	if (LT_VIDEO_MODE == 3){
		x += (TGA_SCR_X>>3);
		y += (TGA_SCR_Y>>3);
	} 
	if (LT_VIDEO_MODE < 2){
		x += (SCR_X>>3);
		y += (SCR_Y>>3);
	} 
	x = x>>1; y = y>>1;
	if (w&1) oddw = 1;
	if (h&1) oddh = 1;
	w = (w>>1) + 2 + oddw;
	h = (h>>1) + 2 + oddh;
	LT_deleting_text = 1;
	//Draw map tiles
	for (yy = y;yy<y+h;yy++){
		for (xx = x;xx<x+w;xx++){
			if (LT_VIDEO_MODE <  2) ntile = (LT_map_data[(yy<<8)+   xx] - TILE_VRAM) >> tilesize;
			if (LT_VIDEO_MODE == 3) ntile = (LT_map_data[(yy<<9)+xx<<1]>>2);
			LT_Edit_MapTile(xx,yy,ntile,0);
		}
	}
	LT_deleting_text = 0;
}

//LT_Draw_Text_Box: prints text inside a box
//-mode 0: simple text box
//-mode 1: simple "talking" text box (writes characters 1 by 1)
//-mode 2: info box no talking, waits for "key1" to close (restores bkg map)	   
//-mode 3: info box talking mode.
//-mode 4: info box no talking, two options key1,key2 (Y/N continue/exit...)
//-mode 5: info box talking, two options.
//-mode 6: manualy delete a mode 0 or 1 box.

// keys = scancodes.
//returns 1 if mode = 4 or mode = 5 and key2 pressed. else 0 
//if bkg consist of an image, this can't restore it.

byte LT_Draw_Text_Box(word x, word y, byte w, byte h, word mode, byte key1, byte key2, char* text){
	int loop = 1;
	byte val = 0;
	LT_VGA_Enable_4Planes();
	if (mode > 6) mode = 6;
	if (mode != 6){
	if (mode > 1) while (LT_Keys[key1]) LT_Read_Joystick();
	if (mode&1){//Talking
		LT_Text_Speak_End = 0;
		while (!LT_Text_Speak_End){
			draw_text_box(x,y,w,h,1,text);
			LT_Play_Music();
			LT_Vsync();
		}
		LT_Text_Speak_End = 0;
	} else draw_text_box(x,y,w,h,0,text);
	if (mode > 1){
		asm in al, 61h; asm and al, 252; asm out 61h, al //Disable speaker
		while (loop){
			LT_Play_Music();
			LT_Read_Joystick();
			if (LT_Keys[key1]) {loop = 0; val = 0;}
			if ((LT_Keys[key2]) && (mode > 3)) { loop = 0; val = 1;}
			LT_vsync();
		}
		while (LT_Keys[key1]) {LT_Play_Music();LT_Read_Joystick();LT_vsync();}
	}
	}
	if (mode > 1) delete_text_Box(x,y,w,h);
	LT_VGA_Return_4Planes();
	return val;
}

//Load and paste 320x200 image for complex images.
extern byte loading_logo;
void LT_Load_Image(char *file,char* dat_string){
	dword VGA_index = 0;
	int i;
	word h = 0;
	word x = 0;
	word y = 0;
	byte plane = 0;
	dword offset = 0;
	
	asm CLI
	if (!loading_logo)LT_Draw_Text_Box(11,18,16,1,0,0,0,       "LOADING:  IMAGE ");
	else {LT_Draw_Text_Box(11,12,16,1,0,0,0,"LOADING/CARGANDO");loading_logo = 0;}
	asm STI
	
	LT_Filename = file;
	LT_Load_BMP(file,dat_string,0,0);
	
	//COPY TO VGA VRAM
	h = LT_tileset_height;

	if (LT_VIDEO_MODE == 0){
		asm CLI //disable interrupts so that loading animation does not interfere
		VGA_index = (304*LT_VRAM_Logical_Width)+(LT_VRAM_Logical_Width*(h-1)); //Second page
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
					int pixel = VGA[VGA_index];//read VRAM latch
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
			VGA_index-=(LT_VRAM_Logical_Width<<1)-4;
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
			asm mov ax,0x0102
			asm mov cl,plane
			asm shl ah,cl
			asm mov dx,0x03C4
			asm out dx,ax
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
			asm STI //Re enable interrupts
		}
	}
	if (LT_VIDEO_MODE == 3){
		LT_TGA_MapPage(1);
		i = 0;
		for (y = 0; y < 200; y++){
			offset = (200-y)*320;
			for(x = 0; x < 160; x++){
				CGA[VGA_index++] = (LT_tile_tempdata[offset]<<4)+LT_tile_tempdata[offset+1];
				offset+=2;
			}
			i++;
			if (i < 4) VGA_index += 8192-160;
			else {VGA_index += 8192;VGA_index &= (8192*4)-1; i = 0;}
		}
	}	
	
	LT_Loaded_Image = 1;
}

// load_16x16 tiles to VRAM
void LT_Load_Tiles(char *file,char* dat_string){
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
	
	asm CLI
	LT_Draw_Text_Box(11,18,16,1,0,0,0,"LOADING: TILESET");
	asm STI
	
	LT_Filename = file;
	LT_Load_BMP(file,dat_string,1,0);
	
	LT_tileset_ntiles = (LT_tileset_width>>4) * (LT_tileset_height>>4);
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
			asm mov ax,0x0102
			asm mov cl,plane
			asm shl ah,cl
			asm mov dx,0x03C4
			asm out dx,ax
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
		_memcpy(&LT_Cycle_paldata[0],&LT_tileset_palette[200*3],8*3);
		_memcpy(&LT_Cycle_paldata[8*3],&LT_tileset_palette[200*3],8*3);
		//Palette 2
		_memcpy(&LT_Cycle_paldata[16*3],&LT_tileset_palette[200*3],8*3);
		_memcpy(&LT_Cycle_paldata[24*3],&LT_tileset_palette[200*3],8*3);
		
		//Polulate palette for parallax, colours 176-239
		_memcpy(&LT_Parallax_paldata[0],&LT_tileset_palette[136*3],64*3);
		_memcpy(&LT_Parallax_paldata[64*3],&LT_tileset_palette[136*3],64*3);
	}
	if (LT_VIDEO_MODE == 3){
		VGA_index = 0;
		for (tileY = h; tileY > 0 ; tileY--){
			asm CLI //disable interrupts so that loading animation does not interfere
			ty = (tileY<<4)-1;
			for (tileX = 0; tileX < w; tileX++){
				offset = (ty*LT_tileset_width) + (tileX<<4);
				//LOAD TILE
				for(y = 0; y < 16; y++){
					int j;
					for (j = 0; j < 4; j++){
						byte pl0 = LT_tile_tempdata[offset++];
						byte pl1 = LT_tile_tempdata[offset++];
						byte pixel = (pl0<<4) | pl1;
						LT_tile_tempdata2[VGA_index] = pixel;
						VGA_index++;
					}
					offset += 8;
					offset -= jx;
				}
				offset = (ty*LT_tileset_width) + (tileX<<4) + 8;
				//LOAD TILE
				for(y = 0; y < 16; y++){
					int j;
					for (j = 0; j < 4; j++){
						byte pl0 = LT_tile_tempdata[offset++];
						byte pl1 = LT_tile_tempdata[offset++];
						byte pixel = (pl0<<4) | pl1;
						LT_tile_tempdata2[VGA_index] = pixel;
						VGA_index++;
					}
					offset += 8;
					offset -= jx;
				}
			}
			asm STI //Re enable interrupts so that loading animation is played again
		}
	}
}


//Parse html files (load map helper functions)
void skip_lines(word f,int lines){
	//0x0D,0x0A win; 0x0A linux Mac;
	while (lines){
		byte c; LT_fread(f,1,&c);
		if (c == 0x0A) lines--;
	}
}

void find_char(word f, byte number, byte character){
	while (number){
		byte c = 0;
		while (c != character) LT_fread(f,1,&c);
		number--;
	}
}

int _sscanf(word f, byte terminate){
	byte c = 0;
	byte n0 = 0; byte n1 = 0; byte n2 = 0;
	while (1){
		LT_fread(f,1,&c);
		if (c == terminate) break;
		if (c == 0x0D) {LT_fseek(f,1,1);LT_fread(f,1,&c);}
		if (c == 0x0A) LT_fread(f,1,&c);
		n0 = n1;n1 = n2;
		n2 = c-48;
	}
	return (int)((n0*100) + (n1*10) + n2);
}

//Load tiled TMX map in CSV format 
//Be sure bkg layer map is the first to be exported, (before collision layer map)
void LT_Load_Map(char *file, char* dat_string){ 
	word f = LT_fopen(file,0);
	byte tile;
	byte buffer[32];
	word index = 0;
	word tindex = 0;
	word tgawidth = 0;
	word tilecount = 0; //Just to get the number of tiles to substract to collision tiles 
	char line[128];
	char name[64]; //name of the layer in TILED

	asm CLI
	LT_Draw_Text_Box(11,18,16,1,0,0,0,"LOADING: TMX MAP");
	asm STI
	
	if(!f) LT_Error("Can't find$",file);
	if (dat_string) DAT_Seek2(f,dat_string);
	else LT_fseek(f, 0, SEEK_SET);
	//read file		
	skip_lines(f,5);
	find_char(f,1,0x22);//find first ", read tilecount
	tilecount = _sscanf(f,0x22);	
	skip_lines(f,3);
	find_char(f,5,0x22);//find 5 " read width,
	LT_map_width = _sscanf(f,0x22);
	find_char(f,1,0x22);//find next " read height
	LT_map_height = _sscanf(f,0x22);
	
	LT_map_ntiles = LT_map_width*LT_map_height;
	if (LT_VIDEO_MODE < 2){
		if ((LT_map_width != 256) || (LT_map_height!=19)){//
			LT_Error("Error, map must be 256x19 $",0);
		}
	}
	skip_lines(f,2); //skip 2 lines: <data encoding="csv">
	//read tile array
	if (LT_VIDEO_MODE == 0){//Store actual tile addresses, to avoid calculations
		for (index = 0; index < LT_map_ntiles-1; index++){
			if (!dat_string) tile = _sscanf(f,0x2C);
			else LT_fread(f,1,&tile);
			LT_map_data[index  ] = TILE_VRAM + ((tile -1)<<5); 
		}
	}
	if (LT_VIDEO_MODE == 1){//Store actual tile addresses, to avoid calculations
		for (index = 0; index < LT_map_ntiles-1; index++){
			if (!dat_string) tile = _sscanf(f,0x2C);
			else LT_fread(f,1,&tile);
			LT_map_data[index  ] = TILE_VRAM + ((tile -1)<<6);
		}
	}
	if (LT_VIDEO_MODE == 3){
		word a = 0;word w = 0;
		for (index = 0; index < LT_map_ntiles-1; index++){
			if (!dat_string) tile = _sscanf(f,0x2C);
			else LT_fread(f,1,&tile);
			a = (tile-1)<<2;
			w = LT_map_width<<1;
			LT_map_data[tindex  ] = a;LT_map_data[tindex+1] = a+2;
			LT_map_data[tindex+w] = a+1;LT_map_data[tindex+w+1] = a+3;
			tindex+=2;
			tgawidth++;
			if (tgawidth == LT_map_width) {tgawidth = 0;tindex += (LT_map_width<<1);}
		}
	}
	//skip 5 lines
	skip_lines(f,5);
	//read collision array
	for (index = 0; index < LT_map_ntiles-1; index++){
		if (!dat_string) tile = _sscanf(f,0x2C);
		else LT_fread(f,1,&tile);
		LT_map_collision[index] = tile -tilecount;
	}
	LT_fclose(f);
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
	LT_map_offset_y = 0;
	LT_map_offset_Endless = 20;
	SCR_X = x<<4;SCR_Y = 0;
	TGA_SCR_X = x<<4;TGA_SCR_Y = 0;
	LT_last_x = LT_current_x;
	//draw map
	if (LT_VIDEO_MODE == 0){
		asm mov dx,0x03CE
		asm mov ax,0x0005	//write mode 0
		asm out dx,ax
		asm mov	dx,0x03CE	  //Set mask register
		asm mov al,0x08
		asm out	dx,al
	}
	if (LT_VIDEO_MODE < 2) {
		LT_Update(0,0);
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
		
	} 
	if (LT_VIDEO_MODE > 2) {
		LT_Update(0,0);
		tiles = 25; 
		LT_flipscroll = 0;
		LT_Setting_Map = 1;
		LT_TGA_MapPage(0);
		for (i = 0;i<320;i+=8){draw_map_column(SCR_X+i,0,(LT_map_offset<<1)+j,tiles);j++;}
		j = 0;
		LT_flipscroll = 1;
		LT_Setting_Map = 1;
		LT_TGA_MapPage(1);
		for (i = 0;i<320;i+=8){draw_map_column(SCR_X+i,0,(LT_map_offset<<1)+j,tiles);j++;}
		LT_flipscroll = 0;
		LT_Setting_Map = 0;	
	} 
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
	if (!LT_deleting_text)LT_map_collision[tile] = col;
	
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
	if (!LT_deleting_text)LT_map_collision[tile] = col;
	
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

void Edit_MapTile_TGA(word x, word y, word ntile){
	word *TILE_ADDRESS = (word*)&LT_tile_tempdata2[0];
	word screen_offset;
	word tile = ntile;
	x = x<<3;y = y<<3;
	x = x>>1;
	y = y>>3;
	screen_offset = (320*y)+x;
	
	//asm cli
	asm{
		push ds
		push di
		push si
		
		mov		ax,tile				//ax Tile number
		mov		cl,5
		shl		ax,cl				//tile number*32 = tile data offset
		
		lds		si,[TILE_ADDRESS]	//ds:si Tile data
		add		si,ax
		mov 	ax,0B800h
		mov 	es,ax
		mov		di,screen_offset			//es:di SCREEN
		
		//COPY TILE
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		sub		di,(8192*3)-156
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		
		pop si
		pop di
		pop ds
	}
	//asm sti
}

void LT_Edit_MapTile_TGA(word x, word y, byte ntile, byte col){
	word ntile1  = ntile<<2;
	word tiled   = (y<<10) + (x<<1);
	word tilecol = (y<<8) + x;
	x = x<<1; y = y<<1;
	LT_TGA_MapPage(tga_page);
	Edit_MapTile_TGA(x,y,ntile1);Edit_MapTile_TGA(x+1,y,ntile1+2);
	Edit_MapTile_TGA(x,y+1,ntile1+1);Edit_MapTile_TGA(x+1,y+1,ntile1+3);
	LT_TGA_MapPage(tga_page+1);
	Edit_MapTile_TGA(x,y,ntile1);Edit_MapTile_TGA(x+1,y,ntile1+2);
	Edit_MapTile_TGA(x,y+1,ntile1+1);Edit_MapTile_TGA(x+1,y+1,ntile1+3);
	
	if (!LT_deleting_text)LT_map_collision[tilecol] = col;
	LT_map_data[tiled    ] = ntile1;  LT_map_data[tiled  +1] = ntile1+2;
	LT_map_data[tiled+512] = ntile1+1;LT_map_data[tiled+513] = ntile1+3;
}

void LT_Set_AI_Sprite(byte sprite_number, byte mode, word x, word y, int sx, int sy, word id);
extern byte LT_flipscroll;
extern int LT_scroll_side;
extern int LT_scroll_top;
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
	
		//mov dx,GC_INDEX +1 //dx = indexregister
		//mov ax,00ffh		
		//out dx,ax
		
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

void draw_map_column_tga(word x, word y, word map_offset, word ntiles){
	word s_offset = 0;
	word m_offset = x>>4;
	word *mapdata = LT_map_data;
	word *tiles = (word*)&LT_tile_tempdata2[0];
	x = x>>1;
	y = y>>3;
	s_offset = (320*y)+x;
	map_offset = map_offset<<1;
	asm{
		push es
		push ds
		push si
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		ax,word ptr es:[bx]			//ax Tile number
		mov		cl,5
		shl		ax,cl				//tile number*32 = tile data offset
		
		lds		si,[tiles]			//ds:si Tile bkg data
		add		si,ax
		mov 	ax,0B800h
		mov 	es,ax
		mov		di,s_offset			//es:di SCREEN
		mov		cx,ntiles
	}
	
	loop_tile:
	asm{
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		sub		di,(8192*3)-156
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		dec 	cx
		mov		ax,cx
		jz 		end_tile
		
		add		map_offset,1024
		
		push es
		push cx
		les		bx,[mapdata]
		add		bx,map_offset
		mov		ax,word ptr es:[bx]			//al Tile number
		mov		cl,5
		shl		ax,cl				//tile number*32 = tile data offset
		
		lds		si,[tiles]			//ds:si Tile bkg data
		add		si,ax
		pop cx
		pop es
		
		sub		di,(8192*3)-156
		
		jmp		loop_tile
	}	
	end_tile:


	asm{
		pop es
		pop ds
		pop si
	}	
	

	if (LT_flipscroll==1){
		//if (!LT_Setting_Map) m_offset -= 2560;
		if (LT_Sprite_Stack < 8){
			int i,j;
			int nsprite;
			for (i = 0; i <19;i++){
				int sprite0 = LT_map_collision[m_offset]; //sprite0-16 for sprite type
				switch (sprite0){
					case 16:
					for (j = 0; j < 3; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>3);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>3,i,LT_scroll_side,0,pos_id_table);//also increase sprite range
							LT_Sprite_Stack++;
							sprite_id_table[pos_id_table] = 1;
							break;
						}
					}
					break;
					case 17:
					for (j = 4; j < 7; j++){
						if (LT_Active_AI_Sprites[j]){
							word pos_id_table = (i<<8) + (x>>3);
							if (sprite_id_table[pos_id_table] == 1) break;
							nsprite = LT_Active_AI_Sprites[j];
							LT_Active_AI_Sprites[j] = 0;
							LT_Sprite_Stack_Table[LT_Sprite_Stack] = nsprite;
							if (sprite[nsprite].init == 0)
								LT_Set_AI_Sprite(nsprite,sprite[nsprite].mode,x>>3,i,LT_scroll_side,0,pos_id_table);
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

void draw_map_row_tga(word x, word y, word map_offset, word ntiles){
	word s_offset = 0;
	word *mapdata = LT_map_data;
	word *tiles = (word*)&LT_tile_tempdata2[0];
	x = x>>1;
	y = y>>3;
	s_offset = (320*y)+x;
	map_offset = map_offset<<1;
	asm{
		push es
		push ds
		push si
		
		les		bx,[mapdata]
		add		bx,map_offset
		mov		ax,word ptr es:[bx]			//ax Tile number
		mov		cl,5
		shl		ax,cl				//tile number*32 = tile data offset
		
		lds		si,[tiles]			//ds:si Tile bkg data
		add		si,ax
		mov 	ax,0B800h
		mov 	es,ax
		mov		di,s_offset			//es:di SCREEN
		mov		cx,ntiles
	}
	
	loop_tile:
	asm{
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		sub		di,(8192*3)-156
		and		di,8191
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		add		di,8192-4
		movsw
		movsw
		
		dec 	cx
		mov		ax,cx
		jz 		end_tile
		
		add		map_offset,2
		
		push es
		push cx
		les		bx,[mapdata]
		add		bx,map_offset
		mov		ax,word ptr es:[bx]			//al Tile number
		mov		cl,5
		shl		ax,cl				//tile number*32 = tile data offset
		
		lds		si,[tiles]			//ds:si Tile bkg data
		add		si,ax
		pop cx
		pop es
		
		sub		di,(8192*3)+160
		
		jmp		loop_tile
	}	
	end_tile:


	asm{
		pop es
		pop ds
		pop si
	}		
}


void (*LT_Scroll_Map)(void);

//Only 19 tiles are updated if we hit a new column
//we update the column at other page in the next frame
byte LT_flipscroll = 0;
byte LT_flipscrollv = 0;
int LT_scroll_side = -1;
int LT_scroll_top = -1;
void LT_Scroll_Map_GFX(void){
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

void LT_Scroll_Map_TGA(void){
	if (!TANDY_SCROLL_X){
		if ((SCR_X-TGA_SCR_X) > 104) TANDY_SCROLL_X = 1;
		if ((SCR_X-TGA_SCR_X) < -104) TANDY_SCROLL_X = -1;
	}
	if (!TANDY_SCROLL_Y){
		if ((SCR_Y > 30)&&(TGA_SCR_Y ==   0)) TANDY_SCROLL_Y = 1;
		if ((SCR_Y < 30)&&(TGA_SCR_Y == 104)) TANDY_SCROLL_Y = -1;
	}

	while (TANDY_SCROLL_X){
		if (TANDY_SCROLL_X == 1) {
			TGA_SCR_X+=4;
			if (TGA_SCR_X > SCR_X) {TGA_SCR_X = SCR_X;TANDY_SCROLL_X = 0;}
		}
		if (TANDY_SCROLL_X ==-1) {
			TGA_SCR_X-=4;
			if (TGA_SCR_X < SCR_X) {TGA_SCR_X = SCR_X;TANDY_SCROLL_X = 0;}
		}
		
		LT_current_x = ((TGA_SCR_X)>>3)<<3;
		LT_current_y = ((TGA_SCR_Y)>>3)<<3;
		
		if (LT_current_x != LT_last_x){
			LT_map_offset = (LT_current_y<<6)+(LT_current_x>>3);
			if (LT_current_x < LT_last_x){
				LT_scroll_side = 1;
				if (LT_current_x>-4) {
					LT_TGA_MapPage(0);
					draw_map_column(LT_current_x,TGA_SCR_Y,LT_map_offset,25);
					LT_TGA_MapPage(1);LT_flipscroll = 1;
					draw_map_column(LT_current_x,TGA_SCR_Y,LT_map_offset,25);
					LT_flipscroll = 0;
				}
			} else { 
				LT_scroll_side = -1;
				LT_TGA_MapPage(0);
				draw_map_column(LT_current_x+312,TGA_SCR_Y,LT_map_offset+39,25);
				LT_TGA_MapPage(1);LT_flipscroll = 1;
				draw_map_column(LT_current_x+312,TGA_SCR_Y,LT_map_offset+39,25);
				LT_flipscroll = 0;
			}
		}
		LT_last_x = LT_current_x;
		LT_Play_Music();
		LT_Update_Scroll_TGA();
	}

	while (TANDY_SCROLL_Y && !TANDY_SCROLL_X){
		if (TANDY_SCROLL_Y == 1) {
			TGA_SCR_Y+=4;
			if (TGA_SCR_Y > 104) {TGA_SCR_Y = 104;TANDY_SCROLL_Y = 0;}
		}
		if (TANDY_SCROLL_Y ==-1) {
			TGA_SCR_Y-=4;
			if (TGA_SCR_Y < 0) {TGA_SCR_Y = 0;TANDY_SCROLL_Y = 0;}
		}
		
		LT_current_x = ((TGA_SCR_X)>>3)<<3;
		LT_current_y = ((TGA_SCR_Y)>>3)<<3;
		LT_map_offset_y = (LT_current_y<<6)+(LT_current_x>>3);
		if (LT_current_y < LT_last_y){
			LT_scroll_top = 1;
			if (LT_current_y>-4){
				LT_TGA_MapPage(0);
				draw_map_row_tga(LT_current_x,TGA_SCR_Y,LT_map_offset_y,40);
				LT_TGA_MapPage(1);
				draw_map_row_tga(LT_current_x,TGA_SCR_Y,LT_map_offset_y,40);
			}
		} else { 
			LT_scroll_top = -1;
			LT_TGA_MapPage(0);
			draw_map_row_tga(LT_current_x,TGA_SCR_Y+192,LT_map_offset_y+(512*24),40);
			LT_TGA_MapPage(1);
			draw_map_row_tga(LT_current_x,TGA_SCR_Y+192,LT_map_offset_y+(512*24),40);
		}
		LT_last_y = LT_current_y;
		LT_Play_Music();
		LT_Update_Scroll_TGA();
	}
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


//Fake explosions
int LT_shake[] = {-4,2,0,-1,2,-6,0,4,-3,0,4,-2,0,-1,2,5,-3,4,-4,2,-2,0};
void LT_Screen_Shake(byte mode){
	word px = SCR_X;//Get position
	word py = SCR_Y;//Get position
	byte p = 0;
	if (mode)while (p !=22){SCR_X = px+LT_shake[p];LT_Update(0,0);p++;}
	else while (p !=22){SCR_Y = py+LT_shake[p];LT_Update(0,0);p++;}
}


//PALETTES
void set_palette(unsigned char *palette){
	asm mov	dx,0x03C8; asm mov al,0; asm out dx,al;
	asm lds si,palette
	asm mov dx,0x03C9 //Palete register
	asm mov cx,256*3
	_in_loop:
		asm LODSB //Load byte from DS:SI into AL, then advance SI
		asm out dx,al
		asm loop _in_loop
}

//Silly easter egg, very difficult to hide in an open source program
//but I tried
void LT_Easter_Egg(){
	if ((LT_VIDEO_MODE == 0) && (LT_DETECTED_CARD == 1)){
		byte *k = LT_Keys;
	
		LT_var = 1;
		asm db 0xBA, 0xC8, 0x03, 0xB0, 0x00, 0xEE, 0xBA, 0xC9, 0x03, 0xB0, 0x00, 0xEE
		asm db 0xEE, 0xEE, 0xB0, 0x12, 0xEE, 0xB0, 0x0E, 0xEE, 0xB0, 0x2A, 0xEE, 0xB0
		asm db 0x1C, 0xEE, 0xB0, 0x2C, 0xEE, 0xB0, 0x12, 0xEE, 0xB0, 0x21, 0xEE, 0xB0
		asm db 0x1E, 0xEE, 0xB0, 0x37, 0xEE, 0xB0, 0x24, 0xEE, 0xB0, 0x12, 0xEE, 0xB0
		asm db 0x10, 0xEE, 0xB0, 0x19, 0xEE, 0xB0, 0x14, 0xEE, 0xB0, 0x00, 0xEE, 0xB0
		asm db 0x26, 0xEE, 0xB0, 0x1A, 0xEE, 0xB0, 0x0B, 0xEE, 0xB0, 0x2C, 0xEE, 0xB0
		asm db 0x2C, 0xEE, 0xB0, 0x2C, 0xEE, 0xB0, 0x00, 0xB9, 0x18, 0x00, 0xEE, 0xE2
		asm db 0xFD, 0xB0, 0x18, 0xEE, 0xEE, 0xEE, 0xB0, 0x21, 0xEE, 0xB0, 0x1E, 0xEE
		asm db 0xB0, 0x37, 0xEE, 0xB0, 0x2C, 0xEE, 0xB0, 0x3B, 0xEE, 0xB0, 0x24, 0xEE
		asm db 0xB0, 0x21, 0xEE, 0xB0, 0x31, 0xEE, 0xB0, 0x33, 0xEE, 0xB0, 0x30, 0xEE
		asm db 0xB0, 0x20, 0xEE, 0xB0, 0x1E, 0xEE, 0xB0, 0x24, 0xEE, 0xB0, 0x14, 0xEE
		asm db 0xB0, 0x2D, 0xEE, 0xB0, 0x35, 0xEE, 0xB0, 0x37, 0xEE, 0xB0, 0x1F, 0xEE
		asm db 0xB0, 0x3F, 0xEE
	}
	return;
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
	LT_memset(LT_Temp_palette,256*3,0x00);//All colours black
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

//79.692

void LT_Fade_in_EGA(){
	byte i = 0;
	byte j = 0;
	byte col= 0;
	word fade = 16*5;
	if (LT_var)return;
	if(LT_DETECTED_CARD == 1)set_palette(LT_tileset_palette);
	if (LT_EGA_FADE_STATE == 1) return;
	while (i < 6){
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			if (col > 7) col+=8;
			asm mov dx,0x03DA;asm in al,dx;
			asm mov dx,0x03C0;
			asm mov al,j; asm out dx,al;  //Index (0-15)
			asm mov al,col; asm out dx,al;//Color (0-23)
			asm mov al,0x20; asm out dx,al;
		}
		fade-=16;
		i++;
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 1;
	//LT_64COL_EGA();
}

void LT_Fade_out_EGA (){
	byte x = 0;
	byte j = 0;
	byte col= 0;
	word fade = 0;
	if (LT_var)return;
	if (LT_EGA_FADE_STATE == 0) return;
	for (x= 0; x<5;x++){
		fade+=16;
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			if (col > 7) col+=8;
			asm mov dx,0x03DA;asm in al,dx;
			asm mov dx,0x03C0;
			asm mov al,j; asm out dx,al;  //Index (0-15)
			asm mov al,col; asm out dx,al;//Color (0-23)
			asm mov al,0x20; asm out dx,al;
		}
		
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 0;
}

void LT_Cycle_Palette_TGA_EGA(byte *p,byte *c,byte speed){
	int x;
	byte col;
	byte index;
	if (LT_VIDEO_MODE == 0){
		for (x= 0; x<4;x++){
			col = c[(LT_Cycle_palframe+x)&3];
			index = p[x];
			if (col > 7) col+=8;
			asm mov dx,0x03DA;asm in al,dx;
			asm mov dx,0x03C0;
			asm mov al,index; asm out dx,al;  //Index (0-15)
			asm mov al,col; asm out dx,al;//Color (0-23)
			asm mov al,0x20; asm out dx,al;
		}
	}
	if (LT_VIDEO_MODE == 3){
		for (x = 0; x < 4; x++){
			col = c[(LT_Cycle_palframe+x)&3];
			index = 0x10+p[x];
			asm mov dx,0x03DA;
			asm mov al,index; asm out dx,al;
			asm mov dx,0x03DE;asm in al,dx;
			asm mov al,col; asm out dx,al;
		}
	}
	if (LT_Cycle_palcounter == speed){
		LT_Cycle_palcounter = 0;
		LT_Cycle_palframe++;
		if (LT_Cycle_palframe == 4) LT_Cycle_palframe = 0;
	}
	LT_Cycle_palcounter++;
}

void LT_Fade_in_TGA(){
	byte i = 0;
	byte j = 0;
	byte col= 0;
	word fade = 16*5;
	if (LT_EGA_FADE_STATE == 1) return;
	while (i < 6){
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			asm mov dx,0x03DA;asm mov al,j;
			asm add al,0x10;asm out dx,al;//Index (0-15)
			asm mov dx,0x03DE;asm mov al,col;
			asm out dx,al;//Color (0-15)
		}
		fade-=16;
		i++;
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 1;
}

void LT_Fade_out_TGA(){
	byte x = 0;
	byte j = 0;
	byte col= 0;
	word fade = 0;
	if (LT_EGA_FADE_STATE == 0) return;
	for (x= 0; x<5;x++){
		fade+=16;
		for (j= 0; j<16;j++){
			col = LT_EGA_FADE[j+fade];
			asm mov dx,0x03DA;asm mov al,j;
			asm add al,0x10;asm out dx,al;//Index (0-15)
			asm mov dx,0x03DE;asm mov al,col;
			asm out dx,al;//Color (0-15)
		}
		
		LT_vsync();LT_vsync();
	}
	LT_EGA_FADE_STATE = 0;
}
