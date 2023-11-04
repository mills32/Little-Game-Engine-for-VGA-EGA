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
	
	//20 fixed sprites:
	//	8x8   (sprites: 0 - 7)	
	//	16x16 (sprites: 8 - 15)
	//	32x32 (sprites: 16 - 10)
	//	64x64 (sprite: 20)
	
##########################################################################*/

//Library functions
#include "lt__eng.h"
#include <stdarg.h>

//Main program
LT_Sprite_State LT_Player_State;


int running = 1;
int x,y = 0;
int i,j = 0;
int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {108-16,116-16,124-16,132-16,140-16,148-16};
int game = 0;
int random;

//Sprite animations, you have up to 8 animations per sprite, 8 frames each. they will always loop at 8th frame
//AI sprites and player sprite have their first 6 animations resrved for common movements 
//Little robot animations
byte Player_Animation[] = {		//Animations for player are fixed at these offsets
	16,16,16,16,17,17,17,17,	// 0 UP
	18,18,18,18,19,19,19,19,	// 1 DOWN
	8,9,10,11,12,13,14,15,		// 2 LEFT
	0,1,2,3,4,5,6,7,			// 3 RIGHT
	25,25,25,25,25,25,25,25,	// 4 JUMP/FALL LEFT
	24,24,24,24,24,24,24,24,	// 5 JUMP/FALL RIGHT
	20,20,20,21,21,22,22,23,	// 6 custom (die)
	26,27,28,29,29,28,27,26,	// 7 custom (happy)
	
};
byte Menu_Cursor_Animation[] = {0,1,2,3,3,3,3,3};
byte Ship_Animation[] = {//
	0,1,0,1,0,1,0,1,
	0,0,1,0,1,0,1,0,
	0,0,0,0,1,1,1,1,
	0,0,0,0,1,1,1,1,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,2,2,2,3,
	0,0,0,0,1,1,1,1,
};
byte Player_Animation_2[] = {//Prehistorik man animations	
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	5,5,6,6,7,7,8,8,
	0,0,1,1,2,2,3,3,
	9,9,9,9,9,9,9,9,
	4,4,4,4,4,4,4,4,
	4,4,4,4,4,4,4,4,
};
byte Enemy0_Animation[] = {//
	0,0,1,1,0,0,1,1,
	2,2,3,3,2,2,3,3,
	4,4,5,5,4,4,5,5,	
	6,6,7,7,6,6,7,7,		
	0,0,0,0,0,0,0,0,	
};
byte Enemy1_Animation[] = {//Bouncing balls
	0,1,2,3,4,3,2,1,
	0,1,2,3,4,3,2,1,
	0,1,2,3,4,3,2,1,	
	0,1,2,3,4,3,2,1,		
	0,1,2,3,4,3,2,1,	
};
byte Enemy2_Animation[] = {//Walking bots
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	8,9,10,11,12,13,14,15,	
	0,1,2,3,4,5,6,7,		
	0,0,0,0,0,0,0,0,	
};
byte Ball_Animation[] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,	//Fall/Die
	};
byte Rocket_Animation[] = {0,1,0,2,0,1,0,2};

//Custom functions at the end of this file
void Print_Info_Wait_ENTER(word x, word y, int w, int h, unsigned char* text);
void Display_Intro();
void Load_TopDown();
void Run_TopDown();
void Load_Platform();
void Run_Platform();
void Load_Platform1();
void Run_Platform1();
void Load_Sonic();
void Run_Sonic();
void Load_Puzzle();
void Run_Puzzle();
void Load_Shooter();
void Run_Shooter();

