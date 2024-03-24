#include "lt__eng.h"

#define MISC_OUTPUT         0x03c2    // VGA misc. output register
#define S_MEMORY_MODE		0x03C4	
#define INPUT_STATUS_0		0x03da
#define AC_MODE_CONTROL		0x10	  //Index of Mode COntrol register in AC
#define AC_INDEX			0x03c0	  //Attribute controller index register
#define CRTC_INDEX			0x03d4	  // CRT Controller Index
  
#define SC_INDEX            0x03c4    // VGA sequence controller
#define SC_DATA             0x03c5

#define GC_INDEX            0x03ce    // VGA graphics controller
#define GC_DATA             0x03cf

#define MAP_MASK            0x02
#define ALL_PLANES          0xff02
 
 
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
byte LT_SCROLL_MODE = 0;//0 original; 1 alternative; 2 for fast PC
byte LT_DETECTED_CARD = 0;
byte LT_MUSIC_MODE = 0;//1 ADLIB; 0 TANDY
byte LT_MUSIC_ENABLE = 1;// disable-enable
byte LT_SFX_MODE = 0;
byte LT_LANGUAGE = 0;


byte LT_ENDLESS_SIDESCROLL = 0;
byte LT_IMAGE_MODE = 0;	//0 map; 1 image
byte LT_SPRITE_MODE = 0; //0 fast, no delete; 1 slow, delete

extern byte *VGA;
extern byte *CGA;
extern word TILE_VRAM;
extern word FONT_VRAM;
word LT_setupfile;

// define LT_old_..._handler to be a pointer to a function
void interrupt (*LT_old_time_handler)();
void LT_vsync();

void LT_Fade_in_VGA();
void LT_Fade_in_EGA();
void LT_Fade_in_TGA();
void LT_Fade_out_VGA();
void LT_Fade_out_EGA();
void LT_Fade_out_TGA();
void LT_Load_Tiles_EGA_VGA();
void LT_WaitVsync_VGA();
void CalibrateMaxHblankLength();
void LT_WaitVsync_VGA_Compatible();
void LT_WaitVsync_VGA_crappy();
void LT_WaitVsync_TGA();
void LT_Scroll_Map_GFX();
void LT_Scroll_Map_TGA();
extern void (*LT_Draw_Sprites)();
extern void (*LT_Draw_Sprites_Fast)();
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
extern unsigned char far *LT_sprite_data; 
extern unsigned char far *tandy_bkg_data; 
extern unsigned char far *LT_tile_tempdata; 
extern unsigned char far *LT_tile_tempdata2; 
extern byte far *sprite_id_table;
extern word far *LT_map_data;
extern byte far *LT_map_collision;
//Variable functions
void LT_Print_VGA(word x, word y, word w, char *string);
void LT_Print_EGA(word x, word y, word w, char *string);
void LT_Print_TGA(word x, word y, word w, char *string);
extern unsigned char far *LT_CGA_TGA_FONT;
void LT_Edit_MapTile_VGA(word x, word y, byte ntile, byte col);
void LT_Edit_MapTile_EGA(word x, word y, byte ntile, byte col);
void LT_Edit_MapTile_TGA(word x, word y, byte ntile, byte col);
extern void (*LT_Print)(word,word,word,char*);
void LT_Print_Variable_VGA(byte x, byte y, word var);
extern void (*LT_Print_Variable)(byte,byte,word);
void Check_Graphics_Card();

void draw_map_column_vga(word x, word y, word map_offset, word ntiles);
void draw_map_column_ega(word x, word y, word map_offset, word ntiles);
void draw_map_column_tga(word x, word y, word map_offset, word ntiles);
extern void (*draw_map_column)(word, word, word, word);
extern void LT_vsync();
extern void (*LT_WaitVsync)();
byte LT_Wait_kbhit();
unsigned char far *LT_data;

typedef struct tagIMFsong{				// structure for adlib IMF song, or MOD pattern data
	word size;
	word offset;
	byte filetype; //0 1 - imf0 imf1
	byte far *sdata;
} IMFsong;

extern IMFsong LT_music;	

byte far LT_setup_data[] = {"\
#SETUP---------\n\
VIDEO=0        \n\
SCROLL=0       \n\
MUSIC=3        \n\
SFX=0          \n\
JOYX=AABB      \n\
JOYY=AABB0     \n\
LANG=0         \n\
#SAVE----------\n\
LEV_A=0LIV_A=0 \n\
LEV_B=0LIV_B=0 $"
};

