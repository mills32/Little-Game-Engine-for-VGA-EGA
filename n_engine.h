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
#include <mem.h>
#undef outp
#define VIDEO_INT           0x10      // the BIOS video interrupt.
#define SET_MODE            0x00      // BIOS func to set the video mode.
#define VGA_256_COLOR_MODE  0x13      // use to set 256-color mode. 
#define TEXT_MODE           0x03      // use to set 80x25 text mode. 

#define SCREEN_WIDTH        320       // width in pixels of mode 0x13
#define SCREEN_HEIGHT       200       // height in pixels of mode 0x13
#define NUM_COLORS          256       // number of colors in mode 0x13

#define MISC_OUTPUT         0x03c2    // VGA misc. output register

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

#define ADLIB_PORT 			0x388

/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

extern int SCR_X;
extern int SCR_Y;
extern float t1,t2; //debug
extern int SIN[]; 

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
extern word *my_clock;
extern word start;

typedef struct tagCOLORCYCLE			/* the structure for a map. */
{
	byte frame;
	byte counter;
	byte *palette;
} COLORCYCLE;

typedef struct tagMAP					/* the structure for a map. */
{
	word width;
	word height;
	word ntiles;
	byte *data;
} MAP;

typedef struct tagTILE					/* the structure for a bitmap. */
{
	word width;
	word height;
	word ntiles;
	byte palette[256*3];
	byte *tdata;
} TILE;

typedef struct tagSPRITEFRAME				/* the structure for a sprite. */
{
	byte *rle_data;
} SPRITEFRAME;

typedef struct tagSPRITE				/* the structure for a sprite. */
{
	word width;
	word height;
	byte palette[256*3];
	byte init;
	word pos_x;
	word pos_y;
	word last_x;
	word last_y;
	byte frame;
	byte base_frame;
	byte nframes;
	byte *bkg_data;
	SPRITEFRAME *rle_frames;	
} SPRITE;

typedef struct tagIMFsong{
	long size;
	long offset;
	byte filetype; //0 1 - imf0 imf1
	byte *sdata;
} IMFsong;

extern IMFsong song;	//just one song in memory at a time

void check_hardware();
void ADLIB_Detect();

int read_keys();

/*MCGA/VGA Hardware scroll*/
void MCGA_Scroll(word x, word y);
void MCGA_WaitVBL();

void MCGA_ClearScreen();

void set_mode(byte mode);
void reset_mode(byte mode);

/* load plain bmp */
void load_plain_bmp(char *file,TILE *b);

/* load_16x16 tiles */
void load_tiles(char *file,TILE *b);
void unload_tiles(TILE *t);

//Load tiled TMX map in XML format
void load_map(char *file, MAP *map);
void set_map(MAP map, TILE *t, int x, int y);
void draw_map_column(MAP map, TILE *t, word x, word y, word map_offset);
void draw_map_row(MAP map, TILE *t, word x, word y, word map_offset);
void unload_map(MAP *map);

//update screen
void scroll_map(MAP map, TILE *t, int dir);

void load_sprite(char *file,SPRITE *s, byte size);
void draw_sprite(SPRITE *b, word x, word y, byte frame);
void unload_sprite(SPRITE *s);

//set_palette                                                           
void set_palette(unsigned char *palette);
void cycle_init(COLORCYCLE *cycle,unsigned char *palette);
void cycle_palette(COLORCYCLE *cycle, byte speed);

/* draw_bitmap */
void draw_plain_bitmap(TILE *bmp, word x, word y);

/*wait*/
void wait(word ticks);

void opl2_clear(void);

void set_timer(word freq_div);

void reset_timer();

void Load_Song(char *fname);

void unload_song(IMFsong *song);