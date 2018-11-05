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
#include "fmod.h"

#define version 05
#define GUSfreq(x)              83117/x


byte GUSPeek(dword Loc){
	asm{
		mov  dx, [Base]
		add  dx, word ptr [CommandPort]
		mov  ax, word ptr [DRAMAddrLo]
		out  dx, ax

		inc  dx
		mov  ax, word ptr [Loc]
		out  dx, ax
		dec  dx
		mov  ax, word ptr [DRAMAddrHi]
		out  dx, ax
		add  dx, 2
		mov  ax, word ptr [Loc+2]
		out  dx, ax

		add  dx, 2
		in   ax, dx
	}
	return 4;
}

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

void GUSSetVolume(byte Voice, byte Volume){
	asm{
	push es
	push bx

	mov  bx, word ptr Voice
	mov  ax, word ptr [mute+bx]			// check for muted channel
	cmp  ax, 0
	je   nomute
	mov  Volume, 0				// if it is set the channels volume to 0
	}
nomute:
	asm{
	mov  dx, [Base]
	add  dx, SelectVoice
	mov  al, Voice
	out  dx, al
	inc  dx
	mov  ax, SetVolume
	out  dx, ax
	inc  dx
	mov  bx, word ptr Volume  	// put requested volume in here
	shl  bx, 1					// we are looking up a word table, so *2
	mov  ax, word ptr[GUSvol+bx]// now reference the right GUS volume.
	out  dx, ax

	pop  bx
	pop  es
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

void GUSSetFreq(byte Voice, word Freq){
	asm{
	mov  dx, [Base]
	add  dx, SelectVoice
	mov  al, Voice
	out  dx, al
	inc  dx
	mov  ax, SetVoiceFreq
	out  dx, ax
	inc  dx
	mov  ax, Freq
	out  dx, ax
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

void GUSPlayVoice(byte Voice, byte Mode, dword VBegin, dword VStart, dword VEnd){
	asm{
	mov  dx, [Base]
	add  dx, SelectVoice
	mov  al, Voice
	out  dx, al

	xor  ax, ax
	mov  al, Voice
	push ax
	
	//stop voice
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
	
	pop  ax
	//voice stopped
	
	mov  dx, [Base]
	add  dx, 0103h		        //CommandPort 103h

	//VSTART
	mov  al, 2
	out  dx, al
	inc  dx                  	//DataLowPort 104h
	mov  ax, Word Ptr [VStart]
	push cx
	mov  cx, Word Ptr [VStart+2]
	mov  bx,cx
	
	mov  cl,7
	shr  ax,cl
	shr  cx,cl
	mov  cl,9
	shl  bx,cl
	or   ax,bx
	
	pop  cx
	out  dx, ax

	dec     dx                  //CommandPort 103h
	mov  al, 3
	out  dx, al
	inc     dx                  //DataLowPort 104h
	mov     ax, Word Ptr [VStart]
	mov  cl, 9
	shl  ax, cl
	out  dx, ax

	// VBEGIN;
	dec     dx                                      //now CommandPort 103h
	mov     ax, SampleStartLo               //0Ah
	out  	dx, ax
	inc     dx                                      //DataLowPort 104h
	mov     ax, Word Ptr [VBegin]    //high part in ax
	push cx
	mov     cx, Word Ptr [VBegin+2]  //low part in cx
	
	mov  bx,cx
	mov  cl,7
	shr  ax,cl
	shr  cx,cl
	mov  cl,9
	shl  bx,cl
	or   ax,bx	
	
	pop  cx
	out  dx, ax

	dec     dx                                      //CommandPort 103h
	mov     ax, SampleStartHi
	out  	dx, ax
	inc     dx                                      //DataLowPort 104h
	mov     ax, Word Ptr [VBegin]
	mov  cl, 9
	shl  ax, cl
	out  dx, ax

	//VEND
	dec     dx                                      //CommandPort 103h
	mov  al, 4
	out  dx, al
	inc     dx                                      //DataLowPort 104h
	mov     ax, Word Ptr [VEnd]
	push cx
	mov     cx, Word Ptr [VEnd+2]

	mov  bx,cx
	mov  cl,7 
	shr  ax,cl
	shr  cx,cl
	mov  cl,9
	shl  bx,cl
	or   ax,bx	
	
	pop  cx
	out  dx, ax

	dec     dx                                      //CommandPort 103h
	mov  al,5
	out  dx,al
	inc     dx                                      //DataLowPort 104h
	mov     ax,Word Ptr [VEnd]
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
	textcolor(7);
	textbackground(0);
	printf("ERROR #%d : ", code);

	switch (code) {
		case 1 : printf("ULTRASND Environment variable not found..\r\n");
				printf("Most likely cause GUS not present..\r\n");
				break;
		case 2 : printf("Module not found.");
				break;
		case 3 : printf("Unknown format.");
				break;
		case 4 : printf("Out of GUS DRAM! go spend some more money..");
				break;
		case 5 : printf("Not enough memory!..");
				break;
		case 6 : printf("Usage FMOD <module name>\r\n");
				break;
		default : ;
	};
	printf("\r\n");
	exit(1);
}

void SetTimerSpeed() {
	int Speed;
	if (bpm == 0) Speed = 0;
	else Speed = 1193180/(bpm*2/5);
	asm {
		mov dx, 0x43
		mov al, 0x36
		out dx, al
		mov dx, 0x40
		mov ax, Speed
		out dx, al
		mov cl, 8
		shr ax,cl
		out dx, al
	}
}

void UpdateNote() {
	byte track, sample, eparmx, eparmy;
	word soff;
	int period;

	// calculate where in the pattern buffer we should be according to
	// the pattern number and row.
	offset = (long)(channels)*256L*MOD.order[ord]+(4L*(long)(row)*channels);

	// new row? now we loop through each channel until we have finished
	for (track=0; track<channels; track++) {
		current = (struct Note *)(patbuff+offset);	// sit our note struct at the
											// right offset in pattn buffer
		period = current -> period;        	// get period
		sample = current -> number;         // get instrument #
		eparmx = current -> eparm >> 4;     // get effect param x
		eparmy = current -> eparm & 0xF;    // get effect param y

		// for the interface, the next 3 lines just store the effect # to
		// remember for later
		geffect[track] = current -> effect;
		if (geffect[track] == 0xE) geffect[track]+=2+eparmx;
		if (current->effect == 0 && current -> eparm > 0) geffect[track] = 32;

		if (sample > 0) {                   // if instrument > 0 set volume
			lastins[track] = sample - 1;    // remember the sample #
			volume[track] = MOD.inst[sample-1].volume;  // set chan's vol
			GUSSetVolume(track, volume[track]*mastervol/64);
		}
		if (period >= 0) {                  // if period >= 0 set freq
			midival[track] = period;        // remember the note
			// if not a porta effect, then set the channels frequency to the
			// looked up amiga value + or - any finetune
			if (current->effect != 0x3 && current->effect != 0x5) freq[track]=
				freqtab[midival[track]+MOD.inst[lastins[track]].finetune];
		}
		soff = 0;                           // sample offset = nothing now

		switch (current -> effect) {
			case 0x0: break;  // dont waste my time in here!!!
			//   0x1: not processed on tick 0
			//   0x2: not processed on tick 0
			case 0x3:
			case 0x5: porto[track] = freqtab[midival[track]+MOD.inst[lastins[track]].finetune];
					  if (current -> eparm > 0 && current->effect == 0x3)
					  portsp[track] = current -> eparm;
					  period = -1;
					  break;
			case 0x4: if (eparmx > 0) vibspe[track] = eparmx;
					  if (eparmy > 0) vibdep[track] = eparmy;
					  break;
			//   0x6: not processed on tick 0
			case 0x7: if (eparmx > 0) tremspe[track] = eparmx;
					  if (eparmy > 0) tremdep[track] = eparmy;
					  break;
			case 0x8: if (current -> eparm == 0xa4) panval[track]=7;
					  else panval[track] = (current -> eparm >> 3)-1;
					  if (panval[track] < 0) panval[track] = 0;
					  GUSSetBalance(track, panval[track]);
					  break;
			case 0x9: soff = current -> eparm << 8;
					  if (soff > MOD.inst[lastins[track]].length) soff =
								 MOD.inst[lastins[track]].length;
					  break;
			//   0xA: processed in UpdateEffect() (tick based)
			case 0xB: ord = current->eparm;
					  row = 0;
					  if (ord >= MOD.songLength) ord=0;
					  break;
			case 0xC: volume[track] = current -> eparm;
					  if (volume[track] < 0)  volume[track] = 0;
					  if (volume[track] > 64) volume[track] = 64;
					  GUSSetVolume(track, volume[track]*mastervol/64);
					  break;
			case 0xD: row = eparmx*10 + eparmy -1;
					  if (row > 63) row =0;
					  ord++;
					  if (ord >= MOD.songLength) ord=0;
					  break;
			case 0xF: if (current->eparm < 0x20) speed = current->eparm;
					  else { bpm = current->eparm; SetTimerSpeed(); }
					  break;
			case 0xE: switch (eparmx) {
						  case 0x1: freq[track] -= eparmy;
									break;
						  case 0x2: freq[track] += eparmy;
									break;
						  //   0x3: not supported (glissando)
						  //   0x4: not supported (set vibrato waveform)
						  case 0x5: MOD.inst[sample-1].finetune = eparmy;
									if (MOD.inst[sample-1].finetune > 7)
									MOD.inst[sample-1].finetune -= 16;
									break;
						  case 0x6: if (eparmy == 0) patlooprow = row;
									else {
										if (patloopno == 0) patloopno=eparmy;
										else patloopno--;
										if (patloopno > 0) row = patlooprow-1;
									}
									break;
						  //   0x6: not supported (set tremolo waveform)
						  case 0x8: panval[track] = eparmy;
									GUSSetBalance(track, eparmy);
									break;
						  //   0x9: not processed on tick 0
						  case 0xA: volume[track] += eparmy;
									if (volume[track] > 64) volume[track]=64;
									GUSSetVolume(track, volume[track]*mastervol/64);
									break;
						  case 0xB: volume[track] -= eparmy;
									if (volume[track] < 0)  volume[track]=0;
									GUSSetVolume(track, volume[track]*mastervol/64);
									break;
						  //   0xC: not processed on tick 0
						  case 0xD: period = -1;
									break;
						  case 0xE: patdelay = eparmy;
									break;
						  //   0xF: not supported (Invert loop)
					  };
					  break;
		};

		// set the frequency on the GUS for the voice, as long as its > 0
		if (freq[track] > 0) GUSSetFreq(track, GUSfreq(freq[track]));
		// only play the note if there is a note or there is an effect 9xy
		if (period >= 0 || soff > 0x00FF) {
			sinepos[track]=0;                  // retrig waveform
			sineneg[track]=0;
			// if sample has a loop then loop it.
			if (MOD.inst[lastins[track]].loopend > 2) GUSPlayVoice(track, 8,
				MOD.inst[lastins[track]].offset+soff,
				MOD.inst[lastins[track]].offset+MOD.inst[lastins[track]].loopstart,
				MOD.inst[lastins[track]].offset+MOD.inst[lastins[track]].loopend);
			// else just play the sample straight and no loop.
			else GUSPlayVoice(track, 0,
				MOD.inst[lastins[track]].offset+soff,
				MOD.inst[lastins[track]].offset,
				MOD.inst[lastins[track]].offset+MOD.inst[lastins[track]].length);
		}
		offset += 4;                         // increment our note pointer.
	}
}

void doporta(byte track) {
	if (freq[track] < porto[track]) {
		freq[track] += portsp[track];
		if (freq[track] > porto[track]) freq[track]=porto[track];
	}
	if (freq[track] > porto[track]) {
		freq[track] -= portsp[track];
		if (freq[track] < 1) freq[track]=1;
		if (freq[track] < porto[track]) freq[track]=porto[track];
	 }
	 GUSSetFreq(track, GUSfreq(freq[track]));
}

void dovibrato(byte track) {
	int vib;

	vib = vibdep[track]*sintab[sinepos[track]] >> 7; // div 128
	if (sineneg[track] == 0) GUSSetFreq(track, GUSfreq(freq[track]+vib));
	else                     GUSSetFreq(track, GUSfreq(freq[track]-vib));

	sinepos[track]+=vibspe[track];
	if (sinepos[track] > 31) {
		sinepos[track] -=32;
		sineneg[track] = ~sineneg[track];		// flip pos/neg flag
	}
}

void dotremolo(byte track) {
	int vib;

	vib = tremdep[track]*sintab[sinepos[track]] >> 6; // div64
	if (sineneg[track] == 0) {
		if (volume[track]+vib > 64) vib = 64-volume[track];
		GUSSetVolume(track, (volume[track]+vib)*mastervol/64);
	}
	else {
		if (volume[track]-vib < 0) vib = volume[track];
		GUSSetVolume(track, (volume[track]-vib)*mastervol/64);
	}

	sinepos[track]+= tremspe[track];
	if (sinepos[track] > 31) {
		sinepos[track] -=32;
		sineneg[track] = ~sineneg[track];			// flip pos/neg flag
	}
}

void UpdateEffect(int tick) {
	byte track, effect, eparmx, eparmy;

	if (row < 1) return;                    // if at row 0 nothing to update
	offset -= (4L * channels);              // go back 4 bytes in buffer
											// becuase update note did +4
	for (track=0; track<channels; track++) {// start counting through tracks
		current = (struct Note *)(patbuff+offset); // point our Note to right spot

		effect = current -> effect;         // grab the effect number
		eparmx = current -> eparm >> 4;     // grab the effect parameter x
		eparmy = current -> eparm & 0xF;    // grab the effect parameter y

		if (freq[track] == 0) goto skip;

		switch(effect) {
			case 0x0: if (current -> eparm > 0) {
						  switch (tick%3) {
							  case 0: GUSSetFreq(track, GUSfreq(freq[track]));
									  break;
							  case 1: GUSSetFreq(track, GUSfreq(freqtab[midival[track]+(8*eparmx)+MOD.inst[lastins[track]].finetune]));
									  break;
							  case 2: GUSSetFreq(track, GUSfreq(freqtab[midival[track]+(8*eparmy)+MOD.inst[lastins[track]].finetune]));
									  break;
						  };
					  }
					  break;

			case 0x3: doporta(track);
					  break;

			case 0x4: dovibrato(track);
					  break;

			case 0x1: freq[track]-= current -> eparm;      	// subtract freq
					  GUSSetFreq(track, GUSfreq(freq[track]));
					  if (freq[track] < 54) freq[track]=54;	// stop at C-5
					  break;

			case 0x2: freq[track]+= current -> eparm;
					  GUSSetFreq(track, GUSfreq(freq[track]));
					  break;


			case 0x5: doporta(track);
					  volume[track] += eparmx - eparmy;
					  if (volume[track] < 0)  volume[track] = 0;
					  if (volume[track] > 64) volume[track] = 64;
					  GUSSetVolume(track, volume[track]*mastervol/64);
					  break;

			case 0x6: dovibrato(track);
					  volume[track] += eparmx - eparmy;
					  if (volume[track] < 0)  volume[track] = 0;
					  if (volume[track] > 64) volume[track] = 64;
					  GUSSetVolume(track, volume[track]*mastervol/64);
					  break;

			case 0x7: dotremolo(track);
					  break;

			case 0xA: volume[track] += eparmx - eparmy;
					  if (volume[track] < 0)  volume[track] = 0;
					  if (volume[track] > 64) volume[track] = 64;
					  GUSSetVolume(track, volume[track]*mastervol/64);
					  break;

			case 0xE: switch(eparmx) {
						  case 0xC: if (eparmy == tick) {
										volume[track] = 0;
										GUSSetVolume(track, volume[track]);
									}
									break;

						  case 0x9: if (tick % eparmy == 0) {
									if (MOD.inst[lastins[track]].loopend > 2)
										 GUSPlayVoice(track, 8, MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].loopstart,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].loopend);
									else GUSPlayVoice(track, 0, MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].length);
									}
									break;
						  case 0xD:	if (eparmy==tick) {
									if (MOD.inst[lastins[track]].loopend > 2)
										 GUSPlayVoice(track, 8, MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].loopstart,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].loopend);
									else GUSPlayVoice(track, 0, MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset,
										   MOD.inst[lastins[track]].offset+
										   MOD.inst[lastins[track]].length);
									}
									break;
					  };
					  break;
		};
skip:
		offset+=4;      // increment our buffer to next note
	}
}

