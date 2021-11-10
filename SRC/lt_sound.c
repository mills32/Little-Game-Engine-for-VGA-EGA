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

void (*PCM_Drums)(void);

void PCM_Drums_OFF(){};
//int note = 7.78;
void PCM_Drums_ON(){
	//B0-B2, set frequency for channels 0, 1 and 2. Use to play drums.
	if (LT_music.sdata[LT_music.offset] == 0xB0) sb_play_sample(0,11025);
	if (LT_music.sdata[LT_music.offset] == 0xB1) sb_play_sample(1,11025);
	if (LT_music.sdata[LT_music.offset] == 0xB2) sb_play_sample(2,11025);
	//note+=7.78;
};

void LT_Play_Music(){
	//byte *ost = music_sdata + music_offset;
	while (!LT_imfwait){
        LT_imfwait = LT_music.sdata[LT_music.offset+2];
        opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		PCM_Drums();
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
	//fseek(imfile, 0L, SEEK_END);
	//LT_music.size = ftell(imfile);
	//fstat(fileno(imfile),&filestat);
    fseek(imfile, 0L, SEEK_SET);
	LT_music.size = fread(LT_music.sdata, 1, 65535,imfile);

	fclose(imfile);
	
	for (offset = 0; offset < LT_music.size; offset+= 4){
		LT_music.sdata[offset1] = LT_music.sdata[offset];
		LT_music.sdata[offset1+1] = LT_music.sdata[offset+1];
		LT_music.sdata[offset1+2] = LT_music.sdata[offset+2] | (LT_music.sdata[offset+3]) << 8;
		offset1 += 3;
	}
	LT_music.size = offset1;
	
	if (enable_pcm) PCM_Drums = &PCM_Drums_ON;
	else PCM_Drums = &PCM_Drums_OFF;
}

void LT_Stop_Music(){
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
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

//SOUND BLASTER PCM
short sb_base; // default 220h
char sb_irq; // default 7
char sb_dma; // default 1
void interrupt ( *old_irq )();

volatile int playing;

unsigned char *dma_buffer;
int page;
int sb_offset;

typedef struct{
	int offset;
	int size;
} audio_sample;

audio_sample sample[16];
int LT_sb_nsamples = 0;
int LT_sb_offset = 0;

int reset_dsp(short port){
	outportb( port + SB_RESET, 1 );
	delay(10);
	outportb( port + SB_RESET, 0 );
	delay(10);
	if( ((inportb(port + SB_READ_DATA_STATUS) & 0x80) == 0x80) && (inportb(port + SB_READ_DATA) == 0x0AA )) {
		sb_base = port;
		return 1;
	}
	return 0;
}

void write_dsp( unsigned char command ){
	while( (inportb( sb_base + SB_WRITE_DATA ) & 0x80) == 0x80 );
	outportb( sb_base + SB_WRITE_DATA, command );
}

int sb_detect(){
	char temp;
	char *BLASTER;
	sb_base = 0;

	// possible values: 210, 220, 230, 240, 250, 260, 280
	for( temp = 1; temp < 9; temp++ ) {
		if( temp != 7 ) {
			if( reset_dsp( 0x200 + (temp << 4) ) ) break;
		}
	}
	if( temp == 9 ) return 0;

	BLASTER = getenv("BLASTER");
	sb_dma = 0;
	for( temp = 0; temp < strlen( BLASTER ); ++temp ) {
		if((BLASTER[temp] | 32) == 'd') sb_dma = BLASTER[temp + 1] - '0';
	}

	for( temp = 0; temp < strlen( BLASTER ); ++temp ) {
		if((BLASTER[temp] | 32) == 'i') {
			sb_irq = BLASTER[temp + 1] - '0';
			if( BLASTER[temp + 2] != ' ' ) sb_irq = sb_irq * 10 + BLASTER[ temp + 2 ] - '0';
		}
	}

	return sb_base != 0;
}

void interrupt sb_irq_handler(){
	inportb(sb_base + SB_READ_DATA_STATUS);
	outportb( 0x20, 0x20 );
	/*if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		outportb( 0xA0, 0x20 );
	}*/
	//playing = 0;
}

void init_irq(){
	old_irq = getvect( sb_irq + 8 );
	setvect( sb_irq + 8, sb_irq_handler );
	outportb( 0x21, inportb( 0x21 ) & !(1 << sb_irq) );
	// save the old irq vector
	/*if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		if( sb_irq == 2) old_irq = getvect( 0x71 );
		if( sb_irq == 10) old_irq = getvect( 0x72 );
		if( sb_irq == 11) old_irq = getvect( 0x73 );
	} else {
		old_irq = getvect( sb_irq + 8 );
	//}
	// set our own irq vector
	if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		if( sb_irq == 2) setvect( 0x71, sb_irq_handler );
		if( sb_irq == 10) setvect( 0x72, sb_irq_handler );
		if( sb_irq == 11) setvect( 0x73, sb_irq_handler );
	} else {
		setvect( sb_irq + 8, sb_irq_handler );
	//}
	// enable irq with the mainboard's PIC
	/*if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		if( sb_irq == 2) outportb( 0xA1, inportb( 0xA1 ) & 253 );
		if( sb_irq == 10) outportb( 0xA1, inportb( 0xA1 ) & 251 );
		if( sb_irq == 11) outportb( 0xA1, inportb( 0xA1 ) & 247 );
		outportb( 0x21, inportb( 0x21 ) & 251 );
	} else {
		outportb( 0x21, inportb( 0x21 ) & !(1 << sb_irq) );
	//}*/
}

void deinit_irq(){
	setvect( sb_irq + 8, old_irq );
	outportb( 0x21, inportb( 0x21 ) & (1 << sb_irq) );
	// restore the old irq vector
	/*if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		if( sb_irq == 2) setvect( 0x71, old_irq );
		if( sb_irq == 10) setvect( 0x72, old_irq );
		if( sb_irq == 11) setvect( 0x73, old_irq );
	} else {
		setvect( sb_irq + 8, old_irq );
	//}
	// enable irq with the mainboard's PIC
	if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
		if( sb_irq == 2) outportb( 0xA1, inportb( 0xA1 ) | 2 );
		if( sb_irq == 10) outportb( 0xA1, inportb( 0xA1 ) | 4 );
		if( sb_irq == 11) outportb( 0xA1, inportb( 0xA1 ) | 8 );
		outportb( 0x21, inportb( 0x21 ) | 4 );
	} else {
		outportb( 0x21, inportb( 0x21 ) & (1 << sb_irq) );
	//}*/
}

void assign_dma_buffer(){
	long linear_address;
	//dma_buffer = (unsigned char*) calloc(32768,sizeof(byte));
	linear_address = FP_SEG(LT_tile_tempdata);
	linear_address = (linear_address << 4)+FP_OFF(LT_tile_tempdata);
	
	page = linear_address >> 16;
	sb_offset = linear_address & 0xFFFF;
	
	//printf("DMA OFFSET 0x%04X\n",FP_OFF(LT_tile_tempdata));
}

void sb_init(){
	sb_detect();
	//init_irq();
	assign_dma_buffer();
	write_dsp( SB_ENABLE_SPEAKER );
}

void sb_set_freq(int freq){
	write_dsp(SB_SET_PLAYBACK_FREQUENCY);
	write_dsp(256-1000000/freq);
}

void sb_deinit(){
	write_dsp( SB_DISABLE_SPEAKER );
	//free( dma_buffer );
	//deinit_irq();
}

void LT_Clear_Samples(){
	LT_sb_nsamples = 0;
	LT_sb_offset = 0;
	memset(LT_tile_tempdata,0x00,sizeof(byte)*32768);
}

void sb_load_sample(char *file_name){
	FILE *raw_file;
	if (LT_sb_offset > 31*1024) LT_Error("Not enough RAM to load sample",file_name);
	sample[LT_sb_nsamples].offset = LT_sb_offset;
	
	raw_file = fopen(file_name, "rb");
	fseek(raw_file, 0x2C, SEEK_SET);
	sample[LT_sb_nsamples].size = fread(LT_tile_tempdata+LT_sb_offset, 1, 32*1024, raw_file);
	LT_sb_offset += sample[LT_sb_nsamples].size;
	LT_sb_nsamples++;
	fclose(raw_file);
}

void sb_play_sample(char sample_number, int freq){
	int pos = sample[sample_number].offset;
	int length = sample[sample_number].size -1;
	if (LT_SFX_MODE == 2){
	//if (!playing) {
		playing = 1;
		
		//Playback
		outportb( MASK_REGISTER, 4 | 1 /*sb_dma*/ );
		outportb( MSB_LSB_FLIP_FLOP, 0 );
		outportb( MODE_REGISTER, 0x48 | 1 /*sb_dma*/ );
		//switch( sb_dma ) //DMA_CHANNEL_0 DMA_CHANNEL_1 DMA_CHANNEL_3
		outportb( DMA_CHANNEL_1, page); 
		
		// program the DMA controller
		outportb( sb_dma << 1, (sb_offset+pos) & 0xFF );
		outportb( sb_dma << 1, (sb_offset+pos) >> 8 );
		outportb((sb_dma << 1) + 1, length & 0xFF );
		outportb((sb_dma << 1) + 1, length >> 8 );
		
		write_dsp(SB_SET_PLAYBACK_FREQUENCY);
		write_dsp(256-1000000/freq);
		
		outportb(MASK_REGISTER, sb_dma);
		write_dsp(SB_SINGLE_CYCLE_8PCM);
		write_dsp(length & 0xFF);
		write_dsp(length >> 8);
	//}
	}
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
	if (LT_PC_Speaker_Playing == 0) {
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
	}
}


