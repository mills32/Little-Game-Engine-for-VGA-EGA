/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program.                     
	Remember to compile in the LARGE memory model!                        

	Please feel free to copy this source code.                            

##########################################################################*/

#include "n_engine.h"

//Predefined struct:
IMFsong song;	//Ok... 1 song in memory. 

//GLOBAL VARIABLES

//debugging
float t1 = 0;
float t2 = 0;

//This points to video memory.
byte *MCGA=(byte *)0xA0000000L; 

//timer     		
void interrupt (*old_time_handler)(void); 	
long time_ctr;

//ADLIB
const int opl2_base = 0x388;	
unsigned char shadow_opl[256];
long next_event;

// this points to the 18.2hz system clock. 
word *my_clock=(word *)0x0000046C;    		
word start;

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 0;
int current_x = 0;
int last_x = 0;
int current_y = 0;
int last_y = 0;
int scrollU_posy = 0;
int scrollD_posy = 0;
int scrollL_posx = 0;
int scrollR_posx = 0;
int map_offset = 0;
int fixD = 0;
int fixL = 0;

//end of global variables

int read_keys(){
	//Taken from commander keen source :)
	byte temp,k,key;
	k = inportb(0x60);	// Get the scan code
	// Tell the XT keyboard controller to clear the key
	outportb(0x61,(temp = inportb(0x61)) | 0x80);
	outportb(0x61,temp);
	//clear buffer
	*(char*)(0x0040001A) = 0x20;
    *(char*)(0x0040001C) = 0x20;
	switch (k){
		case 0x48: return 0; //UP
		case 0x50: return 1; //DOWN
		case 0x4B: return 2; //LEFT
		case 0x4D: return 3; //RIGHT
		case 0x20: return 4; //D (JUMP)
		case 0x1f: return 5; //S (ACTION)
		case 0x01: return 6; //ESC
		default: return -1;
	}
}

void check_hardware(){
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
	wait(30);
	return;
	_8086_8088:
	printf("\nCPU: 8088 - Little engine will work a bit slow\n");
	printf("\nCPU: 8086 - Little engine will work great!!\n (Just don't use more than 4 sprites)\n");
	wait(20);
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

void ADLIB_Detect(){ //from ASS
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
		wait(20);
        return;
    } else {
		printf("AdLib card not detected.\n");
		wait(20);
        return;
    }
}

void MCGA_WaitVBL(){
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

//Hardware scrolling
void MCGA_Scroll(word x, word y){
	byte p[4] = {0,2,4,6};
	byte pix = x & 3; 	//pixel panning 
	x=x>>2;				//x/4
	y=(y<<6)+(y<<4);	//(y*64)+(y*16) = y*80;

	//change pixel panning register 
	inp(0x3da);
	outp(0x3c0, 0x33);		
	outp(0x3c0, p[pix]);
	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
	outport(0x03d4, 0x0C | (x+y & 0xff00));
	outport(0x03d4, 0x0D | (x+y << 8));
}

void MCGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&MCGA[0],0,(320*200)/4);
}

void set_mode(byte mode){
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = mode;
	int86(0x10, &regs, &regs);
	//320x200
	// turn off write protect */
    word_out(0x03d4, V_RETRACE_END, 0x2c);
    outp(MISC_OUTPUT, 0xe7);
	//do not touch... working on old crts!
	/*word_out(0x03d4,H_TOTAL, 80);  /**/             
	word_out(0x03d4,H_DISPLAY_END, (304>>2)-1);   //HORIZONTAL RESOLUTION = 304   
	//word_out(0x03d4,H_BLANK_START, 0);        
	//word_out(0x03d4,H_BLANK_END,(320>>2)-1);         
	//word_out(0x03d4,H_RETRACE_START,3); /**/      
	//word_out(0x03d4,H_RETRACE_END,(320>>2)-1);        
	/*word_out(0x03d4,V_TOTAL, 80);  /**/             
	/*word_out(0x03d4,OVERFLOW, 80);  /**/            
	/*word_out(0x03d4,MAX_SCAN_LINE,0);/**/         
	//word_out(0x03d4,V_RETRACE_START, 0x0f); /**/      
	//word_out(0x03d4,V_RETRACE_END, 200); /**/        
	word_out(0x03d4,V_DISPLAY_END, 176<<1);  //VERTICAL RESOLUTION = 176
	/*word_out(0x03d4,UNDERLINE_LOCATION, 80); /**/   
	/*word_out(0x03d4,V_BLANK_START, 80);   /**/      
	/*word_out(0x03d4,V_BLANK_END, 80);    /**/      
    /*word_out(0x03d4,MODE_CONTROL, 80);  /**/     
	//word_out(0x03d4, 0x13, (320>>2)-1);    		/* OFFSET			*/
	
    // set vertical retrace back to normal
    word_out(0x03d4, V_RETRACE_END, 0x8e);
}

