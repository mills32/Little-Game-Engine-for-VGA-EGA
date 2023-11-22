#include "lt__eng.h"

//debugging
float t1 = 0;
float t2 = 0;

//timer     		
long time_ctr;

// this points to the 18.2hz system clock for debug in dosbox (does not work on PCEM). 
word *my_clock=(word *)0x0000046C; 
   		
word start;
int Free_RAM;
byte LT_VIDEO_MODE = 0;
byte LT_DETECTED_CARD = 0;
byte LT_MUSIC_MODE = 1;//1 ADLIB; 0 TANDY
byte LT_SFX_MODE = 1;
byte LT_LANGUAGE = 0;


byte LT_ENDLESS_SIDESCROLL = 0;
byte LT_IMAGE_MODE = 0;	//0 map; 1 image
byte LT_SPRITE_MODE = 0; //0 fast, no delete; 1 slow, delete

extern byte *VGA;
extern word TILE_VRAM;
extern word FONT_VRAM;
FILE *LT_setupfile;
FILE *LT_mapsetupfile;
int LT_Load_Logo = 0;
// define LT_old_..._handler to be a pointer to a function
void interrupt (*LT_old_time_handler)(void); 	
void interrupt (*LT_old_key_handler)(void);
void LT_vsync();

void LT_Fade_in_VGA();
void LT_Fade_in_EGA();
void LT_Fade_in_TGA();
void LT_Fade_out_VGA();
void LT_Fade_out_EGA();
void LT_Fade_out_TGA();
void LT_Load_Tiles_EGA_VGA();
void LT_WaitVsync_VGA();
void LT_WaitVsync_TGA();
void LT_Scroll_Map_GFX();
void LT_Scroll_Map_TGA();
extern void (*LT_Draw_Sprites)(void);
extern void (*LT_Draw_Sprites_Fast)(void);
void LT_Draw_Sprites_EGA_VGA();
void LT_Draw_Sprites_Fast_EGA_VGA();
void LT_Draw_Sprites_Tandy();
void LT_Draw_Sprites_Fast_Tandy();
extern void (*LT_Load_Music)(char*,char*);
extern void (*LT_Play_Music)(void);
void Load_Music_Adlib(char *fname, char *dat_string);
void Play_Music_Adlib();
void Load_Music_Tandy(char *fname, char *dat_string);
void Play_Music_Tandy();

//Physical width in bytes of screen
extern word LT_VRAM_Logical_Width;	
extern byte sprite_size;
extern LT_sprite_data_offset;
extern unsigned char *LT_sprite_data; 
extern unsigned char *tandy_bkg_data; 
extern word *sprite_id_table;
//Variable functions
void LT_Print_VGA(word x, word y, word w, char *string);
void LT_Print_EGA(word x, word y, word w, char *string);
void LT_Edit_MapTile_VGA(word x, word y, byte ntile, byte col);
void LT_Edit_MapTile_EGA(word x, word y, byte ntile, byte col);
extern void (*LT_Print)(word,word,word,char*);
void LT_Print_Variable_VGA(byte x, byte y, word var);
extern void (*LT_Print_Variable)(byte,byte,word);

void draw_map_column_vga(word x, word y, word map_offset, word ntiles);
void draw_map_column_ega(word x, word y, word map_offset, word ntiles);
void draw_map_column_tga(word x, word y, word map_offset, word ntiles);
extern void (*draw_map_column)(word, word, word, word);

//70614
void write_crtc(unsigned int port, unsigned char reg, unsigned char val){
	outportb(port, reg);
	outportb(port+1,val);
}

void LT_Text_Mode(){
	union REGS regs;
	if (LT_DETECTED_CARD == 3){
		outportb(0x3D4, 0x0A);
		outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 14);
		outportb(0x3D4, 0x0B);
		outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 15);
	}
	if (LT_VIDEO_MODE == 4){
		LT_WaitVsync_TGA();//If you don't do this, there is weird behaviour on dos
		asm mov dx,03DFh
		asm mov al,0x00
		asm out dx,al
	}
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
}

void LT_Error(char *error, char *file){
	asm STI; //enable interrupts
	LT_Text_Mode();
	printf("%s %s \n",error,file);
	sleep(4);
	LT_ExitDOS();
}

//Seek files inside LDAT files (custom format similar to WAD)

