/*****************************************************************************
	Based on K1n9_Duk3's IMF Player - A simple IMF player for DOS
	And on Apogee Sound System (ASS) and Wolfenstein 3-D (W3D)
		ASS is Copyright (C) 1994-1995 Apogee Software, Ltd.
		W3D is Copyright (C) 1992 Id Software, Inc.
*******************************************************************************/

#include "lt__eng.h"

extern void interrupt (*LT_old_time_handler)(void);
void DAT_Seek2(word fp,char *dat_string); 
void LT_fread(word file_handle,word bytes,void *buffer);
unsigned long LT_fseek(word file_handle,unsigned long bytes,byte origin);
void LT_Error(char *error, char *file);
extern unsigned char far *LT_tile_tempdata; 
//ADLIB
word LT_music_size;
word LT_music_offset;
byte LT_music_filetype; //0 1 - imf0 imf1
byte far *LT_music_sdata;

extern byte LT_MUSIC_ENABLE;
extern byte LT_MUSIC_MODE;
int ADLIB_PORT = 0x0388;
int ADLIB_LPTPORT = 0;
int TANDY_PORT = 0x00C0;
int vgm_loop = 0;
int vgm_loop_size = 0;
int vgm_ok = 0;
long next_event;
word LT_imfwait;
byte *LT_music_data;

void (*LT_Load_Music)(char*,char*);
void (*LT_Play_Music)(void);


void opl2_out(unsigned char reg, unsigned char data){
	asm mov dx, ADLIB_PORT
	asm mov al, reg
	asm out dx, al
	asm mov cx,52; asm rep nop;//Wait at least 3.3 microseconds
	asm inc dx
	asm mov al, data
	asm out dx, al
	//delay 2
	//asm mov cx,304; asm rep nop; // Wait at least 23 microseconds
	
	//mov cx, 35
	//.delay_2:
	//	in al, dx
	//	loop .delay_2
}

void opl2LPT_out(unsigned char reg,unsigned char data){
	// Select OPL2 register
	asm mov dx,ADLIB_LPTPORT; asm mov al, reg; asm out dx, al;
	asm add dx,2;	//LPT_CTRL
	asm mov al,13; asm out dx, al;
	asm mov al,9; asm out dx, al;
	asm mov al,13; asm out dx, al;
	asm mov cx,52; asm rep nop; //Wait at least 3.3 microseconds
	// Set value
	asm sub dx,2;	//LPT_DATA
	asm mov al,data; asm out dx, al;
	asm add dx,2;	//LPT_CTRL
	asm mov al,12; asm out dx, al;
	asm mov al,8; asm out dx, al;
	asm mov al,12; asm out dx, al;
	asm mov cx,304; asm rep nop; // Wait at least 23 microseconds
}


void opl2_clear(void){
	int i;
    for (i=1; i< 256; opl2_out(i++, 0));    //clear all registers
}

void LT_Adlib_Detect(){ 
    byte status1, status2;

    opl2_out(4, 0x60);
    opl2_out(4, 0x80);

	asm mov dx,ADLIB_PORT
	asm in al,dx
    asm mov status1,al;
    
    opl2_out(2, 0xFF);
    opl2_out(4, 0x21);
	
	asm mov dx,ADLIB_PORT
    asm mov cx,100
	_loop:
		asm in al,dx
		asm loop _loop

	asm in al,dx
    asm mov status2,al;
    
    opl2_out(4, 0x60);
    opl2_out(4, 0x80);
    if ( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) ){
        unsigned char i;
		for (i=1; i<=0xF5; opl2_out(i++, 0));    //clear all registers
		opl2_out(1, 0x20);  // Set WSE=1
		_printf("AdLib card detected.\n\r$");
        return;
    } else {
		_printf("AdLib card not detected.\n\r$");
        return;
    }
	
}

