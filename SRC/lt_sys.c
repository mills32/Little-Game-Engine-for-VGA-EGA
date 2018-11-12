
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
        and     ax, 0f000h      // if bits 12-15 are set, then
        cmp     ax, 0f000h      //   processor is an 8086/8088
        je      _8086_8088		// jump if processor is 8086/8088
	}
	printf("\nCPU: 286+ - Little engine will work great!!\n");
	sleep(5);
	return;
	_8086_8088:
	printf("\nCPU: 8088 - Little engine will work a bit slow\n");
	printf("\nCPU: 8086 - Little engine will work great!!\n (Just don't use more than 4 sprites)\n");
	sleep(5);
	return;
}

void LT_Init(){
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = VGA_256_COLOR_MODE;
	int86(0x10, &regs, &regs);
	
	// turn off write protect */
    word_out(0x03d4, V_RETRACE_END, 0x2c);
    outp(MISC_OUTPUT, 0xe7);
	
	//This was tested and working on old crts!          
	word_out(0x03d4,H_DISPLAY_END, (304>>2)-1);		//HORIZONTAL RESOLUTION = 304 
	word_out(0x03d4,V_DISPLAY_END, 176<<1);  		//VERTICAL RESOLUTION = 176
	
	//This was not tested, it should center the screen with black borders on old crts
	//Modern VGA monitors and TV don't care, they just center the image
	
	//word_out(0x03d4,H_BLANK_START, (320>>2)-1);        
	//word_out(0x03d4,H_BLANK_END,0);
	//word_out(0x03d4,H_RETRACE_START,3); /**/ 
	//word_out(0x03d4,H_RETRACE_END,(320>>2)-1);  
	
	//word_out(0x03d4,V_BLANK_START, 8>>1);   /**/ 
	//word_out(0x03d4,V_BLANK_END, 184>>1);    /**/ 
	//word_out(0x03d4,V_RETRACE_START, 0x0f); /**/ 
	//word_out(0x03d4,V_RETRACE_END, 200); /**/  
	
    // set vertical retrace back to normal
    word_out(0x03d4, V_RETRACE_END, 0x8e);
	
	LT_old_time_handler = getvect(0x1C);
	LT_old_key_handler = getvect(9);    
	LT_install_key_handler();
	
	//Allocate 272kb RAM for:
	// 64kb Tileset
	// 64kb Temp Tileset
	// 16kb Temp Sprites
	// 32kb Map + 32kb collision map
	// 64kb Music 
	if ((LT_tileset.tdata = farcalloc(65535,sizeof(byte))) == NULL){
		printf("Error allocating 64kb for tile data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}	
	if ((LT_tile_datatemp = farcalloc(65535,sizeof(byte))) == NULL){
		printf("Error allocating 64kb for temp data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);		
	}
	if ((LT_sprite_tiledatatemp = farcalloc(16384,sizeof(byte))) == NULL){
		printf("Error allocating 16kb for temp data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);	
	}
	if ((LT_map.data = farcalloc(32767,sizeof(byte))) == NULL){
		printf("Error allocating 32kb for map\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}
	if ((LT_map.collision = farcalloc(32767,sizeof(byte))) == NULL){
		printf("Error allocating 32kb for collision map\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);	
	}
	if ((LT_music.sdata = farcalloc(65535,sizeof(byte))) == NULL){
		printf("Error allocating 64kb for music data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);	
	}
	MCGA_SplitScreen(0x0ffff);
}

void LT_ExitDOS(){
	union REGS regs;
	MCGA_Fade_out();
	MCGA_SplitScreen(0x0ffff);
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
	StopMOD();
	LT_Stop_Music();
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_unload_font();
	LT_reset_key_handler();
	
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