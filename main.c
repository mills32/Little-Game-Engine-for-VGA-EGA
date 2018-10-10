/***********************
*  LITTLE GAME ENGINE  *
************************/
#include "n_engine.h"

unsigned char palette_cycle_water[] = {//generated with gimp, then divided by 4 (max = 64).
	0x0f,0x0f,0x0f,	//colour 0
	//2 copies of 8 colour cycle
	0x35,0x3b,0x3f,//
	0x26,0x39,0x3f,
	0x18,0x36,0x3f,
	0x09,0x32,0x3e,
	0x0f,0x34,0x3f,
	0x17,0x36,0x3f,
	0x1c,0x3a,0x3e,
	0x24,0x3e,0x3d,
	0x35,0x3b,0x3f,//
	0x26,0x39,0x3f,
	0x18,0x36,0x3f,
	0x09,0x32,0x3e,
	0x0f,0x34,0x3f,
	0x17,0x36,0x3f,
	0x1c,0x3a,0x3e,
	0x24,0x3e,0x3d,
	0x35,0x3b,0x3f
};


int i = 0;
int j = 0;

int frame = 0;
int frame1 = 0;
int Speed = 0;
int Scene = 0;

void main(){
	MAP map_logo,map_level,map_space;
	TILE tileset_logo,tileset_level,tileset_space;			
	SPRITE sprite_player,sprite_ship0,sprite_ship1,sprite_enemy1,sprite_enemy2,sprite_enemy3;
	COLORCYCLE cycle_water;
	
	//system("cls");
	//check_hardware(); //causes garbage
	//ADLIB_Detect(); 
	
	printf("Loading...\n");
	
	/*load_map("GFX/logo.tmx",&map_logo);
	load_tiles("GFX/logo.bmp",&tileset_logo);
	
	set_mode(0x13);
	set_palette(tileset_logo.palette);
	
	//draw map 
	for (i = 0;i<304;i+=16){
		draw_map_column(map_logo,&tileset_logo,i,0,j);
		j++;
	}
	
	j = 0;
	while(Scene == 0){
	
		if (read_keys() == 6) Scene = 1;
		if (j == 100) Scene = 1;
		while(inportb(0x3DA)&8);
		while(!(inportb(0x3DA)&8)); 
		j++;
	}

	free(tileset_logo.tdata);
	tileset_logo.tdata = NULL;
	free(map_logo.data);
	map_logo.data = NULL;
	*/
	Scene = 1;

	i = 0;
	j = 0;
	
	//load_tiles("GFX/stileset.bmp",&tileset_space);
	load_map("GFX/map.tmx",&map_level);
	load_tiles("GFX/tileset.bmp",&tileset_level);
	//load_sprite("GFX/player.bmp",&sprite_player,16);
	//load_sprite("GFX/ship1.bmp",&sprite_ship1,32);
	
	set_mode(0x13);
	set_palette(tileset_level.palette);

	set_map(map_level,&tileset_level,0,0);
	
	/*
	start=*my_clock;
	for (i = 0;i<100;i++) draw_RLE_sprite(&sprite_ship0,0,0,0);
	t1=(*my_clock-start)/18.2;
	start=*my_clock;
	for (i = 0;i<100;i++) draw_RLE_sprite(&sprite_ship1,0,0,0);
	t2=(*my_clock-start)/18.2;	
	*/

	//open IMF
	//"music/64style.imf" "music/fox.imf" "music/game.imf" "music/luna.imf" "music/menu.imf" "music/riddle.imf" "music/space.imf"
	//set_timer(500); 
	//Load_Song("music/menu.imf");
	//opl2_clear();
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);

	i = 0;
	j = 0;
	
	//sprite_player.pos_x = 130;
	//sprite_player.pos_y = 70;
	//draw_sprite(&sprite_player,0,0,0);
	/*MAIN LOOP SCROLL X*/
	while(Scene == 1){
	
		//SCR_X and SCR_Y are global variables predefined
		MCGA_Scroll(SCR_X,SCR_Y);  
		MCGA_WaitVBL(); //Draw everything just after VBL!!
		/*if (read_keys() == 0){sprite_player.pos_y--; Speed++;}
		if (read_keys() == 1){sprite_player.pos_y++; Speed++;}
		if (read_keys() == 2){sprite_player.pos_x--; Speed++; update_tilesL(map_level,&tileset_level);}
		if (read_keys() == 3){sprite_player.pos_x++; Speed++; update_tilesD(map_level,&tileset_level);} 
		*/

		//scroll_map function updates the SCR_X and SCR_Y variables
		if (read_keys() == 0) scroll_map(map_level,&tileset_level,0);
		if (read_keys() == 1) scroll_map(map_level,&tileset_level,1);
		if (read_keys() == 2) scroll_map(map_level,&tileset_level,2);
		if (read_keys() == 3) scroll_map(map_level,&tileset_level,3);
		//draw_sprite(&sprite_ship1,SCR_X+130,SCR_Y+80,0);
		
		cycle_palette(&cycle_water,2);
		

		//MCGA_Scroll(sprite_player.pos_x-130,sprite_player.pos_y-70);
		
		if(Speed == 4){Speed = 0; frame++;}
		
		//move sprite

		if (read_keys() == 6) Scene = -1; //esc exit
		//if (j == 600) Scene = -1; //esc exit
		i+=3;
		if(frame == 6) frame = 0;
		if (i > 360) i = 0;
		
	}
	reset_mode(TEXT_MODE); 
	
	//opl2_clear();
	//reset_timer();
	
	//free ram, if not, program it will crash a lot
	//unload_song(&song);
	//unload_tiles(&tileset_space);
	unload_tiles(&tileset_level);
	unload_map(&map_level);
	//unload_sprite(&sprite_player);
	
	//debug
	printf("Copy rle odd = %f\n",t1);
	printf("Copy rle even = %f\n",t2);
	
	return;
}
