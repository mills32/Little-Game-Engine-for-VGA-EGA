/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program.                     
	Remember to compile in the LARGE memory model!                        

	Please feel free to copy this source code.                            

	Used:	K1n9_Duk3's IMF Player - A simple IMF player for DOS
			Copyright (C) 2013-2016 K1n9_Duk3

			Based on the Apogee Sound System (ASS) and Wolfenstein 3-D (W3D)

			ASS is Copyright (C) 1994-1995 Apogee Software, Ltd.
			W3D is Copyright (C) 1992 Id Software, Inc.
	
##########################################################################*/

#include "n_engine.h"

//Predefined structs
/*******************
OK let's do this console style, (of course You can do whatever you want).
At any moment there will only be:
You'll have to unload a music/map/tilset before loading another.
********************/
IMFsong LT_music;	// One song in ram stored at "LT_music"
MAP LT_map;			// One map in ram stored at "LT_map"
TILE LT_tileset;	// One tileset in ram stored at "LT_tileset"
ANIMATION LT_Loading_Animation;// Animation for loading screen
unsigned char *LT_tile_datatemp; //Temp storage of non tiled data.
unsigned char *LT_sprite_tiledatatemp; //Temp storage of tiled sprites to be converted to RLE
SPRITE lt_gpnumber0, lt_gpnumber1, lt_gpnumber2; // 4 sprites for debug printing

//GLOBAL VARIABLES

//debugging
float t1 = 0;
float t2 = 0;

//This points to video memory.
byte *MCGA=(byte *)0xA0000000L; 

//timer     		
void interrupt (*LT_old_time_handler)(void); 	
long time_ctr;

//keyboard
void interrupt (*LT_old_key_handler)(void);
int LT_Keys[256];

//ADLIB
const int opl2_base = 0x388;
long next_event;
long LT_imfwait;

// this points to the 18.2hz system clock for debug in dosbox (does not work on PCEM). 
word *my_clock=(word *)0x0000046C;    		
word start;
int Free_RAM;

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 0;
int LT_current_x = 0;
int LT_last_x = 0;
int LT_current_y = 0;
int LT_last_y = 0;
int LT_map_offset = 0;
int LT_map_offset_Endless = 0;
int LT_follow = 0;
byte LT_Gravity = 0;
byte LT_SideScroll = 0;

//Player
byte tile_number = 0;		//Tile a sprite is on
byte tilecol_number = 0;	//Collision Tile a sprite is on
byte tile_number_VR = 0;	//Tile vertical right
byte tile_number_VL = 0;	//Tile vertical left
byte tile_number_HU = 0;	//Tile horizontal up
byte tile_number_HD = 0;	//Tile horizontal down

int LT_player_jump_pos[] = 
{
	-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

//Loading palette
unsigned char LT_Loading_Palette[] = {
	0x00,0x00,0x00,	//colour 0
	0xff,0xff,0xff	//colour 1
};

byte LT_font_data[4096];

unsigned char OurFont[28] = {
	    0x00,          /* 00000000b */
            0x7F,          /* 01111111b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x7F,          /* 01111111b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x00,          /* 00000000b */
            0x00,          /* 00000000b */

            0x00,          /* 00000000b */
            0x7E,          /* 01111110b */
            0x66,          /* 01100110b */
            0x66,          /* 01100110b */
            0x66,          /* 01100110b */
            0x7F,          /* 01111111b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x63,          /* 01100011b */
            0x7F,          /* 01111111b */
            0x00,          /* 00000000b */
	    0x00           /* 00000000b */
};

//end of global variables

void LT_VGA_Font(char *file){
	/*union REGS pregs;
	struct SREGS sregs;
	void far *mptr = OurFont;

	pregs.x.ax = 0x1100;
	pregs.x.dx = 0x41;       // 65  'A'
	pregs.x.cx = 0x02;       // two char's to change
	pregs.h.bh = 0x0E;
	pregs.h.bl = 0x00;
	pregs.x.bp = FP_OFF(mptr);
	sregs.es = FP_SEG(mptr);
	int86x(0x10, &pregs, &pregs, &sregs);
	*/
	return;
}

//So easy, and so difficult to find samples of this...
void interrupt LT_Loading(void){
	LT_Draw_Animation(&LT_Loading_Animation);
}

void LT_Set_Loading_Interrupt(){
	unsigned long spd = 1193182/50;
	int i;
	set_palette(LT_Loading_Animation.palette);
	//change color 0, 1, 2 (black and white)
	outp(0x03c8,0); 
	outp(0x03c9,LT_Loading_Palette[0]);
	outp(0x03c9,LT_Loading_Palette[0]);
	outp(0x03c9,LT_Loading_Palette[1]);
	
	//clear screen
	MCGA_ClearScreen();
	//Print "Loading", without deleting the animation.
	//Every character fills a 8x8 cell in the 64Kb VRAM, so:
	gotoxy(17,15);
	printf("LOADING\r");

	//center loading animation
	LT_Loading_Animation.pos_x = 138;
	LT_Loading_Animation.pos_y = 72;
	//set timer
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte*/
	//set interrupt
	setvect(0x1C, LT_Loading);		//interrupt 1C not available on NEC 9800-series PCs.
}

void LT_Delete_Loading_Interrupt(){
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, LT_old_time_handler);
}

