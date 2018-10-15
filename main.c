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
int Speed = 0;
int Scene = 0;

void main(){		
	SPRITE sprite_player,sprite_ship0,sprite_ship1,sprite_enemy1,sprite_enemy2,sprite_enemy3;
	COLORCYCLE cycle_water;
	
	system("cls");
	//check_hardware(); //causes garbage
	//ADLIB_Detect(); 
	
	LT_Init();
	
	printf("Loading...\n");
	load_map("GFX/logo.tmx");
	load_tiles("GFX/logo.bmp");
	LT_Set_Map(0,0);
	
	wait(20);
	
	Scene = 1;

	i = 0;
	j = 0;
	
	load_map("GFX/mario.tmx");
	load_tiles("GFX/mariotil.bmp");
	load_sprite("GFX/s_mario.bmp",&sprite_player,16);
	load_sprite("GFX/ship1.bmp",&sprite_ship1,32);
	LT_load_font("GFX/font.bmp");

	//open IMF
	//"music/64style.imf" "music/fox.imf" "music/game.imf" "music/luna.imf" "music/menu.imf" "music/riddle.imf" "music/space.imf"
	//set_timer(500); 
	//Load_Song("music/menu.imf");
	//opl2_clear();
	
	//animate colours
	//cycle_init(&cycle_water,palette_cycle_water);

	i = 0;
	j = 0;
	
	sprite_player.pos_x = 130;
	sprite_player.pos_y = 70;

	LT_Set_Map(0,0);
	
	/*MAIN LOOP SCROLL X*/
	while(Scene == 1){
	
		//SCR_X and SCR_Y are global variables predefined
		MCGA_Scroll(SCR_X,SCR_Y);  
		MCGA_WaitVBL(); //Draw everything just after VBL!!

		//scroll_map 
		LT_scroll_follow(&sprite_player);
		LT_scroll_map();
		
		draw_sprite(&sprite_player,sprite_player.pos_x,sprite_player.pos_y,frame);
		LT_gprint(SCR_X,240,160);
		
		LT_move_player(&sprite_player);
		
		//cycle_palette(&cycle_water,2);
		
		if(Speed == 8){Speed = 0; frame++;}
		Speed++;
		//move sprite

		if (read_keys() == 6) Scene = -1; //esc exit
		//if (j == 1200) Scene = -1; //esc exit
		if(frame == 15) frame = 0;
	}
	
	//opl2_clear();
	//reset_timer();
	
	//free ram, if not, program it will crash a lot
	LT_Unload_Music();
	LT_unload_tileset();
	LT_unload_map();
	LT_unload_sprite(&sprite_player);
	LT_unload_font();
	
	LT_ExitDOS();
	
	//debug
	printf("Copy rle odd = %f\n",t1);
	printf("Copy rle even = %f\n",t2);
	
	return;
}


	/*
	start=*my_clock;
	for (i = 0;i<100;i++) draw_RLE_sprite(&sprite_ship0,0,0,0);
	t1=(*my_clock-start)/18.2;
	start=*my_clock;
	for (i = 0;i<100;i++) draw_RLE_sprite(&sprite_ship1,0,0,0);
	t2=(*my_clock-start)/18.2;	
	*/