int tick = 0;
void interrupt mod_player() {
	tick++;                  					// increment the tick value
	if (tick >= speed) {
		tick = 0;                               // set the tick to nothing
		if (row == 64) {                        // if end of pattn (64)
			ord++;                              // next order
			if (ord >= MOD.songLength) ord=0;   // if end goto 1st order
			row = 0;                            // start at top of pattn
		}

		if (patdelay == 0) {                    // if there is no pat delay
			UpdateNote();          				// Update and play the note
			row++;                              // increment the row
		}
		else patdelay --;                       // else decrement pat delay
	}
	else UpdateEffect(tick);                    // Else update the effects

	// return the computer to what it was doing before
	outportb(0x20, 0x20);
}

void StopMOD() {
	int count=0;
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C,oldmodhandler);       // put the old timer back on its vector
	bpm=0;                        	  // set bpm =0 (code for settimerspeed)
	SetTimerSpeed();              	  // tell settimerspeed to reset to normal and stop all voices
	for (count=0; count<channels; count++) GUSStopVoice(count);
	farfree(patbuff);                 // free the pattern data memory.
}

void PlayMOD() {
	int count;
	unsigned long spd = 1193182/50;
	
	for (count=0; count<channels; count++) {
		GUSSetVolume(count, 0);       			// turn the volume right down
		GUSSetBalance(count, panval[count]); 	// set the default panning
		GUSPlayVoice(count, 0,0,0,0); 			// set all voices at 0
		GUSStopVoice(count);          			// stop all the voices.

		volume[count]=0;						// clear all variables of
		freq[count]=0;							// rubbish
		midival[count]=0;
		lastins[count]=0;
		porto[count]=0;
		portsp[count]=0;
		vibspe[count]=0;
		vibdep[count]=0;
		tremspe[count]=0;
		tremdep[count]=0;
		sinepos[count]=0;
		sineneg[count]=0;
		geffect[count]=0;

	}
	speed = 6;                         // start using default speed 6
	bpm = 125;                         // start using default BPM of 125
	row = 0;                           // start at row 0
	ord = 0;                           // start at order 0
	SetTimerSpeed();                   // set the timer to 125BPM
	oldmodhandler = _dos_getvect(0x1C); // save old timer's handler vector
	//set interrupt and start playing music
	
	_disable(); //disable interrupts
	outportb(0x43, 0x36);
	outportb(0x40, spd);	//lo-byte
	outportb(0x40, spd >> 8);	//hi-byte
	setvect(0x1C,  mod_player);		//interrupt 1C not available on NEC 9800-series PCs.
	_enable();  //enable interrupts	
}

