/*

************************
*  LITTLE GAME ENGINE  *
************************

##########################################################################
	
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	Sprite loader and bliter from xlib
	
	Please feel free to copy this source code.  
	
	This is a 16-bit program, Remember to compile in the COMPACT memory model.
	If your code becomes huge, you might have to use LARGE memory model.	
	
	All code is 8086 / 8088 compatible, because my first PC was one of these,
	and people programmed games for 286 without realizing many games could
	work on slower PCs, we didn't mind low FPS back then.
	
	Do NOT define big arrays inside functions. If you must, only use text:
		- DON't use this: unsigned char text[] = "MY TEXT";
		- use this:       unsigned char *text  = "MY TEXT";
		this avoids the program trying to use libraries (which contain 
		"N_SCOPY@" function) and avoids making the code bigger. 
	
	LT_MODE = player movement modes
		MODE TOP = 0;
		MODE PLATFORM = 1;
		MODE PUZZLE = 2;
		MODE SIDESCROLL = 3;
	
	SPRITES:
		8x8   (8 sprites:  0 -  7th)	
		16x16 (8 sprites:  8 - 15th)
		32x32 (4 sprites: 16 - 19th)
		64x64 (1  sprite:      20th, does not restore BKG)
	
	When you use two different sprites in a scene:
		LT_Set_AI_Sprites(A,B,0,0);
	If sprite "A" is at sprite[0], sprite B must be at least sprite[2].
	If the map is going to have more than one "A" sprite in a certain spot
	(for example 3), sprite "B" must be loaded at sprite[4] at least
	
	LT_GET_VIDEO reeturns the video mode selected in the setup function.
		0 = mode 0x10 16 colors EGA/VGA
		1 = mode 0x13 + mode X, 256 colors VGA
		2 = CGA
		3 = TANDY
		4 = Hercules
	
	LT_Logo and all "Load" functions can load ordinary files, or read a 
	custom DAT format created for the engine.
		To load ordinary files:
			-use the first argument in any load function
			-write a zero '0' in the second argument
		To load data from LDAT files:
			-write the DAT name in the first argument
			-write the file name in the second argument
	
##########################################################################*/

//Library functions
#include "lt__eng.h"

//Main program
int running = 1;
int x,y = 0;
int i,j = 0;
int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {108-16,116-16,124-16,132-16,140-16,148-16,156-16};
int game = 0;
int random;

