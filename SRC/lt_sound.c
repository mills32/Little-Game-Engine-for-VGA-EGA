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
	asm mov dx, 00388h
	asm mov al, reg
	asm out dx, al
	
	//Wait at least 3.3 microseconds
	asm mov cx,6
	wait:
		asm in ax,dx
		asm loop wait	//for (i = 0; i < 6; i++) inp(lpt_ctrl);
	
	asm inc dx
	asm mov al, data
	asm out dx, al
	
	//for( i = 35; i ; i-- )inport(0x388);
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
		//sleep(3);
        return;
    } else {
		printf("AdLib card not detected.\n");
		//sleep(3);
        return;
    }
}

void LT_Play_Music(){
	//byte *ost = music_sdata + music_offset;
	while (!LT_imfwait){
        LT_imfwait = LT_music.sdata[LT_music.offset+2];
        opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		LT_music.offset+=3;
	}
	LT_imfwait--;
	//ending song loop
	if (LT_music.offset > LT_music.size) {LT_music.offset = 0; LT_imfwait = 0; opl2_clear();}
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
}

void LT_Load_Music(char *fname, int enable_pcm){
	
	//long size = 0;
	word offset = 0;
	word offset1 = 0;
	FILE *imfile = fopen(fname, "rb");
	unsigned char rb[16];
	//struct stat filestat;
	
	opl2_clear();
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	//setvect(0x1C, LT_old_time_handler);
	enable_pcm += 0;
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
	fseek(imfile, 0L, SEEK_END);
	LT_music.size = ftell(imfile);
	//fstat(fileno(imfile),&filestat);
    fseek(imfile, 0L, SEEK_SET);
	fread(LT_music.sdata, 1, LT_music.size,imfile);

	fclose(imfile);
	
	for (offset = 0; offset < LT_music.size; offset+= 4){
		LT_music.sdata[offset1] = LT_music.sdata[offset];
		LT_music.sdata[offset1+1] = LT_music.sdata[offset+1];
		LT_music.sdata[offset1+2] = LT_music.sdata[offset+2] | (LT_music.sdata[offset+3]) << 8;
		offset1 += 3;
	}
	LT_music.size = offset1;
}

