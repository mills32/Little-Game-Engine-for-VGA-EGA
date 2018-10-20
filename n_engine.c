/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program.                     
	Remember to compile in the LARGE memory model!                        

	Please feel free to copy this source code.                            

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
unsigned char *LT_tile_datatemp; //Temp storage of non tiled data.
SPRITE lt_gpnumber0, lt_gpnumber1, lt_gpnumber2, lt_gpnumber3; // 4 sprites for debug printing

//GLOBAL VARIABLES

//debugging
float t1 = 0;
float t2 = 0;

//This points to video memory.
byte *MCGA=(byte *)0xA0000000L; 

//timer     		
void interrupt (*old_time_handler)(void); 	
long time_ctr;

//keyboard
void interrupt (*LT_old_key_handler)(void);
int LT_Keys[256];

//ADLIB
const int opl2_base = 0x388;	
unsigned char shadow_opl[256];
long next_event;

// this points to the 18.2hz system clock. 
word *my_clock=(word *)0x0000046C;    		
word start;
int Free_RAM;

//Map Scrolling
byte scr_delay = 0; //copy tile data in several frames to avoid slowdown
int SCR_X = 0;
int SCR_Y = 0;
int current_x = 0;
int last_x = 0;
int current_y = 0;
int last_y = 0;
int map_offset = 0;
int LT_follow = 0;

