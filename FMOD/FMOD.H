#define TRUE 1
#define FALSE 0

typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;

extern byte GUSPeek(dword);
extern void GUSPoke(dword, byte);
extern void GUSSetVolume(byte Voice, byte Volume);
extern void GUSSetBalance(byte, byte);
extern void GUSSetFreq(byte, word);
extern void GUSPlayVoice(byte, byte, dword, dword, dword);
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
byte  portsp[8];            // porta speed
byte  vibspe[8];            // vibrato speed
byte  vibdep[8];            // vibrato depth
byte tremspe[8];            // tremolo speed
byte tremdep[8];            // tremolo depth
byte sinepos[8];            // position in sine wave
byte sineneg[8];            // toggle to add or subtract sine value
byte geffect[8];            // effect played at the time (for interface)

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


// This table is for the interface to display the notes.
char *notetab[37] = {
	"   ","C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","B-1",
	"C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","B-2",
	"C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","B-3"
};


// This table is for vibrato and contains half a sine wave.
byte sintab[32] = {
	   0, 24, 49, 74, 97,120,141,161,
	 180,197,212,224,235,244,250,253,
	 255,253,250,244,235,224,212,197,
	 180,161,141,120, 97, 74, 49, 24
};

byte speed, bpm, channels;  // speed, bpm, channels, figure it out.
byte patdelay=0;            // variable storing number of times to delay patn
byte patlooprow=0, patloopno=0; // pattern loop variables.
char row, mastervol=64;     // same here, current row, and master volume
int  ord;                   // current order being played
int clock=0;                // a floating point value for our home made clock
							// take all references to this out, its just for
							// the interface and I think it slows things down
long filelen;               // size of file, again for the interface

// SAMPLE STRUCT
typedef struct {
	char name[23];			// instrument name
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

// SINGLE NOTE STRUCT
struct Note {
	byte number:5;          // sample being played              5 bits
	int period:11;          // frequency being played at     + 11 bits = 16.
	byte effect;            // effect number                 +  8
	byte eparm;             // effect parameter              +  8 = 4 bytes!
} *current;

char huge *patbuff;         // buffer that holds our pattern data
dword offset;               // offset of note in pattern data buffer.

// PROTOTYPES
void interrupt (*oldmodhandler)(void);

