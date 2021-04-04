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

const unsigned char palette_cycle_water[] = {//generated with gimp, then divided by 4 (max = 64).
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

const unsigned char palette_cycle_space[] = {//generated with gimp, then divided by 4 (max = 64).
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

extern byte LT_AI_Stack_Table[];

int running = 1;
int x,y = 0;
int i,j = 0;
int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {7.3*16,4*16,7.3*16,5*16,7.3*16,6*16,7.3*16,7*16,7.3*16,8*16};
int game = 0;
int random;


#ifndef M_PI
#define M_PI 3.14159
#endif

unsigned int SINEX[256];
unsigned int SINEY[256];

void Display_Intro(){
	int mode = 0;
	int speed = 1;

	LT_Fade_out();
	SCR_X = 0;SCR_Y = 0;
	
	LT_WaitVsync();
	if (LT_VIDEO_MODE) LT_Load_Image("GFX/DOTT.bmp");
	else LT_Load_Image("GFX/DOTT_EGA.bmp");
	if (LT_MUSIC_MODE) LT_Load_Music("music/ADLIB/letsgpcm.imf",1);
	else LT_Load_Music("music/ADLIB/letsg.imf",0);
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare80.wav");
	//sb_load_sample("MUSIC/samples/comun.wav");
	
	LT_Fade_in();
	while (!LT_Keys[LT_ESC]) {
		LT_WaitVsync();
		LT_Play_Music();
		
		if (SCR_Y == 480-241)LT_Draw_Text_Box(13,43,14,6,1);
		if (SCR_Y < 480-240)SCR_Y++;
		else {
			LT_Print(14,45,"   NEW GAME   ",1);
			LT_Print(14,47,"   CONTINUE   ",1);
			LT_Print(14,49,"   MUSIC: 0   ",1);
		}
		
		/*if ((mode == 1)&&(speed == 1)){
			if (SCR_Y > 0)SCR_Y--;
			else mode = 0;				
		}
		if ((mode == 0)&&(speed == 1)){
			if (SCR_Y < 427-240)SCR_Y++;
			else mode = 1;
		}*/
		
		//speed++;
		//if (speed == 2) speed = 0;
		
	}
	Clearkb();
	LT_Fade_out();

	SCR_Y = 0;
	LT_WaitVsync();
}

void Load_Test(){
	LT_Set_Loading_Interrupt(); 
	for( i = 0; i < 256; ++i ) {
		SINEX[ i ] = 320 * ( (sin( 2.0 * M_PI * i / 256.0 ) + 1.0 ) / 2.0 );
		SINEY[ i ] = 200 * ( (sin( 2.0 * M_PI * i / 128.0 ) + 1.0 ) / 2.0 );
	}
	LT_Load_Map("gfx/lisa.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("gfx/lisa_VGA.bmp");
	else LT_Load_Tiles("GFX/lisa_EGA.bmp");
	LT_Load_Sprite("GFX/ship.bmp",28,32);
	LT_Clone_Sprite(29,28);
	LT_Delete_Loading_Interrupt();

	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(28,160,120);
	LT_Init_Sprite(29,100,120);
	LT_Set_Map(0,0);
	LT_MODE = 0;
	Scene = 0;
	i = 0;
	LT_Set_Sprite_Animation(28,4,12,4);
	LT_Set_Sprite_Animation(29,4,12,4);
}

void Run_Test(){
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	int A = 0;
	
	while(Scene == 0){
		if (sprite[28].pos_x-160 < SINEX[0]) sprite[28].pos_x++;
		else Scene = 1;
		if (sprite[28].pos_y-120 < SINEY[0]) sprite[28].pos_y++;
		sprite[29].pos_x = sprite[28].pos_x - 64;
		sprite[29].pos_y = sprite[28].pos_y;
		
		LT_scroll_follow(28);
		LT_Draw_Sprites();
		LT_scroll_map();
		LT_WaitVsync();		
	}
	
	while(Scene == 1){
		LT_scroll_follow(28);
		LT_Draw_Sprites();
		sprite[28].pos_x = SINEX[i & 255]+160;
		sprite[28].pos_y = SINEY[i & 255]+120;
		sprite[29].pos_x = sprite[28].pos_x - 64;
		sprite[29].pos_y = sprite[28].pos_y;
		
		i++;
		A++;
		//scroll_map and draw borders
		LT_scroll_map();
		if (LT_Keys[LT_ESC]) {Scene = 2;game = 0;}
		if (LT_Keys[LT_JUMP]) LT_Play_PC_Speaker_SFX(Speaker_Jump);
		if (LT_Keys[LT_ACTION]) LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
		LT_WaitVsync();
	}
	Clearkb();
	LT_unload_sprite(28);
	LT_unload_sprite(29);
	Scene = 2;game = 0;
}

void Load_Menu(){
	LT_Set_Loading_Interrupt(); 
	
	LT_Load_Map("GFX/menu.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("GFX/menu_VGA.bmp");
	else LT_Load_Tiles("GFX/menu_EGA.bmp");
	LT_Load_Sprite("GFX/cursorb.bmp",16,16);
	LT_Load_Sprite("GFX/Rocketc.bmp",32,64);
	
	LT_Load_Music("music/ADLIB/what.imf",1);
	
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/in_hole.wav");
	sb_load_sample("MUSIC/samples/eing.wav");
	sb_load_sample("MUSIC/samples/hit.wav");
	sb_load_sample("MUSIC/samples/byebye.wav");
	sb_load_sample("MUSIC/samples/touch.wav");
	sb_load_sample("MUSIC/samples/pop.wav");
	
	LT_Delete_Loading_Interrupt();
	
	//PlayMOD(0);

	VGA_SplitScreen(44);
	
	//Init_Sprites
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,0,0);
	LT_Init_Sprite(32,0,0);
	LT_Set_Map(0,0);
	
	i = 0;
	LT_MODE = 0;
	game = 1;
}

void Run_Menu(){
	int change = 0; //wait between key press
	int transition = 0;
	Scene = 1;
	menu_option = 0;
	
	x = 0;
	y = 0;
	LT_Draw_Text_Box(28,4,8,8,1);
	LT_Print(29,5,"-<>[\\]^_",1);
	LT_Print(29,6,"=TESTING",1);
	LT_Set_Sprite_Animation(16,0,8,6);
	while(Scene == 1){
		LT_Draw_Sprites_Fast();
		
		SCR_X = -54-(LT_SIN[i]>>1);
		
		sprite[16].pos_x = menu_pos[menu_option] + (LT_COS[j]>>3);
		sprite[16].pos_y = transition+menu_pos[menu_option+1];
		
		sprite[32].pos_x = 24;
		sprite[32].pos_y = 56+ (LT_COS[j]>>3);
		
		if (i > 360) i = 0;
		i+=2;
		if (j > 356) {j = 0; sprite[16].anim_counter = 0;}
		j+=8;
		
		if (LT_Keys[LT_UP] == 1) change = 1;
		if (LT_Keys[LT_DOWN] == 1) change = 2;
			
		if (change == 1){
			transition--;
			if (transition == -17){
				sb_play_sample(7,11025);
				menu_option -=2; 
				game-=2;
				change = 0;
				transition = 0;
			}
		}
		if (change == 2) {
			transition++;
			if (transition == 17){
				sb_play_sample(7,8000);
				menu_option +=2; 
				game+=2;
				change = 0;
				transition = 0;
			}
		}
		
		if (menu_option < 0) menu_option = 0;
		if (menu_option > 8) menu_option = 8;		
		if (game < 1) game = 1;
		if (game > 9) game = 9;	
		
		if (LT_Keys[LT_ENTER]) {Scene = 2; sb_play_sample(5,8000);}
		if (LT_Keys[LT_JUMP]) {Scene = 0;game = 11;}
		if (LT_Keys[LT_ESC]) {Scene = 2;game = 255; running = 0;}
		LT_Play_Music();
		LT_WaitVsync();

		y++;
	}
	LT_unload_sprite(16);
	LT_unload_sprite(32);
}

void Load_TopDown(){

	LT_Set_Loading_Interrupt(); 
	
	LT_Load_Map("GFX/Topdown.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("gfx/top_VGA.bmp");
	else LT_Load_Tiles("GFX/top_EGA.bmp");

	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/enemy2.bmp",17,16);

	LT_Load_Music("music/adlib/save.imf",1);
	if(LT_VIDEO_MODE)LT_SetWindow("GFX/win_VGA.bmp");
	else LT_SetWindow("GFX/win_EGA.bmp");
	
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/in_hole.wav");
	sb_load_sample("MUSIC/samples/boing.wav");
	sb_load_sample("MUSIC/samples/splash.wav");

	LT_Delete_Loading_Interrupt();
	
	LT_MODE = 0;
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	Scene = 2;
}

void Run_TopDown(){
	int n;
	int dying = 0;
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,8*16,4*16);
	LT_Set_AI_Sprites(17,7);
	LT_Set_Map(0,0);
	Scene = 2;
	//Place AI manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		//Draw sprites first to avoid garbage
		LT_scroll_follow(16);
		LT_Draw_Sprites();
		
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//LT_Print_Window_Variable(32,LT_Player_Col.tile_number);
		LT_Print_Window_Variable(32,LT_Sprite_Stack/*cards>>1*/);
		
		cycle_palette(&cycle_water,4);
		
		if (!dying){
			//In this mode sprite is controlled using U D L R
			LT_Player_Col = LT_move_player(16);
		
			//Player animations
			if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,8,2);
			else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,8,8,2);
			else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(16,16,2,6);
			else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(16,18,2,6);
			else sprite[16].animate = 0;
		
			//Move the enemies
			LT_Update_AI_Sprites();
		
			//Flip
			for (n = 1; n != LT_Sprite_Stack; n++){
				if (sprite[LT_Sprite_Stack_Table[n]].mspeed_x) LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[n],4,2,8);
				if (sprite[LT_Sprite_Stack_Table[n]].mspeed_y) LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[n],0,2,8);	
			}
		}
		//If collision tile = ?, end level
		if ((LT_Player_Col.tilecol_number == 11) || (LT_Keys[LT_ESC])){
			Scene = 1; game = 0;
		}
		
		LT_Play_Music();
		
		LT_WaitVsync();
		
		if (LT_Player_Col_Enemy()) dying = 1;
		
		if (dying) {
			LT_Set_Sprite_Animation(16,20,4,8);
			if (sprite[16].frame == 23){
				sb_play_sample(2,11025);
				sprite[16].init = 0;
				Scene = 1;
				game = 2;
				sleep(1);
				if (LT_VIDEO_MODE == 1)LT_Fade_out(); 
			}
		}
	}
	LT_unload_sprite(16); //manually free sprites
	LT_unload_sprite(17);
}