//Main function
void main(){
	//Redirect stderr to a log file
	freopen("stderr.log", "w", stderr);
	// Use WAD file content
	LT_Use_WAD();
	LT_Setup();	//Allways the first, loads setup and initializes engine
	if (LT_VIDEO_MODE == 4) LT_Logo("logo_EGA.bmp");
	if (LT_VIDEO_MODE == 0) LT_Logo("logo_EGA.bmp");	//If you want a logo
	if (LT_VIDEO_MODE == 1) LT_Logo("logo_VGA.bmp");
	//Load this or the engine will crash
	LT_Load_Font("GFX/0_fontV.bmp"); //Load a font
	//Load a custom loading animation for Loading process
	if (LT_VIDEO_MODE == 0) LT_Load_Animation("GFX/loadinge.bmp");
	if (LT_VIDEO_MODE == 1) LT_Load_Animation("GFX/loading.bmp");
	if (LT_VIDEO_MODE == 4) LT_Load_Animation("GFX/loadingt.bmp");
	LT_Set_Animation(4); //Loading animation speed
	//That's it, let's play
	LT_MODE = 0;
	game = 0;
	//MENU
	while (running){
		switch (game){
			case 0: Display_Intro(); break;
			case 1: Load_TopDown(); game = 2; break;
			case 2: Run_TopDown(); break;
			case 3: Load_Platform(); game = 4; break;
			case 4: Run_Platform(); break;
			case 5: Load_Platform1(); game = 6; break;
			case 6: Run_Platform1(); break;
			case 7: Load_Puzzle(); game = 8; break;
			case 8: Run_Puzzle(); break;
			case 9: Load_Shooter(); game = 10; break;
			case 10: Run_Shooter(); break;
		} 
	}
	
	LT_ExitDOS();
	
}

//Game functions 

//LT_Draw_Text_Box prints stuff inside a box.
//If you want the box to "talk", set LT_Text_Speak_End to 0, then
//	paste LT_Draw_Text_Box inside a loop and wait for it to finish (LT_Text_Speak_End = 1)
void Print_Info_Wait_ENTER(word x, word y, int w, int h, unsigned char* text){
	LT_Text_Speak_End = 0;
	while (!LT_Text_Speak_End){
		LT_Draw_Text_Box(x,y,w,h,1,text);
		LT_WaitVsync();
	}
	LT_Text_Speak_End = 0;
	Clearkb();
	while (!LT_Keys[LT_ENTER]);
	Clearkb();
	LT_Delete_Text_Box(x,y,w,h);
}

