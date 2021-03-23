
#include "lt__eng.h"

extern byte LT_PICO8_palette[];
extern const unsigned char LT_logo_palette[];
extern byte LT_Setup_Map_Char[];
extern byte LT_Setup_Map_Col[];

COLORCYCLE cycle_logo;
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
extern byte *VGA;
extern byte *EGA;
FILE *LT_setupfile;

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

void LT_Check_CPU(){
	asm{
		pushf                   // push original FLAGS
        pop     ax              // get original FLAGS
        mov     cx, ax          // save original FLAGS
        and     ax, 0fffh       // clear bits 12-15 in FLAGS
        push    ax              // save new FLAGS value on stack
        popf                    // replace current FLAGS value
        pushf                   // get new FLAGS
        pop     ax              // store new FLAGS in AX
        and     ax, 0f000h      // if bits 12-15 are set, then processor is an 8086/8088
        cmp     ax, 0f000h       
        je      _8086_8088		// jump if processor is 8086/8088
	}
	printf("\nCPU: 286+ - Little engine will work great!!\n");
	sleep(5);
	return;
	_8086_8088:
	printf("\nCPU: 8088 - Little engine will work a bit slow\n");
	printf("\nCPU: 8086 - Little engine will work OK!!\n");
	sleep(5);
	return;
}

void LT_Text_Mode(){
	union REGS regs;
	LT_Fade_out();
	//Enable cursor
	outportb(0x3D4, 0x0A);
	outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 14);
	outportb(0x3D4, 0x0B);
	outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 15);
	
	VGA_SplitScreen(0x0ffff);
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
}

void LT_Error(char *error, char *file){
	asm STI; //enable interrupts
	LT_Text_Mode();
	printf("%s %s \n",error,file);
	sleep(2);
	LT_ExitDOS();
	exit(1);
} 

void Disable_Cursor(){
	outportb(0x3D4, 0x0A);
	outportb(0x3D5, 0x20);
}

void Enable_Cursor(){
	outportb(0x3D4, 0x0A);
	outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 14);
	outportb(0x3D4, 0x0B);
	outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 15);
}

void print_gfxmode(int mode){
	if (mode == 1){
		gotoxy(16,8); textattr(RED << 4 | WHITE); cprintf("EGA 16 COLORS");
		gotoxy(16,9); textattr(BLUE << 4 | WHITE); cprintf("VGA 16 COLORS");
		gotoxy(16,10); cprintf("VGA 256 COLORS");
		gotoxy(49,7); cprintf("EGA ORIGINAL MODE");
		gotoxy(49,8); cprintf(" 16 FIXED COLORS ");
		gotoxy(49,9); cprintf(" FOR SLOW 8088'S ");
	}
	if (mode == 2){
		gotoxy(16,9); textattr(RED << 4 | WHITE); cprintf("VGA 16 COLORS");
		gotoxy(16,8); textattr(BLUE << 4 | WHITE); cprintf("EGA 16 COLORS");
		gotoxy(16,10); cprintf("VGA 256 COLORS");
		gotoxy(49,7); cprintf(" VGA IN EGA MODE ");
		gotoxy(49,8); cprintf(" PICO-8  PALETTE ");
		gotoxy(49,9); cprintf(" FOR SLOW 8088'S ");
	}
	if (mode == 3){
		gotoxy(16,10); textattr(RED << 4 | WHITE); cprintf("VGA 256 COLORS");
		gotoxy(16,8); textattr(BLUE << 4 | WHITE); cprintf("EGA 16 COLORS");
		gotoxy(16,9); cprintf("VGA 16 COLORS");
		gotoxy(49,7); cprintf("   VGA MODE X    ");
		gotoxy(49,8); cprintf("   256 COLORS    ");
		gotoxy(49,9); cprintf("FOR FAST 8086/286");
	}
}

void print_scrollmode(int mode){
	if (mode == 0) {
		XGA_TEXT_MAP[(10*160)+58] = 48;
		textattr(BLUE << 4 | WHITE);
		gotoxy(49,7); cprintf("HARDWARE SCROLL 0");
		gotoxy(49,8); cprintf("FOR EGA/VGA CARDS");
		gotoxy(49,9); cprintf("                 ");
	}
	if (mode == 1) {
		XGA_TEXT_MAP[(10*160)+58] = 49;
		textattr(BLUE << 4 | WHITE);
		gotoxy(49,7); cprintf("HARDWARE SCROLL 1");
		gotoxy(49,8); cprintf("FOR SVGA CARDS   ");
		gotoxy(49,9); cprintf("                 ");
	}
}