//So easy, and so difficult to find samples of this...
void interrupt LT_Key_Handler(void){
	unsigned char temp,keyhit;
	asm{
	cli
    in    al, 060h      
    mov   keyhit, al
    in    al, 061h       
    mov   bl, al
    or    al, 080h
    out   061h, al     
    mov   al, bl
    out   061h, al      
    mov   al, 020h       
    out   020h, al
    sti
    }
	if (keyhit & 0x80){ keyhit &= 0x7F; LT_Keys[keyhit] = 0;} //KEY_RELEASED;
    else LT_Keys[keyhit] = 1; //KEY_PRESSED;
}

void LT_install_key_handler(){
	int i;
	LT_old_key_handler=getvect(9);    //save current vector
	LT_old_key_handler();
	setvect(9,LT_Key_Handler);     //install new one
	for (i = 0; i != 256; i++) LT_Keys[i] = 0;
}

void LT_reset_key_handler(){
	setvect(9,LT_old_key_handler);     
}

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

static void ADLIB_SendOutput(int reg,int data){
   int i;
   //asm CLI //disable interrupts
    
   outp(ADLIB_PORT, reg );
   for( i = 6; i ; i-- )inp( ADLIB_PORT );
   outp(ADLIB_PORT + 1, data );
   for( i = 35; i ; i-- )inp( ADLIB_PORT );
   
   //asm STI //enable interrupts
}

void LT_Adlib_Detect(){ 
    int status1, status2, i;

    ADLIB_SendOutput(4, 0x60);
    ADLIB_SendOutput(4, 0x80);

    status1 = inp(ADLIB_PORT);
    
    ADLIB_SendOutput(2, 0xFF);
    ADLIB_SendOutput(4, 0x21);

    for (i=100; i>0; i--) inp(ADLIB_PORT);

    status2 = inp(ADLIB_PORT);
    
    ADLIB_SendOutput(4, 0x60);
    ADLIB_SendOutput(4, 0x80);

    if ( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) ){
        unsigned char i;
		for (i=1; i<=0xF5; ADLIB_SendOutput(i++, 0));    //clear all registers
		ADLIB_SendOutput(1, 0x20);  // Set WSE=1
		printf("AdLib card detected.\n");
		sleep(3);
        return;
    } else {
		printf("AdLib card not detected.\n");
		sleep(3);
        return;
    }
}

//Hardware scrolling
void MCGA_Scroll(word x, word y){
	byte p[4] = {0,2,4,6};
	byte pix = x & 3; 	//pixel panning 
	x=x>>2;				//x/4
	y=(y<<6)+(y<<4);	//(y*64)+(y*16) = y*80;
	
  while ((inp(0x03da) & 0x08));
	//change pixel panning register 
	inp(0x3da);
	outp(0x3c0, 0x33);
	outp(0x3c0, p[pix]);
	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	outport(0x03d4, 0x0C | (x+y & 0xff00));
	outport(0x03d4, 0x0D | (x+y << 8));
  while (!(inp(0x03da) & 0x08));
}

void MCGA_WaitVBL(){ //This does not work well outside MCGA_Scroll
	int i;
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

void MCGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&MCGA[0],0,(320*200)/4);
	SCR_X = 0;
	SCR_Y = 0;
	MCGA_Scroll(SCR_X,SCR_Y);
}

void MCGA_SplitScreen(int line){
	MCGA_WaitVBL();
    asm{
	push    ax
    push    cx
    push    dx
	//Set the split screen scan line.
    cli		// make sure all the registers get set at once
	}
	word_out(CRTC_INDEX, LINE_COMPARE, line); // set bits 7-0 of the split screen scan line

	asm{
    mov    ah,byte ptr [line+1]
    and    ah,1
    mov    cl,4
    shl    ah,cl                      // move bit 8 of the split split screen scan line into position for the Overflow reg
    mov    al,OVERFLOW

	//The Split Screen, Overflow, and Line Compare registers all contain part of the split screen start scan line on the VGA
    out    dx,al                      //set CRTC Index reg to point to Overflow
    inc    dx                         //point to CRTC Data reg
    in     al,dx                      //get the current Overflow reg setting
    and    al,not 10h                 //turn off split screen bit 8
    or     al,ah                      //insert the new split screen bit 8 (works in any mode)
    out    dx,al                      //set the new split screen bit 8
    dec    dx                         //point to CRTC Index reg
    mov    ah,byte ptr [line+1]
    and    ah,2
    mov    cl,3
    ror    ah,cl                      // move bit 9 of the split split screen scan line into position for the Maximum Scan Line register
    mov    al,MAX_SCAN_LINE
    out    dx,al                      //set CRTC Index reg to point to Maximum Scan Line
    inc    dx                         //point to CRTC Data reg
    in     al,dx                      //get the current Maximum Scan Line setting
    and    al,not 40h                 //turn off split screen bit 9
    or     al,ah                      //insert the new split screen bit 9 (works in any mode)
    out    dx,al                      //set the new split screen bit 9

	//Turn on split screen pal pen suppression, so the split screen
	//won't be subject to pel panning as is the non split screen portion.

	mov  dx,INPUT_STATUS_0
	in   al,dx                  	//Reset the AC Index/Data toggle to index state
	mov  al,AC_MODE_CONTROL+20h 	//Bit 5 set to prevent screen blanking
	mov  dx,AC_INDEX				//Point AC to Index/Data register
	out  dx,al
	inc  dx							//Point to AC Data reg (for reads only)
	in   al,dx						//Get the current AC Mode Control reg
	or   al,20h						//Enable split scrn Pel panning suppress.
	dec  dx							//Point to AC Index/Data reg (for writes only)
	out  dx,al						//Write the new AC Mode Control setting with split screen pel panning suppression turned on	
	
    sti
    pop    dx
    pop    cx
    pop    ax
    }
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
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
	LT_Stop_Music();
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_unload_font();
	LT_reset_key_handler();
	MCGA_SplitScreen(0x0ffff);
	exit(1);
}