void reset_mode(byte mode){
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = mode;
	int86(0x10, &regs, &regs);
}

void fskip(FILE *fp, word num_bytes){
	int i;
	for (i=0; i<num_bytes; i++) fgetc(fp);
}

void load_plain_bmp(char *file,TILE *b){
	FILE *fp;
	long index;
	word num_colors;
	word x;

	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		exit(1);
	}
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&b->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&b->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	if ((b->tdata = (byte *) malloc(b->width*b->height)) == NULL){
		fclose(fp);
		printf("Error allocating memory for file %s.\n",file);
		exit(1);
	}

	for(index=0;index<num_colors;index++){
		b->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		b->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		b->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		x=fgetc(fp);
	}

	for(index=(b->height-1)*b->width;index>=0;index-=b->width)
		for(x=0;x<b->width;x++)
		b->tdata[(word)index+x]=(byte)fgetc(fp);

	fclose(fp);
}

/* load_16x16 tiles */
void load_tiles(char *file,TILE *b){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;
	unsigned char *tile_datatemp; //Temp storage of non tiled data.
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
		exit(1);
	} 
	fgetc(fp);
	fgetc(fp);

	fskip(fp,16);
	fread(&b->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&b->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	if (num_colors==0) num_colors=256;
	for(index=0;index<num_colors;index++){
		b->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		b->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		b->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		fgetc(fp);
	}

	if ((b->tdata = malloc(b->width*b->height)) == NULL){
		fclose(fp);
		printf("Error allocating memory tile data %s.\n",file);
		exit(1);
	}	
	if ((tile_datatemp = malloc(b->width*b->height)) == NULL){
		fclose(fp);
		printf("Error allocating memory for temp data %s.\n",file);
		exit(1);
	}

	for(index=(b->height-1)*b->width;index>=0;index-=b->width)
		for(x=0;x<b->width;x++)
			tile_datatemp[index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<b->height;tileY+=16){
		for (tileX=0;tileX<b->width;tileX+=16){
			offset = (tileY*b->width)+tileX;
			for(x=0;x<16;x++){
				memcpy(&b->tdata[index],&tile_datatemp[offset+(x*b->width)],16);
				index+=16;
			}
		}
	}
	free(tile_datatemp);
	tile_datatemp = NULL;
	
	b->ntiles = (b->width>>4) * (b->height>>4);
	b->width = 16;
	b->height = 200;
}

void unload_tiles(TILE *t){
	t->width = NULL;
	t->height = NULL;
	t->ntiles = NULL;
	*t->palette = NULL;
	free(t->tdata); t->tdata = NULL;
}

//Load tiled TMX map in XML format
void load_map(char *file, MAP *map){ 
	FILE *f = fopen(file, "rb");
	word start_data = 0;
	word end_data = 0;
	byte tile = 0;
	word index = 0;
	char line[64];
	char name[64]; //name of the layer in TILED
	if(!f){
		printf("can't find %s.\n",file);
		exit(1);
	}
	//read file
	fseek(f, 0, SEEK_SET);			
	while(start_data == 0){	//read lines 
		memset(line, 0, 64);
		fgets(line, 64, f);
		if((line[1] == '<')&&(line[2] == 'l')){
			sscanf(line," <layer name=\"%24[^\"]\" width=\"%i\" height=\"%i\"",&name,&map->width,&map->height);
			start_data = 1;
		}
	}
	if ((map->data = (byte *) malloc(map->width*map->height)) == NULL){
		fclose(f);
		printf("Error allocating memory for map\n");
		exit(1);
	}
	while(end_data == 0){
		memset(line, 0, 64);
		fgets(line, 64, f);
		if ((line[3] == '<')&&(line[4] == 't')){
			sscanf(line,"   <tile gid=\"%i\"/>",&tile);
			map->data[index] = tile -1;
			index++;
		}
		if((line[2] == '<')&&(line[3] == '/'))end_data = 1;
	}
	map->ntiles = map->width*map->height;
	fclose(f);
}

