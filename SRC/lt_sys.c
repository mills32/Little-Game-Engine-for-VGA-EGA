
#include "lt__eng.h"


const unsigned char LT_logo_palette[] = {//generated with gimp, then divided by 4 (max = 64).
	//2 copies of 8 colour cycle
	0x35,0x3b,0x3f,//
	0x26,0x39,0x3f,
	0x18,0x36,0x3f,
	0x09,0x32,0x3e,
	0x0f,0x34,0x3f,
	0x17,0x36,0x3f,
	0x1c,0x3a,0x3e,
	0x24,0x3e,0x3d,
	0x35,0x3b,0x3f,//
	0x26,0x39,0x3f,
	0x18,0x36,0x3f,
	0x09,0x32,0x3e,
	0x0f,0x34,0x3f,
	0x17,0x36,0x3f,
	0x1c,0x3a,0x3e,
	0x24,0x3e,0x3d,
	0x35,0x3b,0x3f
};

extern byte LT_Setup_Map_Char[];
extern byte LT_Setup_Map_Col[];


//debugging
float t1 = 0;
float t2 = 0;

//timer     		
long time_ctr;

// this points to the 18.2hz system clock for debug in dosbox (does not work on PCEM). 
word *my_clock=(word *)0x0000046C; 
byte *XGA_TEXT_MAP=(byte *)0xB8000000L;
   		
word start;
int Free_RAM;
byte LT_VIDEO_MODE = 0;
byte LT_SCROLL_MODE = 0;
byte LT_MUSIC_MODE = 1;
byte LT_SFX_MODE = 1;
byte LT_BLASTER_PORT = 220;
byte LT_BLASTER_IRQ = 5;
byte LT_BLASTER_DMA = 1;
byte LT_LANGUAGE = 0;
byte LT_DETECTED_CARD = 0;

byte LT_ENDLESS_SIDESCROLL = 0;
byte LT_IMAGE_MODE = 0;	//0 map; 1 image
byte LT_SPRITE_MODE = 0; //0 fast, no delete; 1 slow, delete

extern byte *VGA;
extern byte *EGA;
FILE *LT_setupfile;
int LT_Load_Logo = 0;
// define LT_old_..._handler to be a pointer to a function
void interrupt (*LT_old_time_handler)(void); 	
void interrupt (*LT_old_key_handler)(void);

word ScrnPhysicalByteWidth = 0;		//Physical width in bytes of screen
word ScrnPhysicalPixelWidth = 0;	//Physical width in pixels of screen
word ScrnPhysicalHeight = 0;		//Physical Height of screen
word ScrnLogicalByteWidth = 0;		//Logical width in bytes of screen
word ScrnLogicalPixelWidth = 0;		//Logical width in pixels of screen
word ScrnLogicalHeight = 0;			//Logical Height of screen

extern unsigned char *LT_sprite_data; 

int LT_Selected_Text = RED << 4 | WHITE;
int LT_Not_Selected_Text = BLUE << 4 | WHITE;

void write_crtc(unsigned int port, unsigned char reg, unsigned char val){
	outportb(port, reg);
	outportb(port+1,val);
}

void LT_Text_Mode(){
	union REGS regs;
	LT_Fade_out();
	if (LT_DETECTED_CARD > 2){
		outportb(0x3D4, 0x0A);
		outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 14);
		outportb(0x3D4, 0x0B);
		outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 15);
		VGA_SplitScreen(0x0ffff);
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
	exit(1);
}

void print_scrollmode(int mode){
	if (mode == 0) {
		XGA_TEXT_MAP[(8*160)+54] = 48;
		textattr(0x1F);
		gotoxy(49,7); cprintf("HARDWARE SCROLL 0");
		gotoxy(49,8); cprintf("FOR VGA CARDS");
		gotoxy(49,9); cprintf("                 ");
	}
	if (mode == 1) {
		XGA_TEXT_MAP[(8*160)+54] = 49;
		textattr(0x1F);
		gotoxy(49,7); cprintf("HARDWARE SCROLL 1");
		gotoxy(49,8); cprintf(" FOR SVGA CARDS  ");
		gotoxy(49,9); cprintf("                 ");
	}
}