void fskip(FILE *fp, word num_bytes){
	int i;
	for (i=0; i<num_bytes; i++) fgetc(fp);
}

void LT_Load_BKG(char *file){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;

	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	for(index=(LT_tileset.height-1)*LT_tileset.width;index>=0;index-=LT_tileset.width)
		for(x=0;x<LT_tileset.width;x++)
			LT_tileset.tdata[index+x]=(byte)fgetc(fp);
	fclose(fp);
}

void LT_Draw_BKG(){
	dword bitmap_offset = 0;
	dword screen_offset = 0;
	int j;
	set_palette(LT_tileset.palette);
	for(j=0;j<174;j++){
		memcpy(&MCGA[screen_offset],&LT_tileset.tdata[bitmap_offset],LT_tileset.width);
		bitmap_offset+=LT_tileset.width;
		screen_offset+=320;
	}
}

//load static animations without transparency
void LT_Load_Animation(char *file,ANIMATION *s, byte size){
	FILE *fp;
	long index,offset;
	word num_colors;
	byte x,y;
	word i,j;
	byte tileX;
	byte tileY;
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 

	fskip(fp,18);
	fread(&s->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&s->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		s->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		s->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}
	
	if ((s->data = farcalloc(s->height*s->width,sizeof(byte))) == NULL){
		printf("Error allocating animation data\n");
		sleep(2);
		LT_ExitDOS();
		exit(1);		
	}
	
	for(index=(s->height-1)*s->width;index>=0;index-=s->width)
		for(x=0;x<s->width;x++)
			LT_tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;


	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			for(x=0;x<size;x++){
				memcpy(&s->data[index],&LT_tile_datatemp[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	
	s->nframes = (s->width>>4) * (s->height>>4);
	s->width = size;
	s->height = size;
	s->frame = 0;
	s->baseframe = 0;
	s->anim_speed = 0;
	s->anim_counter = 0;
}

void LT_Set_Animation(ANIMATION *s, int baseframe, int frames, int speed){
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void LT_Draw_Animation(ANIMATION *s){
	word screen_offset = (s->pos_y<<8)+(s->pos_y<<6)+s->pos_x;
	word size = s->width;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	unsigned char *data;
	byte frame;
	
	//animation
	if (s->animate == 1){
		s->frame = s->baseframe + s->anim_counter;
		if (s->anim_speed == s->speed){
			s->anim_speed = 0;
			s->anim_counter++;
			if (s->anim_counter == s->aframes) s->anim_counter = 0;
		}
		s->anim_speed++;
	}
	
	data = (unsigned char *) &s->data;
	frame = s->frame;
	asm{
		push ds
		push di
		push si
		
		lds		bx,dword ptr[data]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		ax,word ptr [frame]
		mov		cl,10
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address							
		mov 	ax,0A000h
		mov 	es,ax
		mov 	ax,size
	}
	copy_tile_a:	
	asm{
		mov 	cx,size2
		rep		movsw				
		add 	di,next_scanline
		dec 	ax
		jnz		copy_tile_a
		
		pop si
		pop di
		pop ds
	}	
}

void LT_Unload_Animation(ANIMATION *s){
	free(s->data); s->data = NULL;
}

/* load_16x16 tiles */
void load_tiles(char *file){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;

	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&LT_tileset.width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&LT_tileset.height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		LT_tileset.palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		LT_tileset.palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	for(index=(LT_tileset.height-1)*LT_tileset.width;index>=0;index-=LT_tileset.width)
		for(x=0;x<LT_tileset.width;x++)
			LT_tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<LT_tileset.height;tileY+=16){
		for (tileX=0;tileX<LT_tileset.width;tileX+=16){
			offset = (tileY*LT_tileset.width)+tileX;
			for(x=0;x<16;x++){
				memcpy(&LT_tileset.tdata[index],&LT_tile_datatemp[offset+(x*LT_tileset.width)],16);
				index+=16;
			}
		}
	}
	
	LT_tileset.ntiles = (LT_tileset.width>>4) * (LT_tileset.height>>4);
}

void LT_unload_tileset(){
	LT_tileset.width = NULL;
	LT_tileset.height = NULL;
	LT_tileset.ntiles = NULL;
	*LT_tileset.palette = NULL;
	free(LT_tileset.tdata); LT_tileset.tdata = NULL;
}

//Load tiled TMX map in CSV format
//Be sure bkg layer map is the first to be exported, (before collision layer map)
void load_map(char *file){ 
	FILE *f = fopen(file, "rb");
	word start_bkg_data = 0;
	word start_col_data = 0;
	word tile = 0;
	word index = 0;
	byte tilecount = 0; //Just to get the number of tiles to substract to collision tiles 
	char line[128];
	char name[64]; //name of the layer in TILED

	if(!f){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}
	//read file
	fseek(f, 0, SEEK_SET);			
	while(start_bkg_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 't')){ // get tilecount
			sscanf(line," <tileset firstgid=\"%i[^\"]\" name=\"%24[^\"]\" tilewidth=\"%i[^\"]\" tileheight=\"%i[^\"]\" tilecount=\"%i[^\"]\"",&tilecount,&tilecount,&tilecount,&tilecount,&tilecount);
		}
		if((line[1] == '<')&&(line[2] == 'l')){// get map dimensions
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&LT_map.width,&LT_map.height);
			start_bkg_data = 1;
		}
	}
	LT_map.ntiles = LT_map.width*LT_map.height;
	if (LT_map.ntiles > 32767){
		printf("Error, map is bigger than 32 kb");
		sleep(2);
		LT_ExitDOS();
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">

	//read tile array
	for (index = 0; index < LT_map.ntiles; index++){
		fscanf(f, "%i,", &tile);
		LT_map.data[index] = tile-1;
	}
	//skip 
	while(start_col_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 'l')){
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&LT_map.width,&LT_map.height);
			start_col_data = 1;
		}
	}
	fgets(line, 64, f); //skip line: <data encoding="csv">
	//read collision array
	for (index = 0; index < LT_map.ntiles; index++){
		fscanf(f, "%d,", &tile);
		LT_map.collision[index] = tile -tilecount;
	}
	fclose(f);
}