void Load_Music_Adlib(char *filename, char *dat_string){
	//Read tags
	word file;
	//unsigned char *vgm_dat = &LT_tile_tempdata[0];
	unsigned char *vgm_dat2 = &LT_music_sdata[0];

	char header[5];
	dword tag_offset = 0,data_offset = 0, opl_clock = 0;
	dword size = 0;
	dword file_pos = 0;
	
	opl2_clear();
	LT_memset(vgm_dat2,0,(64*1024));
	
	asm CLI
	LT_Draw_Text_Box(11,18,16,1,0,0,0,"LOADING:  MUSIC ");
	asm STI
	file = LT_fopen(filename,0); 
	if(!file) LT_Error("Can't find file $",filename);
	if (dat_string) {DAT_Seek2(file,dat_string); filename = dat_string;}
	else LT_fseek(file, 0, SEEK_SET);
	//Check vgm
	LT_fread(file,4,header);
	if ((header[0] != 0x56) || (header[1] != 0x67) || (header[2] != 0x6d) || (header[3] != 0x20)){
		LT_fclose(file);LT_Error("Not a VGM file $",filename);
	}
	//get absolute file size, absolute tags offset
	LT_fread(file,4,&size); size +=4;
	LT_fseek(file,0x0C, SEEK_CUR);//0x14
	LT_fread(file,4,&tag_offset);//0x18
	if (tag_offset != 0) tag_offset += 0x14;
	//get loop start loop length.
	LT_fseek(file,0x04, SEEK_CUR);//0x1C
	LT_fread(file,2,&vgm_loop);//0x1E
	LT_fseek(file,2,SEEK_CUR);//0x20
	LT_fread(file,2,&vgm_loop_size);//0x22
	//get absolute data offset.
	file_pos = LT_fseek(file,0x12, SEEK_CUR);//0x34
	LT_fread(file,4,&data_offset);//0x38
	file_pos+=4;
	if (!data_offset) data_offset = 0x42;
	else data_offset+=0x34;
	//calculate data size
	size -= data_offset;
	if (tag_offset != 0) size -= (size - tag_offset);
	if (size > 0xFFFF) {LT_fclose(file); LT_Error("VGM bigger than 64kB $",dat_string);}
	LT_music_size = size;
	//What chip is it? vgm_chip 1 ym3812; 2 ym3526; 3 y8950; 4 SN76489
	file_pos -= 0x2C;
	LT_fseek(file,file_pos, SEEK_SET);//0x0C
	LT_fread(file, 4,&opl_clock);//0x10
	//if (!opl_clock) {fclose(f); vgm_ok = 0;return;}
	//else 
	//vgm_ok = 1;
	//Read data to music_data
	data_offset-=0x10;
	LT_fseek(file,data_offset, SEEK_CUR);
	LT_fread(file, LT_music_size,&LT_music_sdata[0]);
	LT_fclose(file);
	
	//Process_VGM(vgm_dat2,vgm_dat,filename);
	LT_music_offset = 0;
	LT_imfwait = 0;
	LT_music_data = &LT_music_sdata[0];
}


//Adlib SFX INS format 
const unsigned char OPL2_op_table[9] =
  {0x00, 0x01, 0x02, 0x03, 0x09, 0x0a, 0x10, 0x11, 0x12};

const unsigned short OPL2_notetable[12] =
  {340,363,385,408,432,458,485,514,544,577,611,647};

void LT_Play_AdLib_SFX(unsigned char *ins, byte chan, byte octave, byte note){
	unsigned char op = OPL2_op_table[chan];
	unsigned char *data = ins;
	word chanfreq = OPL2_notetable[note];
	word chanoct = octave << 2;
	opl2_out(0xb0 + chan, 0);	// stop old note
	
	// set instrument data
	opl2_out(0x20 + op, data[1]);	//car misc
	opl2_out(0x23 + op, data[0]);	//mod misc
	opl2_out(0x60 + op, data[5]);	//car AD
	opl2_out(0x63 + op, data[4]);	//mod AD
	opl2_out(0x80 + op, data[7]);	//car SR
	opl2_out(0x83 + op, data[6]);	//mod SR
	opl2_out(0xe0 + op, data[10]);	//car wave
	opl2_out(0xe3 + op, data[9]);	//mod wave
	opl2_out(0xc0 + chan, data[8]);	//feedback
	
	//set frequency
	opl2_out(0xa0 + chan, chanfreq & 255);
	opl2_out(0xb0 + chan, ((chanfreq & 768) >> 8) + chanoct | 32);//32 = key push
	
	//set vol
	opl2_out(0x40 + op, data[3]);	//car scale vol
	opl2_out(0x43 + op, data[2]);	//mod scale vol
	
}

//TANDY
void tandy_clear(){
	byte frequency = 0;
	byte attenuation = 0x0F;
	byte i = 0;
	byte reg = 0;
	for (i = 0; i < 4; i++){
		outportb(TANDY_PORT, 0x80 | (reg<<4) | frequency); reg++;
		outportb(TANDY_PORT, 0x80 | (reg<<4) | attenuation); reg++;
	}
}