void unload_map(MAP *map){
	map->width = NULL;
	map->height = NULL;
	map->ntiles = NULL;
	free(map->data); map->data = NULL;
}

void set_map(MAP map, TILE *t, int x, int y){
	//UNDER CONSTRUCTION
	int i = 0;
	int j = 0;
	scrollR_posx = x+304;
	scrollD_posy = y+174;
	map_offset = (map.width*y)+x;
	SCR_X = x<<4;
	SCR_Y = y<<4;
	//draw map 
	for (i = 0;i<320;i+=16){draw_map_column(map,t,i,0,map_offset+j);j++;}	
}

void draw_map_column(MAP map, TILE *t, word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &t->tdata;
	unsigned char *mapdata = map.data;
	word tile_offset = 0;
	word width = map.width;
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

void draw_map_row(MAP map, TILE *t, word x, word y, word map_offset){
	unsigned char *tiledata = (unsigned char *) &t->tdata;
	unsigned char *mapdata = map.data;
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
void scroll_map(MAP map, TILE *t){
	current_x = SCR_X-(SCR_X & 15);
	current_y = SCR_Y-(SCR_Y & 15);
	if (current_y < last_y) {
		draw_map_row(map,t,current_x,current_y,map_offset-map.width);
		map_offset -= map.width;
	}
	if (current_y > last_y) { 
		draw_map_row(map,t,current_x,current_y+176,map_offset+(12*map.width));
		map_offset += map.width;
	}
	if (current_x < last_x) { 
		draw_map_column(map,t,current_x,current_y,map_offset-1); 
		map_offset--;
	}
	if (current_x > last_x) { 	
		draw_map_column(map,t,current_x+304,current_y,map_offset+20);
		map_offset++;
	}
	last_x = current_x;
	last_y = current_y;
}

//load RLE sprites with transparency (size = 8,16,32)
void load_sprite(char *file,SPRITE *s, byte size){
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
	
	unsigned char *sprite_datatemp; 
	unsigned char *sprite_tiledatatemp; 
	
	fp = fopen(file,"rb");
	if(!fp){
		printf("can't find %s.\n",file);
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
	
	if ((sprite_datatemp = (byte *) malloc(s->width*s->height)) == NULL){
		fclose(fp);
		printf("Error allocating memory for temp data %s.\n",file);
		exit(1);
	}
	if ((sprite_tiledatatemp = (byte *) malloc(s->width*s->height)) == NULL){
		fclose(fp);
		printf("Error allocating memory tile data %s.\n",file);
		exit(1);
	}

	for(index=(s->height-1)*s->width;index>=0;index-=s->width)
		for(x=0;x<s->width;x++)
		sprite_datatemp[(word)index+x]=(byte)fgetc(fp);
	fclose(fp);

	index = 0;
	
	//Rearrange tiles one after another in memory (in a column)
	for (tileY=0;tileY<s->height;tileY+=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			for(x=0;x<size;x++){
				memcpy(&sprite_tiledatatemp[index],&sprite_datatemp[offset+(x*s->width)],size);
				index+=size;
			}
		}
	}
	s->nframes = (s->width>>4) * (s->height>>4);
	s->width = size;
	s->height = size;
	free(sprite_datatemp);

	//calculate frames size
	s->rle_frames = (SPRITEFRAME*) malloc(s->nframes);
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
				current_byte = sprite_tiledatatemp[(size*y)+x+frame]; 
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
		s->rle_frames[j].rle_data = malloc(frame_size);
		
		//copy RLE data to struct
		memcpy(&s->rle_frames[j].rle_data[0],&number_of_runs,2);
		foffset+=2;
		for(i = 0; i < number_of_runs; i++) {
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_sskip[i],2); //bytes to skip on vga screen
			foffset+=2;
			memcpy(&s->rle_frames[j].rle_data[foffset],&run_length[i],2); //Number of bytes of pixel data that follow
			foffset+=2;
			offset+=run_bskip[i];
			memcpy(&s->rle_frames[j].rle_data[foffset],&sprite_tiledatatemp[offset+frame],run_length[i]); //copy pixel data;
			foffset+=run_length[i];
			offset+=run_length[i];
		}
	}
	free(sprite_tiledatatemp);

	s->bkg_data = (byte *) malloc(size*size); /*allocate memory for the 16x16 bkg chunk erased every time you draw the sprite*/
	s->init = 0;
	s->frame = 0;
}