void print_musicmode(int music){
	if (music == 5){
		gotoxy(16,13); textattr(RED << 4 | WHITE); cprintf("ADLIB");
		gotoxy(16,14); textattr(BLUE << 4 | WHITE); cprintf("SOUND BLASTER");
		gotoxy(49,7); cprintf("MUSIC WILL USE   ");
		gotoxy(49,8); cprintf("OPL2/YM3812 ONLY ");
		gotoxy(49,9); cprintf("                 ");
	}
	if (music == 6){
		gotoxy(16,13); textattr(BLUE << 4 | WHITE); cprintf("ADLIB");
		gotoxy(49,7); cprintf("MUSIC WILL USE   ");
		gotoxy(49,8); cprintf("OPL2/YM3812 AND  ");
		gotoxy(49,9); cprintf("SOUND BLASTER PCM");
		gotoxy(16,14); textattr(RED << 4 | WHITE); cprintf("SOUND BLASTER");
	}
}

void print_sfxmode(int sfx){
	if (sfx == 7){
		gotoxy(16,16); textattr(RED << 4 | WHITE); cprintf("PC SPEAKER");
		gotoxy(16,17); textattr(BLUE << 4 | WHITE); cprintf("SOUND BLASTER");
		gotoxy(49,7); cprintf("SIMPLE SOUND     ");
		gotoxy(49,8); cprintf("EFFECTS PLAYED BY");
		gotoxy(49,9); cprintf("INTERNAL SPEAKER ");
	}
	if (sfx == 8){
		gotoxy(16,16); textattr(BLUE << 4 | WHITE); cprintf("PC SPEAKER");
		gotoxy(49,7); cprintf("8 BIT PCM SAMPLES");
		gotoxy(49,8); cprintf("PLAYED BY SOUND  ");
		gotoxy(49,9); cprintf("BLASTER CARD     ");
		gotoxy(16,17); textattr(RED << 4 | WHITE); cprintf("SOUND BLASTER");
	}
}

void print_langmode(int lang){
	if (lang == 'E'){
		gotoxy(17,19); textattr(RED << 4 | WHITE); cprintf("ENGLISH");
		gotoxy(17,20); textattr(BLUE << 4 | WHITE); cprintf("SPANISH");
	}
	if (lang == 'S'){
		gotoxy(17,19); textattr(BLUE << 4 | WHITE); cprintf("ENGLISH");
		gotoxy(17,20); textattr(RED << 4 | WHITE); cprintf("SPANISH");
	}
}

void Read_Setup(){
	byte buffer[256];
	fread(buffer,1,256,LT_setupfile);
	LT_VIDEO_MODE = buffer[0x16]-48;
	print_gfxmode(LT_VIDEO_MODE);
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
	LT_LANGUAGE = buffer[0x55];
	print_langmode(LT_LANGUAGE);
	
	textattr(BLUE << 4 | WHITE);
	gotoxy(49,7); cprintf("                 ");
	gotoxy(49,8); cprintf("     WAITING     ");
	gotoxy(49,9); cprintf("                 ");
}

void Save_Setup(){
	fseek(LT_setupfile,0x16,SEEK_SET);
	fputc(LT_VIDEO_MODE+48,LT_setupfile);
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
	fputc(LT_LANGUAGE,LT_setupfile);
}

