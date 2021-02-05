/* __________________________________________________________________________
  | FireMod 1.05 CopyRight (c) 1995 by FireLight                             |
  | Source Code Released for use with FireLight's mod coders howto document. |
  | Contact at firelght@yoyo.cc.monash.edu.au                                |
  | Didnt comment the interface code, who wants a heap of printf comments.   |
  |--------------------------------------------------------------------------|
  | Legal Notice:  This source is public domain.  This code cannot be        |
  | redistributed commercially, in any form whether source code or binary    |
  | without the prior consent of the author.  Use of this product does not   |
  | cost anything, but on using this you are expected to adhere to the       |
  | following conditions:                                                    |
  | The author is not liable in any way for any loss of data, property,      |
  | brains, or hearing due to the use of this product.  Upon using this      |
  | product you have agreed that the author, Brett Paterson shall not be held|
  | responsible for any misfortune encountered its use.                      |
  |__________________________________________________________________________|

*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <alloc.h>
#undef outp

#define version 05
#define GUSfreq(x)              83117/x

typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;

byte CPU_086 = 0;

typedef struct tagIMFsong{
	long size;
	long offset;
	byte filetype;
	byte *sdata;
} IMFsong;

byte *mod_data;

void LT_Error(char *error, char *file);

extern IMFsong LT_music;
extern void GUSPoke(dword, byte);
extern void GUSSetVolume(byte Volume);
extern void GUSSetBalance(byte, byte);
extern void GUSSetFreq(word);
extern void GUSPlayVoice(byte, dword, dword, dword);
extern void GUSReset(byte);
extern void GUSStopVoice(byte);

dword gusdram = 0;
word Base = 0;
byte mute[8] = {0,0,0,0,0,0,0,0};
char volume[8] = {0,0,0,0,0,0,0,0}; //Volume of channel

word GUSvol[] = {
	0x1500,
	0x9300,0xA900,0xB400,0xBC00,0xC180,0xC580,0xC980,0xCD80,
	0xCF40,0xD240,0xD440,0xD640,0xD840,0xDA40,0xDC40,0xDE40,
	0xDEF0,0xDFA0,0xE1A0,0xE2A0,0xE3A0,0xE4A0,0xE5A0,0xE6A0,
	0xE7A0,0xE8A0,0xE9A0,0xEAA0,0xEBA0,0xECA0,0xEDA0,0xEEA0,
	0xEEF0,0xEFE0,0xEF60,0xF1E0,0xF160,0xF1E0,0xF260,0xF2E0,
	0xF360,0xF3E0,0xF460,0xF4E0,0xF560,0xF5E0,0xF660,0xF6E0,
	0xF760,0xF7E0,0xF860,0xF8E0,0xF960,0xF9E0,0xFA60,0xFAF0,
	0xFB70,0xFBF0,0xFC70,0xFCF0,0xFD70,0xFD90,0xFDB0,0xFDD0
};

word CommandPort = 0x103;
word DRAMAddrLo = 0x43;
word DRAMAddrHi = 0x44;
word StatusPort = 6;              
              
word SelectVoice = 0x102;
word Select_channel = 0;
word Set_volume = 0; 
word Set_freq = 0;                       
word DataLowPort = 0x104;             
word DataHighPort = 0x105;            
word DRAMIOPort = 0x107;              
word WriteVoiceMode = 0;    
word SetVoiceFreq = 1;      
word LoopStartLo = 2;       
word LoopStartHi = 3;       
word SampleEndLo = 4;       
word SampleEndHi = 5;       
word VolRampRate = 6;       
word VolRampStart = 7;      
word VolRampEnd = 8;        
word SetVolume = 9;         
word SampleStartLo = 0x0A;     
word SampleStartHi = 0x0B;     
word VoiceBalance = 0x0C;      
word VolumeCtrl = 0x0D;        
word VoicesActive = 0x0E;      
word DMACtrl = 0x41;           

word Initialize = 0x4C;        
word ReadVolume = 0x89;       
word VoicePosLo = 0x8A;        
word VoicePosHi = 0x8B;        
word ReadVolCtrl = 0x8C;       

int     freq[8];            // amiga frequency of each channel
char  panval[8] = { 3,12,12, 3, 3,12,12, 3 };
int  midival[8];            // midi value of channel
char lastins[8];            // instrument # for each channel
int    porto[8];            // note to port to value


word freqtab[296] = {                   // The sorted amiga table.
	907,900,894,887,881,875,868,862,    // Finetune -8 to -1
	856,850,844,838,832,826,820,814,    // C-1 to finetune +7
	808,802,796,791,785,779,774,768,    // C#1 to finetune +7
	762,757,752,746,741,736,730,725,    // D-1 to finetune +7
	720,715,709,704,699,694,689,684,    // D#1 to finetune +7
	678,675,670,665,660,655,651,646,    // E-1 to finetune +7
	640,636,632,628,623,619,614,610,    // F-1 to finetune +7
	604,601,597,592,588,584,580,575,    // F#1 to finetune +7
	570,567,563,559,555,551,547,543,    // G-1 to finetune +7
	538,535,532,528,524,520,516,513,    // G#1 to finetune +7
	508,505,502,498,494,491,487,484,    // A-1 to finetune +7
	480,477,474,470,467,463,460,457,    // A#1 to finetune +7
	453,450,447,444,441,437,434,431,    // B-1 to finetune +7
	428,425,422,419,416,413,410,407,    // C-2 to finetune +7
	404,401,398,395,392,390,387,384,    // C#2 to finetune +7
	381,379,376,373,370,368,365,363,    // D-2 to finetune +7
	360,357,355,352,350,347,345,342,    // D#2 to finetune +7
	339,337,335,332,330,328,325,323,    // E-2 to finetune +7
	320,318,316,314,312,309,307,305,    // F-2 to finetune +7
	302,300,298,296,294,292,290,288,    // F#2 to finetune +7
	285,284,282,280,278,276,274,272,    // G-2 to finetune +7
	269,268,266,264,262,260,258,256,    // G#2 to finetune +7
	254,253,251,249,247,245,244,242,    // A-2 to finetune +7
	240,238,237,235,233,232,230,228,    // A#2 to finetune +7
	226,225,223,222,220,219,217,216,    // B-2 to finetune +7
	214,212,211,209,208,206,205,203,    // C-3 to finetune +7
	202,200,199,198,196,195,193,192,    // C#3 to finetune +7
	190,189,188,187,185,184,183,181,    // D-3 to finetune +7
	180,179,177,176,175,174,172,171,    // D#3 to finetune +7
	170,169,167,166,165,164,163,161,    // E-3 to finetune +7
	160,159,158,157,156,155,154,152,    // F-3 to finetune +7
	151,150,149,148,147,146,145,144,    // F#3 to finetune +7
	143,142,141,140,139,138,137,136,    // G-3 to finetune +7
	135,134,133,132,131,130,129,128,    // G#3 to finetune +7
	127,126,125,125,123,123,122,121,    // A-3 to finetune +7
	120,119,118,118,117,116,115,114,    // A#3 to finetune +7
	113,113,112,111,110,109,109,108,    // B-3 to finetune +7
};


byte speed, bpm, channels;  // speed, bpm, channels, figure it out.
byte patdelay=0;            // variable storing number of times to delay patn
byte patlooprow=0, patloopno=0; // pattern loop variables.
char row = 64;    			// current row
byte mastervol = 1;			// master volume /64
int  ord;                   // current order being played

long filelen;               // size of file, again for the interface

// SAMPLE STRUCT
typedef struct {
	word length;            // sample length
	char finetune;          // sample finetune value
	byte volume;            // sample default volume
	word loopstart;         // sample loop start
	word loopend;           // sample loop length
	dword offset;           // offset of sample in dram
} Sample;

// SONG STRUCT
struct {
	char name[20];          // song name
	Sample inst[31];        // instrument headers
	byte songLength;        // song length
	byte numpats;           // number of physical patterns
	byte order[128];        // pattern playing orders
} MOD;

void LT_ExitDOS();

// SINGLE NOTE STRUCT
struct Note {
	byte number;          
	int frequency;          
	byte effect;            
	byte eparm;
	byte d0;
	byte d1;
	byte d2;
} *current;

//LT_music.sdata buffer that holds our pattern data
dword offset = 0;
word offset1 = 0;               // offset of note in pattern data buffer.

// PROTOTYPES
extern void interrupt (*LT_old_time_handler)(void); 

void GUSPoke(dword Loc, byte B){
	asm{
		push ax
		push cx
		push dx
		push bx

		mov  dx, [Base]
		add  dx, CommandPort
		mov  ax, DRAMAddrLo
		out  dx, ax

		inc  dx            				// now base+104
		mov  ax, word ptr [Loc]
		out  dx, ax
		dec  dx                       	// now base+103
		mov  ax, DRAMAddrHi
		out  dx, ax

		add  dx, 2                      // now base+105
		mov  al, byte ptr [Loc+2]
		out  dx, al

		add  dx, 2                   	// now base+107
		mov  al, B
		out  dx, al
		
		pop ax
		pop cx
		pop dx
		pop bx
	}
}

void GUSSetVolume(byte Volume){
	asm{
	mov  dx,Set_volume
	mov  ax, 9
	out  dx, ax
	inc  dx
	mov  bx, word ptr Volume  	// put requested volume in here
	shl  bx, 1					// we are looking up a word table, so *2
	mov  ax, word ptr[GUSvol+bx]// now reference the right GUS volume.
	out  dx, ax
	}
}

void GUSSetFreq(word Freq){
	asm{
	mov  dx, Set_freq
	mov  ax, SetVoiceFreq
	out  dx, ax
	inc  dx
	mov  ax, Freq
	out  dx, ax
	}
}

void GUSSetBalance(byte Voice, byte Balance){
	asm{
	mov  dx, [Base]
	add  dx, SelectVoice
	mov  al, Voice
	out  dx, al
	inc  dx
	mov  ax, VoiceBalance
	out  dx, ax
	inc  dx
	inc  dx
	mov  al, Balance
	out  dx, al
	}
}

void GUSStopVoice(byte Voice){
	asm{
	mov  dx, [Base]
	add  dx, SelectVoice
	mov  al, Voice
	out  dx, al

	inc  dx
	mov  al, 080h
	out  dx, al

	add  dx, 2
	in   ax, dx
	mov  cx, ax

	sub  dx, 2
	mov  al, 0
	out  dx, al
	add  dx, 2
	and  cx, 0dfh
	or   cx, 3
	mov  ax, cx
	out  dx, ax

	push dx
	push ax
	mov  dx, 0300h
	in   al, dx
	in   al, dx
	in   al, dx
	in   al, dx
	in   al, dx
	in   al, dx
	in   al, dx
	in   al, dx
	pop  ax
	pop  dx	
	
	sub  dx, 2
	mov  al, 0
	out  dx, al
	add  dx, 2
	mov  ax, cx
	out  dx, ax
	}
}

void GUSPlayVoice(byte Mode, dword VBegin, dword VStart, dword VEnd){
	asm{
	mov  dx,Set_volume			//CommandPort 103h
	mov  al,2
	out  dx, al
	
	//VSTART
	inc  dx                  	//DataLowPort 104h
	mov  ax, Word Ptr [VStart]
	mov  cl,7
	shr  ax,cl
	out  dx,ax
	dec  dx                  //CommandPort 103h
	mov  al,3
	out  dx,al
	inc  dx                  //DataLowPort 104h
	mov  ax, Word Ptr [VStart]
	mov  cl, 9
	shl  ax, cl
	out  dx, ax

	// VBEGIN;
	dec  dx                                      //now CommandPort 103h
	mov  ax, SampleStartLo
	out  dx, ax
	inc  dx                                      //DataLowPort 104h
	mov  ax, Word Ptr [VBegin]
	mov  cl,7
	shr  ax,cl
	out  dx,ax
	/*
	dec  dx                                      //CommandPort 103h
	mov  ax, SampleStartHi
	out  dx, ax
	inc  dx                                      //DataLowPort 104h
	mov  ax, Word Ptr [VBegin]
	mov  cl, 9
	shl  ax, cl
	out  dx, ax
	*/

	//VEND
	dec  dx                                      //CommandPort 103h
	mov  al, 4
	out  dx, al
	inc  dx                                      //DataLowPort 104h
	mov  ax, Word Ptr [VEnd]
	mov  cl,7 
	shr  ax,cl
	out  dx, ax

	dec  dx                                      //CommandPort 103h
	mov  al,5
	out  dx,al
	inc  dx                                      //DataLowPort 104h
	mov  ax,Word Ptr [VEnd]
	mov  cl,9 
	shl  ax,cl
	out  dx,ax

	// MODE
	dec  dx                                 // now 103h
	mov  al, 0
	out  dx, al
	add  dx, 2                              // now 105h
	mov  al, Mode
	out  dx, al
	}
}