void LT_unload_map(){
	LT_map.width = NULL;
	LT_map.height = NULL;
	LT_map.ntiles = NULL;
	free(LT_map.data); LT_map.data = NULL;
	free(LT_map.collision); LT_map.collision = NULL;
}

void LT_Set_Map(int x, int y){
	//UNDER CONSTRUCTION
	int i = 0;
	int j = 0;
	LT_map_offset = (LT_map.width*y)+x;
	LT_map_offset_Endless = 20;
	SCR_X = x<<4;
	SCR_Y = y<<4;
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
	MCGA_Scroll(SCR_X,SCR_Y);
	//draw map 
	for (i = 0;i<320;i+=16){draw_map_column(SCR_X+i,SCR_Y,LT_map_offset+j);j++;}	
	set_palette(LT_tileset.palette);
}

void LT_Edit_MapTile(word x, word y, byte ntile){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	unsigned char *mapdata = LT_map.data;
	word screen_offset = (y<<8)+(y<<6)+x;
	asm{
		push ds
		push di
		push si
		
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		ax,LT_map_offset
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		mov		di,screen_offset		//es:di screen address							
		mov 	ax,0A000h
		mov 	es,ax
		mov 	ax,16
	}
	copy_tile:	
	asm{
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		dec 	ax
		jnz		copy_tile
		
		pop si
		pop di
		pop ds
	}
}

void draw_map_column(word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	unsigned char *mapdata = LT_map.data;
	word tile_offset = 0;
	word width = LT_map.width;
	word screen_offset = (y<<8)+(y<<6)+x;
	asm{
		push ds
		push di
		push si
		
		mov		dx,12		//12 tiles height column
		
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		word ptr [tile_offset],ds
		mov		word ptr [tile_offset+2],si

		mov		ax,map_offset
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		mov		di,screen_offset		//es:di screen address							
	}
	loop_tile:
	asm{
		mov 	ax,0A000h
		mov 	es,ax
		mov 	ax,16
	}
	copy_tile:	
	asm{
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		dec 	ax
		jnz		copy_tile
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		add		ax,[width]
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		dec		dx
		jnz		loop_tile
		
		pop si
		pop di
		pop ds
	}
}

void draw_map_row( word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &LT_tileset.tdata;
	unsigned char *mapdata = LT_map.data;
	word tile_offset = 0;
	//word width = map.width;
	word screen_offset = (y<<8)+(y<<6)+x;
	asm{
		push ds
		push di
		push si
		
		mov		dx,20					//20 tiles height column
		
		lds		bx,dword ptr[tiledata]					
		lds		si,ds:[bx]				//ds:si data address
		
		mov		word ptr [tile_offset],ds
		mov		word ptr [tile_offset+2],si

		mov		ax,map_offset
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		mov		di,screen_offset		//es:di screen address							
	}
	loop_tile:
	asm{
		mov 	ax,0A000h
		mov 	es,ax
		mov 	ax,16
	}
	copy_tile:	
	asm{
		mov 	cx,8
		rep		movsw				
		add 	di,320-16
		dec 	ax
		jnz		copy_tile
		
		mov		ds,word ptr [tile_offset]
		mov		si,word ptr [tile_offset+2]

		mov		ax,map_offset
		inc		ax
		mov		map_offset,ax
		
		les		bx,[mapdata]
		add		bx,ax
		mov		ax,es:[bx]
		mov		cl,8
		shl		ax,cl
		add		si,ax
		
		sub		di,320*16
		add		di,16						//next horizontal tile position
		dec		dx
		jnz		loop_tile
		
		pop si
		pop di
		pop ds
	}
}