extern unsigned char LT_JOYSTICK;
extern unsigned short LT_JOY0_DEAD_X0;
extern unsigned short LT_JOY0_DEAD_X1;
extern unsigned short LT_JOY0_DEAD_Y0;
extern unsigned short LT_JOY0_DEAD_Y1;

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
	//8x8
	0x6A80,0x6A90,0x6AA0,0x6AB0,0x6AC0,0x6AD0, 0x6AE0,0x6AF0, //32-24 = 8
	//16x16
	0x6B00,0x6B30,0x6B60,0x6B90,0x6BC0,0x6BF0, 0x6C20,0x6C50, //96-36 = 60
	//32x32
	0x6C80,0x6D20,0x6DC0,0x6E60,
};


/////////////////////////////////////////////////////
//I defined my own functions to avoid using any libs
//Most of them are bios/dos functions
/////////////////////////////////////////////////////

void _printf(byte *text){
	asm mov ah,0x09
	asm lds dx,text
	asm int 21h
}

void LT_Text_Mode(){
	if (LT_DETECTED_CARD == 3){
		outportb(0x3D4, 0x0A);
		outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 14);
		outportb(0x3D4, 0x0B);
		outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 15);
	}
	if (LT_VIDEO_MODE == 3){
		LT_WaitVsync_TGA();//If you don't do this, there is weird behaviour on dos
		asm mov dx,03DFh
		asm mov al,0x00
		asm out dx,al
	}
	asm mov ax,0x0003
	asm int 0x10
}

void LT_Exit(){
	asm mov	ax,0x4C00
	asm int	0x21
}

void LT_memset(void *ptr, byte val, word number){
	void *data = ptr;
	asm push es
	asm push di
	asm mov cx,number
	asm les di,data
	asm mov al,val
	asm rep stosb	//ax to es:di
	asm pop di
	asm pop es
}

void LT_Error(char *error, char *file);

void LT_free(void far* ptr) {
    if (ptr) {
		word segment = FP_SEG(ptr);
		asm mov ax,segment
		asm mov es,ax
		asm mov ax,0x4900
		asm int 21h
		asm jc _error:
		/// in both Microsoft C and Turbo C, far* returned in DX:AX
		return;
		_error:
		return;
	}
}

/*
typedef struct {
	byte type; // 'M'=in chain; 'Z'=at end
	word owner; // PSP of the owner
	word size; // in 16-byte paragraphs
	byte unusedC33;
	byte dos4C8T;
} MCB;

MCB far *get_mcb(void){
	asm mov ah, 52h
	asm int 21h
	asm mov dx, es:[bx-2]
	asm xor ax, ax
	/// in both Microsoft C and Turbo C, far* returned in DX:AX
}

MCB far *Get_MCBs(){
	MCB far *mcb = get_mcb();
	MCB far *last_mcb;
	for (;;){
		//printf("\n%04X %04X\n",FP_SEG(mcb),mcb->size);
		switch (mcb->type){
			case 'M' : // Mark : belongs to MCB chain
				last_mcb = mcb;
				mcb = MK_FP(FP_SEG(mcb) + mcb->size + 1, 0);
			break;
			case 'Z' : // Zbikowski : end of MCB chain
				return last_mcb;
			default :
				LT_Error("MCB corrupted\n\r$",0);
		}
	}
}

void far *LT_calloc(word blocks){
	void far *pointer = farcalloc(blocks<<4,1);
	if (!pointer)LT_Error("not enough ram$",0);
	return pointer;
}

void far *my_malloc(unsigned long numbytes) { 
    unsigned long *cur_loc; 
    MCB *cur_loc_mcb; 
	unsigned long *pointer; 
	unsigned long *last_valid_address = (unsigned long*)sbrk(0); //grab the last valid address from the OS
	unsigned long *mem_start = last_valid_address; 
	
	numbytes = numbytes + 16; 
	pointer = 0; 
	cur_loc = mem_start; //Begin searching at the start of managed memory 
	//Keep going until we have searched all allocated space
	while(cur_loc != last_valid_address) { 
		//cur_loc_mcb use it as a struct. 
		//cur_loc is a void pointer to calculate addresses. 
		cur_loc_mcb = (MCB *)cur_loc; 
		if(!cur_loc_mcb->owner){
			if(cur_loc_mcb->size >= numbytes) { 
				// We've found a location
				cur_loc_mcb->owner = 3; 
				pointer = cur_loc; 
				break; 
			} 
		} 
		//current block not suitable, move to the next one 
		cur_loc = (unsigned long*) (cur_loc + cur_loc_mcb->size); 
	}
	//we still don't have a valid location, ask the operating system for more memory 
	if(! pointer) { 
		// Move the program break numbytes further 
		sbrk(numbytes); 
		// The new memory will be where the last valid address left off 
		pointer = last_valid_address; 
		// move the last valid address forward numbytes 
		last_valid_address = last_valid_address + numbytes; 
		// We need to initialize the mem_control_block
		cur_loc_mcb = (MCB *) pointer; 
		cur_loc_mcb->owner = 3; 
		cur_loc_mcb->size = numbytes>>4; 
	} 
	pointer = pointer + 16; 
	return pointer; 
}*/