void DAT_Seek(FILE *fp,char *dat_string){
	unsigned char line = 0;
	char name[17]; //name of the file inside DAT
	unsigned char data_name = 0;
	dword LDAT_Offset = 0;
	
	//Check LDAT file
	//Seek data
	fseek(fp,32,SEEK_SET);
	while(data_name == 0){//read 16 byte lines 
		memset(name, 0, 16);
		fgets(name, 17, fp);
		if (!stricmp(name,dat_string)) data_name = 1;
		else fseek(fp,16,SEEK_CUR);
		line++;
		if (line == 64) {fclose(fp); LT_Error("Can't find inside DAT file: ",dat_string);}
	}
	fread(&LDAT_Offset,sizeof(LDAT_Offset),1,fp);//read offset of file in DAT file
	fseek(fp,LDAT_Offset,SEEK_SET);
}

void Read_Setup(){
	byte buffer[256];
	fread(buffer,1,256,LT_setupfile);
	LT_VIDEO_MODE = buffer[0x16]-48;
	LT_SFX_MODE = buffer[0x30]-48;
	LT_LANGUAGE = buffer[0x55]-48;
}

void Save_Setup(){
	fseek(LT_setupfile,0x16,SEEK_SET);
	fputc((LT_VIDEO_MODE&3)+48,LT_setupfile);
	fseek(LT_setupfile,0x30,SEEK_SET);
	fputc(LT_SFX_MODE+48,LT_setupfile);
	fseek(LT_setupfile,0x55,SEEK_SET);
	fputc(LT_LANGUAGE+48,LT_setupfile);
}

void Check_Graphics_Card(){
	union REGS regs;
	regs.h.ah=0x1A;
	regs.h.al=0x00;
	regs.h.bl=0x32;
	int86(0x10, &regs, &regs);
	if (regs.h.al == 0x1A) {printf("\nCard detected: VGA\n"); LT_DETECTED_CARD = 1;LT_VIDEO_MODE = 1;}
	else {
		regs.h.ah=0x12;
		regs.h.bh=0x10;
		regs.h.bl=0x10;
		int86(0x10, &regs, &regs);
		if (regs.h.bh == 0) {
			printf("\nCard detected: EGA");
			LT_DETECTED_CARD = 0;
		} else {
			regs.h.ah=0x0F;
			regs.h.bl=0x00;
			int86(0x10, &regs, &regs);
			if (regs.h.al == 0x07) {
				printf("You need at least a CGA card to run this\n");
				exit(1);
			} else {printf("\nCard detected: CGA or TANDY\n");LT_DETECTED_CARD = 2;}
		}
	}
	
	sleep(1);
	Clearkb();
}

