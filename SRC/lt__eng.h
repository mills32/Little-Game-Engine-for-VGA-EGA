/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/
	
	Sprite compiler and bliter from xlib

	Please feel free to copy this source code.                            

##########################################################################*/

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h> //use stat to get correct file size
#include <dos.h>
#include <alloc.h>
#include <mem.h>
#include <math.h>
#undef outp

#define SC_INDEX            0x03c4    // VGA sequence controller
#define SC_DATA             0x03c5

#define GC_INDEX            0x03ce    // VGA graphics controller
#define GC_DATA             0x03cf

#define MAP_MASK            0x02
#define ALL_PLANES          0xff02
#define MEMORY_MODE         0x04
#define CRTC_DATA           0x03d5

#define VIDEO_INT           0x10      // the BIOS video interrupt.
#define SET_MODE            0x00      // BIOS func to set the video mode.
#define VGA_256_COLOR_MODE  0x13      // use to set 256-color mode. 
#define TEXT_MODE           0x03      // use to set 80x25 text mode. 

#define SCREEN_WIDTH        320       // width in pixels of mode 0x13
#define SCREEN_HEIGHT       240       // height in pixels of mode 0x13
#define NUM_COLORS          256       // number of colors in mode 0x13

#define AC_HPP              0X20 | 0X13    // Horizontal Pel Panning Register

#define DE_MASK				0x01     //display enable bit in status register 1

#define MISC_OUTPUT         0x03c2    // VGA misc. output register
#define S_MEMORY_MODE		0x03C4	
#define INPUT_STATUS_0		0x03da
#define AC_MODE_CONTROL		0x10	  //Index of Mode COntrol register in AC
#define AC_INDEX			0x03c0	  //Attribute controller index register
#define CRTC_INDEX			0x03d4	  // CRT Controller Index
#define H_TOTAL             0x00      // CRT controller registers
#define H_DISPLAY_END       0x01
#define H_BLANK_START       0x02
#define H_BLANK_END         0x03
#define H_RETRACE_START     0x04
#define H_RETRACE_END       0x05
#define V_TOTAL             0x06
#define OVERFLOW            0x07
#define PRESET_ROW_SCAN     0x08 
#define MAX_SCAN_LINE       0x09
#define V_RETRACE_START     0x10
#define V_RETRACE_END       0x11
#define V_DISPLAY_END       0x12
#define OFFSET              0x13
#define UNDERLINE_LOCATION  0x14
#define V_BLANK_START       0x15
#define V_BLANK_END         0x16
#define MODE_CONTROL        0x17
#define LINE_COMPARE		0x18      

#define ADLIB_PORT 			0x388

//KEYS
#define LT_ESC				0x01
#define LT_ENTER			0x1C
#define LT_ACTION			0x1F //S
#define LT_JUMP				0x20 //D
#define LT_UP				0x48
#define LT_DOWN				0x50
#define LT_LEFT				0x4B
#define LT_RIGHT			0x4D


/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

extern int Free_RAM;
extern int SCR_X;
extern int SCR_Y;
extern float t1,t2; //debug
extern int LT_SIN[]; 
extern int LT_COS[]; 

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
extern word *my_clock;
extern word start;

extern int LT_Keys[];

extern byte LT_MODE;

typedef struct tagBITMAP{				// structure for a bitmap
	word width;
	word height;
	byte palette[256*3];
	byte *data;
} BITMAP;

typedef struct tagANIMATION{			// structure for an animation
	word width;
	word height;
	byte palette[256*3];
	byte animate;
	byte speed;
	byte anim_counter;
	byte anim_speed;
	byte baseframe;
	byte aframes;
	word pos_x;
	word pos_y;
	byte frame;
	byte nframes;
	byte *data;	
} ANIMATION;

typedef struct tagCOLORCYCLE{			// structure for colour cycle
	byte frame;
	byte counter;
	byte *palette;
} COLORCYCLE;

typedef struct tagMAP{					// structure for a map
	word width;
	word height;
	word ntiles;
	byte *data;
	byte *collision;
} MAP;

typedef struct tagTILE{					// structure for tiles
	word width;
	word height;
	word ntiles;
	byte palette[256*3];
	byte *tdata;
} TILE;

typedef struct tagSPRITEFRAME{			// structure for a sprite frame
	char *compiled_code;
} SPRITEFRAME;