void far *LT_calloc0(word para){
	word addr = 0;
    asm mov ah,48h
    asm mov bx,para
    asm int 21h
    asm jc    _error
    asm mov addr,ax
    return (void far *) MK_FP(addr,0);
    _error:
	//if AX = 8, BX = free paragraphs
    return 0;
}

byte LT_strlen(char *st1);
void LT_Error(char *error, char *file){
	byte len = LT_strlen(file);
	file[len] = '$';
	LT_reset_key_handler();
	LT_Stop_Music();
	LT_Disable_Speaker();
	LT_Fade_out();
	LT_Text_Mode();
	//system("mem");
	LT_free(LT_data);
	_printf(error);
	_printf(file);
	LT_sleep(2);
	LT_Exit();
}

word LT_fcreate(unsigned char *file){
	unsigned char *path = file;
	word file_handle = 0;
	asm push ds
	asm mov ah,3Ch		//create file
	asm mov cx,0x04 	//0000 0100 = archive
	asm lds dx,path		//filename to create
	asm int 21h
	asm jc _error
	asm mov file_handle,ax
	return file_handle;
	_error:
	return 0;
}

word LT_fopen(unsigned char *file, byte mode){
	word file_handle = 0;
	asm push ds
	asm mov ah,3Dh		//open file
	asm mov al,mode 	//read only 00, w 01, rw 02
	asm lds dx,file		//filename to open
	asm int 21h
	asm mov file_handle,ax
	asm pop ds
	asm cmp file_handle,2
	asm jz _no_file
	return file_handle;
	_no_file:
	return 0;
}

void LT_fread(word file_handle,word bytes,void *buffer){
	asm push ds
	asm mov ah,3Fh
	asm mov bx,file_handle
	asm mov cx,bytes
	asm mov dx,word ptr buffer
	asm mov ds,word ptr [buffer+2]
	asm int 21h
	asm pop ds
}

void LT_fwrite(word file_handle,word bytes,unsigned char *buffer){
	unsigned char *write_buffer = buffer;
	asm push ds
	asm mov ah,40h
	asm mov cx,bytes
	asm mov bx,file_handle
	asm lds dx,write_buffer
	asm int 21h
	asm pop ds
}

unsigned long LT_fseek(word file_handle,unsigned long bytes,byte origin){
	unsigned long file_pos = 0;
	asm mov ah,42h
	asm mov al,origin //(0,1,2 SEEK_SET SEEK_CUR SEEK_END)
	asm mov bx,file_handle
	asm mov cx,word ptr [bytes+2]
	asm mov dx,word ptr bytes
	asm int 21h
	asm jc    _error
	asm mov word ptr [file_pos],ax
	asm mov word ptr [file_pos+2],dx//new pointer location if CF not set
	return file_pos;
	_error:
	return 0;
}

void LT_fclose(word file_handle){
	asm mov ah,0x3E
	asm mov bx,file_handle		//filename to close
	asm int 21h
}

byte LT_stricmp(char *st1, char *st2){
	byte i = 0;
	while(1){
		byte a = st1[i];byte b = st2[i];
		if (i && !a && !b) return 0;
		//to lower case if required
		if ((a>64)&&(a<91)) a+=32;
		if ((b>64)&&(b<91)) b+=32;
		//compare
		if (a != b) return 1;
		if (i == 15) return 0;
		i++;
	}
	//return 1;
}

byte LT_strlen(char *st1){
	byte i = 0;
	while(1){
		byte a = st1[i];
		if(!a) return i;
		i++;
	}
}


void interrupt (far * LT_getvect(byte intr))(){
	word _segment = 0;
	word _offset = 0;
	asm	mov	ah, 0x35
	asm	mov	al, intr
	asm	int	0x21
	asm xchg ax,bx
	asm mov dx,es
	asm mov _segment,dx
	asm mov _offset,ax
	return( (void interrupt (*)()) ((unsigned long)MK_FP(_segment,_offset)));
}

