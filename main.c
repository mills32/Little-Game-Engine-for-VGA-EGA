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

SPRITE sprite_cursor, sprite_player, sprite_enemy1, sprite_enemy2, sprite_enemy3, sprite_enemy4, sprite_ship;
COLORCYCLE cycle_water;

int i = 0;
int j = 0;
byte *col;

int Scene = 0;
int menu_option = 0;
int menu_pos[6] = {88,58,100,100,200,60};
int game = 0;
int random;


void Set_Logo(){
	Load_map("GFX/Logo.tmx");
	load_tiles("GFX/Logotil.bmp");
	LT_Set_Map(0,0);
}

void Run_Logo(){
	while (SCR_Y < 152){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_map();
		SCR_Y++;
		if (LT_Keys[1]) SCR_Y= 200; //esc exit
	}
	sleep(1);
}

void Set_Menu(){
	Load_map("GFX/menu.tmx");
	load_tiles("GFX/menutil.bmp");
	LT_Load_Sprite("GFX/cursor.bmp",&sprite_cursor,16);
	LT_Load_Music("music/menu4.imf");
	LT_Start_Music(700);
	LT_Set_Map(0,0);
	MCGA_SplitScreen(63);
	i = 0;
	LT_SideScroll = 0;
	sleep(1);
}

void Run_Menu(){
	int delay = 0; //wait between key press
	Scene = 1;
	game = 0;
	menu_option = 0;

	while(Scene == 1){
		MCGA_Scroll(SCR_X,144); //Scroll tittle
		SCR_X = LT_SIN[i]>>1;
		
		sprite_cursor.pos_x = menu_pos[menu_option] + (LT_COS[i]>>2);
		sprite_cursor.pos_y = menu_pos[menu_option+1] + (LT_SIN[i]>>3);
		LT_Draw_Sprite(&sprite_cursor);
		
		if (i > 360) i = 0;
		i+=2;
		
		if (delay == 10){
			if (LT_Keys[LT_LEFT]) {menu_option -=2; game--;}
			if (LT_Keys[LT_RIGHT]) {menu_option +=2; game++;}
			delay = 0;
		}
		delay++;
		
		if (menu_option < 0) menu_option = 0;
		if (menu_option > 4) menu_option = 4;		
		if (game < 0) game = 0;
		if (game > 2) game = 2;	
		
		if (LT_Keys[LT_ENTER]) Scene = 2;
		if (LT_Keys[LT_ESC]) {
			LT_unload_sprite(&sprite_cursor);
			LT_ExitDOS();
		}
	}
	LT_unload_sprite(&sprite_cursor);
	sleep(1);
	MCGA_SplitScreen(0x0ffff); //disable split screen
}

void Set_TopDown(){
	Scene = 2;
	load_map("GFX/Topdown.tmx");
	load_tiles("GFX/Toptil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);

	LT_Load_Music("music/top_down.imf");
	LT_Start_Music(700);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	sprite_player.pos_x = 8*16;
	sprite_player.pos_y = 4*16;
	
	LT_Set_Map(0,0);
	LT_Gravity = 0;
}

void Run_TopDown(){
	Scene = 2;
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_scroll_follow(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//LT_gprint(LT_map.collision[((sprite_player.pos_y>>4) * LT_map.width) + (sprite_player.pos_x>>4)],240,160);
		
		col = LT_move_player(&sprite_player);
		
		//If collision tile = ?, end level
		if (col[1] == 6) Scene = 1;
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,6,4);
		else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(&sprite_player,12,6,4);
		else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(&sprite_player,18,6,4);
		else sprite_player.animate = 0;
		
		cycle_palette(&cycle_water,2);

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
	}
	
	LT_unload_sprite(&sprite_player); //manually free sprites
}

