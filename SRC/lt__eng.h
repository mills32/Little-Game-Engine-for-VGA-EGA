/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/
	
	Sprite compiler and bliter from xlib

	Please feel free to copy this source code.                            

##########################################################################*/

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

//SOUND
#define ADLIB_PORT 			0x388
#define SB_RESET 0x6
#define SB_READ_DATA 0xA
#define SB_READ_DATA_STATUS 0xE
#define SB_WRITE_DATA 0xC

#define SB_ENABLE_SPEAKER 0xD1
#define SB_DISABLE_SPEAKER 0xD3
#define SB_SET_PLAYBACK_FREQUENCY 0x40
#define SB_SINGLE_CYCLE_8PCM 0x14
#define SB_SINGLE_CYCLE_4ADPCM 0x74
#define SB_SINGLE_CYCLE_3ADPCM 0x76
#define SB_SINGLE_CYCLE_2ADPCM 0x16

#define MASK_REGISTER 0x0A
#define MODE_REGISTER 0x0B
#define MSB_LSB_FLIP_FLOP 0x0C
#define DMA_CHANNEL_0 0x87
#define DMA_CHANNEL_1 0x83
#define DMA_CHANNEL_3 0x82



//KEYS
#define LT_ESC				0x01
#define LT_ENTER			0x1C
#define LT_ACTION			0x1F //S
#define LT_JUMP				0x20 //D
#define LT_UP				0x48
#define LT_DOWN				0x50
#define LT_LEFT				0x4B
#define LT_RIGHT			0x4D

//SPRITE ACTIONS
#define LT_SPR_STOP			0x00
#define LT_SPR_UP			0x01
#define LT_SPR_DOWN			0x02
#define LT_SPR_LEFT			0x03
#define LT_SPR_RIGHT		0x04


/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

extern int Free_RAM;
extern int SCR_X;
extern int SCR_Y;
extern int LT_SIN[]; 
extern int LT_COS[]; 

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
extern word *my_clock;
extern word start;

extern int LT_Keys[];
void Clearkb();

extern byte LT_MODE;
extern byte LT_VIDEO_MODE;
extern byte LT_MUSIC_MODE;
extern byte LT_SCROLL_MODE;
extern byte LT_SFX_MODE;
extern byte LT_LANGUAGE;
extern byte LT_ENDLESS_SIDESCROLL;
extern byte LT_IMAGE_MODE;	//0 map; 1 image
extern byte LT_SPRITE_MODE; //0 fast, no delete; 1 slow, delete
extern int LT_Text_Speak_End;
extern word LT_Scanline;
extern word LT_tileset_width;
extern word LT_tileset_height;
extern word LT_tileset_ntiles;
extern byte LT_tileset_palette[256*3];
extern byte *LT_tileset_data;
extern word LT_map_width;
extern word LT_map_height;
extern word LT_map_ntiles;
extern word *LT_map_data;
extern byte *LT_map_collision;
extern byte LT_AI_Sprite[];
extern byte LT_AI_Stack;
extern byte LT_Sprite_Stack;
extern byte LT_Sprite_Stack_Table[];
extern int LT_PC_Speaker_Playing;

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
	word *ega_size;
} ANIMATION;

typedef struct tagSPRITEFRAME{			// structure for a sprite frame
	char *compiled_code;
} SPRITEFRAME;

typedef struct tagSPRITE{				// structure for a sprite
	word width;
	word height;
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
	byte ai_stack;	//FOR AI
	word jump;
	word jump_frame;
	word climb;
	word tile_x;
	word tile_y;
	word pos_x;	
	word pos_y;
	word last_x;
	word last_y;
	word last_last_x;
	word last_last_y;
	int mspeed_x;
	int mspeed_y;
	int speed_x;
	int speed_y;
	word s_x;		//To control speed
	word s_y;
	word id_pos;
	word fixed_sprite_number;
	word state;		//Motion
	word frame;
	word last_frame;
	word nframes;
	byte animations[64];
	word bkg_data;	//restore bkg
	word *ega_size;
	word size;
	word siz;
	word next_scanline;
	word s_delete;
	word code_size;
	word tga_sprite_data_offset[32];
	SPRITEFRAME frames[32];	
} SPRITE;

typedef struct tagIMFsong{				// structure for adlib IMF song, or MOD pattern data
	word size;
	word offset;
	byte filetype; //0 1 - imf0 imf1
	byte *sdata;
} IMFsong;