void LT_setvect(byte intr, void interrupt (far *func)()){
	word _segment = FP_SEG(func);
	word _offset = FP_OFF(func);
	asm	push	ds
	asm	mov	ah, 0x25
	asm	mov	al, intr
	asm	mov ds,_segment; asm mov dx,_offset	
	asm	int	0x21
	asm	pop	ds
}

void LT_sleep(byte seconds){
	seconds*=60;
	while (seconds){seconds--;LT_vsync();}
}


byte LT_GET_VIDEO(){return LT_VIDEO_MODE;}
byte LT_GET_MUSIC(){return LT_MUSIC_MODE;}

/////////////////////////////////////
/////////////////////////////////////

//Seek files inside LDAT files (custom format similar to WAD)

void DAT_Seek2(word fp,char *dat_string){
	unsigned char line = 0;
	char name[17]; //name of the file inside DAT
	unsigned char data_name = 0;
	dword LDAT_Offset = 0;
	
	//Check LDAT file
	//Seek data
	LT_fseek(fp,32,SEEK_SET);
	while(data_name == 0){//read 16 byte lines 
		LT_memset(name, 0, 17);//set all = 0
		LT_fread(fp,16,name);
		if (!LT_stricmp(name,dat_string)) data_name = 1;
		else LT_fseek(fp,16,SEEK_CUR);//advance
		line++;
		if (line == 64) {LT_fclose(fp); LT_Error("Can't find file inside DAT file: $",dat_string);}
	}
	LT_fread(fp,sizeof(LDAT_Offset),(byte*)&LDAT_Offset);//read offset of file in DAT file
	LT_fseek(fp,LDAT_Offset,SEEK_SET);
}


void Read_Setup(){
	byte buffer[0xB0];
	LT_fread(LT_setupfile,0xB0,buffer);
	LT_VIDEO_MODE = buffer[0x16]-48;
	LT_SCROLL_MODE = buffer[0x27]-48;
	LT_MUSIC_MODE = buffer[0x36]-48;
	LT_SFX_MODE = buffer[0x44]-48;
	LT_JOY0_DEAD_X0 = (unsigned short)buffer[0x55];
	LT_JOY0_DEAD_X1 = (unsigned short)buffer[0x57];
	LT_JOY0_DEAD_Y0 = (unsigned short)buffer[0x65];
	LT_JOY0_DEAD_Y1 = (unsigned short)buffer[0x67];
	LT_JOYSTICK = buffer[0x69]-48;
	LT_LANGUAGE = buffer[0x75]-48;
	
	switch (LT_SCROLL_MODE){
		case 0: LT_WaitVsync = &LT_WaitVsync_VGA; break;
		case 1: LT_WaitVsync = &LT_WaitVsync_VGA_Compatible; break;
		case 2: LT_WaitVsync = &LT_WaitVsync_VGA_crappy; break;
	}
}

void Save_Setup(){
	LT_setup_data[0x16] = (LT_VIDEO_MODE&3)+48;
	LT_setup_data[0x27] = LT_SCROLL_MODE+48;
	LT_setup_data[0x36] = LT_MUSIC_MODE+48;
	LT_setup_data[0x44] = LT_SFX_MODE+48;
	LT_setup_data[0x55] = LT_JOY0_DEAD_X0;
	LT_setup_data[0x57] = LT_JOY0_DEAD_X1;
	LT_setup_data[0x65] = LT_JOY0_DEAD_Y0;
	LT_setup_data[0x67] = LT_JOY0_DEAD_Y1;
	LT_setup_data[0x69] = LT_JOYSTICK+48;
	LT_setup_data[0x75] = LT_LANGUAGE+48;
	LT_setup_data[0x16] = (LT_VIDEO_MODE&3)+48;
	LT_fseek(LT_setupfile,0,0);
	LT_fwrite(LT_setupfile,0xB0,LT_setup_data);
}


void CGA_Mode(byte pal_number, byte bkg_color){
	byte mode = 4;
	//PAL:  00h/10h = mode 4 pal0 dark/light BKG-RED-GREEN-YELLOW
	//      20h/30h = mode 4 pal1 dark/light BKG-CYAN-MAGENTA-WHITE
	//      40h/50h = mode 5 pal2 dark/light BKG-RED-CYAN-WHITE
	pal_number = pal_number <<4;
	if (pal_number > 0x30) {mode = 5;pal_number -= 0x40;}
	//CGA MODE
	asm mov ah,0x00
	asm mov al,mode
	asm int 0x10
	
	//CGA PALETTE
	asm mov dx,0x3D9
	asm mov al,pal_number
	asm add al,bkg_color
	asm out dx,al
}