void LT_Setup(){
	int i = 0;
	int j = 0;
	int start = 0;
	int blaster_set = 0;
	byte character = 0;
	byte color = 0;
	LT_setupfile = fopen("config.txt","rb+");
	if (!LT_setupfile) {
		system("cls");
		printf("config.txt is missing\nNew default file created");
		LT_setupfile = fopen("config.txt","w");
		fprintf(LT_setupfile,"#SETUP\n------\nVIDEO=3\nSCROLL=0\nMUSIC=6\nSFX=8\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		printf("#SETUP\n------\nVIDEO=3\nSCROLL=0\nMUSIC=6\nSFX=8\nBLASP=220\nBLASI=5\nBLASD=1\nLANG=0\n#SAVE\n-----\nLEVEL_A=0 ENERGY_A=0\nLEVEL_B=0 ENERGY_B=0");
		fclose(LT_setupfile);
		sleep(2);
		LT_setupfile = fopen("config.txt","rb+");
	}
	
	Disable_Cursor();
	
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
					case 49: LT_VIDEO_MODE = 1; print_gfxmode(1); break;
					case 50: LT_VIDEO_MODE = 2; print_gfxmode(2); break;
					case 51: LT_VIDEO_MODE = 3; print_gfxmode(3); break;
					case 52: LT_SCROLL_MODE++; print_scrollmode(LT_SCROLL_MODE & 1); break;
					case 53: LT_MUSIC_MODE = 5; print_musicmode(5);break;
					case 54: LT_MUSIC_MODE = 6; print_musicmode(6);break;
					case 55: LT_SFX_MODE = 7; print_sfxmode(7);break;
					case 56: LT_SFX_MODE = 8; print_sfxmode(8);break;
					case 57: 
						blaster_set = 1; 
						gotoxy(51,14); textattr(BLACK << 4 | YELLOW);
						cprintf("   ");
						gotoxy(59,14); cprintf(" ");
						gotoxy(65,14); cprintf(" ");
						textattr(BLUE << 4 | WHITE);
						gotoxy(49,7); cprintf("   SET BLASTER   ");
						gotoxy(49,8); cprintf(" DEFAULT  VALUES ");
						gotoxy(49,9); cprintf("P=220 IRQ=5 DMA=1");
						Enable_Cursor();
						textattr(BLACK << 4 | YELLOW);
						gotoxy(51,14);
					break;
					case 69: LT_LANGUAGE = 'E'; print_langmode(LT_LANGUAGE); break;
					case 101: LT_LANGUAGE = 'E'; print_langmode(LT_LANGUAGE); break;
					case 83: LT_LANGUAGE = 'S'; print_langmode(LT_LANGUAGE); break;
					case 115: LT_LANGUAGE = 'S'; print_langmode(LT_LANGUAGE); break;
					
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
					character = BLUE << 4 | WHITE;
					XGA_TEXT_MAP[(160*13)+101] = character;
					XGA_TEXT_MAP[(160*13)+103] = character;
					XGA_TEXT_MAP[(160*13)+105] = character;
					XGA_TEXT_MAP[(160*13)+117] = character;
					XGA_TEXT_MAP[(160*13)+129] = character;
					blaster_set = 0; 
					Disable_Cursor();
				}
				Clearkb();
			}
		}
		
		while( inp( INPUT_STATUS_0 ) & 0x08 );
		while( !(inp( INPUT_STATUS_0 ) & 0x08 ) );
	}
	if (start == 2) {
		fclose(LT_setupfile);
		Enable_Cursor();
		system("cls");
		exit(1);
	}

	//SAVE SETTINGS
	Save_Setup();

	fclose(LT_setupfile);
	system("cls");
	
	if (LT_VIDEO_MODE == 3) LT_VIDEO_MODE = 1;
	else LT_VIDEO_MODE = 0;
	if (LT_MUSIC_MODE == 6) LT_MUSIC_MODE = 1;
	else LT_MUSIC_MODE = 0;
	
	
	if (LT_VIDEO_MODE == 0) VGA_EGAMODE_CustomPalette(LT_PICO8_palette);
	
	Enable_Cursor();
	LT_Adlib_Detect();
    LT_init();
	sb_init();//After LT_Init
	if (LT_VIDEO_MODE)LT_Load_Font("GFX/0_font.bmp");
	else LT_Load_Font("GFX/0_font_E.bmp");
}