void draw_sprite(SPRITE *s, word x, word y, byte frame){
	unsigned char *data = (unsigned char *) &s->rle_frames[frame].rle_data;
	unsigned char *bkg_data = (unsigned char *) &s->bkg_data;
	word screen_offset0 = (s->last_y<<8)+(s->last_y<<6)+s->last_x;
	word screen_offset1 = (y<<8)+(y<<6)+x;
	word init = s->init;
	word size = s->height;
	word size2 = s->height>>1;
	word next_scanline = 320 - s->width;
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
		shr     ax,1
		jc		rle_odd
		
		shr		cx,1	//counter / 2 because we copy words
		rep 	movsw	//copy pixel data from ds:si to es:di
		dec 	dx		//DX = Number of runs
		jnz 	rle_3
		jmp		rle_exit
	}
	rle_odd:
	asm{
		rep 	movsb	//copy pixel data from ds:si to es:di
		dec 	dx		//DX = Number of runs	
		jnz 	rle_3
	}
	rle_exit:
	asm{
		pop si
		pop di
		pop ds
	}
	s->last_x = x;
	s->last_y = y;
}

void unload_sprite(SPRITE *s){
	int i;
	s->width = NULL;
	s->height = NULL;
	*s->palette = NULL;
	s->init = NULL;
	s->pos_x = NULL;
	s->pos_y = NULL;
	s->last_x = NULL;
	s->last_y = NULL;
	s->frame = NULL;
	s->base_frame = NULL;
	free(s->bkg_data); s->bkg_data = NULL;
	for (i=0;i<s->nframes;i++){
		free(&s->rle_frames[i].rle_data);
		s->rle_frames[i].rle_data = NULL;
	}
	s->nframes = NULL;
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

/* draw_bitmap */
void draw_plain_bitmap(TILE *bmp, word x,word y){
	dword screen_offset = (y<<8)+(y<<6)+x;
	dword bitmap_offset = 0;
	int j;
	for(j=0;j<bmp->height;j++){
		memcpy(&MCGA[screen_offset],&bmp->tdata[bitmap_offset],bmp->width);
		bitmap_offset+=bmp->width;
		screen_offset+=SCREEN_WIDTH;
	}
}

//wait
void wait(word ticks){
  word start;
  start=*my_clock;
  while (*my_clock-start<ticks){
    *my_clock=*my_clock;
  }
}

void opl2_out(unsigned char reg, unsigned char data){
	outportb(opl2_base, reg);
	outportb(opl2_base + 1, data);
	shadow_opl[reg] = data;
}

void opl2_clear(void){
	int i;
	for (i = 0; i < 256; i++)opl2_out(i, 0);
}

void interrupt play_music(void){
	if (time_ctr > next_event){
		opl2_out(song.sdata[song.offset], song.sdata[song.offset+1]);
		next_event += (song.sdata[song.offset+2] | (song.sdata[song.offset+3]) << 8);
		song.offset+=4;
		if (song.offset > song.size)song.offset = 0;
	}
	time_ctr++;
	//outportb(0x20, 0x20);	//PIC, EOI
}

void set_timer(word freq_div){
	unsigned long spd = 1193182;
	old_time_handler = getvect(0x1C);
	spd /= freq_div;
	outportb(0x43, 0x36);
	outportb(0x40, spd % 0x100);	//lo-byte
	outportb(0x40, spd / 0x100);	//hi-byte*/
	setvect(0x1C, play_music);
}

void reset_timer(){
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte*/
	setvect(0x1C, old_time_handler);
}

void Load_Song(char *fname){
	FILE *imfile = fopen(fname, "rb");
	unsigned char rb[16];
	struct stat filestat;
	
	if (!imfile){
		set_mode(TEXT_MODE);
		printf("Can't find %s.\n",fname);
		exit(1);
	}
	if (fread(rb, sizeof(char), 2, imfile) != 2){
		set_mode(TEXT_MODE);
		printf("Error openning %s.\n",fname);
		exit(1);
	}
	
	//IMF
	if (rb[0] == 0 && rb[1] == 0){
		song.filetype = 0;
		song.offset = 0L;
	}
	else {
		song.filetype = 1;
		song.offset = 2L;
	}
	//get file size:
	fstat(fileno(imfile),&filestat);
    song.size = filestat.st_size;
    fseek(imfile, 0, SEEK_SET);
	
	free(song.sdata);
	if ((song.sdata = (unsigned char *) malloc(song.size)) == NULL){
		fclose(imfile);
		set_mode(TEXT_MODE);
		printf("Error allocating memory for music data %s.\n",fname);
		exit(1);
	}
	fread(song.sdata, 1, song.size,imfile);
	fclose(imfile);
}

void unload_song(IMFsong *song){
	song->size = NULL;
	song->offset = NULL;
	song->filetype = NULL;
	free(song->sdata); song->sdata = NULL;
}

///////////////////////////////////////
int SIN[720] = {
 9,  9,  9,  9, 10, 10, 10, 10, 10, 10,
11, 11, 11, 11, 11, 11, 12, 12, 12, 12,
12, 12, 13, 13, 13, 13, 13, 13, 14, 14,
14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
15, 16, 16, 16, 16, 16, 16, 16, 16, 17,
17, 17, 17, 17, 17, 17, 17, 17, 18, 18,
18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 20, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
18, 17, 17, 17, 17, 17, 17, 17, 17, 17,
16, 16, 16, 16, 16, 16, 16, 16, 15, 15,
15, 15, 15, 15, 15, 14, 14, 14, 14, 14,
14, 13, 13, 13, 13, 13, 13, 12, 12, 12,
12, 12, 12, 11, 11, 11, 11, 11, 11, 10,
10, 10, 10, 10, 10, 9, 9, 9, 9, 9,
8, 8, 8, 8, 8, 8, 7, 7, 7, 7,
7, 7, 6, 6, 6, 6, 6, 6, 5, 5,
5, 5, 5, 5, 5, 4, 4, 4, 4, 4,
4, 3, 3, 3, 3, 3, 3, 3, 3, 2,
2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
5, 6, 6, 6, 6, 6, 6, 7, 7, 7,
7, 7, 7, 8, 8, 8, 8, 8, 8, 9,
 9,  9,  9,  9, 10, 10, 10, 10, 10, 10,
11, 11, 11, 11, 11, 11, 12, 12, 12, 12,
12, 12, 13, 13, 13, 13, 13, 13, 14, 14,
14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
15, 16, 16, 16, 16, 16, 16, 16, 16, 17,
17, 17, 17, 17, 17, 17, 17, 17, 18, 18,
18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 20, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
18, 17, 17, 17, 17, 17, 17, 17, 17, 17,
16, 16, 16, 16, 16, 16, 16, 16, 15, 15,
15, 15, 15, 15, 15, 14, 14, 14, 14, 14,
14, 13, 13, 13, 13, 13, 13, 12, 12, 12,
12, 12, 12, 11, 11, 11, 11, 11, 11, 10,
10, 10, 10, 10, 10, 9, 9, 9, 9, 9,
8, 8, 8, 8, 8, 8, 7, 7, 7, 7,
7, 7, 6, 6, 6, 6, 6, 6, 5, 5,
5, 5, 5, 5, 5, 4, 4, 4, 4, 4,
4, 3, 3, 3, 3, 3, 3, 3, 3, 2,
2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
5, 6, 6, 6, 6, 6, 6, 7, 7, 7,
7, 7, 7, 8, 8, 8, 8, 8, 8, 9
};