//update rows and colums
void LT_scroll_map(){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	
	if (LT_current_x != LT_last_x) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_x < LT_last_x) 
			draw_map_column(LT_current_x,LT_current_y,LT_map_offset); 
		else  
			draw_map_column(LT_current_x+304,LT_current_y,LT_map_offset+19);
	}
	if (LT_current_y != LT_last_y) {
		LT_map_offset = ((SCR_Y>>4)*LT_map.width)+(SCR_X>>4);
		if (LT_current_y < LT_last_y)
			draw_map_row(LT_current_x,LT_current_y,LT_map_offset);
		else 
			draw_map_row(LT_current_x,LT_current_y+176,LT_map_offset+(11*LT_map.width));
	}
	
	LT_last_x = LT_current_x;
	LT_last_y = LT_current_y;
}

//Endless Side Scrolling Map
void LT_Endless_SideScroll_Map(int y){
	LT_current_x = (SCR_X>>4)<<4;
	LT_current_y = (SCR_Y>>4)<<4;
	if (LT_current_x > LT_last_x) { 
		draw_map_column(LT_current_x+304,LT_current_y,(LT_map_offset_Endless%LT_map.width)+(y*LT_map.width));
		LT_map_offset_Endless++;
	}
	LT_last_x = LT_current_x;
}

void LT_scroll_follow(SPRITE *s){
	//LIMITS
	int y_limU = SCR_Y + 64;
	int y_limD = SCR_Y + 120;
	int x_limL = SCR_X + 80;
	int x_limR = SCR_X + 200;
	int wmap = LT_map.width<<4;
	int hmap = LT_map.height<<4;
	int x = abs(s->last_x - s->pos_x);
	int y = abs(s->last_y - s->pos_y);
	//clamp limits
	if ((SCR_X > -1) && ((SCR_X+303)<wmap) && (SCR_Y > -1) && ((SCR_Y+175)<hmap)){
		if (s->pos_y > y_limD) SCR_Y+=y;
		if (s->pos_y < y_limU) SCR_Y-=y;
		if (s->pos_x < x_limL) SCR_X-=x;
		if (s->pos_x > x_limR) SCR_X+=x;
	}
	if (SCR_X < 0) SCR_X = 0; 
	if ((SCR_X+304) > wmap) SCR_X = wmap-304;
	if (SCR_Y < 0) SCR_Y = 0; 
	if ((SCR_Y+176) > hmap) SCR_Y = hmap-176; 
}

//load RLE sprites with transparency (size = 8,16,32)
void LT_Load_Sprite(char *file,SPRITE *s, byte size){
	FILE *fp;
	long index,offset;
	word num_colors;
	byte x,y;
	word i,j;
	word frame = 0;
	byte tileX;
	byte tileY;
	
	word number_of_runs = 0;
	byte pixel = 0; //this is to know if a scan line is empty
	word e_sskip = 0;
	word e_bskip = 0;
	word foffset = 0; //offset in final data
	word frame_size;
	word run_sskip[256];
	byte run_bskip[256];
	byte run_startx[256];
	word run_length[256];
	byte current_byte;
	byte last_byte = 0;
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	} 

	fskip(fp,18);
	fread(&s->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&s->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
	}

	for(index=(s->height-1)*s->width;index>=0;index-=s->width)
		for(x=0;x<s->width;x++)
		LT_tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			for(x=0;x<size;x++){
				memcpy(&LT_sprite_tiledatatemp[index],&LT_tile_datatemp[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	s->nframes = (s->width>>4) * (s->height>>4);
	s->width = size;
	s->height = size;

	//calculate frames size
	s->rle_frames = farcalloc(s->nframes,sizeof(SPRITEFRAME));
	//CONVERT AND STORE EVERY FRAME IN RLE FORMAT
	for(j=0;j<s->nframes;j++){
		offset = 0;
		foffset = 0;
		frame = (size*size)*j;
		number_of_runs = 0;
		e_sskip = 0;
		e_bskip = 0;
		for(y = 0; y < size; y++) {
			last_byte = 0;
			pixel = 0;
			for(x = 0; x < size; x++) {
				//Get pixel value
				current_byte = LT_sprite_tiledatatemp[(size*y)+x+frame]; 
				//I found a Pixel, start a run
				if (last_byte == 0 && current_byte != 0){
					run_startx[number_of_runs] = x;
					run_sskip[number_of_runs] = e_sskip;
					run_bskip[number_of_runs] = e_bskip;
					e_sskip = 0;
					e_bskip = 0;
					pixel = 1;
				}
				//I found the end of a run
				if (last_byte != 0 && current_byte == 0){
					run_length[number_of_runs] = x - run_startx[number_of_runs];
					number_of_runs++;
					pixel = 2;
				}
				//If 0, skip
				if (current_byte == 0){e_sskip++;e_bskip++;}
				last_byte = current_byte;
			}
			//I didn't find the end of a run, and the scan line ended
			if (pixel == 1){
				run_length[number_of_runs] = x - run_startx[number_of_runs];
				number_of_runs++;
				e_bskip = 0;
			}
			//I didn't find any pixel, or the last part was empty, and the scan line ended. Do nothing
			//Next scan line
			e_sskip+=320-size;
		}
		
		//ALLOCATE SPACE FOR CURRENT FRAME
		frame_size = 2; //word "number of runs"
		for(i = 0; i < number_of_runs; i++) 
			frame_size+= (4 + run_length[i]); //word skip + word number of pixels + length
		s->rle_frames[j].rle_data = (byte*) farcalloc(frame_size,sizeof(byte));

		//copy RLE data to struct
		memcpy(&s->rle_frames[j].rle_data[0],&number_of_runs,2);
		foffset+=2;
		for(i = 0; i < number_of_runs; i++) {
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_sskip[i],2); //bytes to skip on vga screen
			foffset+=2;
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_length[i],2); //Number of bytes of pixel data that follow
			foffset+=2;
			offset+=run_bskip[i];
			memcpy(&s->rle_frames[j].rle_data[foffset],&LT_sprite_tiledatatemp[offset+frame],run_length[i]); //copy pixel data;
			foffset+=run_length[i];
			offset+=run_length[i];
		}
	}

	s->bkg_data = (byte *) farcalloc(size*size,sizeof(byte)); /*allocate memory for the 16x16 bkg chunk erased every time you draw the sprite*/
	s->init = 0;
	s->frame = 0;
	s->baseframe = 0;
	s->ground = 0;
	s->jump = 2;
	s->jump_frame = 0;
	s->animate = 0;
	s->anim_speed = 0;
	s->anim_counter = 0;
}

