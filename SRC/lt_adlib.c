/*****************************************************************************
	Based on K1n9_Duk3's IMF Player - A simple IMF player for DOS
	And on Apogee Sound System (ASS) and Wolfenstein 3-D (W3D)
		ASS is Copyright (C) 1994-1995 Apogee Software, Ltd.
		W3D is Copyright (C) 1992 Id Software, Inc.
*******************************************************************************/

#include "lt__eng.h"

extern void interrupt (*LT_old_time_handler)(void); 

void LT_Error(char *error, char *file);

//ADLIB
IMFsong LT_music;	// One song in ram stored at "LT_music"

const int opl2_base = 0x388;
long next_event;
long LT_imfwait;

void opl2_out(unsigned char reg, unsigned char data){
	_disable(); //disable interrupts
	outportb(opl2_base, reg);
	outportb(opl2_base + 1, data);
	_enable();
}

void opl2_clear(void){
	int i;
    for (i=1; i< 256; opl2_out(i++, 0));    //clear all registers
}

void LT_Adlib_Detect(){ 
    int status1, status2, i;

    opl2_out(4, 0x60);
    opl2_out(4, 0x80);

    status1 = inp(ADLIB_PORT);
    
    opl2_out(2, 0xFF);
    opl2_out(4, 0x21);

    for (i=100; i>0; i--) inp(ADLIB_PORT);

    status2 = inp(ADLIB_PORT);
    
    opl2_out(4, 0x60);
    opl2_out(4, 0x80);

    if ( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) ){
        unsigned char i;
		for (i=1; i<=0xF5; opl2_out(i++, 0));    //clear all registers
		opl2_out(1, 0x20);  // Set WSE=1
		printf("AdLib card detected.\n");
		sleep(3);
        return;
    } else {
		printf("AdLib card not detected.\n");
		sleep(3);
        return;
    }
}

void interrupt play_music(void){
    while (!LT_imfwait){
        LT_imfwait = LT_music.sdata[LT_music.offset+2] | (LT_music.sdata[LT_music.offset+3]) << 8;
        opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		LT_music.offset+=4;
		if (LT_music.offset > LT_music.size) LT_music.offset = 4;
	}
	
	LT_imfwait--;
	outportb(0x20, 0x20);	//PIC, EOI
}

void do_play_music(){
    while (!LT_imfwait){
        LT_imfwait = LT_music.sdata[LT_music.offset+2] | (LT_music.sdata[LT_music.offset+3]) << 8;
        opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		LT_music.offset+=4;
		if (LT_music.offset > LT_music.size) LT_music.offset = 4;
	}
	
	LT_imfwait--;
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
	
	if (!imfile) LT_Error("Can't find ",fname);
		
	if (fread(rb, sizeof(char), 2, imfile) != 2) LT_Error("Error in ",fname);
	
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
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, LT_old_time_handler);
	opl2_clear();
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