void Load_Music_Tandy(char *filename, char *dat_string){
	//Read tags
	word file;
	unsigned char *vgm_dat = &LT_music_sdata[0];
	//unsigned char *vgm_dat2 = &LT_music_sdata[0];
	char header[5];
	long tag_offset = 0,data_offset = 0, opl_clock = 0;
	dword size = 0;
	dword file_pos = 0;
	tandy_clear();
	LT_memset(vgm_dat,0,32768);

	asm CLI
	LT_Draw_Text_Box(11,18,16,1,0,0,0,"LOADING:  MUSIC ");
	asm STI
	
	file = LT_fopen(filename,0); 
	
	if(!file) LT_Error("Can't find file $",filename);
	if (dat_string) {DAT_Seek2(file,dat_string);filename = dat_string;}
	else LT_fseek(file, 0, SEEK_SET);
	
	//Check vgm
	LT_fread(file, 4, header);
	if ((header[0] != 0x56) || (header[1] != 0x67) || (header[2] != 0x6d) || (header[3] != 0x20)){
		LT_fclose(file);LT_Error("Not a VGM file $",filename);
	}

	//get absolute file size, absolute tags offset
	LT_fread(file,4,&size); size +=4;
	LT_fseek(file,0x0C, SEEK_CUR);//0x14
	LT_fread(file,4,&tag_offset);//0x18
	if (tag_offset != 0) tag_offset += 0x14;
	
	//get loop start loop length.
	LT_fseek(file,0x04, SEEK_CUR);//0x1C
	LT_fread(file, 2, &vgm_loop);//0x1E
	LT_fseek(file, 2, SEEK_CUR);//0x20
	LT_fread(file, 2, &vgm_loop_size);//0x22
	
	//get absolute data offset.
	file_pos = LT_fseek(file,0x12, SEEK_CUR);//0x34
	LT_fread(file, 4, &data_offset);//0x38
	file_pos +=4; 
	if (!data_offset) data_offset = 0x42;
	else data_offset+=0x34;
	
	//calculate data size
	size -= data_offset;
	if (tag_offset != 0) size -= (size - tag_offset);
	LT_music_size = size;
	if (LT_music_size > 32767) {vgm_ok = 0; LT_Error("VGM bigger than 32kB $",dat_string);}
	//What chip is it? vgm_chip 1 ym3812; 2 ym3526; 3 y8950; 4 SN76489
	file_pos -= 0x2C;
	LT_fseek(file,file_pos, SEEK_SET);//0x0C
	LT_fread(file, 4,&opl_clock);//0x10
	if (!opl_clock) {LT_fclose(file); vgm_ok = 0;return;}
	else vgm_ok = 1;
	//Read data to music_data
	data_offset-=0x10;
	LT_fseek(file,data_offset, SEEK_CUR);
	LT_fread(file,LT_music_size,vgm_dat);
	LT_fclose(file);
	
	//Process_VGM(vgm_dat2,vgm_dat,filename);
	LT_music_offset = 0;
	LT_imfwait = 0;
	LT_music_data = &LT_music_sdata[0];
	
	/*file = LT_fopen("test.mus","wb");
	LT_fwrite(vgm_dat2, 1, vgm_data_size, file);
	LT_fclose(file);*/
}

//Simplified VGM play function
//	Only plays 50/60/70 Hz VGMs
//	Delays are 1 byte, because they are never > 255 in 60 Hz ADLIB/TANDY VGMs
//	VGM commands 0x70 to 0x7F are very small delays, not used here 
void Play_Music_VGM(){
	//word ds_;
	//unsigned char val = 0, reg = 0;
	/*byte dat = 0;

	if (!LT_MUSIC_ENABLE) return;
	while (!LT_imfwait){
		byte dat = LT_music_data[LT_music_offset]; 
		switch(dat){
			case 0x50: //write val+reg SN76489
				LT_music_offset++;
				reg = LT_music_data[LT_music_offset++];
				asm mov dx, TANDY_PORT//0x00C0
				asm mov al, reg; asm out dx, al;
				LT_imfwait = 0;
			break;
			case 0x5A: //write (YM3812, YM3526, Y8950)
			case 0x5B:
			case 0x5C:
				LT_music_offset++;
				reg = LT_music_data[LT_music_offset++];
				val = LT_music_data[LT_music_offset++];
				asm mov dx, ADLIB_PORT; asm mov al, reg; asm out dx, al;
				asm mov cx,6
				wait: asm in ax,dx; asm loop wait;
				asm inc dx; asm mov al, val; asm out dx, al;
				LT_imfwait = 0;
			break;
			case 0x61:
				LT_imfwait += LT_music_data[LT_music_offset+1];
				LT_music_offset+=3;
			break;
			case 0x62: LT_imfwait++; LT_music_offset++; break;
			case 0x63: LT_imfwait += 2; LT_music_offset++; break;
			case 0x66:LT_music_offset = 0; break;//end 
			default: LT_music_offset++; break;
		}
	}
	LT_imfwait--;*/

	if (!LT_MUSIC_ENABLE) return;
	
	asm push ds; asm push si;
	asm les si,LT_music_data;
	asm mov si,LT_music_offset;
	asm mov bx,0
	
	_READ:
		asm cmp LT_imfwait,0; asm je _START; asm jmp _DEC;
		_START:
		asm mov bl,byte ptr es:[si]; asm inc si;
		_TANDY:
		asm cmp bl,0x50; asm jne _OPL2;
			asm mov al,byte ptr es:[si]; asm inc si;
			asm mov dx, TANDY_PORT; asm out dx, al;
			asm mov LT_imfwait,0
			asm jmp _NEXT
		_OPL2:
		asm sub bl,0x5A;
		asm cmp bl,0x02; asm jg _DELAY;	// 5A YM3812, 5B YM3526, 5C Y8950
			asm mov ax,es:[si]; asm add si,2;
			asm mov dx, ADLIB_PORT; asm out dx,al;
			asm mov cx,52; asm rep nop;//wait at least 3.3 microseconds
			asm inc dx; asm xchg ah,al; asm out dx, al;
			asm mov LT_imfwait,0
			asm jmp _NEXT
		_DELAY:
		asm cmp bl,0x61-0x5A; asm jne _DELAY1;
			asm mov ax,es:[si]; asm add si,2; asm add byte ptr LT_imfwait,al;
			asm jmp _NEXT 
		_DELAY1:
		asm sub bl,0x61-0x5A;
		asm cmp bl,0x02; asm jg _END;//delay command => 62 or 63 
			asm add LT_imfwait,bx;
			asm jmp _NEXT
		_END:
		asm cmp bl,0x05; asm jne _NEXT;//end command = 66 - 61;
			asm mov si,0 //reset pointer
		_NEXT:
		asm cmp LT_imfwait,0; asm jne _DEC; 
		asm jmp _READ;
	_DEC:
	asm dec LT_imfwait;
	asm mov LT_music_offset,si
	asm pop si; asm pop ds;
}