void LT_Setup(){
	byte character = 0;
	system("cls");
	Check_Graphics_Card();
	LT_setupfile = fopen("config.txt","rb+");
	if (!LT_setupfile) {
		printf("config.txt is missing\nNew default file created");
		LT_setupfile = fopen("config.txt","w");
		fprintf(LT_setupfile,"#SETUP\n------\nVIDEO=0\nSCROLL=0\nMUSIC=1\nSFX=1\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		printf("#SETUP\n------\nVIDEO=0\nSCROLL=0\nMUSIC=1\nSFX=1\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		fclose(LT_setupfile);
		sleep(2);
		LT_setupfile = fopen("config.txt","rb+");
	}
	
	Read_Setup();
	if (LT_DETECTED_CARD == 1){
		printf("Select video card / Selecciona tarjeta de video\n\n0 = EGA, 16 col\n1 = VGA, 256 col");
		while (!kbhit());
		character = getch() - 48;
		if ( character > 1 )character = 1;
		LT_VIDEO_MODE = character;
		Clearkb();
	}
	if (LT_DETECTED_CARD == 0) LT_VIDEO_MODE = 0; //EGA
	if (LT_DETECTED_CARD == 2) {
		printf("Select video card / Selecciona tarjeta de video\n\n3 = CGA, 4 col\n4 = TANDY, 16 col");
		while (!kbhit());
		character = getch() - 48;
		if ( character != 4 ) character = 4;
		LT_VIDEO_MODE = character;
		Clearkb();
	}
	system("cls");
	printf("Select sound card / Selecciona tarjeta de sonido\n\n0 = TANDY 3 voice\n1 = ADLIB");
	while (!kbhit());
	character = getch() - 48;
	if ( character > 1 ) character = 1;
	LT_MUSIC_MODE = character;
	Clearkb();
	system("cls");
	printf("Select sound effect mode / Selecciona modo de efectos de sonido\n\n0 = PC Speaker\n1 = ADLIB");
	while (!kbhit());
	character = getch() - 48;
	if ( character > 1 ) character = 1;
	LT_SFX_MODE = character;
	Clearkb();
	system("cls");
	//SAVE SETTINGS
	Save_Setup();

	fclose(LT_setupfile);
	
	LT_Adlib_Detect();
    LT_init();
}

void LT_Null(){//Debug
	int i = 0;
	i = i+1;
}

void LT_Init(){
	unsigned char *temp_buf;
	long linear_address;
	short page1, page2;
	int square_pixels = 1;
	union REGS regs;
	word i;
	word *sprite_bkg;
	byte sprite_number;
	//FIXED SPRITE BKG ADDRESSES IN VRAM
	word spr_VGA[20] = {//D500	
		//8x8
		0xD500,0xD518,0xD530,0xD548,0xD560,0xD578,0xD590,0xD5A8,
		//16x16
		0xD5C0,0xD610,0xD660,0xD6B0,0xD700,0xD750,0xD7A0,0xD7F0,
		//32x32
		0xD840,0xD960,0xDA80,0xDBA0,
	};
	word spr_EGA[20] = {//A680	
		/*//8x8
		0x6A80,0x6A90,0x6AA0,0x6AB0,0x6AC0,0x6AD0, 0x6AE0,0x6AF0, //32-24 = 8
		//16x16
		0x6B00,0x6B30,0x6B60,0x6B90,0x6BC0,0x6BF0, 0x6C20,0x6C50, //96-36 = 60
		//32x32
		0x6C80,0x6D20,0x6DC0,0x6E60,*/
		//8x8
		0x6A80,0x6A90,0x6AA0,0x6AB0,0x6AC0,0x6AD0, 0x6AE0,0x6AF0, //32-24 = 8
		//16x16
		0x6B00,0x6B30+6,0x6B60+12,0x6B90+20,0x6BC0,0x6BF0, 0x6C20,0x6C50, //96-36 = 60
		//32x32
		0x6C80,0x6D20,0x6DC0,0x6E60,
	};
	
	if (LT_VIDEO_MODE == 0){
		LT_WaitVsync = &LT_WaitVsync_VGA;
		LT_Fade_in = &LT_Fade_in_EGA;
		LT_Fade_out = &LT_Fade_out_EGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_EGA_VGA;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_EGA_VGA;
		
		LT_sprite_data_offset = 512;
		
		LT_Fade_out();
		//SET MODE
		regs.h.ah = 0x00;
		regs.h.al = 0x0D;	//EGA/VGA 320x200x16 
		int86(0x10, &regs, &regs);
		memset(VGA,0x00,(320*200)/8); 

		if (LT_DETECTED_CARD){//if VGA
			//Set VGA so that we have 60Hz (320x240), but leave screen size at 320x200
			asm cli
			outport(0x3D4, 0x0011); // Turn off write protect to CRTC registers
			outport(0x3D4, 0x0B06); // New vertical total=525 lines, bits 0-7
			outport(0x3D4, 0x3E07); // New vertical total=525 lines, bits 8-9
			if (square_pixels){
				outport(0x3D4, 0xB910); // Vsync start=scanline 185
				outport(0x3D4, 0x8F12); // Vertical display end=scanline 399, bits 0-7
				outport(0x3D4, 0xB815); // Vertical blanking start=scanline 440, bits 0-7
				outport(0x3D4, 0xE216); // Adjust vblank end position
				outport(0x3D4, 0x8511); // Vsync length=2 lines + turn write protect back on
			} else {
				outport(0x3D4, 0x0B16); // Adjust vblank end position=scanline 524
				outport(0x3D4, 0xD710); // Vsync start=scanline 215
				outport(0x3D4, 0x8911); // Vsync length=2 lines + turn write protect back on
			}
			asm sti
		}
		word_out(0x03d4,OFFSET,22);
		LT_VRAM_Logical_Width = 44;
	
		//Read Mode: all planes
		asm mov dx,GC_INDEX
		asm mov ax,0x0509	//Read all planes	Write mode 1	
		asm out dx,ax
		asm mov dx,GC_INDEX + 1
		asm mov ax,0x00ff	
		asm out dx,ax
		
		LT_Print = &LT_Print_EGA;
		LT_Print_Variable = &LT_Null;
		draw_map_column = &draw_map_column_ega;
		LT_Edit_MapTile = &LT_Edit_MapTile_EGA;
		LT_Scroll_Map = &LT_Scroll_Map_GFX;
		sprite_bkg = &spr_EGA[0];
		
		FONT_VRAM = 0x6F00;
		TILE_VRAM = 0x7100;
		sprite_size = 3;
	}
	
	if (LT_VIDEO_MODE == 1){//VGA 320 x 200 x 256 MODE X
		LT_WaitVsync = &LT_WaitVsync_VGA;
		LT_Fade_in = &LT_Fade_in_VGA;
		LT_Fade_out = &LT_Fade_out_VGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_EGA_VGA;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_EGA_VGA;
		LT_sprite_data_offset = 16*1024;
		LT_Fade_out();
		VGA_ClearPalette();
		memset(VGA,0x00,(320*200)/4); 
		
		//SET MODE X
		regs.h.ah = 0x00;
		regs.h.al = 0x13;
		int86(0x10, &regs, &regs);
		
		outp(SC_INDEX,  MEMORY_MODE);       // turn off chain-4 mode
		outp(SC_DATA,   0x06);
		outport(SC_INDEX, ALL_PLANES);      // set map mask to all 4 planes
		outp(CRTC_INDEX,UNDERLINE_LOCATION);// turn off long mode
		outp(CRTC_DATA, 0x00);
		outp(CRTC_INDEX,MODE_CONTROL);      // turn on byte mode
		outp(CRTC_DATA, 0xe3);
	
		//Set VGA so that we have 60Hz (320x240), but leave screen size at 320x200
		asm cli
		outport(0x3D4, 0x0011); // Turn off write protect to CRTC registers
		outport(0x3D4, 0x0B06); // New vertical total=525 lines, bits 0-7
		outport(0x3D4, 0x3E07); // New vertical total=525 lines, bits 8-9
		if (square_pixels){
			outport(0x3D4, 0xB910); // Vsync start=scanline 185
			outport(0x3D4, 0x8F12); // Vertical display end=scanline 399, bits 0-7
			outport(0x3D4, 0xB815); // Vertical blanking start=scanline 440, bits 0-7
			outport(0x3D4, 0xE216); // Adjust vblank end position
			outport(0x3D4, 0x8511); // Vsync length=2 lines + turn write protect back on
		} else {
			outport(0x3D4, 0x0B16); // Adjust vblank end position=scanline 524
			outport(0x3D4, 0xD710); // Vsync start=scanline 215
			outport(0x3D4, 0x8911); // Vsync length=2 lines + turn write protect back on
		}
		asm sti

		//delete vram
		asm mov ax,0xA000
		asm mov es,ax
		asm mov di,0			//es:di destination
		asm mov al,0
		asm mov cx,65535
		asm rep STOSB 			//Store AL at address ES:DI.
	
		//LOGICAL WIDTH = 320 + 32
		LT_VRAM_Logical_Width = 88; //width in 4 pixel chunks
		word_out(0x03d4,OFFSET,44);//width in 8 pixel "characters"
		
		// set vertical retrace back to normal
		word_out(0x03d4, V_RETRACE_END, 0x8e);
		
		
		outport(0x03CE,0x0106);	// Select VRAM B8000h-BFFFFh; Chain O/E OFF; Text mode OFF 
		
		LT_Print = &LT_Print_VGA;
		LT_Print_Variable = &LT_Print_Variable_VGA;
		draw_map_column = &draw_map_column_vga;
		LT_Edit_MapTile = &LT_Edit_MapTile_VGA;
		LT_Scroll_Map = &LT_Scroll_Map_GFX;
		sprite_bkg = &spr_VGA[0];
		
		FONT_VRAM = 0xDCC0;
		TILE_VRAM = 0xE0C0;
		sprite_size = 2;
	}
	if (LT_VIDEO_MODE == 4){//Tandy
		LT_WaitVsync = &LT_WaitVsync_TGA;
		LT_Fade_in = &LT_Fade_in_TGA;
		LT_Fade_out = &LT_Fade_out_TGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_Tandy;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_Tandy;
		LT_sprite_data_offset = 2048;
		LT_Fade_out();
		
		regs.h.ah = 0x00;
		regs.h.al = 0x09;		// =>320x200 (08 = 160x200)
		int86(0x10, &regs, &regs);
		
		LT_Print = &LT_Null;
		LT_Print_Variable = &LT_Null;
		draw_map_column = &draw_map_column_tga;
		LT_Edit_MapTile = &LT_Null;
		LT_Scroll_Map = &LT_Scroll_Map_TGA;
		
		FONT_VRAM = 0x00;
		TILE_VRAM = 0x00;
		sprite_size = 2;
	}
	if (LT_MUSIC_MODE == 0){
		LT_Load_Music = &Load_Music_Tandy;
		LT_Play_Music = &Play_Music_Tandy;
	}
	if (LT_MUSIC_MODE == 1){
		LT_Load_Music = &Load_Music_Adlib;
		LT_Play_Music = &Play_Music_Adlib;
	}
	LT_old_time_handler = getvect(0x1C);
	LT_old_key_handler = getvect(9);    
	LT_install_key_handler();
	
	//Allocate:
	// 64kb Temp Data (Load Tilesets, Load Sprites)
	// 8kb Map + 8kb collision map
	// 64kb Music 
	// 64Kb for sprites
	// 16 kb for Tandy sprites BKG
	// Asume EXE file around 96KB 
	// Add 16 Kb of used defined data (palette tables...)
	// Then we need around 400 Kb of Free Ram
	//
	// If you use tandy, you need 64kb more for video ram
	
	//Allocate 64KB block for temp data
	LT_tile_tempdata = farcalloc(32768,sizeof(byte));
	//Allocate the the second 32 KB of temp data just after the first
	LT_tile_tempdata2 = farcalloc(32768,sizeof(byte));
	
	//if ((LT_map_data = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of map data",0);
	//if ((LT_map_collision = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of collision map data",0);
	if ((LT_music.sdata = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 64 Kb of music data",0);
	
	if ((LT_map_data = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate map data",0);
	if ((LT_map_collision = farcalloc(8192*2,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate collision data",0);
	
	//14 fixed sprites:
	//	8x8   ( 8 sprites: 0 - 7) 200 bytes/frame
	//	16x16 ( 8 sprites: 8 - 15) 900 bytes/frame, you can use a lot of frames in this sprites
	//	32x32 ( 4 sprites: 16 - 19) 4kb/frame, you can use 2 sprites wih 8 frames at the same time
	//	64x64 ( 1 sprite: 20) 14 kb/frame, you can only use 3 frames for this sprite.
	
	if ((sprite = farcalloc(22,sizeof(SPRITE))) == NULL) LT_Error("Not enough RAM to allocate 22 predefined sprite structs",0);
	if ((LT_sprite_data = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate sprite data",0);
	//STORE BKG AT FIXED VRAM ADDRESS
	for (sprite_number = 0; sprite_number != 20; sprite_number++){
		sprite[sprite_number].bkg_data = sprite_bkg[sprite_number];
	}
	if (LT_VIDEO_MODE == 4) if ((tandy_bkg_data = farcalloc(16*32*22,1))==NULL) LT_Error("Not enough RAM to allocate tandy sprite structs",0);
	if ((sprite_id_table = farcalloc(256*19*2,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate sprite id table",0);
}

void Draw_Roll(){
	byte page = 0;
	word roll = 200+32;
	word drawing_offset = 0;
	word bitmap_offset;
	word lwidth;
	word flip_offset;
	word page0;
	word page1;
	word skip2, skip3, skip4, skip19, skip26, skip64, skiphalf;
	word col1,col2,col3;
	word mode = LT_VIDEO_MODE;

	if (LT_VIDEO_MODE == 0){
		bitmap_offset = 40*(32+224+32+32+224+32)+(40*(200+32)); //End of image data in VRAM
		lwidth = 40;
		col1 = 0x8888;
		col2 = 0x7777;
		col3 = 0xFFFF;
		flip_offset = (224+32)*40; 
		page0 = (32*lwidth) + 0x0C;
		page1 = ((32+224+32)*lwidth) + 0x0C;
	}
	if (LT_VIDEO_MODE == 1){
		bitmap_offset = 80*(32+240+32+32+240+32)+(80*(200+32)); //End of image data in VRAM
		lwidth = 80;											//Actually it wrapped around 
		col1 = 0x0101;											//and it is at the start of VRAM
		col2 = 0x0202;
		col3 = 0x0303;
		flip_offset = (240+32)*80; 
		page0 = (32*lwidth) + 0x0C;
		page1 = (304*lwidth) + 0x0C;
	}
	
	//skip lines
	skip2 = lwidth*2;
	skip3 = lwidth*3;
	skip4 = lwidth*4;
	skip19 = lwidth*19;
	skip26 = lwidth*26;
	skip64 = lwidth*64;
	skiphalf = lwidth/2;
	
	// Write all planes
	asm mov dx, 0x03C4
	asm mov ax, 0x0F02
	asm out dx, ax

	while (roll){
		//asm sti
		
		//wait for the vertical retrace end
		asm mov		dx,003dah
		WaitVsync:
		asm in      al,dx
		asm test    al,08h
		asm jz		WaitVsync
		WaitNotVsync:
		asm in      al,dx
		asm test    al,08h
		asm jnz		WaitNotVsync
		
		if (page&1){//VGA screen HIGH_ADDRESS
			asm mov dx,0x03D4 	//VGA PORT
			asm mov ax,page0
			asm out dx,ax
			drawing_offset-=flip_offset;
		} else {
			asm mov dx,0x03D4
			asm mov ax,page1
			asm out dx,ax
			drawing_offset+=flip_offset;
		}
		
		asm push ds
		asm push di
		asm push si
		
		//Setup transfers
		asm mov ax,0xA000
		asm mov ds,ax
		asm mov si,bitmap_offset			//ds:si source
		asm mov es,ax
		asm mov di,drawing_offset			//es:di destination
		
		
		//ROLLING BAR/////////////
		//color lines = 2-Nor 1-34 1-58 1-91 2-Nor 1-4 1-Nor 2-4 1-Nor 1-4 12-Nor 1-58 1-Nor 1-58 1-92 2-34 1-23 
		// VRAM to VRAM transfer read mode
		if (mode == 0){
			asm mov dx,0x03CE
			asm mov ax,0x0105	//Write mode 1		
			asm out dx,ax 
		}
		if (mode == 1){
			asm mov dx, 0x03CE
			asm mov ax, 0x0008
			asm out dx, ax
		}

		asm mov bx,lwidth
		
		//Draw 2 lines of normal image at the top of the roller
		asm mov cx,bx
		asm rep movsb
		asm sub	si,skip2
		asm mov cx,bx
		asm rep movsb
		asm sub	si,skip64	//go back some bytes to draw the rolling data	
		asm add di,skip2
		
		asm add si,skip4
		asm mov cx,bx
		asm rep movsb
		asm add si,skip3
		asm mov cx,bx
		asm rep movsb
		asm add si,skip2
		asm mov cx,bx
		asm rep movsb
		asm add si,bx
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		
		asm sub si,bx
		asm mov cx,bx
		asm rep movsb
		
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		
		asm mov cx,bx
		asm rep movsb
		asm mov cx,bx
		asm rep movsb
		asm add si,bx
		asm mov cx,bx
		asm rep movsb
		asm add si,skip2
		asm mov cx,bx
		asm rep movsb
		asm add si,skip3
		asm mov cx,bx
		asm rep movsb
		asm add si,skip4
		asm mov cx,bx
		asm rep movsb
		
		asm sub di,skip26 // go back to upper part of bar
		
		//Write pixels (flat colour lines, colours 0 1 2 3)
		asm mov dx, 0x03CE
		if (mode == 0){
			asm mov ax, 0x0205	//write mode 2
			asm out dx, ax
		}
		asm mov ax, 0xFF08
		asm out dx, ax

		//Draw upper shadow
		asm mov cx,skiphalf
		asm mov ax,col1
		asm rep stosw
		asm mov cx,skiphalf
		asm mov ax,col2
		asm rep stosw
		asm add di,skip2

		//Draw bright part
		asm mov cx,bx
		asm mov ax,col3
		asm rep stosw
		asm add di,skip19 //Skip lines

		//Draw bottom shadow
		asm mov cx,skiphalf
		asm mov ax,col1
		asm rep stosw
		asm mov cx,skiphalf
		asm mov ax,0x0000
		asm rep stosw
			
		//	asm pop cx
		//	asm loop copyloop
	
		asm pop ds
		asm pop di
		asm pop si
		
		page++;
		drawing_offset+=lwidth;
		bitmap_offset-=lwidth;
		roll--;
		
		//asm cli
	}
}

extern int LT_PC_Speaker_Size;

void LT_Logo(char *file, char *dat_string){
	byte Game_Boy_Adlib[11] = {0xC4,0x1A,0x06,0x8E,0xA6,0xE8,0xF0,0xA0,0x00,0x00,0x02};
	int GameBoy_Sound[24] = {72,72,72,72,72,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84};
	int logotimer = 0;
	LT_Load_Logo = 1;
	LT_PC_Speaker_Size = 24;
	if (LT_VIDEO_MODE != 2)LT_Load_Image(file,dat_string);
	if (LT_VIDEO_MODE == 0){
		word_out(0x03d4,OFFSET,20);
		LT_Fade_in();
		Draw_Roll();
		if (!LT_SFX_MODE) LT_Play_PC_Speaker_SFX(GameBoy_Sound);
		if (LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,3,7);//first note
		while (logotimer < 120){
			if ((logotimer == 8) && LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,4,7);//second note
			logotimer++;
			//wait for the vertical retrace end
			asm mov		dx,003dah
			WaitVsync0:
			asm in      al,dx
			asm test    al,08h
			asm jz		WaitVsync0
			WaitNotVsync0:
			asm in      al,dx
			asm test    al,08h
			asm jnz		WaitNotVsync0
		}
		LT_Fade_out();
		word_out(0x03d4,OFFSET,22);
	}
	if (LT_VIDEO_MODE == 1){
		word_out(0x03d4,OFFSET,40);
		LT_Fade_in();
		Draw_Roll();
		if (!LT_SFX_MODE) LT_Play_PC_Speaker_SFX(GameBoy_Sound);
		if (LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,3,7);//first note
		while (logotimer < 120){
			if ((logotimer == 8) && LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,4,7);//second note
			logotimer++;
			//wait for the vertical retrace end
			asm mov		dx,003dah
			WaitVsync:
			asm in      al,dx
			asm test    al,08h
			asm jz		WaitVsync
			WaitNotVsync:
			asm in      al,dx
			asm test    al,08h
			asm jnz		WaitNotVsync
		}
		LT_Fade_out();
		word_out(0x03d4,OFFSET,44);
	}
	if (LT_VIDEO_MODE == 4){
		LT_Fade_in();
		if (!LT_SFX_MODE) LT_Play_PC_Speaker_SFX(GameBoy_Sound);
		if (LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,3,7);//first note

		while (logotimer < 120){
			if ((logotimer == 8) && LT_SFX_MODE) LT_Play_AdLib_SFX(Game_Boy_Adlib,0,4,7);//second note
			logotimer++;
			//wait for the vertical retrace end
			//Wait Vsync
			asm mov		dx,03DAh
			WaitNotVsync2:
			asm in      al,dx
			asm test    al,08h
			asm jnz		WaitNotVsync2
			WaitVsync2:
			asm in      al,dx
			asm test    al,08h
			asm jz		WaitVsync2
		}
		LT_Fade_out();
	}
	
	LT_Load_Logo = 0;
	LT_PC_Speaker_Size = 16;
	Clearkb();
}

void LT_ExitDOS(){
	LT_Text_Mode();
	LT_Stop_Music();
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte
	setvect(0x1C, LT_old_time_handler);
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_reset_key_handler();
	if (sprite) {farfree(sprite); sprite = NULL;} 
	if (sprite_id_table) {farfree(sprite_id_table); sprite_id_table = NULL;}
	exit(1);
}

//Pre calculated SIN and COS, Divide to use for smaller circles.
int LT_SIN[365] = {
0, 0, 1, 1, 3, 5, 6, 8, 10, 12, 13,
15, 17, 19, 20, 22, 24, 25, 27, 29, 30,
32, 34, 35, 37, 39, 40, 42, 43, 45, 46,
48, 50, 51, 52, 54, 55, 57, 58, 60, 61,
62, 64, 65, 66, 68, 69, 70, 71, 73, 74,
75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
85, 86, 87, 88, 89, 89, 90, 91, 92, 92,
93, 93, 94, 95, 95, 96, 96, 97, 97, 97,
98, 98, 98, 99, 99, 99, 99, 99, 99, 99,
99, 100, 99, 99, 99, 99, 99, 99, 99, 99,
98, 98, 98, 97, 97, 97, 96, 96, 95, 95,
94, 93, 93, 92, 92, 91, 90, 89, 89, 88,
87, 86, 85, 84, 83, 82, 81, 80, 79, 78,
77, 76, 75, 74, 73, 71, 70, 69, 68, 66,
65, 64, 62, 61, 60, 58, 57, 55, 54, 52,
51, 50, 48, 46, 45, 43, 42, 40, 39, 37,
35, 34, 32, 30, 29, 27, 25, 24, 22, 20,
19, 17, 15, 13, 12, 10, 8, 6, 5, 3,
1, 0, -2, -4, -6, -7, -9, -11, -13, -14,
-16, -18, -20, -21, -23, -25, -26, -28, -30, -31,
-33, -35, -36, -38, -40, -41, -43, -44, -46, -47,
-49, -50, -52, -53, -55, -56, -58, -59, -61, -62,
-63, -65, -66, -67, -69, -70, -71, -72, -74, -75,
-76, -77, -78, -79, -80, -81, -82, -83, -84, -85,
-86, -87, -88, -89, -90, -90, -91, -92, -93, -93,
-94, -94, -95, -96, -96, -97, -97, -98, -98, -98,
-99, -99, -99, -100, -100, -100, -100, -100, -100, -100,
-100, -100, -100, -100, -100, -100, -100, -100, -100, -100,
-99, -99, -99, -98, -98, -98, -97, -97, -96, -96,
-95, -94, -94, -93, -93, -92, -91, -90, -90, -89,
-88, -87, -86, -85, -84, -83, -82, -81, -80, -79,
-78, -77, -76, -75, -74, -72, -71, -70, -69, -67,
-66, -65, -63, -62, -61, -59, -58, -56, -55, -53,
-52, -51, -49, -47, -46, -44, -43, -41, -40, -38,
-36, -35, -33, -31, -30, -28, -26, -25, -23, -21,
-20, -18, -16, -14, -13, -11, -9, -7, -6, -4, -2,
-1, 0, 0
};

int LT_COS[365] = {
99, 100, 99, 99, 99, 99, 99, 99, 99, 99,
98, 98, 98, 97, 97, 97, 96, 96, 95, 95,
94, 93, 93, 92, 92, 91, 90, 89, 89, 88,
87, 86, 85, 84, 83, 82, 81, 80, 79, 78,
77, 76, 75, 74, 73, 71, 70, 69, 68, 66,
65, 64, 62, 61, 60, 58, 57, 55, 54, 52,
51, 50, 48, 46, 45, 43, 42, 40, 39, 37,
35, 34, 32, 30, 29, 27, 25, 24, 22, 20,
19, 17, 15, 13, 12, 10, 8, 6, 5, 3,
1, 0, -2, -4, -6, -7, -9, -11, -13, -14,
-16, -18, -20, -21, -23, -25, -26, -28, -30, -31,
-33, -35, -36, -38, -40, -41, -43, -44, -46, -47,
-49, -50, -52, -53, -55, -56, -58, -59, -61, -62,
-63, -65, -66, -67, -69, -70, -71, -72, -74, -75,
-76, -77, -78, -79, -80, -81, -82, -83, -84, -85,
-86, -87, -88, -89, -90, -90, -91, -92, -93, -93,
-94, -94, -95, -96, -96, -97, -97, -98, -98, -98,
-99, -99, -99, -100, -100, -100, -100, -100, -100, -100,
-100, -100, -100, -100, -100, -100, -100, -100, -100, -100,
-99, -99, -99, -98, -98, -98, -97, -97, -96, -96,
-95, -94, -94, -93, -93, -92, -91, -90, -90, -89,
-88, -87, -86, -85, -84, -83, -82, -81, -80, -79,
-78, -77, -76, -75, -74, -72, -71, -70, -69, -67,
-66, -65, -63, -62, -61, -59, -58, -56, -55, -53,
-52, -51, -49, -47, -46, -44, -43, -41, -40, -38,
-36, -35, -33, -31, -30, -28, -26, -25, -23, -21,
-20, -18, -16, -14, -13, -11, -9, -7, -6, -4, -2,
-1, 0, 0 ,0,
 0, 1, 1, 3, 5, 6, 8, 10, 12, 13,
15, 17, 19, 20, 22, 24, 25, 27, 29, 30,
32, 34, 35, 37, 39, 40, 42, 43, 45, 46,
48, 50, 51, 52, 54, 55, 57, 58, 60, 61,
62, 64, 65, 66, 68, 69, 70, 71, 73, 74,
75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
85, 86, 87, 88, 89, 89, 90, 91, 92, 92,
93, 93, 94, 95, 95, 96, 96, 97, 97, 97,
98, 98, 98, 99, 99, 99, 99, 99, 99, 99,
};