void LT_Set_Sprite_Animation(SPRITE *s, int baseframe, int frames, int speed){
	s->baseframe = baseframe;
	s->aframes = frames;
	s->speed = speed;
	s->animate = 1;
}

void LT_Draw_Sprite(SPRITE *s){
	unsigned char *data;
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset0 = (s->last_y<<8)+(s->last_y<<6)+s->last_x;
	word screen_offset1 = (s->pos_y<<8)+(s->pos_y<<6)+s->pos_x;
	word init = s->init;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
	
	//animation
	if (s->animate == 1){
		s->frame = s->baseframe + s->anim_counter;
		if (s->anim_speed == s->speed){
			s->anim_speed = 0;
			s->anim_counter++;
			if (s->anim_counter == s->aframes) s->anim_counter = 0;
		}
		s->anim_speed++;
	}
	
	data = (unsigned char *) &s->rle_frames[s->frame].rle_data; 
	
	//Draw a sprite frame
	asm{
		push ds
		push di
		push si
		///Paste destroyed bkg chunk in last frame
		cmp	byte ptr [init],1 //if (s->init == 1)
		jne	short rle_noinit
		
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset0				
		
		lds		bx,[bkg_data]					
		lds		si,ds:[bx]						
		mov 	ax,size
	}
	rle_1:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	di,[next_scanline]
		dec 	ax
		jnz		rle_1
	}
	rle_noinit:
	s->init = 1;
	
	/*Copy bkg chunk before destroying it*/
	asm{
		mov 	ax,0A000h
		mov 	ds,ax						
		mov		si,screen_offset1				
		
		les		bx,[bkg_data]					
		les		di,es:[bx]						
		mov 	ax,size
	}
	rle_2:	
	asm{
		mov 	cx,size2
		rep		movsw				// copy bytes from ds:si to es:di
		add 	si,[next_scanline]
		dec 	ax
		jnz		rle_2
	
	/*copy sprite and destroy bkg*/	
		mov 	ax,0A000h
		mov 	es,ax						
		mov		di,screen_offset1	//es:di = vga		
	
		lds		bx,[data]					
		lds		si,ds:[bx]			//ds:si = data		
	
		lodsw			//DS:SI -> AX (Number of runs)
		
		xchg dx,ax		//DX = Number of runs.
	}
	rle_3:				//DX = Number of runs
	asm{
		lodsw			//DS:SI -> AX. Number of bytes to ad to pointer DI.
		add 	di,ax	
		lodsw			//DS:SI -> AX. Number of bytes of pixel data that follow
		mov 	cx,ax	//CX = AX (counter)
		shr		cx,1	//counter / 2 because we copy words
		jc		rle_odd //if there was a left over, it was odd, go to rle_odd
		
		rep 	movsw	//copy pixel data from ds:si to es:di
		dec 	dx		//DX = Number of runs
		jnz 	rle_3
		jmp		rle_exit
	}
	rle_odd:
	asm{
		rep 	movsw	//copy pixel data from ds:si to es:di
		movsb			//Copy the last byte, because the line was odd
		dec 	dx		//DX = Number of runs	
		jnz 	rle_3
	}
	rle_exit:
	asm pop si
	asm pop di
	asm pop ds
	
	s->last_x = s->pos_x;
	s->last_y = s->pos_y;
	
}