typedef struct tagSPRITE{				// structure for a sprite
	word width;
	word height;
	byte palette[256*3];
	word init;	//init sprite to captute bak data
	word animate;
	word speed;
	word anim_counter;
	word anim_speed;
	word baseframe; //first frame
	word aframes;
	word ground;	//platform touch ground
	byte get_item;	//For player
	byte ntile_item;
	byte col_item;
	byte mode; //FOR AI
	word jump;
	word jump_frame;
	word climb;
	word tile_x;
	word tile_y;
	word pos_x;	
	word pos_y;
	word last_x;
	word last_y;
	float fpos_x;
	float fpos_y;
	int mspeed_x;
	int mspeed_y;
	int speed_x;
	int speed_y;
	word s_x;		//To control speed
	word s_y;
	word misc;
	word state;		//0 no speed
	word frame;
	word nframes;
	word bkg_data;	//restore bkg variables
	word size;
	word siz;
	word next_scanline;
	word s_delete;
	SPRITEFRAME *frames;	
} SPRITE;

typedef struct tagIMFsong{				// structure for adlib IMF song, or MOD pattern data
	long size;
	long offset;
	byte filetype; //0 1 - imf0 imf1
	byte *sdata;
} IMFsong;

typedef struct tagLT_Col{				// structure to return collision data
	byte tile_number;	
	byte tilecol_number;	
	byte col_x;
	byte col_y;	
} LT_Col;

extern IMFsong LT_music;	
extern MAP LT_map;			
extern TILE LT_tileset;	
extern SPRITE LT_Loading_Animation;
extern SPRITE *sprite;
extern unsigned char *LT_tile_tempdata; 

void LT_Adlib_Detect();
void LT_Check_CPU();

//
void LT_Set_Loading_Interrupt();
void LT_Delete_Loading_Interrupt();

//MCGA/VGA Hardware scroll
void VGA_Scroll(word x, word y);
void LT_WaitVsync();
void VGA_ClearScreen();
void VGA_SplitScreen();

void LT_Init();
void LT_ExitDOS();

// GFX
void LT_Load_BKG(char *file);
void LT_Draw_BKG();
void LT_Load_Animation(char *file, byte size);
void LT_Set_Animation(byte baseframe, byte frames, byte speed);
void LT_Unload_Animation();
void LT_SetWindow(char *file);
void LT_ResetWindow();
void LT_Load_Font(char *file);
void LT_Print_Window_Variable(byte x, word var);
void LT_Load_Tiles(char *file);
void LT_unload_tileset();
void LT_Load_Map(char *file);
void LT_Set_Map(int x, int y);
void LT_Edit_MapTile(word x, word y, byte ntile, byte col);
void draw_map_column(word x, word y, word map_offset);
void draw_map_row(word x, word y, word map_offset);
void LT_unload_map();
void LT_scroll_map();
void LT_Endless_SideScroll_Map(int y);

//palettes
void VGA_ClearPalette();                                                           
void set_palette(unsigned char *palette);
void VGA_Fade_in(unsigned char *palette);
void VGA_Fade_out(); 
void cycle_init(COLORCYCLE *cycle,unsigned char *palette);
void cycle_palette(COLORCYCLE *cycle, byte speed);

//SPRITE
void LT_Load_Sprite(char *file,int sprite_number, byte size);
void LT_Clone_Sprite(int sprite_number_c,int sprite_number);
void LT_Set_Sprite_Animation(int sprite_number, byte baseframe, byte frames, byte speed);
void LT_Draw_Sprites();
void LT_Get_Item(int sprite_number, byte ntile, byte col);
void LT_Set_AI_Sprite(int sprite_number, byte mode, word x, word y, int sx, int sy);
LT_Col LT_move_player(int sprite_number);
LT_Col LT_Bounce_Ball(int sprite_number);
void LT_Update_AI_Sprites();
void LT_Reset_Sprite_Stack();
void LT_Reset_AI_Stack();
void LT_Delete_Sprite(int sprite_number);
void LT_unload_sprite(int sprite_number);
void LT_scroll_follow(int sprite_number);

//ADLIB 
void LT_Load_Music(char *filename);
void LT_Start_Music(word freq_div);
void do_play_music();
void LT_Stop_Music();
void LT_Unload_Music();

//GUS
void LT_Init_GUS(byte channels);
int LT_LoadMOD(char *filename);
void PlayMOD(byte mode);//0 normal; 1 disable effects for slow cpu (8088-8086).
void StopMOD();