void Display_Intro(){
	int key_timmer = 0;
	int change = 0; //wait between key press
	int transition = 0;
	int Speaker_Menu[16] = {56,52,51,45,40,30,25,15,10,5,0,0,0,0,0,0};
	int Speaker_Select[16] = {90,80,70,60,50,30,10,5,10,30,50,60,70,60,50,30};
	unsigned char Adlib_Bell_Sound[11] = {0x02,0x07,0x02,0x00,0xF5,0x93,0xCB,0xF0,0x00,0x00,0x01};
	unsigned char Adlib_Select_Sound[11] = {0x62,0x04,0x04,0x1E,0xF7,0xB1,0xF0,0xF0,0x00,0x00,0x03};
	unsigned char Adlib_PacMan_Sound[11] = {0xF9,0xF3,0x00,0x94,0xF3,0x91,0xF0,0xF1,0x0E,0x00,0x03};
	Scene = 1;
	menu_option = 0;
	SCR_X = 0;SCR_Y = 0;
	
	LT_SPRITE_MODE = 0;
	LT_IMAGE_MODE = 1;
	LT_ENDLESS_SIDESCROLL = 0;
	//Enable this to load stuff, it will fade out automatically
	//and show a cute animation
	
	LT_Set_Loading_Interrupt();
		if (LT_VIDEO_MODE == 0){
			LT_Load_Image("GFX/INTREGA.bmp"); //Load a 320x200 bkg image
			LT_Load_Sprite("GFX/orbE.bmp",0,0);
			LT_Load_Sprite("GFX/cursorbe.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("GFX/RocketE.bmp",20,0);
		}
		if (LT_VIDEO_MODE == 1){
			LT_Load_Image("GFX/INTRVGA.bmp"); //Load a 320x240 bkg image
			LT_Load_Sprite("GFX/orb.bmp",0,0);
			LT_Load_Sprite("GFX/cursorb.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("GFX/Rocketc.bmp",20,0);
		}
		if (LT_VIDEO_MODE == 4){
			LT_Load_Image("GFX/INTREGA.bmp"); //Load a 320x200 bkg image
			LT_Load_Sprite("GFX/orbt.bmp",0,0);
			LT_Load_Sprite("GFX/cursorbt.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("GFX/rockett.bmp",20,0);
			
			/*{
				FILE *test = fopen("test2.bin","wb");
				fwrite(&sprite[0].tga_sprite_data_offset[0],2,1,test);
				fwrite(&sprite[9].tga_sprite_data_offset[0],2,1,test);
				fwrite(&sprite[10].tga_sprite_data_offset[0],2,1,test);
				fclose(test);
			}*/
		}
		
		//Some music
		if(LT_MUSIC_MODE) LT_Load_Music("music/ADLIB/menu.vgm");
		if(!LT_MUSIC_MODE) LT_Load_Music("music/TANDY/tandyani.vgm");
	LT_Delete_Loading_Interrupt(); //End loading
	if (LT_VIDEO_MODE == 2)LT_Set_Map(0);
	
	
	LT_Set_Sprite_Animation(8,0);
	LT_Set_Sprite_Animation_Speed(8,6);
	LT_MODE = 0;
	
	//Init Sprite at positions
	LT_Init_Sprite(0,32,32);
	LT_Init_Sprite(8,0,0);
	LT_Init_Sprite(20,0,0);
	
	//This box won't talk
	LT_Text_Speak_End = 0;
	LT_Draw_Text_Box(13,10,12,7,0,"SELECT  DEMO             TOP DOWN    PLATFORM    PLATFORM 1  PUZZLE      SHOOTER   ");
	
	
	while (Scene == 1) {
		
		//sprite[0].pos_x++;
		//sprite[0].pos_y = 64 + (LT_COS[j]>>3);
		//if (sprite[0].pos_x > 320) sprite[0].pos_x = 0;
		
		sprite[0].pos_x = 80 + (LT_COS[j]>>4);
		sprite[8].pos_x = 80 + (LT_COS[j]>>4);
		sprite[8].pos_y = transition+menu_pos[menu_option+1];
		
		sprite[20].pos_x = 230;
		sprite[20].pos_y = 60+ (LT_COS[j]>>3);
		
		if (i > 360) i = 0;
		i+=2;
		if (j > 356) {j = 0; sprite[8].anim_counter = 0;}
		j+=8;
		
		//Select 
		switch (change){
			case 0:
				if (LT_Keys[LT_UP] == 1) change = 1;
				if (LT_Keys[LT_DOWN] == 1) change = 2;
			break;
			case 1:
				if (menu_option > 0) transition--;
				else change = 0;
				if (transition == -8){
					if (LT_SFX_MODE == 0) LT_Play_PC_Speaker_SFX(Speaker_Menu);
					if (LT_SFX_MODE == 1) LT_Play_AdLib_SFX(Adlib_Select_Sound,8,2,2);
					menu_option --; 
					game-=2;
					change = 3;
					transition = 0;
				}
			break;
			case 2:
				if (menu_option < 4) transition++;
				else change = 0;
				if (transition == 8){
					if (LT_SFX_MODE == 0) LT_Play_PC_Speaker_SFX(Speaker_Menu);
					if (LT_SFX_MODE == 1) LT_Play_AdLib_SFX(Adlib_Select_Sound,8,2,2);
					menu_option ++; 
					game+=2;
					change = 3;
					transition = 0;
				}
			break;
			case 3:
				if (key_timmer == 8) {key_timmer = 0; change = 0;}
				key_timmer++;
			break;
		}
		
		if (menu_option < 0) menu_option = 0;
		if (menu_option > 4) menu_option = 4;		
		if (game < 1) game = 1;
		if (game > 9) game = 9;	
		
		if (LT_Keys[LT_ENTER]) {
			Scene = 3; 
			if (LT_SFX_MODE == 0) LT_Play_PC_Speaker_SFX(Speaker_Select);
			if (LT_SFX_MODE == 1) /*adlib*/;
		}
		if (LT_Keys[LT_ESC]) {Scene = 3;game = 255; running = 0;}

		LT_Play_Music();
		LT_Update(0, 0);
	}
	LT_Text_Speak_End = 0;
}

void Load_TopDown(){

	LT_Set_Loading_Interrupt(); 
	if(LT_MUSIC_MODE) LT_Load_Music("music/ADLIB/music.vgm");
	if(!LT_MUSIC_MODE) LT_Load_Music("music/TANDY/platform.vgm");
	LT_Load_Map("GFX/Topdown.tmx");
	if (LT_VIDEO_MODE == 0){
		LT_Load_Sprite("GFX/playere.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/enemy2e.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("GFX/enemy3e.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("gfx/top_EGA.bmp");
	}
	if (LT_VIDEO_MODE == 1){
		LT_Load_Sprite("GFX/player.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/enemy2.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("GFX/enemy3.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("gfx/top_VGA.bmp");
		
	}
	if (LT_VIDEO_MODE == 4){
		LT_Load_Sprite("GFX/playert.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/tilet2.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("GFX/tilet2.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("gfx/top_EGA.bmp");
	}
	LT_Delete_Loading_Interrupt();
	
	LT_MODE = 0;
	
	Scene = 2;
}

void Run_TopDown(){
	int n;
	int dying = 0;
	byte sea_pal_offsets[4] = {1,5,12,13};
	byte sea_pal_cycle[4] = {15,11,11,11};
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,9*16,2*16);
	LT_Set_AI_Sprites(9,12,0,1);
	LT_Set_Sprite_Animation_Speed(8,1);
	
	LT_Set_Map(0);//unused y parameter
	if (LT_VIDEO_MODE)LT_Cycle_palette(1,4);
	else LT_Cycle_Palette_EGA(sea_pal_offsets,sea_pal_cycle,32);
	
	Scene = 2;
	LT_MODE = 0;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	
	Print_Info_Wait_ENTER(1,2,36,3,"A Top down style level, You can use it to create very symple adventure  games. PRESS ENTER.");
	
	while(Scene == 2){
		
		//LT_Print LT_Sprite_Stack/*cards>>1*/);
		
		//water palette animation
		if (LT_VIDEO_MODE)LT_Cycle_palette(1,4);
		else LT_Cycle_Palette_EGA(sea_pal_offsets,sea_pal_cycle,32);
		
		if (!dying){
			//In this mode sprite is controlled using U D L R
			LT_Player_State = LT_move_player(8);
			//Player animations run inside above function
		}
		
		//If collision tile = ?, end level
		if ((LT_Player_State.tilecol_number == 11) || (LT_Keys[LT_ESC])){
			Scene = 1; game = 0;
		}
		if (sprite[8].pos_y > (19*16)){Scene = 1; game = 0;}
		//if ((SCR_X>>1)&1)outportb(0x3d8, 1);
		//else outportb(0x3d8, 9);
		if (LT_Player_Col_Enemy()) dying = 1;
		
		if (dying) {
			LT_Set_Sprite_Animation(8,6);
			LT_Set_Sprite_Animation_Speed(8,8);
			if (sprite[8].frame == 23){
				//sfx
				Scene = 1;
				game = 2;
				sleep(1);
				LT_Fade_out(); 
			}
		}
		
		LT_Play_Music();
		//SCR_X and SCR_Y are predefined global variables 
		LT_Update(1,8);
	}
}

byte Level_cards = 0;
void Load_Platform(){
	LT_Set_Loading_Interrupt(); 
	if(LT_MUSIC_MODE) LT_Load_Music("music/ADLIB/platform.vgm");
	if(!LT_MUSIC_MODE) LT_Load_Music("music/TANDY/platform.vgm");

	if (LT_VIDEO_MODE == 0){
		LT_Load_Sprite("GFX/playerE.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/enemyE.bmp",9,Enemy2_Animation);
		LT_Load_Tiles("GFX/Pla_EGA.bmp");
		LT_Load_Map("GFX/Platform.tmx");
	}
	if (LT_VIDEO_MODE == 1){
		LT_Load_Sprite("GFX/player.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/enemy.bmp",9,Enemy2_Animation);
		LT_Load_Tiles("GFX/Pla_VGA.bmp");
		LT_Load_Map("GFX/Platform.tmx");
	}
	if (LT_VIDEO_MODE == 4){
		LT_Load_Sprite("GFX/playert.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/tilet2.bmp",9,Enemy0_Animation);
		LT_Load_Tiles("GFX/Pla_EGA.bmp");
		LT_Load_Map("GFX/Platform.tmx");
	}
	
	LT_Delete_Loading_Interrupt();
	
	LT_Set_Sprite_Animation_Speed(8,1);
	Scene = 2;
	Level_cards = 0;
}

void Run_Platform(){
	extern unsigned char Enemies;
	byte waterfall_pal_offsets[4] = {1,4,6,13};
	byte waterfall_pal_cycle[4] = {3,9,11,15};
	int Speaker_Crash[16] = {69,3,120,32,39,200,20,60,16,106,12,87,8,70,32,60};
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	byte Speaker_Get_Item_Adlib[] = {0xC8,0xF2,0x00,0x10,0xE6,0xC4,0x4C,0x40,0x0A,0x02,0x00};
	int n;
	byte last_cards = 0;
	int dying = 0;
	Scene = 2;
	
	//Init_Player
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,5*16,4*16);
	LT_Set_AI_Sprites(9,9,1,1);
	
	LT_Set_Map(0);

	//This sets the right waterfall colors
	if (LT_VIDEO_MODE)LT_Cycle_palette(1,2);
	else LT_Cycle_Palette_EGA(waterfall_pal_offsets,waterfall_pal_cycle,3);

	LT_MODE = 1;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	
	Print_Info_Wait_ENTER(1,1,36,4,"Just A Platform game test, I createdthe engine for this kind of game.   Collect items and reach the end.    PRESS ENTER.");
	while (Scene == 2){
		if (!dying){
			//In this mode sprite is controlled using L R and Jump
			LT_Player_State = LT_move_player(8);
			//set player animations
		}
		//EDIT MAP: GET cards
		switch (LT_Player_State.tile_number){
			case 48: LT_Sprite_Edit_Map(8, 116, 0); Level_cards++; break;
			case 49: LT_Sprite_Edit_Map(8, 0, 0); Level_cards++; break;
			case 50: LT_Sprite_Edit_Map(8, 1, 0); Level_cards++; break;
		}
		if (Level_cards != last_cards){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
			if (LT_SFX_MODE == 1)LT_Play_AdLib_SFX(Speaker_Get_Item_Adlib,8,4,0);
		}
		last_cards = Level_cards;
		
		if (LT_Player_State.jump){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 1)/*adlib*/;
		}
		
		//If collision with breakable tile (door in this case)
		if ((LT_Player_State.col_x == 5)){
			if (Level_cards < 8) Print_Info_Wait_ENTER(4,10,31,2,"YOU NEED 10 CARDS TO OPEN DOOR. PRESS ENTER.");
			else { //Open door
				if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
				if (LT_SFX_MODE == 1)/*adlib*/;
				LT_Edit_MapTile(sprite[8].tile_x+1,sprite[8].tile_y,86,0);
				sleep(1);
			}
		}
		//If collision tile = ?, end level
		if ((LT_Player_State.tilecol_number == 11)||(LT_Keys[LT_ESC])){Scene = 1; game = 0;}

		//Info window
		if (LT_Keys[LT_ACTION]){
			unsigned char text[] = "  INFO  WINDOW  LIVES 3 CARDS 0  PRESS ENTER";
			text[30] = Level_cards+48;
			Print_Info_Wait_ENTER(10,8,16,3,text);
		}


		//water palette animation
		if (LT_VIDEO_MODE)LT_Cycle_palette(1,2);
		else LT_Cycle_Palette_EGA(waterfall_pal_offsets,waterfall_pal_cycle,3);
		
		//if water or enemy, reset level
		if (sprite[8].pos_y > (18*16)){
			if (LT_SFX_MODE == 0){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 1)LT_Play_AdLib_SFX(Speaker_Get_Item_Adlib,8,4,0);
			Scene = 1;
			game = 4;
			sleep(1);
			LT_Fade_out();
		}
		if (LT_Player_Col_Enemy()){dying = 1;/*LT_Set_Sprite_Animation(16,0);*/}
		
		if (dying){
			if (LT_SFX_MODE == 0){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 1)/*adlib*/;
			Scene = 1;
			game = 4;
			sleep(1);
			LT_Fade_out();			
		}
		
		if (LT_VIDEO_MODE) LT_Parallax();
		LT_Play_Music();
		LT_Update(1,8);
	}
	Clearkb();
}

void Load_Platform1(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/pre2.tmx");
	if (LT_VIDEO_MODE == 1){
		LT_Load_Tiles("GFX/pre2.bmp");
		LT_Load_Sprite("GFX/pre2s.bmp",16,Player_Animation_2);
		LT_Load_Sprite("GFX/pre2e.bmp",17,Player_Animation_2);
	}
	if (LT_VIDEO_MODE == 0){
		LT_Load_Tiles("GFX/pre2ega.bmp");
		LT_Load_Sprite("GFX/pre2sega.bmp",16,Player_Animation_2);
		LT_Load_Sprite("GFX/pre2eEGA.bmp",17,Player_Animation_2);
	}
	if (LT_VIDEO_MODE == 4){
		LT_Load_Tiles("gfx/pre2ega.bmp");
		LT_Load_Sprite("GFX/playert.bmp",8,Player_Animation);
		LT_Load_Sprite("GFX/tilet2.bmp",9,Enemy0_Animation);
	}
	LT_Set_Sprite_Animation_Speed(16,4);
	LT_Set_Sprite_Animation_Speed(17,8);
	LT_Load_Music("music/ADLIB/music.vgm");
	LT_Delete_Loading_Interrupt();
}

void Run_Platform1(){
	extern unsigned char Enemies;
	
	int Speaker_Crash[16] = {69,3,120,32,39,200,20,60,16,106,12,87,8,70,32,60};
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int n;
	int dying = 0;
	Scene = 2;
	
	//Init_Player
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,4*16,4*16);
	LT_Set_AI_Sprites(17,17,1,1);
	
	LT_Set_Map(0);

	LT_MODE = 1;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;

	Print_Info_Wait_ENTER(1,1,36,6,"Another platform test. 32x32 spritesmight be too much for the 8086, but it can draw two sprites at 60 fps.  Fake parallax using palette cycles  is very fast because it only uses 64colors. PRESS ENTER.");

	while (Scene == 2){
		if (!dying){	
		//In this mode sprite is controlled using L R and Jump
		LT_Player_State = LT_move_player(16);
		
		if (LT_Player_State.jump){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 1)/*adlib*/;
		}
		
		if (LT_Player_Col_Enemy()) {
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Crash);
			if (LT_SFX_MODE == 1)/*adlib*/;
			dying = 1;
		}
		//If collision tile = ?, end level
		if ((LT_Player_State.tilecol_number == 11)||(LT_Keys[LT_ESC])){
			Scene = 1;
			game = 0;
		}
		} else {
			Scene = 1;
			game = 6;
			sleep(1);
			LT_Fade_out();
		}
		if (LT_VIDEO_MODE) LT_Parallax();
		LT_Play_Music();
		LT_Update(1,16);
	}
	Clearkb();
}