byte *LT_move_player(SPRITE *s){
	byte LT_Collision[4];
	int col_x = 0;
	int col_y = 0;
	int size = s->width;
	int siz = s->width -1;
	
	//GET TILE POS
	tile_number = LT_map.data[((s->pos_y>>4) * LT_map.width) + (s->pos_x>>4)];
	tilecol_number = LT_map.collision[((s->pos_y>>4) * LT_map.width) + (s->pos_x>>4)];
  if (LT_Gravity == 1){
	if ((s->ground == 1) && (LT_Keys[LT_D])) {s->ground = 0; s->jump_frame = 0; s->jump = 1;}
	if (s->jump == 1){//JUMP
		col_y = 0;
		if (LT_player_jump_pos[s->jump_frame] < 0){
			tile_number_VR = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1; 
			if (tile_number_VL == 1) col_y = 1; 
		} else {
			tile_number_VR = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + ((s->pos_x+siz)>>4)];
			tile_number_VL = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + (s->pos_x>>4)];
			if (tile_number_VR == 1) col_y = 1; 
			if (tile_number_VL == 1) col_y = 1; 			
			if (tile_number_VR == 2) col_y = 1; 
			if (tile_number_VL == 2) col_y = 1; 
		}
		if (col_y == 0){
			s->pos_y += LT_player_jump_pos[s->jump_frame];
			s->jump_frame++;
			if (s->jump_frame == 76) {s->jump_frame = 0; s->jump = 2;}
		}
		if (col_y == 1){
			s->jump_frame = 0; 
			s->jump = 2;
		}
	}
	if (s->jump == 2){//DOWN
		col_y = 0;
		tile_number_VR = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + ((s->pos_x+siz)>>4)];
		tile_number_VL = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_VR == 1) col_y = 1;
		if (tile_number_VL == 1) col_y = 1;
		if (tile_number_VR == 2) col_y = 1;
		if (tile_number_VL == 2) col_y = 1;
		if (col_y == 0) {s->ground = 0; s->pos_y+=2;}
		if (col_y == 1) s->ground = 1;
	}
  }
  if (LT_Gravity == 0){
	if (LT_Keys[LT_UP]){	//UP
		col_y = 0;
		tile_number_VR = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + ((s->pos_x+siz)>>4)];
		tile_number_VL = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_VR == 1) col_y = 1;
		if (tile_number_VL == 1) col_y = 1;
		if (col_y == 0) s->pos_y--;
	}
	if (LT_Keys[LT_DOWN]){	//DOWN
		col_y = 0;
		tile_number_VR = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + ((s->pos_x+siz)>>4)];
		tile_number_VL = LT_map.collision[(((s->pos_y+size)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_VR == 1) col_y = 1;
		if (tile_number_VL == 1) col_y = 1;
		if (tile_number_VR == 2) col_y = 1;
		if (tile_number_VL == 2) col_y = 1;
		if (col_y == 0) s->pos_y++;
	}
  }
	if (LT_Keys[LT_LEFT]){	//LEFT
		col_x = 0;
		tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x-1)>>4)];
		tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + ((s->pos_x-1)>>4)];	
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x--;
	}
	if (LT_Keys[LT_RIGHT]){	//RIGHT
		col_x = 0;
		tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
		tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x++;
	}
	if (LT_SideScroll == 1){
		col_x = 0;
		tile_number_HU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
		tile_number_HD = LT_map.collision[(((s->pos_y+siz)>>4) * LT_map.width) + ((s->pos_x+size)>>4)];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x++;
		if (s->pos_x < SCR_X+3) s->pos_x = SCR_X+3;
	}
	
	LT_Collision[0] = tile_number;
	LT_Collision[1] = tilecol_number;
	LT_Collision[2] = col_x;
	LT_Collision[3] = col_y;
	
	return LT_Collision;
}
	
void LT_load_font(char *file){
	LT_Load_Sprite(file,&lt_gpnumber0,16);
	LT_Load_Sprite(file,&lt_gpnumber1,16);
	LT_Load_Sprite(file,&lt_gpnumber2,16);
}

void LT_gprint(int var, word x, word y){
	lt_gpnumber0.pos_x = SCR_X+x+34; lt_gpnumber0.pos_y = SCR_Y+y;
	lt_gpnumber1.pos_x = SCR_X+x+17; lt_gpnumber1.pos_y = SCR_Y+y;
	lt_gpnumber2.pos_x = SCR_X+x; lt_gpnumber2.pos_y = SCR_Y+y;
	
	lt_gpnumber0.frame = var % 10;
	LT_Draw_Sprite(&lt_gpnumber0);
	
	var /= 10;
	lt_gpnumber1.frame = var % 10;
	LT_Draw_Sprite(&lt_gpnumber1);
	
	var /= 10;
	lt_gpnumber2.frame = var % 10;
	LT_Draw_Sprite(&lt_gpnumber2);
}
	
void LT_unload_sprite(SPRITE *s){
	int i;
	farfree(s->bkg_data); s->bkg_data = NULL;
	for (i=0;i<s->nframes;i++){
		farfree(s->rle_frames[i].rle_data);
		s->rle_frames[i].rle_data = NULL;
	}
	farfree(s->rle_frames); s->rle_frames = NULL;
	s = NULL;
}

void LT_unload_font(){
	LT_unload_sprite(&lt_gpnumber0);
	LT_unload_sprite(&lt_gpnumber1);
	LT_unload_sprite(&lt_gpnumber2);
}