void LT_Init(){
	unsigned char *temp_buf;
	long linear_address;
	short page1, page2;
	
	union REGS regs;
	word i;
	word *sprite_bkg;
	byte sprite_number;
	word spr_VGA[33] = {	
		//8x8
		0xB280,0xB298,0xB2B0,0xB2C8,0xB2E0,0xB2F8,0xB310,0xB328,
		0xB340,0xB358,0xB370,0xB388,0xB3A0,0xB3B8,0xB3D0,0xB3E8,
		//16x16
		0xB400,0xB450,0xB4A0,0xB4F0,0xB540,0xB590,0xB5E0,0xB630,
		0xB680,0xB6D0,0xB720,0xB770,
		//32x32
		0xB7C0,0xB8E0,0xBA00,0xBB20,
		//64x64
		0xBC40
	};
	word spr_EGA[33] = {	//+2A0+2A0
		//8x8
		0x5400,0x5410,0x5420,0x5430,0x5440,0x5450,0x5460,0x5470,
		0x5480,0x5490,0x54A0,0x54B0,0x54C0,0x54D0,0x54E0,0x54F0,
		//16x16
		0x5500,0x5530,0x5560,0x5590,0x55C0,0x55F0,
		0x5620,0x5650,0x5680,0x56B0,0x56E0,0x5710,
		//32x32
		0x5740,0x57E0,0x5880,0x5920,
		//64x64
		0x59C0
	};
	if (LT_VIDEO_MODE == 0) sprite_bkg = &spr_EGA[0];
	if (LT_VIDEO_MODE == 1) sprite_bkg = &spr_VGA[0];
	
	//FIXED SPRITE BKG ADDRESSES IN VRAM
		
	if (LT_VIDEO_MODE == 1){//VGA 320 x 240 x 256
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
		
		LT_WaitVsync = &LT_WaitVsync0_VGA;
		LT_Print_Window_Variable = &LT_Print_Window_Variable_VGA;
		draw_map_column = &draw_map_column_vga;
		draw_map_row = &draw_map_row_vga;
		LT_Delete_Sprite = &LT_Delete_Sprite_VGA;
		LT_Edit_MapTile = &LT_Edit_MapTile_VGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_VGA;
		LT_Print = &LT_Print_VGA;
	}
	if (LT_VIDEO_MODE == 0){// EGA 320 x 240 x 16
		LT_Fade_in = &LT_Fade_in_EGA;
		LT_Fade_out = &LT_Fade_out_EGA;
		
		LT_Fade_out();
	
		regs.h.ah = 0;
		regs.h.al = 0x0D;
		int86(0x10, &regs, &regs);
		
		/* turn off write protect */
		word_out(CRTC_INDEX, V_RETRACE_END, 0x2c);
		
		word_out(CRTC_INDEX, V_TOTAL, 0x0d);
		word_out(CRTC_INDEX, OVERFLOW, 0x3e);
		word_out(CRTC_INDEX, V_RETRACE_START, 0xea);
		word_out(CRTC_INDEX, V_RETRACE_END, 0xac);
		word_out(CRTC_INDEX, V_DISPLAY_END, 0xdf);
		word_out(CRTC_INDEX, V_BLANK_START, 0xe7);
		word_out(CRTC_INDEX, V_BLANK_END, 0x06);
	
		//LOGICAL WIDTH = 320 + 16
		word_out(0x03d4,OFFSET,21);
	
		// set vertical retrace back to normal
		word_out(0x03d4, V_RETRACE_END, 0x8e);
	
		memset(EGA,0x00,(320*240)/8);
		
		LT_WaitVsync = &LT_WaitVsync0_EGA;
		LT_Print_Window_Variable = &LT_Print_Window_Variable_EGA;
		draw_map_column = &draw_map_column_ega;
		draw_map_row = &draw_map_row_ega;
		LT_Delete_Sprite = &LT_Delete_Sprite_EGA;
		LT_Edit_MapTile = &LT_Edit_MapTile_EGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_EGA;
		LT_Print = &LT_Print_EGA;
		
		VGA_EGAMODE_CustomPalette(LT_PICO8_palette);
	}		

	LT_old_time_handler = getvect(0x1C);
	LT_old_key_handler = getvect(9);    
	LT_install_key_handler();
	
	//Allocate:
	// 64kb Temp Data (Load Tilesets, Load Sprites)
	// 8kb Map + 8kb collision map
	// 64kb Music 
	// 128Kb for sprites
	// Asume EXE file around 128KB Max
	// Add 16 Kb of used defined data (palette tables...)
	// Then we need around 420 Kb of Free Ram
	
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
	
	if ((LT_map_data = farcalloc(8192,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate map data",0);
	if ((LT_map_collision = farcalloc(8192,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate collision data",0);
	
	//33 fixed sprites:
	//	8x8   (16 sprites: 0 - 15)	4kb
	//	16x16 (12 sprites: 16 - 27) <16kb
	//	32x32 ( 4 sprites: 28 - 31) 16kb
	//	64x64 ( 1 sprite : 32) 16 Kb
	
	if ((sprite = farcalloc(33,sizeof(SPRITE))) == NULL) LT_Error("Not enough RAM to allocate 33 predefined sprite structs",0);
	if ((LT_sprite_data = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate sprite data",0);
	//STORE BKG AT FIXED VRAM ADDRESS
	for (sprite_number = 0; sprite_number != 33; sprite_number++){
		sprite[sprite_number].bkg_data = sprite_bkg[sprite_number];
	}
	
	//VGA_SplitScreen(0x0ffff);
}

void LT_Logo(){
	int logotimer = 0;
	LT_Load_Map("GFX/Logo.tmx");
	if (LT_VIDEO_MODE) {
		LT_Load_Tiles("GFX/Ltil_VGA.bmp");
		LT_Fade_out();
	}
	else {LT_Load_Tiles("GFX/Ltil_EGA.bmp");}
	LT_Set_Map(0,0);
	cycle_init(&cycle_logo,LT_logo_palette);
	
	while (logotimer < 128){
		cycle_palette(&cycle_logo,1);
		if (LT_Keys[LT_ESC]) { logotimer = 128;}//if esc released, exit
		LT_WaitVsync();
		logotimer++;
	}
	Clearkb();
}

void LT_ExitDOS(){
	sb_deinit();
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

byte LT_PICO8_palette[] = {//generated with gimp, then divided by 4 (max = 64).
//I don't want to know why the EGA colors have to be arranged like this
//In the VGA registers
	0x00,0x00,0x00,	//0
	0x06,0x0A,0x15,	//1
	0x02,0x21,0x14,	//2
	0x00,0x14,0x35,	//4
	0x1F,0x09,0x14,	//3
	0x3F,0x32,0x42,	//6
	0x2B,0x14,0x0E, //5	
	0x2F,0x31,0x31,	//7
	0x00,0x00,0x00, //Not used in VGA-EGA mode
	0x00,0x00,0x00,		//8
	0x00,0x00,0x00,		//9
	0x00,0x00,0x00,		//10
	0x00,0x00,0x00,		//11
	0x00,0x00,0x00,		//12
	0x00,0x00,0x00,		//13
	0x00,0x00,0x00,		//14
	0x16,0x17,0x18,	//8	//15
    0x20,0x1D,0x26,	//9	//16
	0x00,0x39,0x0D,	//10//17
	0x08,0x2B,0x3F,	//12//18
	0x3F,0x00,0x12,	//11//19
	0x3F,0x1D,0x29,	//14//20
	0x3F,0x38,0x0D, //13//21	
	0x3f,0x3f,0x3f, //15//22	
};
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

byte LT_Setup_Map_Char[] = {"\
.(++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++).\
.!             Little Engine Setup Program - Not Copyrighted 2021             !#\
.?++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*#\
..##############################################################################\
................................................................................\
..............................................   SELECTION  INFO   .............\
.......        VIDEO  MODE        ............!                   !#............\
.......!     1 EGA 16 COLORS     !#...........!                   !#............\
.......!     2 VGA 16 COLORS     !#...........!                   !#............\
.......!     3 VGA 256 COLORS    !#...........?+++++++++++++++++++*#............\
.......!     4 SCROLL MODE = 0   !#............#####################............\
.......        MUSIC  MODE        #.............................................\
.......!     5 ADLIB             !#...........   9 BLASTER SET     .............\
.......!     6 SOUND BLASTER     !#...........! P=220 IRQ=5 DMA=1 !#............\
.......          SFX MODE         #...........?+++++++++++++++++++*#............\
.......!     7 PC SPEAKER        !#............#####################............\
.......!     8 SOUND BLASTER     !#.............................................\
.......          LANGUAGE         #.............................................\
.......!     E: ENGLISH          !#...........    GAME  CONTROLS   .............\
.......!     S: SPANISH          !#...........! ACTION: S D ENTER !#............\
.......?+++++++++++++++++++++++++*#...........! START: ENTER      !#............\
........###########################...........?+++++++++++++++++++*#............\
...............................................#####################............\
................................................................................\
[[[[ EXIT: ESC [[[ SAVE & START: ENTER [[[[[ CHANGE SETTINGS: 1-9 / E S [[[[[[[[\
"
};

byte LT_Setup_Map_Col[] = {"\
.444444444444444444444444444444444444444444444444444444444444444444444444444444.\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
..##############################################################################\
................................................................................\
..............................................333333333333333333333.............\
.......333333333333333333333333333............222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#............#####################............\
.......333333333333333333333333333#.............................................\
.......222222222222222222222222222#...........333333333333333333333.............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......333333333333333333333333333#...........222222222222222222222#............\
.......222222222222222222222222222#............#####################............\
.......222222222222222222222222222#.............................................\
.......333333333333333333333333333#.............................................\
.......222222222222222222222222222#...........333333333333333333333.............\
.......222222222222222222222222222#...........222222222222222222222#............\
.......222222222222222222222222222#...........222222222222222222222#............\
........###########################...........222222222222222222222#............\
...............................................#####################............\
................................................................................\
55551111112222255511111111111111222222255555111111111111111112222222222255555555\
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