void Set_Platform(){
	Scene = 2;
	load_map("GFX/Platform.tmx");
	load_tiles("GFX/Platil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy1,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy2,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy3,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy4,16);

	LT_Load_Music("music/platform.imf");
	LT_Start_Music(700);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
}

void Run_Platform(){
	startp:
	LT_Set_Map(0,0);
	LT_Gravity = 1;
	Scene = 2;
	sprite_player.pos_x = 80;
	sprite_player.pos_y = 70;
	while(Scene == 2){
		//SCR_X and SCR_Y are global variables predefined 
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_follow(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		
		//scroll_map
		LT_scroll_map();
		
		col = LT_move_player(&sprite_player);
		//if water
		if (col[0] == 182) {sprite_player.init = 0; goto startp;}
		//If collision tile = ?, end level
		if (col[1] == 6) Scene = 1;
		
		//set animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,6,4);
		else sprite_player.animate = 0;
		
		//water palette animation
		cycle_palette(&cycle_water,2);

		//esc go to menu
		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
	}
	
	LT_unload_sprite(&sprite_player); //manually free sprites
	LT_unload_sprite(&sprite_enemy1);
	LT_unload_sprite(&sprite_enemy2);
	LT_unload_sprite(&sprite_enemy3);
	LT_unload_sprite(&sprite_enemy4);
}

void Set_Shooter(){
	Scene = 2;
	load_map("GFX/Shooter.tmx");
	load_tiles("GFX/stiles.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/Ship.bmp",&sprite_ship,32);
	
	LT_Load_Music("music/shooter.imf");
}

void Run_Shooter(){
	int timer;
	start:
	timer = 0;
	Scene = 0;
	sprite_ship.pos_x = 8*16;
	sprite_ship.pos_y = 13*16;
	LT_Set_Sprite_Animation(&sprite_ship,0,1,0);
	
	sprite_player.pos_x = -16;
	sprite_player.pos_y = 14*16;
	LT_Set_Sprite_Animation(&sprite_player,0,6,4);
	
	LT_Set_Map(0,5);
	
	while (Scene == 0){
		
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Draw_Sprite(&sprite_ship);
		LT_Draw_Sprite(&sprite_player);
		sprite_player.pos_x++;
		if (sprite_player.pos_x == sprite_ship.pos_x+2) Scene = 1; //esc exit
	}
	
	LT_Set_Sprite_Animation(&sprite_ship,1,4,12);
	while (Scene == 1){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Draw_Sprite(&sprite_ship);
		if (sprite_ship.frame == 4) Scene = 2; //esc exit
	}
	
	sleep(0.5);	
	sprite_ship.animate = 0;
	while (Scene == 2){
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_scroll_follow(&sprite_ship);
		LT_Draw_Sprite(&sprite_ship);
		
		sprite_ship.pos_y--;
		if (SCR_Y == 0) Scene = 3; 
		
		LT_scroll_map();
	}
	LT_Set_Sprite_Animation(&sprite_ship,4,4,5);
	while (Scene == 3){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Draw_Sprite(&sprite_ship);

		timer++;
		if (timer == 100) Scene = 4; 
	}
	
	LT_Start_Music(700);
	LT_SideScroll = 1;
	LT_Gravity = 0;
	LT_Set_Sprite_Animation(&sprite_ship,8,4,4);
	while(Scene == 4){
		
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_Draw_Sprite(&sprite_ship);
		LT_Endless_SideScroll_Map(0);
		col = LT_move_player(&sprite_ship);
		if (col[2] == 1) {
			LT_Set_Sprite_Animation(&sprite_ship,12,4,4);
		}
		if (sprite_ship.frame == 15) {
			sprite_ship.init = 0;
			sleep(2); 
			Scene = -1;
		}
		if (LT_Keys[LT_ESC]) Scene = -1; //esc exit
		SCR_X++;
	}
	
	LT_unload_sprite(&sprite_ship);
	LT_unload_sprite(&sprite_player);
}

void main(){
	
	system("cls");
	//LT_VGA_Font("vgafont.bin");
	//LT_Check_CPU(); //still causing trouble
	//LT_Adlib_Detect(); 
	
	LT_Init();
	gotoxy(17,13);
	printf("LOADING");
	gotoxy(0,0);
	
	//You can use a custom load animation
	LT_Load_Animation("GFX/loading.bmp",&LT_Loading_Animation,32);
	LT_Set_Animation(&LT_Loading_Animation,0,16,2);

	//LOGO
	Set_Logo();
	Run_Logo();

	//MENU
	menu:
	
	LT_Set_Loading_Interrupt(); 
	Set_Menu();
	Run_Menu();

	if (game == 0){
		LT_Stop_Music();
		LT_Set_Loading_Interrupt(); 
		Set_TopDown();
		Run_TopDown();
		LT_Stop_Music();
		goto menu;
	}
	if (game == 1){
		LT_Stop_Music();
		LT_Set_Loading_Interrupt(); 
		Set_Platform();
		Run_Platform();
		LT_Stop_Music();
		goto menu;
	}
	if (game == 2){
		LT_Stop_Music();
		LT_Set_Loading_Interrupt(); 
		Set_Shooter();
		Run_Shooter();
		LT_Stop_Music();
		goto menu;
	}

	exit:

	LT_ExitDOS(); //frees map, tileset, font and music.
	
	return;
}

	//debug
	//printf("debug 1 = %f\n",t1);
	//printf("debug 2 = %f\n",t2);
	//sleep(3);
/*
	start=*my_clock;
	for (i = 0;i<100;i++) LT_Draw_Sprite(&sprite_ship);
	t1=(*my_clock-start)/18.2;
	sleep(1);
	start=*my_clock;
	for (i = 0;i<100;i++) LT_Draw_Sprite2(&sprite_ship);
	t2=(*my_clock-start)/18.2;	
*/
	