/*set_palette*/                                                           
void set_palette(unsigned char *palette){
  int i;
  outp(0x03c8,0); 
  for(i=0;i<256*3;i++) outp(0x03c9,palette[i]);
}

/*init Cycle struct*/  
void cycle_init(COLORCYCLE *cycle,unsigned char *palette){
	cycle->frame = 0;
	cycle->counter = 0;
	cycle->palette = palette;
}

/*Cycle palette*/  
void cycle_palette(COLORCYCLE *cycle, byte speed){
	int i;
	if (cycle->frame == 27) cycle->frame = 3;
	outp(0x03c8,0); 
	for(i=0;i!=27;i++) outp(0x03c9,cycle->palette[i+cycle->frame]);
	if (cycle->counter == speed){cycle->counter = 0; cycle->frame+=3;}
	cycle->counter++;
}

void opl2_out(unsigned char reg, unsigned char data){
	_disable();
	outportb(opl2_base, reg);
	outportb(opl2_base + 1, data);
	_enable();
}

void opl2_clear(void){
    unsigned char i, slot1, slot2;
    static unsigned char slotVoice[9][2] = {{0,3},{1,4},{2,5},{6,9},{7,10},{8,11},{12,15},{13,16},{14,17}};
    static unsigned char offsetSlot[18] = {0,1,2,3,4,5,8,9,10,11,12,13,16,17,18,19,20,21};

    opl2_out(   1, 0x20);   // Set WSE=1
    opl2_out(   8,    0);   // Set CSM=0 & SEL=0
    opl2_out(0xBD,    0);   // Set AM Depth, VIB depth & Rhythm = 0
    /*
    for(i=0; i<9; i++){
        slot1 = offsetSlot[slotVoice[i][0]];
        slot2 = offsetSlot[slotVoice[i][1]];
        opl2_out(0xB0+i, 0);    //turn note off
        opl2_out(0xA0+i, 0);    //clear frequency
        opl2_out(0xE0+slot1, 0);
        opl2_out(0xE0+slot2, 0);
        opl2_out(0x60+slot1, 0xff);
        opl2_out(0x60+slot2, 0xff);
        opl2_out(0x80+slot1, 0xff);
        opl2_out(0x80+slot2, 0xff);
        opl2_out(0x40+slot1, 0xff);
        opl2_out(0x40+slot2, 0xff);
    }*/
}

void interrupt play_music(void){
    while (!LT_imfwait){
        LT_imfwait = LT_music.sdata[LT_music.offset+2] | (LT_music.sdata[LT_music.offset+3]) << 8;
        opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		LT_music.offset+=4;
	}
	
	LT_imfwait--;
	if (LT_music.offset > LT_music.size) LT_music.offset = 4;
	outportb(0x20, 0x20);	//PIC, EOI
}

void interrupt play_music2(void){
	if (time_ctr > next_event){
		next_event += (LT_music.sdata[LT_music.offset+2] | (LT_music.sdata[LT_music.offset+3]) << 8);
		opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		LT_music.offset+=4;
	}
	time_ctr++;
	if (LT_music.offset > LT_music.size) LT_music.offset = 4;
	outportb(0x20, 0x20);	//PIC, EOI
}

void LT_Load_Music(char *fname){
	FILE *imfile = fopen(fname, "rb");
	unsigned char rb[16];
	struct stat filestat;
	
	opl2_clear();
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, LT_old_time_handler);
	
	if (!imfile){
		printf("Can't find %s.\n",fname);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}
	if (fread(rb, sizeof(char), 2, imfile) != 2){
		printf("Error openning %s.\n",fname);
		sleep(2);
		LT_ExitDOS();
		exit(1);
	}
	
	//IMF
	if (rb[0] == 0 && rb[1] == 0){
		LT_music.filetype = 0;
		LT_music.offset = 0L;
	}
	else {
		LT_music.filetype = 1;
		LT_music.offset = 2L;
	}
	//get file size:
	fstat(fileno(imfile),&filestat);
    LT_music.size = filestat.st_size;
	if (LT_music.size > 65535){
		printf("Music file is bigger than 64 Kb %s.\n",fname);
		sleep(2);
		LT_ExitDOS();		
	}
    fseek(imfile, 0, SEEK_SET);

	fread(LT_music.sdata, 1, LT_music.size,imfile);
	fclose(imfile);
}

void LT_Start_Music(word freq_div){
	//set interrupt and start playing music
	unsigned long spd = 1193182/freq_div;
	_disable(); //disable interrupts
	outportb(0x43, 0x36);
	outportb(0x40, spd);	//lo-byte
	outportb(0x40, spd >> 8);	//hi-byte*/
	setvect(0x1C, play_music);		//interrupt 1C not available on NEC 9800-series PCs.
	_enable();  //enable interrupts	 
}

void LT_Stop_Music(){
	opl2_clear();
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, LT_old_time_handler);
}

void LT_Unload_Music(){
	LT_Stop_Music();
	LT_music.size = NULL;
	LT_music.offset = NULL;
	LT_music.filetype = NULL;
	if (LT_music.sdata){
		free(LT_music.sdata); 
		LT_music.sdata = NULL;
	}
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