void GUSReset(byte NumVoices){
	asm{
		push ax
		push cx
		push dx
		push bx
		
		mov     bx, [Base]
		mov     cx, bx
		add     bx, word ptr [CommandPort]
		add     cx, word ptr [DataHighPort]
		mov     dx, bx
		mov     al, byte ptr [Initialize]
		out     dx, al
		mov     dx, cx
		mov     al, 0
		out     dx, al
		mov     dx, [Base]
		
		push dx
		push ax
		mov  dx, 0300h
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		pop  ax
		pop  dx
	
		mov     dx, bx
		mov     al, 4Ch
		out     dx, al
		mov     dx, cx
		mov     al, 1
		out     dx, al
		mov     dx, [Base]
		
		push dx
		push ax
		mov  dx, 0300h
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		in   al, dx
		pop  ax
		pop  dx
	
		mov     dx, bx
		mov     al, byte ptr [DMACtrl]
		out     dx, al
		mov     dx, cx
		mov     al, 0
		out     dx, al
		mov     dx, bx
		mov     al, 45h
		out     dx, al
		mov     dx, cx
		mov     al, 0
		out     dx, al
		mov     dx, bx
		mov     al, 49h
		out     dx, al
		mov     dx, cx
		mov     al, 0
		out     dx, al

		mov     dx, bx
		mov     al, byte ptr [VoicesActive]
		out     dx, al
		mov     dx, cx
		mov     al, byte ptr [NumVoices]
		dec     al
		or      al, 0C0h
		out     dx, al

		mov     dx, cs:[Base]
		add     dx, word ptr [StatusPort]
		in      al, dx
		mov     dx, bx
		mov     al, byte ptr [DMACtrl]
		out     dx, al
		mov     dx, cx
		in      al, dx
		mov     dx, bx
		mov     al, 49h
		out     dx, al
		mov     dx, cx
		in      al, dx
		mov     dx, bx
		mov     al, 8Fh
		out     dx, al
		mov     dx, cx
		in      al, dx

		push    cx
		mov     cx, 32
	}
VoiceClearLoop:
	asm{
		mov     dx, [Base]
		add     dx, word ptr [SelectVoice]
		mov     ax,dx
		dec     ax
		out     dx, ax
		inc     dx
		mov     al, 0
		out     dx, al
		add     dx, 2
		mov     al, 3   ; // Voice Off
		out     dx, al
		sub     dx, 2
		mov     al, 0Dh
		out     dx, al
		add     dx, 2
		mov     al, 3   ; // Ramp Off
		out     dx, al
		loop    VoiceClearLoop
		pop     cx

		mov     dx, bx
		mov     al, byte ptr [DMACtrl]
		out     dx, al
		mov     dx, cx
		in      al, dx
		mov     dx, bx
		mov     al, 49h
		out     dx, al
		mov     dx, cx
		in      al, dx
		mov     dx, bx
		mov     al, 8Fh
		out     dx, al
		mov     dx, cx
		in      al, dx

		mov     dx, bx
		mov     al, byte ptr [Initialize]
		out     dx, al
		mov     dx, cx
		mov     al, 7
		out     dx, al

		mov     cx, word ptr [NumVoices]
	}
SetRampRateLoop:
	asm{
		mov     dx, [Base]
		add     dx, word ptr [SelectVoice]
		mov     al, byte ptr [NumVoices]
		sub     al, 1
		out     dx, al

		mov     dx, [Base]
		add     dx, word ptr [CommandPort]
		mov     al, byte ptr [VolRampRate]
		out     dx, al
		mov     al, 00111111b
		mov     dx, [Base]
		add     dx, word ptr [DataHighPort]
		out     dx, al

		mov     dx, [Base]
		add     dx, word ptr [CommandPort]
		mov     al, byte ptr [SetVolume]
		
		out     dx, al
		mov     al, 0
		
		mov     dx, [Base]
		add     dx, word ptr [DataLowPort]
		out     dx, al
		loop    SetRampRateLoop

		mov  dx, [Base]
		mov  al, 0
		out  dx, al
		
		pop ax
		pop cx
		pop dx
		pop bx
	}
}

