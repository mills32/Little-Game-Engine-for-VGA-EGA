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
int menu_option = 0;
int menu_pos[6] = {30,48,176,112,272,64};

void main(){
	SPRITE sprite_cursor, sprite_player, sprite_enemy1, sprite_enemy2, sprite_enemy3, sprite_ship;
	COLORCYCLE cycle_water;
	
	system("cls");
	//LT_Check_CPU(); //still causing trouble
	//LT_Adlib_Detect(); 
	
	LT_Init();

	printf("Loading...\n");
	Load_map("GFX/logo.tmx");
	load_tiles("GFX/logo.bmp");
	LT_Set_Map(0,0);
	
	//LOGO
	while (SCR_Y < 200){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_map();
		SCR_Y++;
		if (LT_Keys[1]) SCR_Y= 200; //esc exit
	}

	sleep(3);
	Scene = 1;
	Load_map("GFX/menu.tmx");
	load_tiles("GFX/menutil.bmp");
	load_sprite("GFX/cursor.bmp",&sprite_cursor,16);
	
	LT_Set_Map(0,0);
	MCGA_SplitScreen(63);
	LT_Load_Music("music/crooner.imf");
	LT_Start_Music(700);
	
	SCR_Y = 0;
	MCGA_Scroll(SCR_X,SCR_Y);

	//MENU
	i = 0;
	while(Scene == 1){
		
		MCGA_Scroll(SCR_X,144); //Scroll tittle
		SCR_X = LT_SIN[i]>>1;
		
		sprite_cursor.pos_x = menu_pos[menu_option] + (LT_COS[i]>>2);
		sprite_cursor.pos_y = menu_pos[menu_option+1] + (LT_SIN[i]>>2);
		draw_sprite(&sprite_cursor,sprite_cursor.pos_x,sprite_cursor.pos_y,0);
		
		if (i > 360) i = 0;
		i+=2;
		if (LT_Keys[1]) Scene = 2; //esc exit
	}
	
	SCR_X = 0;
	MCGA_Scroll(SCR_X,SCR_Y);
	sleep(2);
	
	MCGA_ClearScreen();
	MCGA_SplitScreen(0x0ffff);
	printf("Loading...\n");
	Scene = 2;
	load_map("GFX/luna_map.tmx");
	load_tiles("GFX/lunatil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	load_sprite("GFX/player.bmp",&sprite_player,16);
	load_sprite("GFX/player.bmp",&sprite_enemy1,16);
	load_sprite("GFX/player.bmp",&sprite_enemy2,16);

	LT_Load_Music("music/dintro.imf");
	LT_Start_Music(700);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	sprite_player.pos_x = 80;
	sprite_player.pos_y = 70;
	
	LT_Set_Map(0,0);
	LT_Gravity = 1;
	
	/*MAIN LOOP SCROLL X*/
	while(Scene == 2){
	
		//SCR_X and SCR_Y are global variables predefined 
		MCGA_Scroll(SCR_X,SCR_Y);

		//scroll_map
		LT_scroll_map();
		LT_scroll_follow(&sprite_player);

		draw_sprite(&sprite_player,sprite_player.pos_x,sprite_player.pos_y,frame);
		//LT_gprint(LT_map.collision[((sprite_player.pos_y>>4) * LT_map.width) + (sprite_player.pos_x>>4)],240,160);
		
		LT_move_player(&sprite_player);
		
		cycle_palette(&cycle_water,2);
		
		if(Speed == 8){Speed = 0; frame++;}
		Speed++;

		if (LT_Keys[1]) Scene = -1; //esc exit

		if(frame == 15) frame = 0;
	}
	
	t1 = LT_map.ntiles;
	//free ram, if not, program it will crash a lot
	LT_unload_sprite(&sprite_player); //manually free sprites
	LT_unload_sprite(&sprite_enemy1);
	LT_unload_sprite(&sprite_enemy2);
	LT_unload_sprite(&sprite_cursor);
	LT_ExitDOS(); //frees map, tileset, font and music.
	
	//debug
	printf("debug = %f\n",t1);
	
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