void LT_Start_Music(){
	LT_MUSIC_ENABLE = 1;
	opl2_clear();
	tandy_clear();
}

void LT_Stop_Music(){
	LT_MUSIC_ENABLE = 0;
	opl2_clear();
	tandy_clear();
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

void LT_setvect(byte intr, void interrupt (far *func)());
void LT_Disable_Speaker(){
	
	asm in al, 61h        //Disable speaker
	asm and al, 252       
	asm out 61h, al
	
	/*asm cli
	//reset timer
	asm mov al,0x36
	asm out 0x43,al
	asm mov al,0xFF
	asm out 0x42,al
	asm out 0x42,al
	//outportb(0x43, 0x36);
	//outportb(0x40, 0xFF);	//lo-byte
	//outportb(0x40, 0xFF);	//hi-byte
	LT_setvect(0x1C, LT_old_time_handler);
	asm sti*/
}

int LT_PC_Speaker_Playing = 0;
int LT_PC_Speaker_Offset = 0;
byte *LT_PC_Speaker_SFX;
byte LT_PC_Speaker_Size = 16;
void /*interrupt*/ PC_Speaker_SFX_Player(void){
	byte counter;
	int note;
	//asm cli
	//if (LT_PC_Speaker_Playing == 1){
		if (LT_PC_Speaker_Offset != LT_PC_Speaker_Size){
			counter = LT_PC_Speaker_SFX[LT_PC_Speaker_Offset];
			note = LT_PC_Speaker_Note[counter]; // calculated frequency (1193180/Value)
			if (counter){
				asm mov ax, note
				asm out 42h,al
				asm mov al,ah
				asm out 42h,al
				LT_PC_Speaker_Offset++;
			} else LT_PC_Speaker_Offset = LT_PC_Speaker_Size;
			
		} else {
			LT_PC_Speaker_Playing = 0;
			LT_Disable_Speaker();
		}
	//}
	//asm mov al, 020h      
    //asm out 020h, al    //Send 0x20 to 0x20 port (end of interrupt)
	//asm sti
}

void LT_Play_PC_Speaker_SFX(byte *note_array){//77.556
	//unsigned long spd = 1193182/60;= 19886;
	//if (LT_PC_Speaker_Playing == 0) {
		//LT_PC_Speaker_Playing = 0;
		asm in al, 61h			//Enable speaker
		asm or al, 3
		asm out 61h, al
		asm mov al, 0B6h
		asm out 43h,al
		LT_PC_Speaker_Playing = 1;
		LT_PC_Speaker_Offset = 0;
		LT_PC_Speaker_SFX = &note_array[0];
		
		//set timer
		/*asm mov al,0x36
		asm out 0x43,al
		asm mov al,174	//spd % 0x100 lo-byte
		asm out 0x40,al
		asm mov al,77	//spd / 0x100 hi-byte
		asm out 0x40,al
		//set interrupt
		LT_setvect(0x1C, PC_Speaker_SFX_Player);		//interrupt 1C not available on NEC 9800-series PCs.
		*/
	//}
}