void Load_Platform(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/Platform.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("GFX/Pla_VGA.bmp");
	else LT_Load_Tiles("GFX/Pla_EGA1.bmp");
	if (LT_VIDEO_MODE)LT_SetWindow("GFX/win_VGA.bmp");
	else LT_SetWindow("GFX/win_EGA.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/enemy.bmp",17,16);

	LT_Load_Music("music/ADLIB/faces.imf",1);
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/in_hole.wav");
	sb_load_sample("MUSIC/samples/boing.wav");
	sb_load_sample("MUSIC/samples/splash.wav");
	LT_Delete_Loading_Interrupt();
	
	//animate water
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_MODE = 1;
}

void Run_Platform(){
	extern unsigned char Enemies;
	int Crash[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	int timer = 0;
	int timer2 = 0;
	int timer3 = 0;
	int n;
	byte cards = 0;
	int dying = 0;
	Scene = 2;
	
	//Init_Player
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,4*16,4*16);
	LT_Set_AI_Sprites(17,7);
	
	LT_Set_Map(0,0);
	
	while (Scene == 2){
		
		LT_scroll_follow(16);
		LT_Draw_Sprites();
		
		//EDIT MAP: GET cards
		switch (LT_Player_Col.tile_number){
			case 134: LT_Get_Item(16, 1, 0); cards++; break;
			case 135: LT_Get_Item(16, 22, 0); cards++; break;
			case 136: LT_Get_Item(16, 0, 0); cards++; break;
			case 137: LT_Get_Item(16, 109, 0); cards++; break;
		} 
		
		//scroll_map and draw borders
		LT_scroll_map();
	
		if (!dying){
			//In this mode sprite is controlled using L R and Jump
			LT_Player_Col = LT_move_player(16);
		
			//set player animations
			if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(16,0,8,2);
			else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(16,8,8,2);
			else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(16,16,2,6);
			else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(16,16,2,6);
			else sprite[16].animate = 0;
			
			//Move the enemies
			LT_Update_AI_Sprites();
		
			//Flip
			for (n = 1; n != LT_Sprite_Stack; n++){
				if (sprite[LT_Sprite_Stack_Table[n]].mspeed_x > 0) LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[n],0,8,5);
				if (sprite[LT_Sprite_Stack_Table[n]].mspeed_x < 0) LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[n],8,8,5);	
			}
		}
		//If collision tile = ?, end level
		if ((LT_Player_Col.tilecol_number == 11)||(LT_Keys[LT_ESC])){
			Scene = 1;
			game = 0;
			for (n = 1; n != LT_Sprite_Stack; n++)LT_Delete_Sprite(LT_Sprite_Stack_Table[n]);
			LT_unload_sprite(16); //manually free sprites
			LT_unload_sprite(17);
		}
		
		LT_Print_Window_Variable(32,LT_Sprite_Stack/*cards>>1*/);

		//water palette animation
		cycle_palette(&cycle_water,2);
		
		LT_Play_Music();
		LT_WaitVsync();
		//if water or enemy, reset level
		if (LT_Player_Col.tile_number == 102){
			sb_play_sample(5,11025);
			sprite[16].init = 0;
			Scene = 1;
			game = 4;
			sleep(1);
			if (LT_VIDEO_MODE == 1)LT_Fade_out();
			//for (n = 1; n != LT_Sprite_Stack; n++)LT_Delete_Sprite(LT_Sprite_Stack_Table[n]);	
		}
		if (LT_Player_Col_Enemy())dying = 1;
		
		if (dying){
			LT_Set_Sprite_Animation(16,20,4,8);
			if (sprite[16].frame == 23){
				sb_play_sample(2,11025);
				sprite[16].init = 0;
				Scene = 1;
				game = 4;
				sleep(1);
				if (LT_VIDEO_MODE == 1)LT_Fade_out();
				//for (n = 1; n != LT_Sprite_Stack; n++)LT_Delete_Sprite(LT_Sprite_Stack_Table[n]);				
			}
		}
	}
	Clearkb();
	LT_unload_sprite(16); //manually free sprites
	LT_unload_sprite(17);
}