//Player
byte tile_number = 0;		//Tile a sprite is on
byte tilecol_number = 0;	//Collision Tile a sprite is on
byte tile_number_UDR = 0;	//Tile up or down R
byte tile_number_UDL = 0;	//Tile up or down L
byte tile_number_LRU = 0;	//Tile left or right U
byte tile_number_LRD = 0;	//Tile left or right D
byte LT_Gravity = 0;
byte LT_player_jump = 0;
byte LT_player_jump_frame = 0;
int LT_player_jump_pos[] = 
{
	-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,-1,-1,-1,-1,
	 0,-1, 0,-1, 0,-1,
	 0, 0, 0, 0,
	 0, 1, 0, 1, 0, 1,
	 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

//end of global variables

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

void MCGA_WaitVBL(){
	int i;
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

void MCGA_ClearScreen(){
	outport(0x03c4,0xff02);
	memset(&MCGA[0],0,(320*200)/4);
}

void LT_Init(){
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = VGA_256_COLOR_MODE;
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
	
	//Allocate 256kb RAM for:
	// 64kb Tileset
	// 64kb Temp Tileset
	// 32kb Map + 32kb collision map
	// 64kb Music
	if ((LT_tileset.tdata = malloc(65535*sizeof(byte))) == NULL){
		LT_ExitDOS();
		printf("Error allocating 64kb for tile data\n");
	}	

	if ((LT_tile_datatemp = malloc(65535*sizeof(byte))) == NULL){
		LT_ExitDOS();
		printf("Error allocating 64kb for temp data\n");
	}
	
	if ((LT_map.data = malloc(32767*sizeof(byte))) == NULL){
		LT_ExitDOS();
		printf("Error allocating 32kb for map\n");
	}
	if ((LT_map.collision = malloc(32767*sizeof(byte))) == NULL){
		LT_ExitDOS();
		printf("Error allocating 32kb for collision map\n");
	}
	if ((LT_music.sdata = malloc(65535*sizeof(byte))) == NULL){
		LT_ExitDOS();
		printf("Error allocating 64kb for music data\n");
	}
	LT_install_key_handler();
}

void LT_ExitDOS(){
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = TEXT_MODE;
	int86(0x10, &regs, &regs);
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_unload_font();
	LT_reset_key_handler();
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
		LT_ExitDOS();
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
		LT_ExitDOS();
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
void load_tiles(char *file){
	FILE *fp;
	long index,offset;
	word num_colors;
	word x;
	word tileX;
	word tileY;

	fp = fopen(file,"rb");
	if(!fp){
		LT_ExitDOS();
		printf("can't find %s.\n",file);
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
	LT_tileset.width = 16;
	LT_tileset.height = 200;
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
		LT_ExitDOS();
		printf("can't find %s.\n",file);
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
		LT_ExitDOS();
		printf("Error, map is bigger than 32 kb");
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
	current_x = x;
	current_y = y;
	last_x = x;
	last_y = y;
	map_offset = (LT_map.width*y)+x;
	SCR_X = x<<4;
	SCR_Y = y<<4;
	//draw map 
	for (i = 0;i<320;i+=16){draw_map_column(i,0,map_offset+j);j++;}	
	set_palette(LT_tileset.palette);
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
	current_x = SCR_X-(SCR_X & 15);
	current_y = SCR_Y-(SCR_Y & 15);
	if (current_y < last_y) {
		draw_map_row(current_x,current_y,map_offset-LT_map.width);
		map_offset -= LT_map.width;
	}
	if (current_y > last_y) { 
		draw_map_row(current_x,current_y+176,map_offset+(12*LT_map.width));
		map_offset += LT_map.width;
	}
	if (current_x < last_x) { 
		if (current_x < 0) map_offset = map_offset - LT_map.width;
		draw_map_column(current_x,current_y,map_offset-1); 
		map_offset--;
	}
	if (current_x > last_x) { 
		draw_map_column(current_x+304,current_y,map_offset+20);
		map_offset++;
	}
	last_x = current_x;
	last_y = current_y;
}

void LT_scroll_follow(SPRITE *s){
	//LIMITS
	int y_limU = SCR_Y + 64;
	int y_limD = SCR_Y + 120;
	int x_limL = SCR_X + 80;
	int x_limR = SCR_X + 200;
	int wmap = LT_map.width<<4;
	int hmap = LT_map.height<<4;
	//clamp limits
	if ((SCR_X > -1) && ((SCR_X+303)<wmap) && (SCR_Y > -1) && ((SCR_Y+175)<hmap)){
		if (s->pos_y > y_limD) SCR_Y++;
		if (s->pos_y < y_limU) SCR_Y--;
		if (s->pos_x < x_limL) SCR_X--;
		if (s->pos_x > x_limR) SCR_X++;
	}
	if (SCR_X < 0) SCR_X = 0; 
	if ((SCR_X+304) > wmap) SCR_X = wmap-304;
	if (SCR_Y < 0) SCR_Y = 0; 
	if ((SCR_Y+176) > hmap) SCR_Y = hmap-176; 
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
		LT_ExitDOS();
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
		LT_ExitDOS();
		printf("Error allocating memory for sprite temp data %s.\n",file);
		exit(1);
	}
	if ((sprite_tiledatatemp = (byte *) malloc(s->width*s->height)) == NULL){
		fclose(fp);
		LT_ExitDOS();
		printf("Error allocating memory for sprite data %s.\n",file);
		exit(1);
	}

	for(index=(s->height-1)*s->width;index>=0;index-=s->width)
		for(x=0;x<s->width;x++)
		sprite_datatemp[index+x]=(byte)fgetc(fp);
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
	s->rle_frames = malloc(s->nframes*sizeof(SPRITEFRAME));
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
		s->rle_frames[j].rle_data = (byte*) malloc(frame_size);

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

void LT_move_player(SPRITE *s){
	int col = 0;
	//GET TILE POS
	tile_number = LT_map.data[((s->pos_y>>4) * LT_map.width) + (s->pos_x>>4)];
	tilecol_number = LT_map.collision[((s->pos_y>>4) * LT_map.width) + (s->pos_x>>4)];
	//0x48 UP; 0x50 DOWN; 0x4B LEFT; 0x4D RIGHT;
	//0x20 D (JUMP); 0x1f S (ACTION); 0x01 ESC
		
  if (LT_Gravity == 1){
	if ((LT_player_jump == 0) && (LT_Keys[0x20])) {LT_player_jump_frame = 0; LT_player_jump = 1;}
	if (LT_player_jump == 1){//JUMP
		col = 0;
		tile_number_UDR = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + ((s->pos_x+15)>>4)];
		tile_number_UDL = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_UDR == 1) {LT_player_jump_frame = 0; LT_player_jump = 0;}
		if (tile_number_UDL == 1) {LT_player_jump_frame = 0; LT_player_jump = 0;}
		if (col == 0){
			s->pos_y += LT_player_jump_pos[LT_player_jump_frame];
			LT_player_jump_frame++;
			if (LT_player_jump_frame == 52) {LT_player_jump_frame = 0; LT_player_jump = 0;}
		}
	}
	if (LT_player_jump == 0){//DOWN
		col = 0;
		tile_number_UDR = LT_map.collision[(((s->pos_y+16)>>4) * LT_map.width) + ((s->pos_x+15)>>4)];
		tile_number_UDL = LT_map.collision[(((s->pos_y+16)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_UDR == 1) col = 1;
		if (tile_number_UDL == 1) col = 1;
		if (tile_number_UDR == 2) col = 1;
		if (tile_number_UDL == 2) col = 1;
		if (col == 0) s->pos_y++;
	}
  }
  if (LT_Gravity == 0){
	if (LT_Keys[0x48]){	//UP
		col = 0;
		tile_number_UDR = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + ((s->pos_x+15)>>4)];
		tile_number_UDL = LT_map.collision[(((s->pos_y-1)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_UDR == 1) col = 1;
		if (tile_number_UDL == 1) col = 1;
		if (col == 0) s->pos_y--;
	}
	if (LT_Keys[0x50]){	//DOWN
		col = 0;
		tile_number_UDR = LT_map.collision[(((s->pos_y+16)>>4) * LT_map.width) + ((s->pos_x+15)>>4)];
		tile_number_UDL = LT_map.collision[(((s->pos_y+16)>>4) * LT_map.width) + (s->pos_x>>4)];
		if (tile_number_UDR == 1) col = 1;
		if (tile_number_UDL == 1) col = 1;
		if (tile_number_UDR == 2) col = 1;
		if (tile_number_UDL == 2) col = 1;
		if (col == 0) s->pos_y++;
	}
  }
	if (LT_Keys[0x4B]){	//LEFT
		col = 0;
		tile_number_LRU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x-1)>>4)];
		tile_number_LRD = LT_map.collision[(((s->pos_y+15)>>4) * LT_map.width) + ((s->pos_x-1)>>4)];
		if (tile_number_LRU == 1) col = 1;
		if (tile_number_LRD == 1) col = 1;
		if (col == 0) s->pos_x--;
	}
	if (LT_Keys[0x4D]){	//RIGHT
		col = 0;
		tile_number_LRU = LT_map.collision[((s->pos_y>>4) * LT_map.width) + ((s->pos_x+16)>>4)];
		tile_number_LRD = LT_map.collision[(((s->pos_y+15)>>4) * LT_map.width) + ((s->pos_x+16)>>4)];
		if (tile_number_LRU == 1) col = 1;
		if (tile_number_LRD == 1) col = 1;
		if (col == 0) s->pos_x++;
	}
}
	
void LT_load_font(char *file){
	load_sprite(file,&lt_gpnumber0,16);
	load_sprite(file,&lt_gpnumber1,16);
	load_sprite(file,&lt_gpnumber2,16);
}

void LT_gprint(int var, word x, word y){
	draw_sprite(&lt_gpnumber0,SCR_X+x+34,SCR_Y+y,var % 10);
	var /= 10;
	draw_sprite(&lt_gpnumber1,SCR_X+x+17,SCR_Y+y,var % 10);
	var /= 10;
	draw_sprite(&lt_gpnumber2,SCR_X+x,SCR_Y+y,var % 10);
}
	
void LT_unload_sprite(SPRITE *s){
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
		opl2_out(LT_music.sdata[LT_music.offset], LT_music.sdata[LT_music.offset+1]);
		next_event += (LT_music.sdata[LT_music.offset+2] | (LT_music.sdata[LT_music.offset+3]) << 8);
		LT_music.offset+=4;
		if (LT_music.offset > LT_music.size)LT_music.offset = 0;
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
		LT_ExitDOS();
		printf("Can't find %s.\n",fname);
		exit(1);
	}
	if (fread(rb, sizeof(char), 2, imfile) != 2){
		LT_ExitDOS();
		printf("Error openning %s.\n",fname);
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
    fseek(imfile, 0, SEEK_SET);

	fread(LT_music.sdata, 1, LT_music.size,imfile);
	fclose(imfile);
}

void LT_Unload_Music(){
	LT_music.size = NULL;
	LT_music.offset = NULL;
	LT_music.filetype = NULL;
	free(LT_music.sdata); LT_music.sdata = NULL;
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
