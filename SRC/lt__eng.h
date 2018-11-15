/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

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
#define VIDEO_INT           0x10      // the BIOS video interrupt.
#define SET_MODE            0x00      // BIOS func to set the video mode.
#define VGA_256_COLOR_MODE  0x13      // use to set 256-color mode. 
#define TEXT_MODE           0x03      // use to set 80x25 text mode. 

#define SCREEN_WIDTH        320       // width in pixels of mode 0x13
#define SCREEN_HEIGHT       200       // height in pixels of mode 0x13
#define NUM_COLORS          256       // number of colors in mode 0x13

#define MISC_OUTPUT         0x03c2    // VGA misc. output register
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
	byte *rle_data;
} SPRITEFRAME;

typedef struct tagSPRITE{				// structure for a sprite
	word width;
	word height;
	byte palette[256*3];
	byte init;	//init sprite to captute bak data
	byte animate;
	byte speed;
	byte anim_counter;
	byte anim_speed;
	byte baseframe; //first frame
	byte aframes;
	byte ground;	//platform touch ground
	byte jump;
	byte jump_frame;
	word pos_x;
	word pos_y;
	word last_x;
	word last_y;
	float fpos_x;
	float fpos_y;
	int speed_x;
	int speed_y;
	byte state;		//0 no speed
	byte frame;
	byte nframes;
	byte *bkg_data;
	byte s_delete;
	SPRITEFRAME *rle_frames;	
} SPRITE;

typedef struct tagIMFsong{				// structure for an IMF song, also stores MOD pattern data
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
extern ANIMATION LT_Loading_Animation;

extern unsigned char *LT_tile_datatemp; 
extern unsigned char *LT_sprite_tiledatatemp;

void LT_Adlib_Detect();
void LT_Check_CPU();

//
void LT_Set_Loading_Interrupt();
void LT_Delete_Loading_Interrupt();

//MCGA/VGA Hardware scroll
void MCGA_Scroll(word x, word y);
void MCGA_WaitVBL();
void MCGA_SplitScreen();
void MCGA_ClearScreen();

void LT_Init();
void LT_ExitDOS();

// GFX
void LT_Load_BKG(char *file);
void LT_Draw_BKG();
void LT_Load_Animation(char *file,ANIMATION *s, byte size);
void LT_Set_Animation(ANIMATION *s, byte baseframe, byte frames, byte speed);
void LT_Draw_Animation(ANIMATION *s);
void LT_Unload_Animation(); 
void load_tiles(char *file);
void LT_unload_tileset();
void load_map(char *file);
void LT_Set_Map(int x, int y);
void LT_Edit_MapTile(byte x, byte y, byte ntile, byte col);
void draw_map_column(word x, word y, word map_offset);
void draw_map_row(word x, word y, word map_offset);
void LT_unload_map();
void LT_scroll_map();
void LT_Endless_SideScroll_Map(int y);

//palettes
void MCGA_ClearPalette();                                                           
void set_palette(unsigned char *palette);
void MCGA_Fade_in(unsigned char *palette);
void MCGA_Fade_out(); 
void cycle_init(COLORCYCLE *cycle,unsigned char *palette);
void cycle_palette(COLORCYCLE *cycle, byte speed);

//SPRITE
void LT_Load_Sprite(char *file,SPRITE *s, byte size);
void LT_Clone_Sprite(SPRITE *c,SPRITE *s);
void LT_Set_Sprite_Animation(SPRITE *s, byte baseframe, byte frames, byte speed);
void LT_Draw_Sprite(SPRITE *b);
void LT_Draw_Enemy(SPRITE *b);
void LT_Set_Enemy(SPRITE *s, word x, word y, int sx, int sy);
void LT_Restore_Sprite_BKG(SPRITE *s);
LT_Col LT_move_player(SPRITE *s);
void LT_platform_walker(SPRITE *s);
LT_Col LT_Bounce_Ball(SPRITE *s);
void LT_Delete_Sprite(SPRITE *s);
void LT_load_font(char *file);
void LT_gprint(byte var, word x, word y);
void LT_unload_sprite(SPRITE *s);
void LT_unload_font();
void LT_scroll_follow(SPRITE *s);

//ADLIB 
void LT_Load_Music(char *filename);
void LT_Start_Music(word freq_div);
void LT_Stop_Music();
void LT_Unload_Music();

//GUS
void LT_Init_GUS(byte channels);
int LoadMOD(char *filename);
void PlayMOD(byte mode);//0 normal; 1 disable effects for slow cpu (8088-8086).
void StopMOD();