void Load_Puzzle(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/puz.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("gfx/puz_VGA.bmp");
	else LT_Load_Tiles("GFX/puz_EGA.bmp");
	
	LT_Load_Sprite("GFX/ball.bmp",16,16);
	LT_Load_Sprite("GFX/ball1.bmp",17,16);
	
	LT_Load_Music("music/ADLIB/puzzle.imf",0);
	
	LT_Delete_Loading_Interrupt();
	
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_MODE = 2; //Physics mode
}

void Run_Puzzle(){
	int rotate = 0;
	//int power = 40;
	Scene = 2;
	
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,4*16,3*16);
	LT_Set_AI_Sprites(17,7);
	LT_Set_Map(0,0);
	
	//To simulate floats
	sprite[16].fpos_x = sprite[16].pos_x;
	sprite[16].fpos_y = sprite[16].pos_y;
	sprite[16].speed_x = 0;
	sprite[16].speed_y = 0;
	
	while(Scene == 2){
		LT_scroll_follow(16);
		LT_Draw_Sprites();

		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//In mode 3, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		LT_Player_Col = LT_move_player(16);
		
		if(rotate == 360) rotate = 0;
		if(rotate < 0) rotate = 360;

		//ROTATE 
		if (LT_Keys[LT_RIGHT]) sprite[16].speed_x+=20;
		if (LT_Keys[LT_LEFT]) sprite[16].speed_x-=20;
		if (LT_Keys[LT_UP]) sprite[16].speed_y-=20;
		if (LT_Keys[LT_DOWN]) sprite[16].speed_y+=20;
		
		//sprite[0].pos_x = sprite[16].pos_x+32;
		//sprite[0].pos_y = sprite[16].pos_y;
		
		//HIT BALL
		sprite[16].state = 1; //Move

		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) {Scene = 1; game = 0;}
		if (LT_Keys[LT_ESC]) {Scene = 1; game = 0;} //esc exit
		cycle_palette(&cycle_water,2);
		
		LT_Play_Music();
		LT_WaitVsync();
	}
	
	//LT_Unload_Sprite(0);
	LT_unload_sprite(16); //manually free sprites
}