typedef struct tagLT_Sprite_State{				// structure to return collision data
	byte tile_number;	
	byte tilecol_number;	
	byte col_x;
	byte col_y;
	byte hit;
	byte jump;
	byte action;
	byte move;	//0 1 2 3 4: Stop U D L R 
	byte ground;
} LT_Sprite_State;

extern IMFsong LT_music;	
		
extern SPRITE LT_Loading_Animation;
extern SPRITE *sprite;
extern unsigned char *LT_tile_tempdata; 
extern unsigned char *LT_tile_tempdata2; 

// Engine will use these function pointers to access content files
// they default to stdio.h - re-direct if you have alternate storage (eg: WAD)
extern FILE* (_Cdecl *LT_fopen)(const char *file, const char *mode);
extern int (_Cdecl *LT_fclose)(FILE *stream);
extern int (_Cdecl *LT_fseek)(FILE *stream, long offset, int whence);
extern int (_Cdecl *LT_fgetc)(FILE *stream);
extern char *(_Cdecl *LT_fgets)(char *ptr, int size, FILE *stream);
extern int (_Cdecl *LT_fscanf)(FILE *stream, const char *format, ...);
extern size_t (_Cdecl *LT_fread)(void *ptr, size_t size, size_t count, FILE *stream);
extern long (_Cdecl *LT_ftell)(FILE *stream);

// lt_wad.c redirector
void LT_Use_WAD();

void LT_Adlib_Detect();
void LT_Check_CPU();
void LT_Setup();
void LT_Logo(char *file);
//
void LT_Set_Loading_Interrupt();
void LT_Delete_Loading_Interrupt();
void LT_Set_Scanline_Interrupt();
void LT_Delete_Scanline_Interrupt();


//EGA/VGA Hardware scroll
void LT_Update(int sprite_follow, int sprite);
void VGA_Scroll(word x, word y);
extern void (*LT_WaitVsync)();
void VGA_ClearScreen();

void LT_Init();
void LT_ExitDOS();

// GFX
void LT_Load_Image(char *file);
void LT_Load_Animation(char *file);
void LT_Set_Animation(byte speed);
void LT_Load_Font(char *file);
extern void (*LT_Print_Variable)(byte,byte,word);
void LT_Draw_Text_Box(word x, word y, byte w, byte h, byte mode, char *string);
void LT_Delete_Text_Box(word x, word y, byte w, byte h);
extern void (*LT_Load_Tiles)(char*);
void LT_unload_tileset();
void LT_Load_Map(char *file);
void LT_Set_Map(int x);
extern void (*LT_Edit_MapTile)(word,word,byte,byte);
void LT_unload_map();
extern void (*LT_Scroll_Map)(void);
void LT_Endless_SideScroll_Map(int y);

//palettes
void VGA_ClearPalette();                                                          
void set_palette(unsigned char *palette);
extern void (*LT_Fade_in)(void);
extern void (*LT_Fade_out)(void);
void LT_Cycle_palette(byte palnum, byte speed);
void LT_Parallax();
void LT_Cycle_Palette_EGA(byte *p,byte *c,byte speed);

//SPRITE
void LT_Load_Sprite(char *file,int sprite_number,byte *animations);
void LT_Clone_Sprite(int sprite_number_c,int sprite_number);
void LT_Init_Sprite(int sprite_number,int x,int y);
void LT_Set_Sprite_Animation(int sprite_number, byte anim);
void LT_Set_Sprite_Animation_Speed(int sprite_number,byte speed);
void LT_Sprite_Stop_Animation(int sprite_number);
extern void (*LT_Draw_Sprites)(void);
extern void (*LT_Draw_Sprites_Fast)(void);
void LT_Sprite_Edit_Map(int sprite_number, byte ntile, byte col);

void LT_Set_AI_Sprites(byte first_type, byte second_type, byte mode_0, byte mode_1);
LT_Sprite_State LT_move_player(int sprite_number);
//LT_Sprite_State LT_Bounce_Ball(int sprite_number);
int LT_Player_Col_Enemy();
void LT_Reset_Sprite_Stack();
void LT_Unload_Sprites();
void LT_scroll_follow(int sprite_number);

//ADLIB / Tandy
extern void (*LT_Load_Music)(char*);
extern void (*LT_Play_Music)(void);
void LT_Stop_Music();
void LT_Unload_Music();
void LT_Play_AdLib_SFX(unsigned char *ins, byte chan, byte octave, byte note);

//PC SPEAKER
#define PIT_FREQ 0x1234DD;
void LT_Play_PC_Speaker_SFX(int *note_array);
