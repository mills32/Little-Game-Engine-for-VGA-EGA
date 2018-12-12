/***********************
*  LITTLE GAME ENGINE  *
************************/

/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	Sprite loader and bliter from xlib
	
	This is a 16-bit program, Remember to compile in the LARGE memory model!                        
	
	All code is 8086 / 8088 compatible
	
	Please feel free to copy this source code.                            
	

	LT_MODE = player movement modes

	//MODE TOP = 0;
	//MODE PLATFORM = 1;
	//MODE PUZZLE = 2;
	//MODE SIDESCROLL = 3;
	
	//33 fixed sprites:
	//	8x8   (16 sprites: 0 - 15)	
	//	16x16 (12 sprites: 16 - 27)
	//	32x32 ( 4 sprites: 28 - 31)
	//	64x64 ( 1 sprite: 32)
	
##########################################################################*/

#include "lt__eng.h"

unsigned char palette_cycle_water[] = {//generated with gimp, then divided by 4 (max = 64).
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

unsigned char palette_cycle_space[] = {//generated with gimp, then divided by 4 (max = 64).
	//2 copies of 8 colour cycle
	0x35,0x35,0x3f,//
	0x00,0x00,0x22,
	0x18,0x29,0x3f,
	0x09,0x1d,0x3e,
	0x12,0x0f,0x3f,
	0x17,0x22,0x3f,
	0x1c,0x2d,0x3e,
	0x24,0x33,0x3e,
	0x35,0x35,0x3f,//
	0x00,0x00,0x22,
	0x18,0x29,0x3f,
	0x09,0x1d,0x3e,
	0x12,0x0f,0x3f,
	0x17,0x22,0x3f,
	0x1c,0x2d,0x3e,
	0x24,0x33,0x3e
};

LT_Col LT_Player_Col;
COLORCYCLE cycle_water;

int x,y = 0;
int i,j = 0;
int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {7.3*16,4*16,7.3*16,5*16,7.3*16,6*16,7.3*16,7*16,7.3*16,8*16};
int game = 0;
int random;

void Load_Logo(){
	LT_Load_Map("GFX/Logo.tmx");
	LT_Load_Tiles("GFX/Logotil.bmp");
	VGA_Fade_out();
	LT_Set_Map(0,0);
}

void Run_Logo(){
	while (y < 144){
		LT_WaitVsyncEnd();
		VGA_Scroll(x,y);
		LT_scroll_map();
		y++;
		if (LT_Keys[LT_ESC]) {VGA_Scroll(0,0); goto exit;}//esc exit
		LT_WaitVsyncStart();
	}
	exit:
	sleep(1);
}

void Load_Menu(){
	LT_Set_Loading_Interrupt(); 
	
	LT_Load_Map("GFX/menu.tmx");
	LT_Load_Tiles("GFX/menutil.bmp");
	LT_Load_Sprite("GFX/cursor.bmp",16,16);
	//LT_Load_Music("music/menu2.imf");
	//LT_Start_Music(70);
	LT_LoadMOD("MUSIC/MOD/dyna.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	PlayMOD(0);

	VGA_SplitScreen(44);
	
	LT_Set_Map(0,0);
	
	i = 0;
	LT_MODE = 0;
}

void Run_Menu(){
	int delay = 0; //wait between key press
	Scene = 1;
	game = 0;
	menu_option = 0;
	
	x = 0;
	y = 0;
	
	LT_Set_Sprite_Animation(16,0,8,6);
	while(Scene == 1){
		LT_WaitVsyncEnd();

		LT_Restore_Sprite_BKG(16);
		LT_Draw_Sprite(16);
		
		LT_WaitVsyncStart();
		
		SCR_X = -54-(LT_SIN[i]>>1);
		
		sprite[16].pos_x = menu_pos[menu_option] + (LT_COS[j]>>3);
		sprite[16].pos_y = menu_pos[menu_option+1];
		
		if (i > 360) i = 0;
		i+=2;
		if (j > 356) {j = 0; sprite[16].anim_counter = 0;}
		j+=8;
		
		if (delay == 10){
			if (LT_Keys[LT_UP]) {menu_option -=2; game--;}
			if (LT_Keys[LT_DOWN]) {menu_option +=2; game++;}
			delay = 0;
		}
		delay++;
		
		if (menu_option < 0) menu_option = 0;
		if (menu_option > 8) menu_option = 8;		
		if (game < 0) game = 0;
		if (game > 4) game = 4;	
		
		if (LT_Keys[LT_ENTER]) Scene = 2;
		if (LT_Keys[LT_ESC]) {
			LT_unload_sprite(16);
			LT_ExitDOS();
		}
		
	}
	StopMOD();
	LT_unload_sprite(16);
}

void Load_TopDown(){
	LT_Set_Loading_Interrupt(); 
	
	LT_Load_Map("GFX/Topdown.tmx");
	LT_Load_Tiles("GFX/Toptil.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/enemy2.bmp",17,16);
	LT_Clone_Sprite(18,17);
	LT_Clone_Sprite(19,17);
	LT_Clone_Sprite(20,17);
	LT_Clone_Sprite(21,17);
	LT_Clone_Sprite(22,17);
	LT_Clone_Sprite(23,17);
	
	LT_Load_Music("music/adlib/top_down.imf");
	//LoadMOD("MUSIC/MOD/Beach.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	//PlayMOD(1);
	
	LT_SetWindow("GFX/window.bmp");
	
	LT_Set_Map(0,0);
	LT_MODE = 0;
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	Scene = 2;
}

void Run_TopDown(){
	int n;
	Scene = 2;
	x = 0;
	y = 0;
	sprite[16].pos_x = 8*16;
	sprite[16].pos_y = 4*16;
	
	//Place enemies manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	LT_Set_Enemy(17,13,7,4,0);
	LT_Set_Enemy(18,20,17,4,0);
	LT_Set_Enemy(19,54,4,4,0);
	LT_Set_Enemy(20,36,25,4,0);
	
	LT_Set_Enemy(21,33,18,0,4);
	LT_Set_Enemy(22,39,9,0,4);
	LT_Set_Enemy(23,70,16,0,4);
	
	for (n = 17; n != 21; n++) LT_Set_Sprite_Animation(n,6,2,8);
	for (n = 21; n != 24; n++) LT_Set_Sprite_Animation(n,2,2,8);
	
	while(Scene == 2){
		LT_WaitVsyncEnd();
		//SCR_X and SCR_Y are predefined global variables 
		LT_scroll_follow(16);
		
		//Restore BKG
		for (n = 23; n != 16; n--) LT_Restore_Enemy_BKG(n);
		LT_Restore_Sprite_BKG(16);
		//Draw sprites first to avoid garbage
		LT_Draw_Sprite(16);
		for (n = 17; n != 24; n++)LT_Draw_Enemy(n);
		
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		LT_Print_Window_Variable(32,LT_Player_Col.tile_number);
		
		cycle_palette(&cycle_water,4);
		
		
		//In this mode sprite is controlled using U D L R
		LT_Player_Col = LT_move_player(16);
		
		//Player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,6,6,4);
		else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(16,12,6,4);
		else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(16,18,6,4);
		else sprite[16].animate = 0;
		
		//Move the enemy
		for (n = 17; n != 24; n++) LT_Enemy_walker(n,0);
		
		//Enemy animations
		//for (n = 17; n != 24; n++){
		//	if (sprite[n].speed_x > 0) LT_Set_Sprite_Animation(n,0,6,5);
		//	if (sprite[n].speed_x < 0) LT_Set_Sprite_Animation(n,6,6,5);	}
		
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		LT_WaitVsyncStart();
	}
	//StopMOD(1);
	LT_unload_sprite(16); //manually free sprites
	LT_unload_sprite(17);
}

void Load_Platform(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/Platform.tmx");
	LT_Load_Tiles("GFX/Platil.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/enemy.bmp",17,16);
	LT_Clone_Sprite(18,17);
	LT_Clone_Sprite(19,17);
	LT_Clone_Sprite(20,17);
	LT_Clone_Sprite(21,17);
	LT_Clone_Sprite(22,17);
	LT_Clone_Sprite(23,17);
	
	LT_Load_Music("music/Adlib/platform.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	
	LT_SetWindow("GFX/window.bmp");
	
	//animate water
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_MODE = 1;
}

void Run_Platform(){
	int n;
	startp:
	LT_Set_Map(0,0);
	sprite[16].pos_x = 4*16;
	sprite[16].pos_y = 4*16;
	
	//Place enemies manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	LT_Set_Enemy(17,17,20,2,0);
	LT_Set_Enemy(18,61,19,2,0);
	LT_Set_Enemy(19,61,13,2,0);
	LT_Set_Enemy(20,92,15,2,0);
	LT_Set_Enemy(21,117,18,2,0);
	LT_Set_Enemy(22,163,20,2,0);
	LT_Set_Enemy(23,192,12,2,0);
	
	Scene = 2;
	
	while(Scene == 2){
		LT_WaitVsyncEnd();
		
		LT_scroll_follow(16);

		//Restore BKG
		for (n = 23; n != 16; n--) LT_Restore_Enemy_BKG(n);
		LT_Restore_Sprite_BKG(16);
		
		//Draw sprites first to avoid garbage
		LT_Draw_Sprite(16);
		for (n = 17; n != 24; n++) LT_Draw_Enemy(n);
		
		//scroll_map and draw borders
		LT_scroll_map();
	
		//In this mode sprite is controlled using L R and Jump
		LT_Player_Col = LT_move_player(16);
		
		//set player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,6,6,4);
		else sprite[16].animate = 0;
	
		//Move the enemy
		for (n = 17; n != 24; n++) LT_Enemy_walker(n,1);
		
		//Flip
		for (n = 17; n != 24; n++){
			if (sprite[n].mspeed_x > 0) LT_Set_Sprite_Animation(n,0,6,5);
			if (sprite[n].mspeed_x < 0) LT_Set_Sprite_Animation(n,6,6,5);	
		}
		
		//if water, reset level
		if (LT_Player_Col.tile_number == 182) {
			VGA_Fade_out(); 
			sprite[16].init = 0;
			goto startp;
		}
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;

		//water palette animation
		cycle_palette(&cycle_water,2);

		//esc go to menu
		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		LT_WaitVsyncStart();
	}
	
	LT_unload_sprite(16); //manually free sprites
	LT_unload_sprite(17);
}

void Load_Puzzle(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/puzzle.tmx");
	LT_Load_Tiles("GFX/puztil.bmp");
	
	LT_Load_Sprite("GFX/ball.bmp",16,16);
	LT_Load_Sprite("GFX/cursor2.bmp",0,8);
	LT_Set_Sprite_Animation(0,0,8,2);
	
	LT_Load_Music("music/ADLIB/puzzle.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_Set_Map(0,0);
	LT_MODE = 2; //Physics mode
}

void Run_Puzzle(){
	int rotate = 0;
	int power = 40;
	Scene = 2;
	
	sprite[16].pos_x = 4*16;
	sprite[16].pos_y = 3*16;
	
	//To simulate floats
	sprite[16].fpos_x = sprite[16].pos_x;
	sprite[16].fpos_y = sprite[16].pos_y;
	sprite[16].speed_x = 0;
	sprite[16].speed_y = 0;
	
	while(Scene == 2){
		LT_WaitVsyncEnd();
		//SCR_X and SCR_Y are predefined global variables 
		
		LT_Restore_Sprite_BKG(0);
		LT_Restore_Sprite_BKG(16);
		
		LT_scroll_follow(16);
		LT_Draw_Sprite(16);
		//Draw pointer if ball stopped
		if (sprite[16].state == 0) LT_Draw_Sprite(0);

		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//In mode 3, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		LT_Player_Col = LT_move_player(16);
		
		if(rotate == 360) rotate = 0;
		if(rotate < 0) rotate = 360;
		sprite[0].pos_x = sprite[16].pos_x + 4 + (LT_COS[rotate]>>2);
		sprite[0].pos_y = sprite[16].pos_y + 4 + (LT_SIN[rotate]>>2);
		
		//ROTATE 
		if (LT_Keys[LT_RIGHT]) rotate++;
		if (LT_Keys[LT_LEFT]) rotate--;
		
		//HIT BALL
		if ((LT_Keys[LT_ACTION]) && (sprite[16].state == 0)){
			LT_Delete_Sprite(0);
			sprite[16].state = 1; //Move
			//All this trouble to simulate floats :(
			sprite[16].speed_x = (LT_COS[rotate]/16)*80;
			sprite[16].speed_y = (LT_SIN[rotate]/16)*80;
		}

		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;
		
		cycle_palette(&cycle_water,2);

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		LT_WaitVsyncStart();
	}
	
	LT_Unload_Sprite(0);
	LT_unload_sprite(16); //manually free sprites
}

void Load_Puzzle2(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/arkanoid.tmx");
	LT_Load_Tiles("GFX/atiles.bmp");
	
	LT_Load_Sprite("GFX/bar.bmp",16,16);
	LT_Load_Sprite("GFX/aball.bmp",0,8);
	LT_Load_Music("music/ADLIB/top_down.imf");
	
	LT_Delete_Loading_Interrupt();
	LT_Start_Music(70);
	
	LT_SetWindow("GFX/window.bmp");
	
	LT_Set_Map(0,0);
	LT_MODE = 0; 
}

void Run_Puzzle2(){
	int score = 0;
	Scene = 2;
	
	sprite[16].pos_x = 10*16;
	sprite[16].pos_y = 12*16;
	
	sprite[0].pos_x = 10*16;
	sprite[0].pos_y = 10*16;
	
	//To bounce ball
	sprite[0].speed_x = 1;
	sprite[0].speed_y = -1;
	
	while(Scene == 2){
		LT_WaitVsyncEnd();
		//SCR_X and SCR_Y are predefined global variables 
		VGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(0);
		LT_Restore_Sprite_BKG(16);
		
		LT_Draw_Sprite(16);
		LT_Draw_Sprite(0);

		//simple physics to bounce a ball
		LT_Player_Col = LT_Bounce_Ball(0);
		if (LT_Player_Col.col_x == 5) score++;
		if (LT_Player_Col.col_y == 5) score++;
		
		//ROTATE 
		if (LT_Keys[LT_RIGHT]) sprite[16].pos_x++;
		if (LT_Keys[LT_LEFT]) sprite[16].pos_x--;
		
		//HIT BALL
		if ((LT_Keys[LT_ACTION]) && (sprite[16].state == 0)){
			sprite[0].state = 1; //Move
			sprite[0].speed_x = 1;
			sprite[0].speed_y = -1;
		}

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
		LT_WaitVsyncStart();
	}
	LT_Unload_Sprite(0);
	LT_unload_sprite(16); //manually free sprites
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/Shooter.tmx");
	LT_Load_Tiles("GFX/stiles.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/ship.bmp",28,32);
	
	LT_Load_Music("music/ADLIB/shooter.imf");
	
	LT_Delete_Loading_Interrupt();

	LT_SetWindow("GFX/window.bmp");
}

void Run_Shooter(){
	int timer;
	start:
	timer = 0;
	Scene = 0;
	sprite[28].pos_x = 8*16;
	sprite[28].pos_y = 16*16;
	LT_Set_Sprite_Animation(28,0,1,4);
	sprite[16].pos_x = 0;
	sprite[16].pos_y = 17*16;
	LT_Set_Sprite_Animation(16,0,6,4);
	
	LT_Set_Map(0,5);
	
	cycle_init(&cycle_water,palette_cycle_space);
	
	//WALK TO SHIP
	while (Scene == 0){
		LT_WaitVsyncEnd();

		LT_Restore_Sprite_BKG(16);
		LT_Restore_Sprite_BKG(28);
		
		LT_Draw_Sprite(28);
		LT_Draw_Sprite(16);
		
		sprite[16].pos_x++;
		
		if (sprite[16].pos_x == sprite[28].pos_x+8) Scene = 1; 
		cycle_palette(&cycle_water,2);
		LT_WaitVsyncStart();
	}
	
	//CLOSE SHIP
	LT_Set_Sprite_Animation(28,1,4,12);
	while (Scene == 1){
		LT_WaitVsyncEnd();

		LT_Restore_Sprite_BKG(28);
		LT_Draw_Sprite(28);
		if (sprite[28].frame == 4) Scene = 2; 
		cycle_palette(&cycle_water,2);
		LT_WaitVsyncStart();
	}
	//FLY UP
	sprite[28].animate = 0;
	while (Scene == 2){
		LT_WaitVsyncEnd();

		LT_Restore_Sprite_BKG(28);
		LT_scroll_follow(28);
		LT_Draw_Sprite(28);
		
		sprite[28].pos_y--;
		if (SCR_Y == 0) Scene = 3; 
		
		LT_scroll_map();
		cycle_palette(&cycle_water,2);
		LT_WaitVsyncStart();
	}
	
	//START ENGINE
	LT_Set_Sprite_Animation(28,4,4,5);
	while (Scene == 3){
		LT_WaitVsyncEnd();

		LT_Restore_Sprite_BKG(28);
		LT_Draw_Sprite(28);

		timer++;
		if (timer == 100) Scene = 4; 
		cycle_palette(&cycle_water,2);
		LT_WaitVsyncStart();
	}

	//SIDE SCROLL
	LT_Start_Music(70);
	LT_MODE = 3;
	LT_Set_Sprite_Animation(28,8,4,4);
	while(Scene == 4){
		LT_WaitVsyncEnd();
		SCR_X++;
		LT_Restore_Sprite_BKG(28);
		
		LT_Draw_Sprite(28);
		
		LT_Endless_SideScroll_Map(0);
		
		LT_Player_Col = LT_move_player(28);
		if (LT_Player_Col.col_x == 1) {
			LT_Set_Sprite_Animation(28,12,4,4);
		}
		if (sprite[28].frame == 15) {
			sprite[28].init = 0;
			sleep(2); 
			Scene = -1;
		}
		if (LT_Keys[LT_ESC]) Scene = -1; //esc exit
		
		cycle_palette(&cycle_water,2);
		LT_Print_Window_Variable(32,LT_Player_Col.tile_number);
		
		LT_WaitVsyncStart();
	}
	
	LT_unload_sprite(28);
	LT_unload_sprite(16);
}


void main(){

	system("cls");
	printf("LOADING");
	//LT_Check_CPU(); //still causing trouble
	//LT_Adlib_Detect(); 
	LT_Init_GUS(12);
    LT_init();
	
	//You can use a custom loading animation for the Loading_Interrupt
	LT_Load_Animation("GFX/loading.bmp",32);
	LT_Set_Animation(0,8,2);
	
	LT_Load_Font("GFX/font.bmp");
	
	Load_Logo();
	Run_Logo();
	
	LT_MODE = 0;
	
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
		Load_Puzzle2();
		Run_Puzzle2();
		goto menu;
	}   
	if (game == 4){
		Load_Shooter();
		Run_Shooter();
		goto menu;
	}
	
	LT_ExitDOS();
	
}