void Load_Puzzle2(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/puz1.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("gfx/puz1_VGA.bmp");
	else LT_Load_Tiles("GFX/puz1_EGA.bmp");
	
	LT_Load_Sprite("GFX/bar.bmp",16,16);
	LT_Load_Sprite("GFX/aball.bmp",0,8);
	LT_Load_Music("music/ADLIB/top_down.imf",0);
	
	LT_Delete_Loading_Interrupt();
	//LT_Start_Music(70);
	
	if(LT_VIDEO_MODE)LT_SetWindow("GFX/win_VGA.bmp");
	else LT_SetWindow("GFX/win_EGA.bmp");
	
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,0,0);
	LT_Init_Sprite(0,0,0);
	
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
		//SCR_X and SCR_Y are predefined global variables 
		VGA_Scroll(SCR_X,SCR_Y);
		
		//simple physics to bounce a ball
		LT_Player_Col = LT_Bounce_Ball(0);
		
		LT_Draw_Sprites();

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

		if (LT_Keys[LT_ESC]) {Scene = 1; game = 0;} //esc exit
		
		LT_Play_Music();
		LT_WaitVsync();
	}
	LT_Unload_Sprite(0);
	LT_unload_sprite(16); //manually free sprites
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/Shooter.tmx");
	if (LT_VIDEO_MODE) LT_Load_Tiles("gfx/spa_VGA.bmp");
	else LT_Load_Tiles("GFX/spa_EGA.bmp");
	LT_Reset_Sprite_Stack();
	LT_Load_Sprite("GFX/player.bmp",16,16);
	LT_Load_Sprite("GFX/ship.bmp",28,32);
	LT_Load_Sprite("GFX/rocketb.bmp",29,32);
	LT_Init_Sprite(16,0,0);
	LT_Init_Sprite(28,0,0);
	LT_Init_Sprite(29,0,0);
	LT_Set_AI_Sprites(29,3);
	LT_Load_Music("music/ADLIB/shooter.imf",0);
	
	LT_Delete_Loading_Interrupt();

	if(LT_VIDEO_MODE)LT_SetWindow("GFX/win_VGA.bmp");
	else LT_SetWindow("GFX/win_EGA.bmp");
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
		LT_Draw_Sprites();
		

		sprite[16].pos_x++;
		
		if (sprite[16].pos_x == sprite[28].pos_x+8){
			sprite[16].pos_x = -16; //hide
			Scene = 1; 
		} 
		cycle_palette(&cycle_water,2);
		LT_WaitVsync();
	}
	
	//CLOSE SHIP
	LT_Set_Sprite_Animation(28,1,4,12);
	while (Scene == 1){
		LT_Draw_Sprites();
		if (sprite[28].frame == 4) Scene = 2; 
		cycle_palette(&cycle_water,2);
		LT_WaitVsync();
	}
	//FLY UP
	sprite[28].animate = 0;
	while (Scene == 2){
		LT_scroll_follow(28);
		LT_Draw_Sprites();
		
		sprite[28].pos_y--;
		if (SCR_Y == 0) Scene = 3; 
		
		LT_scroll_map();
		cycle_palette(&cycle_water,2);
		LT_WaitVsync();
	}
	
	//START ENGINE
	LT_Set_Sprite_Animation(28,4,4,5);
	while (Scene == 3){
		LT_Draw_Sprites();

		timer++;
		if (timer == 100) Scene = 4; 
		cycle_palette(&cycle_water,2);
		LT_WaitVsync();
	}

	//SIDE SCROLL
	//LT_Start_Music(70);
	LT_MODE = 3;
	LT_Set_Sprite_Animation(28,8,4,4);
	while(Scene == 4){
		SCR_X++;
		LT_Draw_Sprites();
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
		
		if (LT_Keys[LT_ESC]) {Scene = 0; game = 0;} //esc exit
		
		cycle_palette(&cycle_water,2);
		LT_Print_Window_Variable(32,SCR_X>>4);
		
		LT_Play_Music();
		LT_WaitVsync();
	}
	
	LT_unload_sprite(28);
	LT_unload_sprite(29);
	LT_unload_sprite(16);
}


void main(){
	
	LT_Setup();
	LT_Logo();
	
	//You can use a custom loading animation for the Loading_Interrupt
	LT_Load_Animation("GFX/loading.bmp",32);
	LT_Set_Animation(0,8,2);
	
	Display_Intro();
	
	LT_MODE = 0;
	game = 0;
	//MENU
	while (running){
		switch (game){
			case 0: Load_Menu();Run_Menu(); break;
			case 1: Load_TopDown(); game = 2; break;
			case 2: Run_TopDown(); break;
			case 3: Load_Platform(); game = 4; break;
			case 4: Run_Platform(); break;
			case 5: Load_Puzzle(); game = 6; break;
			case 6: Run_Puzzle(); break;
			case 7: Load_Puzzle2(); game = 8; break;
			case 8: Run_Puzzle2(); break;
			case 9: Load_Shooter(); game = 10; break;
			case 10: Run_Shooter(); break;
			case 11: Load_Test();Run_Test(); break;
		} 
	}
	
	LT_ExitDOS();
	
}