int LoadMOD(char *filename) {
	FILE *handle;
	int count=0, count2=0, pattcount, period;
	dword gusoff, size;
	byte part[4];
	fpos_t pos;

	// if there is no filename extension, then add one
	for (count=0; count<strlen(filename); count++) if (filename[count]=='.') count2=1;
	if (count2==0) strcat(filename,".mod");

	// open the file
	if ((handle = fopen(filename, "rb")) == NULL) return FALSE;

	/**********************/
	/***  VERIFICATION  ***/
	/**********************/
	fseek(handle, 1080, SEEK_SET);				// skip to offset 1080
	for (count=0; count<4; count++) part[count]=fgetc(handle);
												// check what MOD format
	if      (strncmp(part, "M.K.",4) == 0) channels = 4;
	else if (strncmp(part, "6CHN",4) == 0) channels = 6;
	else if (strncmp(part, "8CHN",4) == 0) channels = 8;
	else Error(3);                              // not a recognized format

	fseek(handle, 0, SEEK_SET);                 // start at the beginning.
	fread(MOD.name, 20, 1, handle);       		// read in module name.
	for (count2=0; count2<20; count2++) if (MOD.name[count2]<32) MOD.inst[count].name[count2] = 32;

	/*********************************/
	/***  LOAD SAMPLE INFORMATION  ***/
	/*********************************/
	for (count=0; count<31; count++) {          // go through 31 instruments.
		fread(MOD.inst[count].name, 22, 1, handle);
		for (count2=0; count2<22; count2++) if (MOD.inst[count].name[count2]<32) MOD.inst[count].name[count2] = 32;

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
	if ((patbuff = farcalloc(65535,sizeof(byte))) == NULL) Error(5);
	_fmemset(patbuff, 0, size);                 // wipes buffer clean

	for (count=0; count<4; count++) fgetc(handle);// at 1080 again, skip it

	offset =0;                                  // set buffer offset to 0
	/**************************/
	/***  LOAD PATTERN DATA ***/
	/**************************/
	for (pattcount=0; pattcount <= MOD.numpats; pattcount++) {
		for (row=0; row<64; row++) {   			// loop down through 64 notes.
			for (count=0; count<channels; count++) {
				// point our little note structure to patbuff
				current = (struct Note *)(patbuff + offset);

				// load up 4 bytes of note information from file
				for (count2=0; count2<4; count2++) part[count2]=fgetc(handle);

				// store sample number
				current -> number = ((part[0] & 0xF0) + (part[2] >> 4));

				// get period
				period = ((part[0] & 0xF) << 8) + part[1];

				// do the look up in the table against what is read in.
				// store note (in midi style format)
				current -> period = -1;
				for (count2=1;count2<37; count2++) {
					if (period > freqtab[count2*8]-3 &&
						period < freqtab[count2*8]+3 )
						current -> period = (count2)*8;
				}
				// store effects and arguments
				current -> effect = part[2] & 0xF; 	// Effect
				current -> eparm = part[3];        	// parameter

				offset += 4;                  		// increment buffer pos
			}
		}
	}

	/*************************/
	/***  LOAD SAMPLE DATA ***/
	/*************************/
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

	return TRUE;                                // successful load
}

void LT_Init_GUS(int channels){
	char *ptr;
	ptr = getenv("ULTRASND");               // grab ULTRASND env variable
	if (ptr == NULL) Error(1);              // if it doesnt exist spit it
	else Base = ((ptr[1]-48)*0x10)+0x200;   // else grab the hex base address
	GUSReset(channels);             				// initialize GUS with 14 voices (44.1khz)
	printf("\nGUS OK at Base Port %x.\n", Base);
}

int i = 0;
void main() {
	
	LT_Init_GUS(4);
	
	LoadMOD("MOD/blues2.mod");  				// load in file.

	PlayMOD();                              // GO!
	printf("\nPlaying MOD");
	while (i < 2500){
		i++;
		while ((inp(0x03da) & 0x08));
		while (!(inp(0x03da) & 0x08));
	}
	StopMOD();        // Unhook everything and stop.
}