void Load_Puzzle(){
	
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/puz.tmx");
	if (LT_VIDEO_MODE){
		LT_Load_Tiles("GFX/puz_VGA.bmp");
		LT_Load_Sprite("GFX/ball.bmp",8,Ball_Animation);
		LT_Load_Sprite("GFX/ball1.bmp",9,Enemy0_Animation);
	} else {
		LT_Load_Tiles("GFX/puz_EGA.bmp");
		LT_Load_Sprite("GFX/balle.bmp",8,Ball_Animation);
		LT_Load_Sprite("GFX/ball1e.bmp",9,Enemy0_Animation);
	}
	LT_Load_Music("music/ADLIB/music.vgm");
	
	LT_Delete_Loading_Interrupt();

	LT_MODE = 2; //Physics mode
}

void Run_Puzzle(){
	int Breaking_ground_Time = 0;
	int dying = 0;
	byte trans_pal_offsets[4] = {14,11,6,3};
	byte trans_pal_cycle[4] = {15,11,3,9};
	Scene = 2;
	
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,4*16,4*16);
	LT_Set_AI_Sprites(9,9,0,0);
	LT_Set_Map(0);
	
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	
	Print_Info_Wait_ENTER(1,1,36,3,"This is a puzzle game, the ball has symple physics, like bouncing and   falling slopes. PRESS ENTER");
	
	while(Scene == 2){
		//In mode 2, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		if (!dying){
			LT_Player_State = LT_move_player(8);

			//If collision tile = ?, end level
			if (LT_Player_State.tilecol_number == 11) {Scene = 1; game = 0;}
			if (LT_Keys[LT_ESC]) {Scene = 1; game = 0;} //esc exit
			
			{//Samples to edit tiles on the map
				int col_x = LT_Player_State.col_x;
				int col_y = LT_Player_State.col_y;
				int tx = sprite[8].tile_x;
				int ty = sprite[8].tile_y;
				
				//If collision with breakable tiles
				switch (LT_Player_State.move){
					case 1: if (col_y == 5) LT_Edit_MapTile(tx,ty-1,0,0); break;
					case 2: if (col_y == 5) LT_Edit_MapTile(tx,ty+1,0,0); break;
					case 3: if (col_x == 5) LT_Edit_MapTile(tx-1,ty,0,0); break;
					case 4: if (col_x == 5) LT_Edit_MapTile(tx+1,ty,0,0); break;
				}
			
				//If on top of holes, animate tile 100 until it becomes 103
				if (LT_Player_State.tile_number > 99){
					if (Breaking_ground_Time == 16){
						if (LT_Player_State.tile_number < 103) LT_Sprite_Edit_Map(8,LT_Player_State.tile_number+1,0);
						Breaking_ground_Time = 0;
					}
					Breaking_ground_Time++;
				} else Breaking_ground_Time = 0;
			}
			if (LT_Player_State.tile_number == 103) dying = 1;
			if (LT_Player_Col_Enemy())dying = 1;
		} else {//Reset level
			LT_Set_Sprite_Animation(8,6);
			LT_Set_Sprite_Animation_Speed(8,8);
			if (sprite[8].frame == 7){
				Scene = 1; game = 8;
				sleep(1);
				LT_Fade_out();
			}
		}

		if (LT_VIDEO_MODE)LT_Cycle_palette(1,2);
		else LT_Cycle_Palette_EGA(trans_pal_offsets,trans_pal_cycle,2);
		LT_Play_Music();
		LT_Update(1,8);
	}
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
		Scene = 2;
		LT_Load_Map("GFX/Shooter.tmx");
		if (LT_VIDEO_MODE == 0) {
			LT_Load_Tiles("gfx/spa_EGA.bmp");
			LT_Load_Sprite("GFX/shipe.bmp",16,Ship_Animation);
			LT_Load_Sprite("GFX/rocketbe.bmp",17,Rocket_Animation);
		}
		if (LT_VIDEO_MODE == 1) {
			LT_Load_Tiles("gfx/spa_VGA.bmp");
			LT_Load_Sprite("GFX/ship.bmp",16,Ship_Animation);
			LT_Load_Sprite("GFX/rocketb.bmp",17,Rocket_Animation);
		}
		LT_Load_Music("music/ADLIB/music.vgm");
	
	LT_Delete_Loading_Interrupt();
}