void print_musicmode(int music){
	unsigned char c[] = {0,0,0,0};
	int i;
	for (i = 0; i <4;i++) c[i] = 0x1F;
	c[music] = 0x4F;
	for (i = 1; i <33;i+=2){
		XGA_TEXT_MAP[(10*160)+30+i] = c[1];
		XGA_TEXT_MAP[(11*160)+30+i] = c[2];
	}
	textattr(0x1F);
	if (music == 1){
		gotoxy(49,7); cprintf("MUSIC WILL USE   ");
		gotoxy(49,8); cprintf("OPL2/YM3812 ONLY ");
		gotoxy(49,9); cprintf("                 ");
	}
	if (music == 2){
		gotoxy(49,7); cprintf("MUSIC WILL USE   ");
		gotoxy(49,8); cprintf("OPL2/YM3812 AND  ");
		gotoxy(49,9); cprintf("SOUND BLASTER PCM");
	}
}

void print_sfxmode(int sfx){
	if (sfx == 1){
		gotoxy(16,14); textattr(0x4F); cprintf("PC SPEAKER");
		gotoxy(16,15); textattr(0x1F); cprintf("SOUND BLASTER");
		gotoxy(49,7); cprintf("SIMPLE SOUND     ");
		gotoxy(49,8); cprintf("EFFECTS PLAYED BY");
		gotoxy(49,9); cprintf("INTERNAL SPEAKER ");
	}
	if (sfx == 2){
		gotoxy(16,14); textattr(0x1F); cprintf("PC SPEAKER");
		gotoxy(49,7); cprintf("8 BIT PCM SAMPLES");
		gotoxy(49,8); cprintf("PLAYED BY SOUND  ");
		gotoxy(49,9); cprintf("BLASTER CARD     ");
		gotoxy(16,15); textattr(0x4F); cprintf("SOUND BLASTER");
	}
}

void print_langmode(int lang){
	if (lang == 1){
		gotoxy(16,18); textattr(0x4F); cprintf("ENGLISH");
		gotoxy(16,19); textattr(0x1F); cprintf("SPANISH");
	}
	if (lang == 2){
		gotoxy(16,18); textattr(0x1F); cprintf("ENGLISH");
		gotoxy(16,19); textattr(0x4F); cprintf("SPANISH");
	}
}

void Read_Setup(){
	byte buffer[256];
	fread(buffer,1,256,LT_setupfile);
	LT_SCROLL_MODE = buffer[0x20]-48;
	print_scrollmode(LT_SCROLL_MODE);
	LT_MUSIC_MODE = buffer[0x29]-48;
	print_musicmode(LT_MUSIC_MODE);
	LT_SFX_MODE = buffer[0x30]-48;
	print_sfxmode(LT_SFX_MODE);
	LT_BLASTER_PORT = buffer[0x39];
	XGA_TEXT_MAP[(160*13)+100] = LT_BLASTER_PORT;
	LT_BLASTER_PORT = buffer[0x3A];
	XGA_TEXT_MAP[(160*13)+102] = LT_BLASTER_PORT;
	LT_BLASTER_PORT = buffer[0x3B];
	XGA_TEXT_MAP[(160*13)+104] = LT_BLASTER_PORT;
	LT_BLASTER_IRQ = buffer[0x44];
	XGA_TEXT_MAP[(160*13)+116] = LT_BLASTER_IRQ;
	LT_BLASTER_DMA = buffer[0x4D];
	XGA_TEXT_MAP[(160*13)+128] = LT_BLASTER_DMA;
	LT_LANGUAGE = buffer[0x55]-48;
	print_langmode(LT_LANGUAGE);
	
	textattr(0x1F);
	gotoxy(49,7); cprintf("                 ");
	gotoxy(49,8); cprintf("     WAITING     ");
	gotoxy(49,9); cprintf("                 ");
}