void LT_Calibrate_JoyStick();

extern int LT_PC_Speaker_Size;
void LT_Setup(){
	byte character = 0;
	LT_Text_Mode();//Clear screen
	
	//Check_Ram();
	Check_Graphics_Card();
	LT_Adlib_Detect();
	LT_setup_data[0x16] = LT_DETECTED_CARD+48;
	LT_setupfile = LT_fopen("config.txt",0x02);
	if (!LT_setupfile) {
		_printf("\n\rconfig.txt is missing\n\rNew default file created\n\r$");
		LT_setupfile = LT_fcreate("config.txt");
		LT_fclose(LT_setupfile);
		LT_setupfile = LT_fopen("config.txt",0x01);
		LT_fwrite(LT_setupfile,0xB0,LT_setup_data);
		LT_fclose(LT_setupfile);
		LT_sleep(2);
		LT_setupfile = LT_fopen("config.txt",0x02);
		LT_Text_Mode();
	}
	Read_Setup();
	
	_printf("\n\rRun setup? - Configurar? (Y/N)\n\r$");
	asm in al, 61h; asm and al, 252; asm out 61h, al //Disable speaker

	character = LT_Wait_kbhit();
	Clearkb();
	if ((character == 0x59) || (character == 0x79)){//Y y
		if (LT_DETECTED_CARD == 1){
			_printf("\n\n\rSelect video mode - Selecciona modo de video:\n\r\t> 0 = VGA 16 col (8088 4.77)\n\r\t> 1 = VGA 256 col$");
			character = LT_Wait_kbhit()- 48;
			if ( character > 1 )character = 1;
			LT_VIDEO_MODE = character;
			Clearkb();
			_printf("\n\r\nSet hardware scroll - Configura el scroll hardware:$");
			_printf("\n\r\t> 0 = original VGA$");
			_printf("\n\r\t> 1 = clon VGA (more compatible - mas compatible)$");
			_printf("\n\r\t> 2 = for fast PC - para PC muy rapidos$");
			character = LT_Wait_kbhit()- 48;
			if (character  > 2) character = 2;
			if (character == 0) LT_WaitVsync = &LT_WaitVsync_VGA;
			if (character == 1) LT_WaitVsync = &LT_WaitVsync_VGA_Compatible;
			if (character == 2) LT_WaitVsync = &LT_WaitVsync_VGA_crappy;
			LT_SCROLL_MODE = character;
		}
		if (LT_DETECTED_CARD == 0) {LT_VIDEO_MODE = 0; LT_SCROLL_MODE = 0; LT_WaitVsync = &LT_WaitVsync_VGA;}//EGA
		if (LT_DETECTED_CARD == 2) {
			_printf("\n\n\rSelect video card - Selecciona tarjeta de video:\n\r\t> 2 = CGA, 4 col\n\r\t> 3 = TANDY, 16 col$");
			character = LT_Wait_kbhit()- 48;
			character -= 48;
			if ((character!=2)&&(character!=3)) character = 3;
			LT_VIDEO_MODE = character;
			Clearkb();
			if (LT_VIDEO_MODE == 3) LT_WaitVsync = &LT_WaitVsync_TGA;
		}
	
		_printf("\n\n\rSelect sound card - Selecciona tarjeta de sonido:\n\r\t> 0 = NONE\n\r\t> 1 = TANDY 3 voice\n\r\t> 2 = ADLIB$");
		character = LT_Wait_kbhit()- 48;
		if (character  > 2) LT_MUSIC_MODE = 1;
		if (character == 2) LT_MUSIC_MODE = 1;
		if (character == 1) LT_MUSIC_MODE = 0;
		if (character == 0) LT_MUSIC_MODE = 3;
		Clearkb();
		_printf("\n\n\rSelect sound effect mode - Selecciona modo de efectos de sonido:\n\r\t> 0 = PC Speaker\n\r\t> 1 = ADLIB$");
		character = LT_Wait_kbhit()- 48;
		if ( character > 1 ) character = 1;
		LT_SFX_MODE = character;
		//JOYSTICK
		LT_JOYSTICK = 0;
		_printf("\n\rControl:\n\r\t> 0 = keyboard / teclado\n\r\t> 1 = Joystick$");
		character = LT_Wait_kbhit()- 48;
		Clearkb();
		if (character == 1) {LT_Calibrate_JoyStick();}

		//SAVE SETTINGS
		Save_Setup();
	}
	LT_fclose(LT_setupfile);
	/*if (LT_VIDEO_MODE == 3){
		word RAM = 0;
		asm int 12h; asm mov RAM,ax; //number of contiguous 1k memory blocks found at startup
		if (RAM < 320+64) LT_Error ("TANDY RAM error: requires / requiere 384K",0);
	}*/

    LT_init();
}

