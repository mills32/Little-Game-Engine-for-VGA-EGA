
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

// define LT_old_..._handler to be a pointer to a function
void interrupt (*LT_old_time_handler)(void); 	
void interrupt (*LT_old_key_handler)(void);

word ScrnPhysicalByteWidth = 0;		//Physical width in bytes of screen
word ScrnPhysicalPixelWidth = 0;	//Physical width in pixels of screen
word ScrnPhysicalHeight = 0;		//Physical Height of screen
word ScrnLogicalByteWidth = 0;		//Logical width in bytes of screen
word ScrnLogicalPixelWidth = 0;		//Logical width in pixels of screen
word ScrnLogicalHeight = 0;			//Logical Height of screen

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
	VGA_Fade_out();
	VGA_SplitScreen(0x0ffff);
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
}

void LT_Error(char *error, char *file){
	LT_Text_Mode();
	printf("%s %s \n",error,file);
	sleep(2);
	LT_ExitDOS();
	exit(1);
} 

void LT_Init(){
	union REGS regs;
	word i;

	//FIXED SPRITE BKG ADDRESSES IN VRAM
	byte sprite_number;
	word sprite_bkg[33] = 
	{	
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
	
	LT_old_time_handler = getvect(0x1C);
	LT_old_key_handler = getvect(9);    
	LT_install_key_handler();
	
	//Allocate 192 Kb:
	// 64kb Temp Data (Load Tilesets, Load Sprites)
	// 32kb Map + 32kb collision map
	// 64kb Music 

	if ((LT_tile_tempdata = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 64 Kb of temp data\n",0);
	if ((LT_map.data = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of map data",0);
	if ((LT_map.collision = farcalloc(32767,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 32 Kb of collision map data",0);
	if ((LT_music.sdata = farcalloc(65535,sizeof(byte))) == NULL) LT_Error("Not enough RAM to allocate 64 Kb of music data",0);
	
	//33 fixed sprites:
	//	8x8   (16 sprites: 0 - 15)	
	//	16x16 (12 sprites: 16 - 27)
	//	32x32 ( 4 sprites: 28 - 31)
	//	64x64 (sprite 32)
	
	if ((sprite = farcalloc(33,sizeof(SPRITE))) == NULL) LT_Error("Not enough RAM to allocate 35 predefined sprites",0);
	
	//STORE BKG AT FIXED VRAM ADDRESS
	for (sprite_number = 0; sprite_number != 33; sprite_number++){
		sprite[sprite_number].bkg_data = sprite_bkg[sprite_number];
	}
	//VGA_SplitScreen(0x0ffff);
}

void LT_ExitDOS(){
	LT_Text_Mode();
	StopMOD();
	LT_Stop_Music();
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_reset_key_handler();
	farfree(sprite); sprite = NULL; 
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