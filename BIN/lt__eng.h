/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/
	
	Sprite compiler and bliter from xlib

	Please feel free to copy this source code.                            

##########################################################################*/

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <alloc.h>

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


extern int Free_RAM;
extern int SCR_X;
extern int SCR_Y;
extern char far LT_SIN[]; 
extern char far LT_COS[]; 

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
extern word *my_clock;
extern word start;

extern byte LT_Keys[];
void Clearkb();

extern byte LT_MODE;
extern byte LT_EGA_SPRITES_TRANSLUCENT;
extern byte LT_EGA_TEXT_TRANSLUCENT;
extern byte LT_MUSIC_ENABLE;
extern byte LT_ENEMY_DIST_X;
extern byte LT_ENEMY_DIST_Y;
extern byte LT_SFX_MODE;
extern byte LT_LANGUAGE;
extern byte LT_ENDLESS_SIDESCROLL;
extern byte LT_IMAGE_MODE;	//0 map; 1 image
extern byte LT_SPRITE_MODE; //0 fast, no delete; 1 slow, delete


typedef struct tagSPRITEFRAME{			// structure for a sprite frame
	char far *compiled_code;
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

#define SPR_TILP_NUM 0	
#define SPR_TILC_NUM 1	
#define SPR_COL_X	 2
#define SPR_COL_Y	 3
#define SPR_HIT		 4
#define SPR_JUMP	 5
#define SPR_ACT		 6
#define SPR_MOVE	 7	//0 1 2 3 4 => Stop U D L R 
#define SPR_GND		 8
extern byte LT_Player_State[];

extern SPRITE far *sprite;

void LT_Adlib_Detect();
void LT_Check_CPU();
void LT_Setup();
void LT_Logo(char *file, char *dat_string);
void LT_Start_Loading();
void LT_End_Loading();
void LT_Read_Joystick();
void LT_memset(void *ptr, byte val, word number);

//EGA/VGA Hardware scroll
void LT_Update(int sprite_follow, int sprite);
void LT_Wait(byte seconds);
void LT_sleep(byte seconds);
void VGA_ClearScreen();

//SYS
void LT_ExitDOS();

// GFX
byte LT_GET_VIDEO();
void LT_Load_Image(char *file, char *dat_string);
void LT_Load_Animation(char *file, char *dat_string);
void LT_Set_Animation(byte speed);
void LT_Load_Font(char *file, char *dat_string);
extern void (*LT_Print_Variable)(byte,byte,word);
byte LT_Draw_Text_Box(word x, word y, byte w, byte h, word mode, byte key1, byte key2, char* text);
void LT_Load_Tiles(char *file, char *dat_string);
void LT_Load_Map(char *file, char *dat_string);
void LT_Set_Map(int x);
extern void (*LT_Edit_MapTile)(word,word,byte,byte);
extern void (*LT_Scroll_Map)(void);
void LT_Endless_SideScroll_Map(int y);
void LT_Screen_Shake(byte mode);

//palettes
void VGA_ClearPalette();                                                          
void set_palette(unsigned char *palette);
extern void (*LT_Fade_in)(void);
extern void (*LT_Fade_out)(void);
void LT_Cycle_palette(byte palnum, byte speed);
void LT_Parallax();
void LT_Cycle_Palette_TGA_EGA(byte *p,byte *c,byte speed);
void LT_Easter_Egg();

//SPRITE
void LT_Load_Sprite(char *file, char *dat_string,int sprite_number,byte *animations);
void LT_Clone_Sprite(int sprite_number_c,int sprite_number);
void LT_Init_Sprite(int sprite_number,int x,int y);
void LT_Set_Sprite_Animation(int sprite_number, byte anim);
void LT_Set_Sprite_Animation_Speed(int sprite_number,byte speed);
void LT_Sprite_Stop_Animation(int sprite_number);
extern void (*LT_Draw_Sprites)(void);
extern void (*LT_Draw_Sprites_Fast)(void);
void LT_Sprite_Edit_Map(int sprite_number, byte ntile, byte col);

void LT_Set_AI_Sprites(byte first_type, byte second_type, byte mode_0, byte mode_1);
void LT_move_player(int sprite_number);
//LT_Sprite_State LT_Bounce_Ball(int sprite_number);
int LT_Player_Col_Enemy();
void LT_Reset_Sprite_Stack();
void LT_Unload_Sprites();
void LT_scroll_follow(int sprite_number);

//ADLIB / Tandy
extern void (*LT_Load_Music)(char*,char*);
extern void (*LT_Play_Music)(void);
void LT_Stop_Music();
void LT_Play_AdLib_SFX(unsigned char *ins, byte chan, byte octave, byte note);
byte LT_GET_MUSIC();

//PC SPEAKER
#define PIT_FREQ 0x1234DD;
void LT_Play_PC_Speaker_SFX(byte *note_array);
void LT_Disable_Speaker();