void LT_Null(){//Debug
	int i = 0;
	i = i+1;
}

void LT_ClearScreen();

extern int TGA_SCR_X;
void LT_Init(){
	word RAM = 0;
	byte *check;
	word Min_Ram = 0;
	word data_seg; word data_off;
	int square_pixels = 1;
	union REGS regs;
	word i;
	word *sprite_bkg;
	byte sprite_number;

	if (LT_VIDEO_MODE == 0){//EGA 320x200@60  16 colors
		LT_Fade_in = &LT_Fade_in_EGA;
		LT_Fade_out = &LT_Fade_out_EGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_EGA_VGA;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_EGA_VGA;
		LT_sprite_data_offset = 512;
		
		LT_Fade_out();
		//SET MODE
		asm mov ax,0x000D;asm int 0x10; //EGA/VGA 320x200x16 
		LT_memset(VGA,0x00,(320*200)/8); 
		
		if (LT_DETECTED_CARD){//if VGA
			asm cli
			asm mov dx,CRTC_INDEX
			asm mov ax,0x0011; asm out dx,ax; // Turn off write protect to CRTC registers
			//Set VGA so that we have 60Hz (320x240), but leave screen size at 320x200
			asm mov ax,0x0B06; asm out dx,ax; // New vertical total=525 lines, bits 0-7
			asm mov ax,0x3E07; asm out dx,ax; // New vertical total=525 lines, bits 8-9
			if (square_pixels){
				asm mov ax,0xB910; asm out dx,ax; // Vsync start=scanline 185
				asm mov ax,0x8F12; asm out dx,ax; // Vertical display end=scanline 399, bits 0-7
				asm mov ax,0xB815; asm out dx,ax; // Vertical blanking start=scanline 440, bits 0-7
				asm mov ax,0xE216; asm out dx,ax; // Adjust vblank end position
				asm mov ax,0x8511; asm out dx,ax; // Vsync length=2 lines + turn write protect back on
			} else {
				asm mov ax,0x0B16; asm out dx,ax; // Adjust vblank end position=scanline 524
				asm mov ax,0xD710; asm out dx,ax; // Vsync start=scanline 215
				asm mov ax,0x8911; asm out dx,ax; // Vsync length=2 lines + turn write protect back on
			}
			asm sti
			CalibrateMaxHblankLength();
		}
		asm mov dx,CRTC_INDEX
		asm mov ax,0x1613; asm out dx,ax; // Logical width 22
		
		LT_Print = &LT_Print_EGA;
		LT_Print_Variable = &LT_Null;
		draw_map_column = &draw_map_column_ega;
		LT_Edit_MapTile = &LT_Edit_MapTile_EGA;
		LT_Scroll_Map = &LT_Scroll_Map_GFX;
		sprite_bkg = &spr_EGA[0];
		
		LT_VRAM_Logical_Width = 44;
		FONT_VRAM = 0x6F00;
		TILE_VRAM = 0x7100;
		sprite_size = 3;
	}	
	if (LT_VIDEO_MODE == 1){//VGA 320x200@60 256 colors MODE X
		LT_Fade_in = &LT_Fade_in_VGA;
		LT_Fade_out = &LT_Fade_out_VGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_EGA_VGA;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_EGA_VGA;
		LT_sprite_data_offset = 16*1024;
		LT_Fade_out();
		VGA_ClearPalette();
		LT_memset(VGA,0x00,(320*200)/4); 
		
		//SET MODE X
		asm mov ax,0x0013;asm int 0x10;
		
		asm mov dx,SC_INDEX
		asm mov al,0x04; asm out dx,al;// turn off chain-4 mode
		asm mov dx,SC_DATA
		asm mov al,0x06; asm out dx,al;
		
		asm mov dx,CRTC_INDEX
		asm mov ax,0x0014; asm out dx,ax;// turn off long mode
		asm mov ax,0xe317; asm out dx,ax;// turn on byte mode
		//Set VGA so that we have 60Hz (320x240), but leave screen size at 320x200
		asm cli
		asm mov ax,0x0011; asm out dx,ax;// Turn off write protect to CRTC registers
		asm mov ax,0x0B06; asm out dx,ax;// New vertical total=525 lines, bits 0-7
		asm mov ax,0x3E07; asm out dx,ax;// New vertical total=525 lines, bits 8-9
		if (square_pixels){
			asm mov ax,0xB910; asm out dx,ax;/// Vsync start=scanline 185
			asm mov ax,0x8F12; asm out dx,ax;/// Vertical display end=scanline 399, bits 0-7
			asm mov ax,0xB815; asm out dx,ax;/// Vertical blanking start=scanline 440, bits 0-7
			asm mov ax,0xE216; asm out dx,ax;/// Adjust vblank end position
			asm mov ax,0x8511; asm out dx,ax;/// Vsync length=2 lines + turn write protect back on
		} else {
			asm mov ax,0x0B16; asm out dx,ax;/// Adjust vblank end position=scanline 524
			asm mov ax,0xD710; asm out dx,ax;/// Vsync start=scanline 215
			asm mov ax,0x8911; asm out dx,ax;/// Vsync length=2 lines + turn write protect back on
		}
		asm mov dx,GC_INDEX
		asm mov ax,0x0106; asm out dx,ax;/// Select VRAM B8000h-BFFFFh; Chain O/E OFF; Text mode OFF 
		asm sti
		
		CalibrateMaxHblankLength();
		asm mov dx,CRTC_INDEX
		asm mov ax,0x2C13; asm out dx,ax;// width in 8 pixel "characters" (44)
		
		LT_Print = &LT_Print_VGA;
		LT_Print_Variable = &LT_Print_Variable_VGA;
		draw_map_column = &draw_map_column_vga;
		LT_Edit_MapTile = &LT_Edit_MapTile_VGA;
		LT_Scroll_Map = &LT_Scroll_Map_GFX;
		sprite_bkg = &spr_VGA[0];
		
		LT_VRAM_Logical_Width = 88; //width in 4 pixel chunks
		FONT_VRAM = 0xDCC0;
		TILE_VRAM = 0xE0C0;
		sprite_size = 2;
	}
	if (LT_VIDEO_MODE == 3){
		LT_WaitVsync = &LT_WaitVsync_TGA;
		LT_Fade_in = &LT_Fade_in_TGA;
		LT_Fade_out = &LT_Fade_out_TGA;
		LT_Draw_Sprites = &LT_Draw_Sprites_Tandy;
		LT_Draw_Sprites_Fast = &LT_Draw_Sprites_Fast_Tandy;
		LT_sprite_data_offset = 2048;
		LT_Fade_out();
		
		asm mov ax,0x0009;asm int 0x10; // =>320x200 (08 = 160x200)
		asm mov dx,03DFh;asm mov al,0xC6;asm out dx,al;//show page 6
		
		LT_Print = &LT_Print_TGA;
		LT_Print_Variable = &LT_Null;
		draw_map_column = &draw_map_column_tga;
		LT_Edit_MapTile = &LT_Edit_MapTile_TGA;
		LT_Scroll_Map = &LT_Scroll_Map_TGA;
		
		FONT_VRAM = 0x00;
		TILE_VRAM = 0x00;
		sprite_size = 2;
	}
	if (LT_VIDEO_MODE == 2){//CGA
		byte mode = 0; byte pal = 0;
		asm mov ah,0x00;asm mov al,mode;
		asm int 0x10;
		
		asm mov dx,03D9h
		asm mov al,pal;
		asm out dx,al
	}
	
	LT_ClearScreen();
	
	if (LT_MUSIC_MODE == 0){
		LT_Load_Music = &Load_Music_Tandy;
		LT_Play_Music = &Play_Music_Tandy;
	}
	if (LT_MUSIC_MODE == 1){
		LT_Load_Music = &Load_Music_Adlib;
		LT_Play_Music = &Play_Music_Adlib;
	}
	if (LT_MUSIC_MODE == 3){
		LT_Load_Music = &LT_Null;
		LT_Play_Music = &LT_Null;
	}
	
	LT_old_time_handler = LT_getvect(0x1C);
	LT_destroy_key_handler();
	
	//Allocate mem
	// Minimum 272K: (192K + EXE(64K) + user defined (16K)
	// For tandy, 320 + 64K (for VRAM)
	//LT_data = (byte far *)farcalloc((unsigned long)(271260),1);
	if (LT_VIDEO_MODE>1) Min_Ram = 0x3239; else Min_Ram = 0x29A1;
	
	if (LT_MUSIC_MODE == 1) {
		LT_data = LT_calloc0(Min_Ram +0x1000+1);//160784+44928 + 65536
		if (!LT_data) {
			LT_MUSIC_MODE = 3;
			_printf("LOW RAM: MUSIC OFF");
			LT_sleep(2);
		}
	}
	if (LT_MUSIC_MODE == 0){
		LT_data = LT_calloc0(Min_Ram +0x0800+1);//160784+44928 + 32768
		if (!LT_data) LT_Error("RAM ERROR",0);
	}
	if (LT_MUSIC_MODE == 3) LT_data = LT_calloc0(Min_Ram+1);//160784+44928
	if (!LT_data) LT_Error("RAM ERROR",0);
	
	if (LT_VIDEO_MODE==3){
		//Assume the system has a RAM config which has a complete 64KB 
		//segment at the end.
		check = LT_calloc0(0x1000);
		if (!check) LT_Error("TANDY VRAM ERROR",0);
	}
	//LT_data = (byte far *)LT_calloc0((unsigned long)(271260/16));//170512
	data_seg = FP_SEG(LT_data);data_off = FP_OFF(LT_data);
	RAM++;//Skip the MCB
	LT_tile_tempdata = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x0800;
	LT_tile_tempdata2 = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x0800;
	LT_sprite_data  = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x1000;//64KB
	sprite_id_table  = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x0130;
	sprite = (SPRITE far *) MK_FP(data_seg + RAM,data_off);RAM+=0x4E1;//22 *350 sprite structs
	LT_map_collision = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x0130;//---2741
	if (LT_VIDEO_MODE < 2){
		LT_map_data = (word far *) MK_FP(data_seg + RAM,data_off);RAM+=0x260;
	} else {//if CGA or Tandy
		LT_map_data = (word far *) MK_FP(data_seg + RAM,data_off);RAM+=0x980;
		LT_CGA_TGA_FONT = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x80;
		tandy_bkg_data = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0xF8;
	}
	//music
	if (LT_MUSIC_MODE == 0) {LT_music.sdata = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x0800;}//32KB
	if (LT_MUSIC_MODE == 1)	{LT_music.sdata = (byte far *) MK_FP(data_seg + RAM,data_off);RAM+=0x1000;}//64KB
	
	//STORE BKG AT FIXED VRAM ADDRESS (EGA/VGA)
	for (sprite_number = 0; sprite_number != 20; sprite_number++){
		sprite[sprite_number].bkg_data = sprite_bkg[sprite_number];
	}
	
	//For some reason/bug, on TANDY (PCEM/86Box and probably on real TANDY) the wait vsync function lags a lot
	//until the VRAM offset is set to 8 or higher. Then we can set 0 again
	asm mov dx,0x03D4
	asm mov ax,0x080C;asm out dx,ax
	asm mov ax,0x000C;asm out dx,ax
}