void Save_Setup(){
	fseek(LT_setupfile,0x20,SEEK_SET);
	fputc((LT_SCROLL_MODE&1)+48,LT_setupfile);
	fseek(LT_setupfile,0x29,SEEK_SET);
	fputc(LT_MUSIC_MODE+48,LT_setupfile);
	fseek(LT_setupfile,0x30,SEEK_SET);
	fputc(LT_SFX_MODE+48,LT_setupfile);
	fseek(LT_setupfile,0x39,SEEK_SET);
	fputc(XGA_TEXT_MAP[(160*13)+100],LT_setupfile);
	fputc(XGA_TEXT_MAP[(160*13)+102],LT_setupfile);
	fputc(XGA_TEXT_MAP[(160*13)+104],LT_setupfile);
	LT_BLASTER_PORT = ((XGA_TEXT_MAP[(160*13)+100]-48)*100)+((XGA_TEXT_MAP[(160*13)+102]-48)*10)+(XGA_TEXT_MAP[(160*13)+104]-48);
	fseek(LT_setupfile,0x44,SEEK_SET);
	fputc(XGA_TEXT_MAP[(160*13)+116],LT_setupfile);
	LT_BLASTER_IRQ = XGA_TEXT_MAP[(160*13)+116] - 48;
	fseek(LT_setupfile,0x4D,SEEK_SET);
	fputc(XGA_TEXT_MAP[(160*13)+128],LT_setupfile);
	LT_BLASTER_DMA = XGA_TEXT_MAP[(160*13)+128] - 48;
	fseek(LT_setupfile,0x55,SEEK_SET);
	fputc(LT_LANGUAGE+48,LT_setupfile);
}

void Check_Graphics_Card(){
	union REGS regs;
	regs.h.ah=0x1A;
	regs.h.al=0x00;
	regs.h.bl=0x32;
	int86(0x10, &regs, &regs);
	if (regs.h.al == 0x1A) printf("\nCard detected: VGA\n");
	else {
		regs.h.ah=0x12;
		regs.h.bh=0x10;
		regs.h.bl=0x10;
		int86(0x10, &regs, &regs);
		if (regs.h.bh == 0) {
			printf("\nCard detected: EGA\nYou need a VGA card to run this. I added EGA suppont at first, because it has   hardware scrolling. But sprites in EGA are so slow and complicated, and VRAM to VRAM transfers seem too slow to help...\nAlso if you want to make great games for EGA, use commander keen editors, and   you'll get awesome results.");
			exit(1);
		} else {
			regs.h.ah=0x0F;
			regs.h.bl=0x00;
			int86(0x10, &regs, &regs);
			if (regs.h.al == 0x07) {
				printf("\nCard detected: CGA or TANDY\nYou need a VGA card to run this\nI want to add CGA support in the future, but with non transparent sprites.");
				exit(1);
			} else {
				printf("\nCard detected: Hercules\nYou need a VGA card to run this\n");
				exit(1);
			}
		}
	}	
	sleep(1);
	Clearkb();
}