void Error(int code) {
	asm mov ax, 0x3
	asm int 0x10

	textcolor(15);
	textbackground(1);
	printf("FireMOD 1.%02da - CopyRight (c) FireLight, 1995                                   ", version);
	printf("ERROR #%d : ", code);

	switch (code) {
		case 1 : printf("ULTRASND Environment variable not found..\r\n");
				printf("Most likely cause GUS not present..\r\n");
				LT_ExitDOS();
				break;
		case 2 : printf("Module not found.");
				LT_ExitDOS();
				break;
		case 3 : printf("Unknown format.");
				LT_ExitDOS();
				break;
		case 4 : printf("Out of GUS DRAM! go spend some more money..");
				LT_ExitDOS();
				break;
		case 5 : printf("Not enough memory!..");
				sleep(3);
				LT_ExitDOS();
				break;
		case 6 : printf("Usage FMOD <module name>\r\n");
				LT_ExitDOS();
				break;
		default : ;
	};
	printf("\r\n");
	exit(1);
}

int tick = 0;
//This wil play very very, veeery simple mod files.
//Use just 3 parameters: instrument, note and volume (Max = C3F).
//Again, this is to allow mod music on the 8086 8 MHz.
//Mod files supported may be simple, but awesome tracks can be played
//like the ones included in the amiga version of lemmings 3. 
//69.666
void mod_player_fast() {
	byte track, sample, s, eff;
	word vol, s_offset;
	int tone;
	tick++;                  					
	if (tick >= speed) {
		tick = 0;                              
		if (row == 64) {ord++;if (ord >= MOD.songLength) ord=0;row = 0;}		
		offset1 = (MOD.order[ord]<<11)+(row<<5);
		asm les bx,mod_data
		asm add bx,offset1
		for (track=0; track<4; track++) {
			//Read data
			asm mov al,byte ptr es:[bx]
			asm mov sample,al
			asm mov ax,word ptr es:[bx+1]
			asm mov tone,ax
			asm mov al,byte ptr es:[bx+3]
			asm mov eff,al
			asm mov ax,word ptr es:[bx+4]
			asm mov vol,ax
			asm add bx,8
			
			asm push bx
			//Select channel
			asm mov	 dx, Select_channel
			asm mov  al, track
			asm out  dx, al
		
			if (sample)lastins[track] = sample - 1;
			if (eff)GUSSetVolume(vol);
			s = lastins[track];
			s_offset = MOD.inst[s].offset;
			if (tone >= 0){
				freq[track] = freqtab[tone+MOD.inst[s].finetune];
				if (freq[track] != 0) GUSSetFreq(GUSfreq(freq[track]));
			}
			if (sample){
				if (MOD.inst[s].loopend > 2)
					GUSPlayVoice(8,s_offset,s_offset+MOD.inst[s].loopstart,s_offset+MOD.inst[s].loopend);
				else GUSPlayVoice( 0,s_offset,s_offset,s_offset+MOD.inst[s].length);
			}
			asm pop bx
		}
		row++;
	}
	// return the computer to what it was doing before
	asm mov ah,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
}