void LT_Stop_Music(){
	//reset interrupt
	//outportb(0x43, 0x36);
	//outportb(0x40, 0xFF);	//lo-byte
	//outportb(0x40, 0xFF);	//hi-byte*/
	//setvect(0x1C, LT_old_time_handler);
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


//Adlib SFX INS format 
const unsigned char OPL2_op_table[9] =
  {0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12};

const unsigned short OPL2_notetable[12] =
  {340,363,385,408,432,458,485,514,544,577,611,647};

void LT_Play_AdLib_SFX(unsigned char *ins, byte chan, byte octave, byte note){
	unsigned char op = OPL2_op_table[chan];
	unsigned char *data = ins;
	word chanfreq = OPL2_notetable[note];
	word chanoct = octave << 2;
	
	opl2_out(0xb0 +  chan, 0);	// stop old note
	opl2_out(0x01,0x20);
	
	// set instrument data
	opl2_out(0x23 + op, data[0]);	//mod misc
	opl2_out(0x20 + op, data[1]);	//car misc
	opl2_out(0x43 + op, data[2]);	//mod scale vol
	opl2_out(0x40 + op, data[3]);	//car scale vol
	opl2_out(0x63 + op, data[4]);	//mod AD
	opl2_out(0x60 + op, data[5]);	//car AD
	opl2_out(0x83 + op, data[6]);	//mod SR
	opl2_out(0x80 + op, data[7]);	//car SR
	opl2_out(0xc0 + chan, data[8]);	//feedback
	opl2_out(0xe3 + op, data[9]);	//mod wave
	opl2_out(0xe0 + op, data[10]);	//car wave
	
	//set frequency
	opl2_out(0xa0 + chan, chanfreq & 255);
	opl2_out(0xb0 + chan, ((chanfreq & 768) >> 8) + chanoct | 32);//32 = key push
}



//PC Speaker
/*		
	NOTE VALUES
	-----------
	Octave 0    1    2    3    4    5    6    7
	Note
	 C     0    12   24   36   48   60   72   84
	 C#    1    13   25   37   49   61   73   85
	 D     2    14   26   38   50   62   74   86
	 D#    3    15   27   39   51   63   75   87
	 E     4    16   28   40   52   64   76   88
	 F     5    17   29   41   53   65   77   89
	 F#    6    18   30   42   54   66   78   90
	 G     7    19   31   43   55   67   79   91
	 G#    8    20   32   44   56   68   80   92
	 A     9    21   33   45   57   69   81   93
	 A#    10   22   34   46   58   70   82   94
	 B     11   23   35   47   59   71   83   95
*/
//1193180/Value
int LT_PC_Speaker_Note[96] = {
	//Note		C    C#     D    D#     E     F    F#     G    G#     A    A#     B
	//Octave
	/*0	*/	65535,65535,65535,62799,56818,54235,51877,49716,45892,44192,41144,38490,
	/*1	*/	36157,34091,32248,30594,29102,27118,25939,24351,22946,21694,20572,19245,
	/*2	*/  18357,17292,16345,15297,14551,13715,12969,12175,11473,10847,10286, 9701,
	/*3	*/  9108 ,8584 ,8117 ,7698 ,7231 ,6818 ,6450 ,6088 ,5736 ,5424 ,5121 , 4870,
	/*4	*/  4554 ,4308 ,4058 ,3837 ,3616 ,3419 ,3225 ,3044 ,2875 ,2712 ,2560 , 2415, 
	/*5	*/ 	2281 ,2154 ,2033 ,1918 ,1811 ,1709 ,1612 ,1522 ,1436 ,1356 ,1280 ,1208 ,
	/*6	*/ 	1141 ,1076 ,1015 ,959  ,898  ,854  ,806  ,761  ,718  ,678  ,640  ,604  ,
	/*7	*/ 	570  ,538  ,508  ,479  ,452  ,427  ,403  ,380  ,359  ,339  ,320  ,302  ,	
};

int LT_PC_Speaker_Playing = 0;
int LT_PC_Speaker_Offset = 0;
int *LT_PC_Speaker_SFX;
byte LT_PC_Speaker_Size = 16;
void interrupt PC_Speaker_SFX_Player(void){
	int counter;
	int note;
	if (LT_PC_Speaker_Playing == 1){
		if (LT_PC_Speaker_Offset != LT_PC_Speaker_Size){
			counter = LT_PC_Speaker_SFX[LT_PC_Speaker_Offset];
			note = LT_PC_Speaker_Note[counter]; // calculated frequency (1193180/Value)
			asm mov ax, note
			asm out 42h,al
			asm mov al,ah
			asm out 42h,al
			LT_PC_Speaker_Offset++;
		} else {
			LT_PC_Speaker_Playing = 0;
			asm in al, 61h        //Disable speaker
			asm and al, 252       
			asm out 61h, al
			//remove interrupt
			outportb(0x43, 0x36);
			outportb(0x40, 0xFF);	//lo-byte
			outportb(0x40, 0xFF);	//hi-byte
			setvect(0x1C, LT_old_time_handler);
		}
	}
}

void LT_Play_PC_Speaker_SFX(int *note_array){
	unsigned long spd = 1193182/60;
	//if (LT_PC_Speaker_Playing == 0) {
	LT_PC_Speaker_Playing = 0;
		asm in al, 61h			//Enable speaker
		asm or al, 3
		asm out 61h, al
		asm mov al, 0B6h
		asm out 43h,al
		LT_PC_Speaker_Playing = 1;
		LT_PC_Speaker_Offset = 0;
		LT_PC_Speaker_SFX = &note_array[0];
		
		//set timer
		outportb(0x43, 0x36);
		outportb(0x40, spd % 0x100);	//lo-byte
		outportb(0x40, spd / 0x100);	//hi-byte
		//set interrupt
		setvect(0x1C, PC_Speaker_SFX_Player);		//interrupt 1C not available on NEC 9800-series PCs.
	//}
}