//Sprite animations, you have up to 8 animations per sprite, 8 frames each. they will always loop at 8th frame
//AI sprites and player sprite have their first 6 animations resrved for common movements 
//Little robot animations
byte Player_Animation[] = {	//Animations for player are fixed at these offsets
	16,16,16,16,17,17,17,17,	// 0 UP
	18,18,18,18,19,19,19,19,	// 1 DOWN
	8,9,10,11,12,13,14,15,		// 2 LEFT
	0,1,2,3,4,5,6,7,			// 3 RIGHT
	25,25,25,25,25,25,25,25,	// 4 JUMP/FALL LEFT
	24,24,24,24,24,24,24,24,	// 5 JUMP/FALL RIGHT
	20,20,20,21,21,22,22,23,	// 6 custom 
	26,27,28,29,29,28,27,26,	// 7 custom 
	
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

//SFX declare these outside functions to avoid "N_SCOPY@" error /////////
byte Speaker_Menu[16] = {56,52,51,45,40,30,25,15,10,5,0,0,0,0,0,0};
byte Speaker_Select[16] = {90,80,70,60,50,30,10,5,10,30,50,60,70,60,50,30};
byte Adlib_Bell_Sound[11] = {0x02,0x07,0x02,0x00,0xF5,0x93,0xCB,0xF0,0x00,0x00,0x01};
byte Adlib_Select_Sound[11] = {0x62,0x04,0x04,0x1E,0xF7,0xB1,0xF0,0xF0,0x00,0x00,0x03};
byte Adlib_PacMan_Sound[11] = {0xF9,0xF3,0x00,0x94,0xF3,0x91,0xF0,0xF1,0x0E,0x00,0x03};
byte Adlib_Metal[] = {0x00,0x01,0x00,0x80,0xF8,0xF0,0xF0,0xF0,0x00,0x00,0x01,0x00};
byte Speaker_Crash[16] = {69,3,120,32,39,200,20,60,16,106,12,87,8,70,32,60};
byte Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
byte Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
byte Speaker_Get_Item_Adlib[] = {0xC8,0xF2,0x00,0x10,0xE6,0xC4,0x4C,0x40,0x0A,0x02,0x00};


//Custom functions at the end of this file
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


unsigned char L_ANIMATION[4][16] = {
	"loadinge.bmp","loading.bmp","loadingc.bmp","loadingt.bmp"
};

unsigned char L_LOGO[4][16] = {
	"logo_EGA.bmp","logo_VGA.bmp","logo_CGA.bmp","logo_EGA.bmp"
};

//Main function
void main(){
	//Allways the first, loads setup and initializes engine
	LT_Setup();	
	//Load a font to show text boxes with data, variables and menus
	LT_Load_Font("IMAGES.DAT","font.bmp");
	//If you want a logo
	LT_Logo("IMAGES.DAT",&L_LOGO[LT_GET_VIDEO()][0]);
	//Load a custom loading animation for Loading process
	LT_Load_Animation("IMAGES.DAT",&L_ANIMATION[LT_GET_VIDEO()][0]);
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
void Sprite_Bounce_Left(byte spr){
	//Bounce away the sprite
	sprite[spr].pos_x-=2;LT_Update(0,0);
	sprite[spr].pos_x-=2;sprite[spr].pos_y-=2;LT_Update(0,0);
	sprite[spr].pos_x-=2;sprite[spr].pos_y-=1;LT_Update(0,0);
	sprite[spr].pos_x-=2;sprite[spr].pos_y+=3;LT_Update(0,0);
}

void Display_Intro(){
	int key_timmer = 0;
	int change = 0; //wait between key press
	int transition = 0;
	
	Scene = 1;
	menu_option = 0;
	SCR_X = 0;SCR_Y = 0;
	
	LT_SPRITE_MODE = 0;
	LT_IMAGE_MODE = 1;
	LT_ENDLESS_SIDESCROLL = 0;
	
	//Enable this to load stuff, it will fade out automatically
	//and show a cute animation
	LT_Start_Loading();
		
		if (LT_GET_VIDEO() == 0){
			LT_Load_Image("IMAGES.DAT","INTREGA.BMP"); //Load a 320x200 bkg image
			LT_Load_Sprite("SPRITES.DAT","orbE.bmp",0,0);
			LT_Load_Sprite("SPRITES.DAT","cursorbe.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("SPRITES.DAT","RocketE.bmp",20,0);
		}
		if (LT_GET_VIDEO() == 1){
			LT_Load_Image("IMAGES.DAT","INTRVGA.BMP"); //Load a 320x240 bkg image
			LT_Load_Sprite("SPRITES.DAT","orb.bmp",0,0);
			LT_Load_Sprite("SPRITES.DAT","cursorb.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("SPRITES.DAT","Rocketc.bmp",20,0);
		}
		if (LT_GET_VIDEO() == 3){
			
			LT_Load_Image("IMAGES.DAT","INTREGA.bmp"); //Load a 320x200 bkg image
			LT_Load_Sprite("SPRITES.DAT","orbt.bmp",0,0);
			LT_Load_Sprite("SPRITES.DAT","cursorbt.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
			LT_Load_Sprite("SPRITES.DAT","rockett.bmp",20,0);
			
			
			//FILE *test = fopen("test2.bin","wb");
			//fwrite(&sprite[0].tga_sprite_data_offset[0],2,1,test);
			//fwrite(&sprite[9].tga_sprite_data_offset[0],2,1,test);
			//fwrite(&sprite[10].tga_sprite_data_offset[0],2,1,test);
			//fclose(test);
		}
		
		//Some music
		if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","menu_ADL.vgm");
		if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","menu_TND.vgm");
	LT_End_Loading(); //End loading
	
	LT_Set_Sprite_Animation(8,0);
	LT_Set_Sprite_Animation_Speed(8,6);
	LT_MODE = 0;
	
	//Init Sprite at positions
	LT_Init_Sprite(0,32,32);
	LT_Init_Sprite(8,0,0);
	LT_Init_Sprite(20,0,0);

	//This box won't talk
	LT_Draw_Text_Box(13,10,12,8,0,0,0,"SELECT  DEMO             TOP DOWN    PLATFORM    PLATFORM 1  PUZZLE      SHOOTER     EXIT       ");
	//LT_Easter_Egg();
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
				if (menu_option < 5) transition++;
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
		
		if (menu_option == 4) game = 9;
		if (menu_option < 0) menu_option = 0;
		if (menu_option > 5) menu_option = 5;		
		if (game < 1) game = 1;
		if (game > 9) game = 9;	
		
		if (LT_Keys[LT_ACTION]) {
			if (menu_option == 5){Scene = 3;game = 255; running = 0;}
			else {
				Scene = 3; 
				if (LT_SFX_MODE == 0) LT_Play_PC_Speaker_SFX(Speaker_Select);
				if (LT_SFX_MODE == 1) ;
			}
		}
		if (LT_Keys[LT_ESC]) {Scene = 3;game = 255; running = 0;}
		LT_Play_Music();
		LT_Update(0, 0);
	}
}

void Load_TopDown(){
	LT_Start_Loading(); 
	if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","musi_ADL.vgm");
	if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","musi_TND.vgm");
	LT_Load_Map("MAPS.DAT","Topdown.tmx");
	if (LT_GET_VIDEO() == 0){
		LT_Load_Sprite("SPRITES.DAT","playere.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy2e.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy3e.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("TILESETS.DAT","top_EGA.bmp");
	}
	if (LT_GET_VIDEO() == 1){
		LT_Load_Sprite("SPRITES.DAT","player.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy2.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy3.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("TILESETS.DAT","top_VGA.bmp");
		
	}
	if (LT_GET_VIDEO() == 3){
		LT_Load_Sprite("SPRITES.DAT","playert.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy2t.bmp",9,Enemy0_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy3t.bmp",12,Enemy1_Animation);
		LT_Load_Tiles("TILESETS.DAT","top_EGA.bmp");
	}
	LT_End_Loading();
	
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
	if (LT_GET_VIDEO() == 1)LT_Cycle_palette(1,4);
	else LT_Cycle_Palette_TGA_EGA(sea_pal_offsets,sea_pal_cycle,32);
	
	Scene = 2;
	LT_MODE = 0;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENEMY_DIST_X = 13;
	LT_ENEMY_DIST_Y = 13;
	LT_ENDLESS_SIDESCROLL = 0;
	
	LT_Draw_Text_Box(1,2,36,3, 3,LT_ACTION,0,"A Top down style level, You can use it to create very symple adventure  games. PRESS ACTION.");
	while(Scene == 2){
		
		//LT_Print LT_Sprite_Stack);//cards>>1
		
		//water palette animation
		if (LT_GET_VIDEO() == 1)LT_Cycle_palette(1,4);
		else LT_Cycle_Palette_TGA_EGA(sea_pal_offsets,sea_pal_cycle,32);
		
		if (!dying){
			//In this mode sprite is controlled using U D L R
			LT_move_player(8);
			//Player animations run inside above function
		}
		
		//If collision tile = ?, end level
		if ((LT_Player_State[SPR_TILC_NUM] == 11) || (LT_Keys[LT_ESC])){
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
				LT_sleep(1);
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
	LT_Start_Loading(); 
	if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","plat_ADL.vgm");
	if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","plat_TND.vgm");

	LT_Load_Map("MAPS.DAT","Platform.tmx");
	if (LT_GET_VIDEO() == 0){
		LT_Load_Sprite("SPRITES.DAT","playerE.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemyE.bmp",9,Enemy2_Animation);
		LT_Load_Tiles("TILESETS.DAT","Pla_EGA.bmp");
	}
	if (LT_GET_VIDEO() == 1){
		LT_Load_Sprite("SPRITES.DAT","player.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemy.bmp",9,Enemy2_Animation);
		LT_Load_Tiles("TILESETS.DAT","Pla_VGA.bmp");
	}
	if (LT_GET_VIDEO() == 3){
		LT_Load_Sprite("SPRITES.DAT","playert.bmp",8,Player_Animation);
		LT_Load_Sprite("SPRITES.DAT","enemyt.bmp",9,Enemy2_Animation);
		LT_Load_Tiles("TILESETS.DAT","Pla_EGA.bmp");
	}
	LT_End_Loading();
	
	LT_Set_Sprite_Animation_Speed(8,1);
	Scene = 2;
	Level_cards = 0;
}

void Run_Platform(){
	extern unsigned char Enemies;
	byte waterfall_pal_offsets[4] = {1,4,6,13};
	byte waterfall_pal_cycle[4] = {3,9,11,15};
	int n;
	byte last_cards = 0;
	int dying = 0;
	Scene = 2;
	
	//Init_Player
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,2*16,8*16);
	
	LT_Set_AI_Sprites(9,9,1,1);
	
	LT_Set_Map(0);

	//This sets the right waterfall colors
	if (LT_GET_VIDEO() == 1)LT_Cycle_palette(1,2);
	else LT_Cycle_Palette_TGA_EGA(waterfall_pal_offsets,waterfall_pal_cycle,3);

	LT_MODE = 1;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	LT_ENEMY_DIST_X = 13;
	LT_ENEMY_DIST_Y = 12;
	LT_MUSIC_ENABLE = 0;
	LT_Draw_Text_Box(1,1,36,4,3,LT_ACTION,0,"Just A Platform game test, I createdthe engine for this kind of game.   Collect items and reach the end.    PRESS ACTION.");
	LT_MUSIC_ENABLE = 1;
	while (Scene == 2){
		if (!dying){
			//In this mode sprite is controlled using L R and Jump
			LT_move_player(8);
			//set player animations
		}
		//EDIT MAP: GET cards
		switch (LT_Player_State[SPR_TILP_NUM]){
			case 48: LT_Sprite_Edit_Map(8, 116, 0); Level_cards++; break;
			case 49: LT_Sprite_Edit_Map(8, 0, 0); Level_cards++; break;
			case 50: LT_Sprite_Edit_Map(8, 1, 0); Level_cards++; break;
		}
		if (Level_cards != last_cards){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
			if (LT_SFX_MODE == 1)LT_Play_AdLib_SFX(Speaker_Get_Item_Adlib,8,4,0);
		}
		last_cards = Level_cards;
		
		if (LT_Player_State[SPR_JUMP]){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 1);
		}
		
		//If collision with breakable tile (door in this case)
		if (LT_Player_State[SPR_COL_X] == 5){
			if (Level_cards < 8) LT_Draw_Text_Box(4,10,17,2,3,LT_ACTION,0,"YOU NEED 8 CARDS TO OPEN DOOR.   @");
			else { //Open door
				if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
				if (LT_SFX_MODE == 1)LT_Play_AdLib_SFX(Adlib_Metal,8,4,0);
				Sprite_Bounce_Left(8);
				//and update/open Door tile
				LT_Edit_MapTile(sprite[8].tile_x+1,sprite[8].tile_y,8,0);
				LT_Wait(1);
			}
		}
		
		//If collision tile = ?, end level
		if ((LT_Player_State[SPR_TILC_NUM] == 11)||(LT_Keys[LT_ESC])){Scene = 1; game = 0;}

		//Info window
		if (LT_Keys[LT_ACTION]){
			unsigned char *text = "  INFO  WINDOW  LIVES 3 CARDS 0   A@OK  B@EXIT  ";
			text[30] = Level_cards+48;
			if (LT_Draw_Text_Box(10,8,16,3,4,LT_ACTION,LT_JUMP,text)){Scene = 1; game = 0;}
		}


		//water palette animation
		if (LT_GET_VIDEO() == 1)LT_Cycle_palette(1,2);
		else LT_Cycle_Palette_TGA_EGA(waterfall_pal_offsets,waterfall_pal_cycle,3);
		
		//if water or enemy, reset level
		if (sprite[8].pos_y > (18*16)){
			if (LT_SFX_MODE == 0){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 1)LT_Play_AdLib_SFX(Speaker_Get_Item_Adlib,8,4,0);
			Scene = 1;
			game = 4;
			LT_sleep(1);
			LT_Fade_out();
		}
		if (LT_Player_Col_Enemy()){dying = 1;}//LT_Set_Sprite_Animation(16,0);
		
		if (dying){
			if (LT_SFX_MODE == 0){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 1);
			Scene = 1;
			game = 4;
			LT_sleep(1);
			LT_Fade_out();			
		}
		
		if (LT_GET_VIDEO()==1) LT_Parallax();
		LT_Play_Music();
		LT_Update(1,8);
	}
}

void Load_Platform1(){
	LT_Start_Loading(); 
	Scene = 2;
	if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","musi_ADL.vgm");
	if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","musi_TND.vgm");
	LT_Load_Map("MAPS.DAT","pre2.tmx");
	if (LT_GET_VIDEO() == 1){
		LT_Load_Sprite("SPRITES.DAT","pre2s.bmp",16,Player_Animation_2);
		LT_Load_Sprite("SPRITES.DAT","pre2e.bmp",17,Player_Animation_2);
		LT_Load_Tiles("TILESETS.DAT","pre2vga.bmp");
	}
	if (LT_GET_VIDEO() == 0){
		LT_Load_Sprite("SPRITES.DAT","pre2sega.bmp",16,Player_Animation_2);
		LT_Load_Sprite("SPRITES.DAT","pre2eEGA.bmp",17,Player_Animation_2);
		LT_Load_Tiles("TILESETS.DAT","pre2ega.bmp");
	}
	if (LT_GET_VIDEO() == 3){
		LT_Load_Sprite("SPRITES.DAT","pre2stga.bmp",16,Player_Animation_2);
		LT_Load_Sprite("SPRITES.DAT","pre2etga.bmp",17,Player_Animation_2);
		LT_Load_Tiles("TILESETS.DAT","pre2ega.bmp");
	}
	LT_Set_Sprite_Animation_Speed(16,4);
	LT_Set_Sprite_Animation_Speed(17,8);
	
	LT_End_Loading();
}

void Run_Platform1(){
	extern unsigned char Enemies;
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
	LT_ENEMY_DIST_X = 20;
	LT_ENEMY_DIST_Y = 20;
	LT_MUSIC_ENABLE = 0;
	LT_EGA_TEXT_TRANSLUCENT = 1;
	LT_Draw_Text_Box(1,1,36,6,0,0,0,"Another platform test. 32x32 spritesmight be too much for the 8086, but it can draw two sprites at 60 fps.  Fake parallax using palette cycles  is very fast because it only uses 64colors. PRESS ACTION.");
	LT_EGA_TEXT_TRANSLUCENT = 0;
	LT_MUSIC_ENABLE = 1;
	while (Scene == 2){
		if (!dying){	
		//In this mode sprite is controlled using L R and Jump
		LT_move_player(16);
		
		if (LT_Player_State[SPR_JUMP]){
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 1);
		}
		
		if (LT_Player_Col_Enemy()) {
			if (LT_SFX_MODE == 0)LT_Play_PC_Speaker_SFX(Speaker_Crash);
			if (LT_SFX_MODE == 1);
			dying = 1;
		}
		//If collision tile = ?, end level
		if ((LT_Player_State[SPR_TILC_NUM] == 11)||(LT_Keys[LT_ESC])){
			Scene = 1;
			game = 0;
		}
		} else {
			Scene = 1;
			game = 6;
			LT_sleep(1);
			LT_Fade_out();
		}
		if (LT_GET_VIDEO()) LT_Parallax();
		LT_Play_Music();
		LT_Update(1,16);
	}
}

void Load_Puzzle(){
	LT_Start_Loading(); 
	Scene = 2;
	if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","musi_ADL.vgm");
	if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","musi_TND.vgm");
	LT_Load_Map("MAPS.DAT","puz.tmx");
	if (LT_GET_VIDEO() == 1){
		LT_Load_Sprite("SPRITES.DAT","ball.bmp",8,Ball_Animation);
		LT_Load_Sprite("SPRITES.DAT","ball1.bmp",9,Enemy0_Animation);
		LT_Load_Tiles("TILESETS.DAT","puz_VGA.bmp");
	}
	if (LT_GET_VIDEO() == 0){
		LT_Load_Sprite("SPRITES.DAT","balle.bmp",8,Ball_Animation);
		LT_Load_Sprite("SPRITES.DAT","ball1e.bmp",9,Enemy0_Animation);
		LT_Load_Tiles("TILESETS.DAT","puz_EGA.bmp");
	}
	if (LT_GET_VIDEO() == 3){
		LT_Load_Sprite("SPRITES.DAT","ballt.bmp",8,Ball_Animation);
		LT_Load_Sprite("SPRITES.DAT","ball1t.bmp",9,Enemy0_Animation);
		LT_Load_Tiles("TILESETS.DAT","puz_EGA.bmp");
	}
	
	LT_End_Loading();

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
	LT_ENEMY_DIST_X = 13;
	LT_ENEMY_DIST_Y = 12;
	
	LT_Draw_Text_Box(1,1,36,3,0,0,0,"This is a puzzle game, the ball has symple physics, like bouncing and   falling slopes. PRESS ENTER");
	
	while(Scene == 2){
		//In mode 2, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		if (!dying){
			LT_move_player(8);

			//If collision tile = ?, end level
			if (LT_Player_State[SPR_TILC_NUM] == 11) {Scene = 1; game = 0;}
			if (LT_Keys[LT_ESC]) {Scene = 1; game = 0;} //esc exit
			
			{//Samples to edit tiles on the map
				int col_x = LT_Player_State[SPR_COL_X];
				int col_y = LT_Player_State[SPR_COL_Y];
				int tx = sprite[8].tile_x;
				int ty = sprite[8].tile_y;
				
				//If collision with breakable tiles
				switch (LT_Player_State[SPR_MOVE]){
					case 1: if (col_y == 5) LT_Edit_MapTile(tx,ty-1,0,0); break;
					case 2: if (col_y == 5) LT_Edit_MapTile(tx,ty+1,0,0); break;
					case 3: if (col_x == 5) LT_Edit_MapTile(tx-1,ty,0,0); break;
					case 4: if (col_x == 5) LT_Edit_MapTile(tx+1,ty,0,0); break;
				}
			
				//If on top of holes, animate tile 100 until it becomes 103
				if (LT_Player_State[SPR_TILP_NUM] > 99){
					if (Breaking_ground_Time == 16){
						if (LT_Player_State[SPR_TILP_NUM] < 103) LT_Sprite_Edit_Map(8,LT_Player_State[SPR_TILP_NUM]+1,0);
						Breaking_ground_Time = 0;
					}
					Breaking_ground_Time++;
				} else Breaking_ground_Time = 0;
			}
			if (LT_Player_State[SPR_TILP_NUM] == 103) dying = 1;
			if (LT_Player_Col_Enemy())dying = 1;
		} else {//Reset level
			LT_Set_Sprite_Animation(8,6);
			LT_Set_Sprite_Animation_Speed(8,8);
			if (sprite[8].frame == 7){
				Scene = 1; game = 8;
				LT_sleep(1);
				LT_Fade_out();
			}
		}

		if (LT_GET_VIDEO() == 1)LT_Cycle_palette(1,2);
		else LT_Cycle_Palette_TGA_EGA(trans_pal_offsets,trans_pal_cycle,2);
		LT_Play_Music();
		LT_Update(1,8);
	}
}

void Load_Shooter(){
	LT_Start_Loading(); 
		Scene = 2;
		if(LT_GET_MUSIC() == 1) LT_Load_Music("MUSIC.DAT","musi_ADL.vgm");
		if(LT_GET_MUSIC() == 0) LT_Load_Music("MUSIC.DAT","musi_TND.vgm");
		LT_Load_Map("MAPS.DAT","Shooter.tmx");
		if (LT_GET_VIDEO() == 0) {
			LT_Load_Sprite("SPRITES.DAT","shipe.bmp",16,Ship_Animation);
			LT_Load_Sprite("SPRITES.DAT","rocketbe.bmp",17,Rocket_Animation);
			LT_Load_Tiles("TILESETS.DAT","spa_EGA.bmp");
		}
		if (LT_GET_VIDEO() == 1) {
			LT_Load_Sprite("SPRITES.DAT","ship.bmp",16,Ship_Animation);
			LT_Load_Sprite("SPRITES.DAT","rocketb.bmp",17,Rocket_Animation);
			LT_Load_Tiles("TILESETS.DAT","spa_VGA.bmp");
		}
		if (LT_GET_VIDEO() == 3) {
			LT_Load_Sprite("SPRITES.DAT","shipt.bmp",16,Ship_Animation);
			LT_Load_Sprite("SPRITES.DAT","rocketbt.bmp",17,Rocket_Animation);
			LT_Load_Tiles("TILESETS.DAT","spa_EGA.bmp");
		}
	
	LT_End_Loading();
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
	LT_ENEMY_DIST_X = 20;
	LT_ENEMY_DIST_Y = 20;
	LT_EGA_TEXT_TRANSLUCENT = 1;
	//SIDE SCROLL
	//LT_Start_Music(70);
	LT_MODE = 3;
	
	LT_Draw_Text_Box(1,1,36,5,0,0,0,"Maybe this engine is not the best   for a side scroller shooter, becausesprites spawn very far from the     screen edge, to avoid the update    column. PRESS ENTER");
	
	while(Scene == 2){
		if (!dying){
			SCR_X++;
			LT_move_player(16);
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
				LT_sleep(1);
				LT_Fade_out();
			}
		}
		LT_Cycle_palette(1,2);
		LT_Play_Music();
		LT_Update(0,0);
	}
	LT_EGA_TEXT_TRANSLUCENT = 0;
}