void StopMOD() {
	int count=0;
	for (count=0; count<4; count++) GUSStopVoice(count);
}

void PlayMOD(byte sp) {
	byte channel;
	for (channel = 0; channel < 4; channel++){
		//Select channel
		asm mov	 dx, Select_channel
		asm mov  al, channel
		asm out  dx, al
		GUSSetVolume(63);
		GUSPlayVoice(0,0,0,0);
		GUSStopVoice(channel);
		volume[channel]=63;
		freq[channel]=0;
		midival[channel]=0;
		lastins[channel]=0;
	}
	GUSSetBalance(0,6); GUSSetBalance(1,6);
	GUSSetBalance(2,18);GUSSetBalance(3,18);	

	speed = sp;                         // start using default speed 6
	row = 0;                           // start at row 0
	ord = 0;                           // start at order 0
	
	mod_data = LT_music.sdata;
}

int LT_LoadMOD(char *filename) {
	FILE *handle;
	int count=0, count2=0, pattcount, frequency;
	dword gusoff, size;
	byte dummy[22];
	byte part[4];
	fpos_t pos;

	// if there is no filename extension, then add one
	for (count=0; count<strlen(filename); count++) if (filename[count]=='.') count2=1;
	if (count2==0) strcat(filename,".mod");

	// open the file
	if ((handle = fopen(filename, "rb")) == NULL) return 0;

	/**********************/
	/***  VERIFICATION  ***/
	/**********************/
	fseek(handle, 1080, SEEK_SET);				// skip to offset 1080
	for (count=0; count<4; count++) part[count]=fgetc(handle);
	// check what MOD format
	if (strncmp(part, "M.K.",4) == 0) channels = 4;
	else Error(3);                              // not a recognized format

	fseek(handle, 0, SEEK_SET);                 // start at the beginning.
	fread(MOD.name, 20, 1, handle);       		// read in module name.

	/*********************************/
	/***  LOAD SAMPLE INFORMATION  ***/
	/*********************************/
	for (count=0; count<31; count++) {          // go through 31 instruments.
		fread(&dummy, 22, 1, handle);
		//for (count2=0; count2<22; count2++) if (MOD.inst[count].name[count2]<32) MOD.inst[count].name[count2] = 32;

		part[0] = fgetc(handle);                // get samples length
		part[1] = fgetc(handle);
		MOD.inst[count].length = ((part[0] * 0x100) + part[1]) * 2;

		MOD.inst[count].finetune = fgetc(handle);// get finetune
		if (MOD.inst[count].finetune > 7)
			MOD.inst[count].finetune -= 16;

		MOD.inst[count].volume = fgetc(handle); // get sample default volume

		// get sample loop start
		MOD.inst[count].loopstart = ((fgetc(handle) << 8)+fgetc(handle))*2;
		// get sample loop end
		MOD.inst[count].loopend = ((fgetc(handle) << 8)+fgetc(handle))*2 +
			MOD.inst[count].loopstart;
		// incase the loopend is past the end of the sample fix it
		if (MOD.inst[count].loopend > MOD.inst[count].length)
			MOD.inst[count].loopend = MOD.inst[count].length;
	}

	/**************************************/
	/***  LOAD ORDER AND SIGNATURE DATA ***/
	/**************************************/
	MOD.songLength = fgetc(handle);             // get number of orders.
	fgetc(handle);                              // unused byte, skip it

	MOD.numpats = 0;                            // highest pattern is now 0
	for (count=0; count<128; count++) {
		MOD.order[count] = fgetc(handle);       // get 128 orders.
		if (MOD.order[count] > MOD.numpats)
			MOD.numpats = MOD.order[count];     // get highest pattern.
	}

	_fmemset(LT_music.sdata, 0, size);                 // wipes buffer clean

	for (count=0; count<4; count++) fgetc(handle);// at 1080 again, skip it

	offset = 0;                                  // set buffer offset to 0
	/**************************/
	/***  LOAD PATTERN DATA ***/
	/**************************/
	for (pattcount=0; pattcount <= MOD.numpats; pattcount++) {
		for (row=0; row<64; row++) {   			// loop down through 64 notes.
			for (count=0; count<channels; count++) {
				if (offset > 65536) Error(5);
				// point our little note structure to LT_music.sdata
				current = (struct Note *)(LT_music.sdata + offset);
				// load up 4 bytes of note information from file
				for (count2=0; count2<4; count2++) part[count2]=fgetc(handle);
				// store sample number
				current->number = ((part[0] & 0xF0) + (part[2] >> 4));
				// get frequency
				frequency = ((part[0] & 0xF) << 8) + part[1];
				// do the look up in the table against what is read in.
				// store note (in midi style format)
				current->frequency = -1;
				for (count2=1;count2<37; count2++) {
					if (frequency > freqtab[count2<<3]-3 &&
						frequency < freqtab[count2<<3]+3 )
						current->frequency = count2<<3;
				}
				// store effects and arguments
				current->effect = part[2] & 0xF; 	// Effect
				current->eparm = part[3];        	// parameter
				
				offset += 8;                 		// increment buffer pos
			}
		}
	}

	/********************************/
	/***  LOAD SAMPLE DATA (64Kb)***/
	/********************************/
	gusoff =0;
	for (count = 0; count <31; count++) {
		MOD.inst[count].offset = gusoff;        // get offsets in GUS memory
		// upload each sample, reading and poking them to GUS byte by byte.
		for (count2=0; count2<MOD.inst[count].length; count2++) {
			GUSPoke(gusoff, fgetc(handle));		// read one byte and poke it.
			gusoff++;                           // increment offset
		}
		GUSPoke(gusoff, 0);						// remove ultraclick bugfix
		gusoff++;
	}
	fclose(handle);                             // close mod file
	return 1;                                // successful load
}

void LT_Init_GUS(byte channels){
	char *ptr;
	ptr = getenv("ULTRASND");               	// grab ULTRASND env variable
	if (ptr == NULL) ;//Error(1);              	// if it doesnt exist spit it
	else Base = ((ptr[1]-48)*0x10)+0x200;   	// else grab the hex base address
	GUSReset(channels);             			// initialize GUS with 4 voices (44.1khz)
	Select_channel = Base+0x102;
	Set_freq = Select_channel + 1;
	Set_volume = Base+0x103;
	//printf("\nGUS OK at Base Port %x.\n", Base);
	sleep(1);
}


