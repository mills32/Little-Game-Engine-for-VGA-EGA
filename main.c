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

SPRITE font, pointer, sprite_cursor, sprite_player, sprite_enemy1, sprite_enemy2, sprite_enemy3, sprite_enemy4, sprite_ship;
COLORCYCLE cycle_water;

int i = 0;
int j = 0;

LT_Col LT_Player_Col;

int Scene = 0;
int menu_option = 0;
int menu_pos[8] = {2.5*16,112,6.5*16,112,11.5*16,112,15.5*16,112};
int game = 0;
int random;


void Load_Logo(){
	Load_map("GFX/Logo.tmx");
	load_tiles("GFX/Logotil.bmp");
	MCGA_Fade_out();
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

void Load_Menu(){
	LT_Set_Loading_Interrupt(); 
	
	Load_map("GFX/menu.tmx");
	load_tiles("GFX/menutil.bmp");
	LT_Load_Sprite("GFX/cursor.bmp",&sprite_cursor,16);
	//LT_Load_Music("music/menu2.imf");
	//LT_Start_Music(70);
	LoadMOD("MUSIC/MOD/dyna.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	PlayMOD(0);
	
	MCGA_SplitScreen(63); 
	LT_Set_Map(0,0);
	
	i = 0;
	LT_MODE = 0;
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
		sprite_cursor.pos_y = menu_pos[menu_option+1] + (LT_SIN[i]>>4);
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
		if (menu_option > 6) menu_option = 6;		
		if (game < 0) game = 0;
		if (game > 3) game = 3;	
		
		if (LT_Keys[LT_ENTER]) Scene = 2;
		if (LT_Keys[LT_ESC]) {
			LT_unload_sprite(&sprite_cursor);
			LT_ExitDOS();
		}
	}
	StopMOD();
	LT_unload_sprite(&sprite_cursor);
}

void Load_TopDown(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/Topdown.tmx");
	load_tiles("GFX/Toptil.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);

	//LT_Load_Music("music/top_down.imf");
	//LT_Start_Music(70);
	LoadMOD("MUSIC/MOD/Beach.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	PlayMOD(1);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	sprite_player.pos_x = 8*16;
	sprite_player.pos_y = 4*16;
	
	LT_Set_Map(0,0);
	LT_MODE = 0;
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
		
		LT_gprint(LT_Player_Col.tile_number,240,160);
		
		//In this mode sprite is controlled using U D L R
		LT_Player_Col = LT_move_player(&sprite_player);
		
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 10) Scene = 1;
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,6,4);
		else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(&sprite_player,12,6,4);
		else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(&sprite_player,18,6,4);
		else sprite_player.animate = 0;
		
		cycle_palette(&cycle_water,4);

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
	}
	StopMOD(1);
	LT_unload_sprite(&sprite_player); //manually free sprites
}

void Load_Platform(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/Platform.tmx");
	load_tiles("GFX/Platil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy1,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy2,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy3,16);
	LT_Load_Sprite("GFX/player.bmp",&sprite_enemy4,16);
	LT_Load_Music("music/ADLIB/platform.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	
	LT_Set_Map(0,0);
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_MODE = 1;
}

void Run_Platform(){
	startp:
	sprite_player.pos_x = 80;
	sprite_player.pos_y = 70;
	Scene = 2;
	while(Scene == 2){
		//SCR_X and SCR_Y are global variables predefined 
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_follow(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		
		//scroll_map
		LT_scroll_map();
		
		//In this mode sprite is controled using L R and Jump
		LT_Player_Col = LT_move_player(&sprite_player);
		
		//if water, reset level
		if (LT_Player_Col.tile_number == 182) {
			MCGA_Fade_out(); 
			sprite_player.init = 0;
			LT_Set_Map(0,0);
			goto startp;
		}
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 10) Scene = 1;
		
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

void Load_Puzzle(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/puzzle.tmx");
	load_tiles("GFX/puztil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/ball.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/cursor2.bmp",&pointer,8);
	LT_Set_Sprite_Animation(&pointer,0,8,2);
	
	LT_Load_Music("music/ADLIB/top_down.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	sprite_player.pos_x = 4*16;
	sprite_player.pos_y = 4*16;
	
	LT_Set_Map(0,0);
	LT_MODE = 2; //Physics mode
}

void Run_Puzzle(){
	int rotate = 0;
	Scene = 2;
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_scroll_follow(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		LT_Draw_Sprite(&pointer);
		LT_gprint(LT_Player_Col.tilecol_number,240,160);
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//In mode 2, sprite is controlled using the speed.
		//Also there are basic default physics using the collision tiles
		LT_Player_Col = LT_move_player(&sprite_player);
		
		if(rotate == 360) rotate = 0;
		if(rotate < 0) rotate = 360;
		pointer.pos_x = sprite_player.pos_x + 4 + (LT_COS[rotate]>>2);
		pointer.pos_y = sprite_player.pos_y + 4 + (LT_SIN[rotate]>>2);
		
		//ROTATE
		if (LT_Keys[LT_RIGHT]) rotate++;
		if (LT_Keys[LT_LEFT]) rotate--;
		
		//HIT BALL
		if ((LT_Keys[LT_ACTION])&& (sprite_player.speed_x == 0) &&(sprite_player.speed_y == 0)){
		}
		
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 10) Scene = 1;
		
		cycle_palette(&cycle_water,2);

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
	}
	
	LT_Unload_Sprite(&pointer);
	LT_unload_sprite(&sprite_player); //manually free sprites
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/Shooter.tmx");
	load_tiles("GFX/stiles.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/Ship.bmp",&sprite_ship,32);
	
	LT_Load_Music("music/ADLIB/shooter.imf");
	
	LT_Delete_Loading_Interrupt();
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
	
	LT_Start_Music(70);
	LT_MODE = 3;
	LT_Set_Sprite_Animation(&sprite_ship,8,4,4);
	while(Scene == 4){
		
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_Draw_Sprite(&sprite_ship);
		LT_Endless_SideScroll_Map(0);
		LT_Player_Col = LT_move_player(&sprite_ship);
		if (LT_Player_Col.col_x == 1) {
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
	//LT_Check_CPU(); //still causing trouble
	//LT_Adlib_Detect(); 
	
	LT_Init_GUS(8);
	LT_Init();
	gotoxy(17,13);
	printf("LOADING");
	gotoxy(0,0);
	
	//You can use a custom loading animation for the Loading_Interrupt
	LT_Load_Animation("GFX/loading.bmp",&LT_Loading_Animation,32);
	LT_Set_Animation(&LT_Loading_Animation,0,16,2);

	LT_load_font("GFX/font.bmp");
	
	//LT_MODE = player movement modes
	//MODE TOP = 0;
	//MODE PLATFORM = 1;
	//MODE PUZZLE = 2;
	//MODE SIDESCROLL = 3;
	LT_MODE = 0;
	
	//LOGO
	Load_Logo();
	Run_Logo();

	//MENU
	menu:
	
	Load_Menu();
	Run_Menu();

	if (game == 0){
		Load_TopDown();
		Run_TopDown();
		goto menu;
	}
	if (game == 1){
		Load_Platform();
		Run_Platform();
		goto menu;
	}
	if (game == 2){
		Load_Puzzle();
		Run_Puzzle();
		goto menu;
	}	
	if (game == 3){
		Load_Shooter();
		Run_Shooter();
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
	