void LT_Setup(){
	int i = 0;
	int j = 0;
	int start = 0;
	int blaster_set = 0;
	byte character = 0;
	byte color = 0;
	Check_Graphics_Card();
	LT_setupfile = fopen("config.txt","rb+");
	if (!LT_setupfile) {
		system("cls");
		printf("config.txt is missing\nNew default file created");
		LT_setupfile = fopen("config.txt","w");
		fprintf(LT_setupfile,"#SETUP\n------\nVIDEO=0\nSCROLL=0\nMUSIC=1\nSFX=1\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		printf("#SETUP\n------\nVIDEO=0\nSCROLL=0\nMUSIC=1\nSFX=1\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		fclose(LT_setupfile);
		sleep(2);
		LT_setupfile = fopen("config.txt","rb+");
	}
	
	//menu 80x25 text mode
	system("cls");
	for (i = 0;i <(80*25*2);i+=2){
		//Set Character Map 
		character = LT_Setup_Map_Char[j];
		switch (character){
			case '+': character = 205;break;
			case '*': character = 188;break;
			case '#': character = 178;break;
			case '(': character = 201;break;
			case ')': character = 187;break;
			case '!': character = 186;break;
			case '?': character = 200;break;
			case '.': character = 177;break;
			case '[': character = 223;break;
		}
		XGA_TEXT_MAP[i] = character;
		//Set Color Map 
		color = LT_Setup_Map_Col[j];
		switch (color){
			case '0': color = BLACK << 4 | WHITE;break;
			case '1': color = RED << 4 | WHITE;break;
			case '2': color = BLUE << 4 | WHITE;break;
			case '3': color = CYAN << 4 | WHITE;break;
			case '4': color = MAGENTA << 4 | WHITE;break;
			case '5': color = GREEN << 4 | LIGHTGREEN;break;
			case '.': color = BLUE << 4 | LIGHTGRAY;break;
			case '#': color = BLUE << 4 | DARKGRAY;break;
		}
		XGA_TEXT_MAP[i+1] = color; 
		j++;
	}
	
	Read_Setup();
	
	while (start == 0){
		if (!blaster_set){
			if (kbhit()){
				character = getch();
				switch (character){
					case 49: LT_SCROLL_MODE++; print_scrollmode(LT_SCROLL_MODE & 1); break;
					case 50: LT_MUSIC_MODE = 1; print_musicmode(1);break;
					case 51: LT_MUSIC_MODE = 2; print_musicmode(2);break;
					case 52: LT_SFX_MODE = 1; print_sfxmode(1);break;
					case 53: LT_SFX_MODE = 2; print_sfxmode(2);break;
					case 56: //break;
						blaster_set = 1; 
						gotoxy(51,14); textattr(BLACK << 4 | YELLOW);
						cprintf("   ");
						gotoxy(59,14); cprintf(" ");
						gotoxy(65,14); cprintf(" ");
						textattr(BLUE << 4 | WHITE);
						gotoxy(49,7); cprintf("   SET BLASTER   ");
						gotoxy(49,8); cprintf(" DEFAULT  VALUES ");
						gotoxy(49,9); cprintf("P=220 IRQ=5 DMA=1");
						textattr(BLACK << 4 | YELLOW);
						gotoxy(51,14);
					break;
					case 54: LT_LANGUAGE = 1; print_langmode(LT_LANGUAGE); break;
					case 55: LT_LANGUAGE = 2; print_langmode(LT_LANGUAGE); break;
					
					case 13: start = 1; break;
					case 27: start = 2; break;
				}
				Clearkb();
			}
		} else {
			if (kbhit()) {
				character = getch();
				if ((character>47)&&(character<58)){
					cprintf("%c",character);
					if (blaster_set == 3) gotoxy(59,14);
					if (blaster_set == 4) gotoxy(65,14);
					blaster_set++;
				}
				if ((character == 27) || (blaster_set == 6)){
					XGA_TEXT_MAP[(160*13)+101] = 0x1F;
					XGA_TEXT_MAP[(160*13)+103] = 0x1F;
					XGA_TEXT_MAP[(160*13)+105] = 0x1F;
					XGA_TEXT_MAP[(160*13)+117] = 0x1F;
					XGA_TEXT_MAP[(160*13)+129] = 0x1F;
					blaster_set = 0; 
				}
				Clearkb();
			}
		}
		
		while( inp( INPUT_STATUS_0 ) & 0x08 );
		while( !(inp( INPUT_STATUS_0 ) & 0x08 ) );
	}
	if (start == 2) {
		fclose(LT_setupfile);
		system("cls");
		exit(1);
	}
	/*printf("error");
	sleep(1);
	exit(1);*/
	
	//SAVE SETTINGS
	Save_Setup();

	fclose(LT_setupfile);
	system("cls");
	
	//Check_Graphics_Card();
	
	LT_Adlib_Detect();
    LT_init();
	if ((LT_MUSIC_MODE == 2) || (LT_SFX_MODE == 2)) sb_init();//After LT_Init
	LT_Load_Font("GFX/0_fontV.bmp");
}

void LT_Null(){//Debug
	int i = 0;
	i = i+1;
}

void LT_Init(){
	unsigned char *temp_buf;
	long linear_address;
	short page1, page2;
	
	union REGS regs;
	word i;
	word *sprite_bkg;
	byte sprite_number;
	word spr_VGA[20] = {	
		//8x8
		0xD104,0xD11C,0xD134,0xD14C,0xD164,0xD17C,0xD194,0xD1AC,
		//16x16
		0xD1C4,0xD214,0xD264,0xD2B4,0xD304,0xD354,0xD3A4,0xD3F4,
		//32x32
		0xD444,0xD564,0xD684,0xD7A4,
	};
	
	//FIXED SPRITE BKG ADDRESSES IN VRAM
	{//VGA 320 x 240 x 256 MODE X
		LT_Fade_in = &LT_Fade_in_VGA;
		LT_Fade_out = &LT_Fade_out_VGA;
		
		LT_Fade_out();
		VGA_ClearPalette();
		memset(VGA,0x00,(320*240)/4); 
		
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
	
		//outp(MISC_OUTPUT, 0x00); //Memory map 64 Kb?
		
		/* turn off write protect */
		word_out(CRTC_INDEX, V_RETRACE_END, 0x2c);

		//delete vram
		asm mov ax,0xA000
		asm mov es,ax
		asm mov di,0			//es:di destination
		asm mov al,0
		asm mov cx,65535
		asm rep STOSB 			//Store AL at address ES:DI.
	
		//Read Mode: all planes
		asm mov dx,GC_INDEX
		asm mov ax,0x0509	//Read all planes	Write mode 1	
		asm out dx,ax
		asm mov dx,GC_INDEX + 1
		asm mov ax,0x00ff	
		asm out dx,ax
	
		//320x240 60Hz
		word_out(CRTC_INDEX, V_TOTAL, 0x0d);
		word_out(CRTC_INDEX, OVERFLOW, 0x3e);
		word_out(CRTC_INDEX, V_RETRACE_START, 0xea);
		word_out(CRTC_INDEX, V_RETRACE_END, 0xac);
		word_out(CRTC_INDEX, V_DISPLAY_END, 0xdf);
		word_out(CRTC_INDEX, V_BLANK_START, 0xe7);
		word_out(CRTC_INDEX, V_BLANK_END, 0x06);
		
		//LOGICAL WIDTH = 320 + 16
		word_out(0x03d4,OFFSET,42);
		
		// set vertical retrace back to normal
		word_out(0x03d4, V_RETRACE_END, 0x8e);	
		
		ScrnPhysicalByteWidth = 80;
		ScrnPhysicalPixelWidth = 320;
		ScrnPhysicalHeight = 240;
		ScrnLogicalByteWidth = 84;
		ScrnLogicalPixelWidth = 336;
		ScrnLogicalHeight = 240;
		
		// set vertical retrace back to normal
		word_out(0x03d4, V_RETRACE_END, 0x8e);
		
		LT_WaitVsync = &LT_WaitVsync_VGA;
		LT_Print_Window_Variable = &LT_Print_Window_Variable_VGA;
		draw_map_column = &draw_map_column_vga;
		LT_Draw_Sprites = &LT_Draw_Sprites_VGA;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_VGA;
		LT_Delete_Sprite = &LT_Delete_Sprite_VGA;
		LT_Edit_MapTile = &LT_Edit_MapTile_VGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_VGA;
	}

	LT_old_time_handler = getvect(0x1C);
	LT_old_key_handler = getvect(9);    
	LT_install_key_handler();
	
	//Allocate:
	// 64kb Temp Data (Load Tilesets, Load Sprites)
	// 8kb Map + 8kb collision map
	// 64kb Music 
	// 64Kb for sprites
	// Asume EXE file around 96KB 
	// Add 16 Kb of used defined data (palette tables...)
	// Then we need around 384 Kb of Free Ram
	
	//Allocate the first 32 KB of temp data inside a 64KB block for DMA
	temp_buf = farcalloc(32768,sizeof(byte));
	linear_address = FP_SEG(temp_buf);
	linear_address = (linear_address << 4)+FP_OFF(temp_buf);
	page1 = linear_address >> 16;
	page2 = (linear_address + 32767) >> 16;
	if( page1 != page2 ) {
		LT_tile_tempdata = farcalloc(32768,sizeof(byte));
		free( temp_buf );
	} else LT_tile_tempdata = temp_buf;
	//Allocate the the second 32 KB of temp data just after the first
	LT_tile_tempdata2 = farcalloc(32768,sizeof(byte));
	
	//if ((LT_map_data = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of map data",0);
	//if ((LT_map_collision = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of collision map data",0);
	if ((LT_music.sdata = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 64 Kb of music data",0);
	
	if ((LT_map_data = farcalloc(256*19*2,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate map data",0);
	if ((LT_map_collision = farcalloc(8192,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate collision data",0);
	
	//14 fixed sprites:
	//	8x8   ( 8 sprites: 0 - 7) 200 bytes/frame
	//	16x16 ( 8 sprites: 8 - 15) 900 bytes/frame, you can use a lot of frames in this sprites
	//	32x32 ( 4 sprites: 16 - 19) 4kb/frame, you can use 2 sprites wih 8 frames at the same time
	//	64x64 ( 1 sprite: 20) 14 kb/frame, you can only use 3 frames for this sprite.
	
	if ((sprite = farcalloc(22,sizeof(SPRITE))) == NULL) LT_Error("Not enough RAM to allocate 22 predefined sprite structs",0);
	if ((LT_sprite_data = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate sprite data",0);
	//STORE BKG AT FIXED VRAM ADDRESS
	for (sprite_number = 0; sprite_number != 20; sprite_number++){
		sprite[sprite_number].bkg_data = spr_VGA[sprite_number];
	}
	//VGA_SplitScreen(0x0ffff);
}

void Draw_Roll(){
	byte page = 0;
	int roll = 240+32;
	int drawing_offset = 0;
	int bitmap_offset = 80*(240+240+240+128); //Image at third "page"
	
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
			asm mov ax,0x0A0C	
			asm out dx,ax	//(y & 0xFF00) | 0x0C to VGA port
			drawing_offset-=(240+32)*80;
		} else {
			asm mov dx,0x03D4
			asm mov ax,0x5F0C	//240 lines
			asm out dx,ax
			drawing_offset+=(240+32)*80;
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
		asm mov dx, 0x03CE
		asm mov ax, 0x0008
		asm out dx, ax
		//Draw 2 lines of normal image at the top of the roller
		asm mov cx,80
		asm rep movsb
		asm sub	si,160
		asm mov cx,80
		asm rep movsb
		asm sub	si,64*80	//go back some bytes to draw the rolling data	
		asm add di,80*2		//skip 2 lines
		
		asm add si,80*4
		asm mov cx,80
		asm rep movsb
		asm add si,80*3
		asm mov cx,80
		asm rep movsb
		asm add si,80*2
		asm mov cx,80
		asm rep movsb
		asm add si,80
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		
		asm sub si,80
		asm mov cx,80
		asm rep movsb
		
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		
		asm mov cx,80
		asm rep movsb
		asm mov cx,80
		asm rep movsb
		asm add si,80
		asm mov cx,80
		asm rep movsb
		asm add si,80*2
		asm mov cx,80
		asm rep movsb
		asm add si,80*3
		asm mov cx,80
		asm rep movsb
		asm add si,80*4
		asm mov cx,80
		asm rep movsb
		
		//Write pixels (flat colour lines, colours 0 1 2 3)
		asm mov dx, 0x03CE
		asm mov ax, 0xFF08
		asm out dx, ax
		asm sub di,26*80 // go back to upper part of bar
		
		//Draw upper shadow
		asm mov cx,40
		asm mov ax,0x0101
		asm rep stosw
		asm mov cx,40
		asm mov ax,0x0202
		asm rep stosw
		asm add di,2*80 //Skip lines

		//Draw bright part
		asm mov cx,80
		asm mov ax,0x0303
		asm rep stosw
		asm add di,19*80 //Skip lines

		//Draw bottom shadow
		asm mov cx,40
		asm mov ax,0x0101//95 95
		asm rep stosw
		asm mov cx,40
		asm mov ax,0x0000
		asm rep stosw
			
		//	asm pop cx
		//	asm loop copyloop
	
		asm pop ds
		asm pop di
		asm pop si
		
		page++;
		drawing_offset+=80;
		bitmap_offset-=80;
		roll--;
		
		//asm cli
	}
}
extern int LT_PC_Speaker_Size;
void LT_Logo(){
	int GameBoy_Sound[24] = {72,72,72,72,72,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84};
	int logotimer = 0;
	LT_Load_Logo = 1;
	LT_PC_Speaker_Size = 24;
	LT_Load_Image("Logo.bmp");
	word_out(0x03d4,OFFSET,40);
	
	LT_Fade_in();
	Draw_Roll();
	LT_Play_PC_Speaker_SFX(GameBoy_Sound);
	while (logotimer < 120){
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
	LT_Load_Logo = 0;
	LT_Fade_out();
	word_out(0x03d4,OFFSET,42);
	LT_PC_Speaker_Size = 16;
	Clearkb();
	//LT_Load_Tiles("GFX/Ltil_TR.bmp");
}

void LT_ExitDOS(){
	if ((LT_MUSIC_MODE == 2) || (LT_SFX_MODE == 2))sb_deinit();
	LT_Text_Mode();
	LT_Stop_Music();
	setvect(0x1C, LT_old_time_handler);
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_reset_key_handler();
	farfree(sprite); sprite = NULL; 
	exit(1);
}

byte LT_Setup_Map_Char[] = {"\
.(++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++).\
.!             Little Engine Setup Program - Not Copyrighted 2021             !#\
.?++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*#\
..##############################################################################\
................................................................................\
..............................................   SELECTION  INFO   .............\
..............................................!                   !#............\
.......           VIDEO           ............!                   !#............\
.......!     1 VGA SCROLL: 0     !#...........!                   !#............\
.......        MUSIC  MODE        #...........?+++++++++++++++++++*#............\
.......!     2 ADLIB             !#............#####################............\
.......!     3 SOUND BLASTER     !#.............................................\
.......          SFX MODE         #...........   8 BLASTER SET     .............\
.......!     4 PC SPEAKER        !#...........! P=220 IRQ=5 DMA=1 !#............\
.......!     5 SOUND BLASTER     !#...........?+++++++++++++++++++*#............\
.......!                         !#............#####################............\
.......         LANGUAGE          #.............................................\
.......!     6 ENGLISH           !#.............................................\
.......!     7 SPANISH           !#...........    GAME  CONTROLS   .............\
.......?+++++++++++++++++++++++++*#...........! ACTION: S D       !#............\
........###########################...........! START: ENTER      !#............\
..............................................?+++++++++++++++++++*#............\
...............................................#####################............\
................................................................................\
[[[[ EXIT: ESC [[[ SAVE & START: ENTER [[[[[ CHANGE SETTINGS: 1-6 [[[[[[[[[[[[[[\
"
};

byte LT_Setup_Map_Col[] = {"\
.444444444444444444444444444444444444444444444444444444444444444444444444444444.\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
..##############################################################################\
................................................................................\
..............................................333333333333333333333.............\
..............................................222222222222222222222#............\
.......333333333333333333333333333............222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......333333333333333333333333333#...........222222222222222222222#............\
.......222222222222222222222222222#............#####################............\
.......222222222222222222222222222#.............................................\
.......333333333333333333333333333#...........333333333333333333333.............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#............#####################............\
.......333333333333333333333333333#.............................................\
.......222222222222222222222222222#.............................................\
.......222222222222222222222222222#...........333333333333333333333.............\
.......222222222222222222222222222#...........222222222222222222222#............\
........###########################...........222222222222222222222#............\
..............................................222222222222222222222#............\
...............................................#####################............\
................................................................................\
55551111112222255511111111111111222222255555111111111111111112222255555555555555\
"
};

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