byte Game_Boy_Adlib[11] = {0xC4,0x1A,0x06,0x8E,0xA6,0xE8,0xF0,0xA0,0x00,0x00,0x02};
byte GameBoy_Sound[24] = {72,72,72,72,72,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84};
byte loading_logo = 0;
void LT_Logo(char *file, char *dat_string){
	loading_logo = 1;
	LT_PC_Speaker_Size = 24;
	//LT_Fade_out();
	LT_Load_Image(file,dat_string);
	LT_WaitVsync();
	LT_Fade_in();
	if (!LT_SFX_MODE) LT_Play_PC_Speaker_SFX(GameBoy_Sound);
	if (LT_SFX_MODE) {
		int i =8;
		LT_Play_AdLib_SFX(Game_Boy_Adlib,0,3,7);//first note
		while (i){LT_vsync();i--;}
		LT_Play_AdLib_SFX(Game_Boy_Adlib,0,4,7);//second note
	}
	LT_sleep(2);
	LT_Fade_out();
	LT_PC_Speaker_Size = 16;
	Clearkb();
}

void LT_ExitDOS(){
	LT_reset_key_handler();
	LT_Stop_Music();
	LT_Disable_Speaker();
	LT_Fade_out();
	LT_Text_Mode();
	//system("mem");
	
	LT_free(LT_data);
	LT_Exit();
}

//Pre calculated SIN and COS, Divide to use for smaller circles.
char far LT_SIN[365] = {
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

char far LT_COS[365] = {
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