void Run_Shooter(){
	int dying = 0;
	start:

	Scene = 2;
	
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,4*16,4*16);
	LT_Set_AI_Sprites(17,17,3,3);
	
	LT_Set_Sprite_Animation(17,0);
	LT_Set_Sprite_Animation_Speed(17,8);
	LT_Set_Sprite_Animation(18,0);
	LT_Set_Sprite_Animation_Speed(18,8);
	LT_Set_Sprite_Animation(19,0);
	LT_Set_Sprite_Animation_Speed(19,8);
	LT_Set_Map(0);
	
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	//SIDE SCROLL
	//LT_Start_Music(70);
	LT_MODE = 3;
	
	Print_Info_Wait_ENTER(1,1,36,5,"Maybe this engine is not the best   for a side scroller shooter, becausesprites spawn very far from the     screen edge, to avoid the update    column. PRESS ENTER");
	
	while(Scene == 2){
		if (!dying){
			SCR_X++;
			LT_Player_State = LT_move_player(16);
			if (LT_Keys[LT_ESC]) {Scene = 0; game = 0;} //esc exit
		
			if (LT_Player_Col_Enemy()) {
				LT_Set_Sprite_Animation(16,6);
				dying = 1;
			}
		} 
		if (dying){
			if (sprite[16].frame == 3){
				Scene = 1; 
				game = 10;
				sleep(1);
				LT_Fade_out();
			}
		}
		LT_Cycle_palette(1,2);
		LT_Play_Music();
		LT_Update(0,0);
	}
	Clearkb();
}

