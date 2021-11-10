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

//Main program
LT_Sprite_State LT_Player_State;


int running = 1;
int x,y = 0;
int i,j = 0;
int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {108,116,124,132,140,148};
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

byte Ball_Animation[] = {0,0,0,0,0,0,0,0};
byte Rocket_Animation[] = {0,1,0,2,0,1,0,2};

//Custom functions at the end of this file
void Print_Info_Wait_ENTER(word x, word y, int w, int h, unsigned char* text);
void Display_Intro();
void Load_Test();
void Run_Test();
void Load_TopDown();
void Run_TopDown();
void Load_Platform();
void Run_Platform();
void Load_Platform1();
void Run_Platform1();
void Load_Puzzle();
void Run_Puzzle();
void Load_Shooter();
void Run_Shooter();

//Main function
void main(){
	
	LT_Setup();	//Allways the first, loads setup and initializes engine
	LT_Logo();	//If you want a logo
	LT_Load_Font("GFX/0_fontV.bmp"); //Load a font
	LT_Load_Animation("GFX/loading.bmp"); //Load a custom loading animation for Loading process
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
			case 11: Load_Test();Run_Test(); break;
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
		LT_WaitVsync_VGA();
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
	Scene = 1;
	menu_option = 0;
	SCR_X = 0;SCR_Y = 0;
	
	LT_SPRITE_MODE = 0;
	LT_IMAGE_MODE = 1;
	LT_ENDLESS_SIDESCROLL = 0;
	
	//Enable this to load stuff, it will fade out automatically
	//and show a cute animation
	LT_Set_Loading_Interrupt(); 

		LT_Load_Image("GFX/INTRVGA.bmp"); //Load a 320x240 bkg image
		LT_Load_Sprite("GFX/cursorb.bmp",8, Menu_Cursor_Animation); //Load sprites to one of the fixed structs
		LT_Load_Sprite("GFX/Rocketc.bmp",20,0);
	
		//Some music
		if (LT_MUSIC_MODE == 1) LT_Load_Music("music/ADLIB/letsg.imf",0);
		if (LT_MUSIC_MODE == 2) LT_Load_Music("music/ADLIB/letsgpcm.imf",1);
	
		// WARNING!!!
		// Load samples after loading images, if not, image temporary storage will overwritte them
		
		LT_Clear_Samples(); //Clear sound ram and populate ir with new samples
		sb_load_sample("MUSIC/samples/drum.wav"); //Sample 0 reserved for music
		sb_load_sample("MUSIC/samples/snare.wav"); //Sample 1 reserved for music
		sb_load_sample("MUSIC/samples/explode.wav"); //Sample 2 reserved for music
		
		//SFX samples
		sb_load_sample("MUSIC/samples/in_hole.wav"); 
		sb_load_sample("MUSIC/samples/eing.wav");
		sb_load_sample("MUSIC/samples/hit.wav");
		sb_load_sample("MUSIC/samples/byebye.wav");
		sb_load_sample("MUSIC/samples/touch.wav");
		sb_load_sample("MUSIC/samples/pop.wav");
	
	LT_Delete_Loading_Interrupt(); //End loading
	LT_Fade_in(); //Fade in because we loaded a background image
	
	LT_Set_Sprite_Animation(8,0);
	LT_Set_Sprite_Animation_Speed(8,6);
	LT_MODE = 0;
	
	//Init Sprite at positions
	LT_Init_Sprite(8,0,0);
	LT_Init_Sprite(20,0,0);	
	
	//This box won't talk
	LT_Text_Speak_End = 1;
	LT_Draw_Text_Box(13,12,12,7,0,"SELECT  DEMO             TOP DOWN    PLATFORM    PLATFORM 1  PUZZLE      SHOOTER   ");

	while (Scene == 1) {
		
		sprite[8].pos_x = 80 + (LT_COS[j]>>4);
		sprite[8].pos_y = transition+menu_pos[menu_option+1];
		
		sprite[20].pos_x = 230;
		sprite[20].pos_y = 140+ (LT_COS[j]>>3);
		
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
					if (LT_SFX_MODE == 1) LT_Play_PC_Speaker_SFX(Speaker_Menu);
					if (LT_SFX_MODE == 2) sb_play_sample(7,8000);
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
					if (LT_SFX_MODE == 1) LT_Play_PC_Speaker_SFX(Speaker_Menu);
					if (LT_SFX_MODE == 2) sb_play_sample(7,8000);
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
			if (LT_SFX_MODE == 1) LT_Play_PC_Speaker_SFX(Speaker_Select);
			if (LT_SFX_MODE == 2) sb_play_sample(5,8000);
		}
		if (LT_Keys[LT_JUMP]) {Scene = 0;game = 11;}
		if (LT_Keys[LT_ESC]) {Scene = 3;game = 255; running = 0;}
		LT_Play_Music();
		LT_Update(0, 0);
	}
	LT_Fade_out();

	LT_Text_Speak_End = 0;
}

void Load_Test(){
	LT_Set_Loading_Interrupt(); 
		LT_Load_Map("gfx/lisa.tmx");
		LT_Load_Tiles("GFX/lis_VGA.bmp");
		LT_Load_Sprite("GFX/ship.bmp",16,Ship_Animation);
		LT_Clone_Sprite(17,16);
	LT_Delete_Loading_Interrupt();

	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(16,160,120);
	LT_Init_Sprite(17,100,120);
	
	LT_Set_Map(0); //This will fade in automatically
	
	LT_MODE = 0;
	Scene = 0;
	i = 0;
	LT_Set_Sprite_Animation(16,0);
	LT_Set_Sprite_Animation_Speed(16,32);
	LT_Set_Sprite_Animation(17,0);
	LT_Set_Sprite_Animation_Speed(17,32);
}

void Run_Test(){
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	int A = 0;
	LT_ResetWindow();
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;

	while(Scene == 0){
		if (sprite[16].pos_x-160 < LT_SIN[0]) sprite[16].pos_x++;
		else Scene = 1;
		if (sprite[16].pos_y-120 < LT_COS[0]) sprite[16].pos_y++;
		sprite[17].pos_x = sprite[16].pos_x - 64;
		sprite[17].pos_y = sprite[16].pos_y;
		
		LT_Update(1,16);	
	}
	
	while(Scene == 1){
		
		sprite[16].pos_x = LT_SIN[i & 255] + 160;
		sprite[16].pos_y = LT_COS[i & 255] + 120;
		sprite[17].pos_x = sprite[16].pos_x - 64;
		sprite[17].pos_y = sprite[16].pos_y;
		
		i++;
		A++;
		
		if (LT_Keys[LT_ESC]) {Scene = 2;game = 0;}
		if (LT_Keys[LT_JUMP]) LT_Play_PC_Speaker_SFX(Speaker_Jump);
		if (LT_Keys[LT_ACTION]) LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
		
		//scroll_map and draw borders
		LT_Update(1,16);
	}
	Clearkb();
	Scene = 2;game = 0;
}

void Load_TopDown(){

	LT_Set_Loading_Interrupt(); 
	
	LT_Load_Map("GFX/Topdown.tmx");
	LT_Load_Tiles("gfx/top_VGA.bmp");

	LT_Load_Sprite("GFX/player.bmp",8,Player_Animation);
	LT_Load_Sprite("GFX/enemy2.bmp",9,Enemy0_Animation);
	LT_Load_Sprite("GFX/enemy3.bmp",12,Enemy1_Animation);

	LT_Load_Music("music/adlib/save.imf",1);
	
	
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/in_hole.wav");
	sb_load_sample("MUSIC/samples/boing.wav");
	sb_load_sample("MUSIC/samples/splash.wav");

	LT_Delete_Loading_Interrupt();
	LT_SetWindow("GFX/win_VGA.bmp");
	
	LT_MODE = 0;
	
	Scene = 2;
}

void Run_TopDown(){
	int n;
	int dying = 0;
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,8*16,4*16);
	LT_Set_AI_Sprites(9,12,0,1);
	LT_Set_Sprite_Animation_Speed(8,1);
	LT_Set_Map(0);//unused y parameter
	
	Scene = 2;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	
	Print_Info_Wait_ENTER(1,2,36,3,"A Top down style level, You can use it to create very symple adventure  games. PRESS ENTER.");
	
	while(Scene == 2){
		
		//LT_Print_Window_Variable(32,LT_Player_Col.tile_number);
		LT_Print_Window_Variable(32,LT_Sprite_Stack/*cards>>1*/);
		
		LT_Cycle_palette(1,4);
		
		if (!dying){
			//In this mode sprite is controlled using U D L R
			LT_Player_State = LT_move_player(8);
			//Player animations run inside above function
		}
		
		//If collision tile = ?, end level
		if ((LT_Player_State.tilecol_number == 11) || (LT_Keys[LT_ESC])){
			Scene = 1; game = 0;
		}
		
		//if ((SCR_X>>1)&1)outportb(0x3d8, 1);
		//else outportb(0x3d8, 9);
		if (LT_Player_Col_Enemy()) dying = 1;
		
		if (dying) {
			LT_Set_Sprite_Animation(8,6);
			LT_Set_Sprite_Animation_Speed(8,8);
			if (sprite[8].frame == 23){
				sb_play_sample(2,11025);
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
	
	Scene = 2;
	LT_Load_Map("GFX/Platform.tmx");
	LT_Load_Tiles("GFX/Pla_VGA.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",8,Player_Animation);
	LT_Set_Sprite_Animation_Speed(8,1);
	LT_Load_Sprite("GFX/enemy.bmp",9,Enemy2_Animation);
	if (LT_MUSIC_MODE == 2) LT_Load_Music("music/ADLIB/facespcm.imf",1);
	if (LT_MUSIC_MODE == 1) LT_Load_Music("music/ADLIB/faces.imf",0);
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/in_hole.wav");
	sb_load_sample("MUSIC/samples/boing.wav");
	sb_load_sample("MUSIC/samples/splash.wav");
	sb_load_sample("MUSIC/samples/Door.wav");
	LT_Delete_Loading_Interrupt();
	
	LT_SetWindow("GFX/win_VGA.bmp");
	Level_cards = 0;
}

void Run_Platform(){
	extern unsigned char Enemies;
	
	int Speaker_Crash[16] = {69,3,120,32,39,200,20,60,16,106,12,87,8,70,32,60};
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	int n;
	byte last_cards = 0;
	int dying = 0;
	Scene = 2;
	
	//Init_Player
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,5*16,0);
	LT_Set_AI_Sprites(9,9,1,1);
	
	LT_Set_Map(0);

	LT_Print_Window_Variable(32,Level_cards);

	LT_MODE = 1;
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;

	Print_Info_Wait_ENTER(1,1,36,4,"Just A Platform game test, I createdthe engine for this kind of game.   Collect items and reach the end.    PRESS ENTER.");
	
	while (Scene == 2){
		//EDIT MAP: GET cards
		switch (LT_Player_State.tile_number){
			case 123: LT_Get_Item(8, 1, 0); Level_cards++; LT_Print_Window_Variable(32,Level_cards>>1);break;
			case 124: LT_Get_Item(8, 22, 0); Level_cards++; LT_Print_Window_Variable(32,Level_cards>>1);break;
			case 125: LT_Get_Item(8, 0, 0); Level_cards++; LT_Print_Window_Variable(32,Level_cards>>1);break;
			case 126: LT_Get_Item(8, 109, 0); Level_cards++; LT_Print_Window_Variable(32,Level_cards>>1);break;
		}
		if (Level_cards != last_cards){
			if (LT_SFX_MODE == 1)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
			if (LT_SFX_MODE == 2)sb_play_sample(3,11025);
		}
		last_cards = Level_cards;
		
		if (!dying){
			//In this mode sprite is controlled using L R and Jump
			LT_Player_State = LT_move_player(8);
			//set player animations
		}
		
		if (LT_Player_State.jump){
			if (LT_SFX_MODE == 1)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 2)sb_play_sample(4,11025);
		}
		
		//If collision with breakable tile (door in this case)
		if ((LT_Player_State.col_x == 5)){
			if (Level_cards < 20) Print_Info_Wait_ENTER(16,10,18,2,"YOU NEED 10 CARDS TO OPEN DOOR. PRESS ENTER.");
			else { //Open door
				if (LT_SFX_MODE == 1)LT_Play_PC_Speaker_SFX(Speaker_Get_Item);
				if (LT_SFX_MODE == 2)sb_play_sample(6,11025);
				LT_Edit_MapTile_VGA((sprite[8].pos_x+16)>>4,(sprite[8].pos_y)>>4,86,0);
				sleep(1);
			}
		}
		//If collision tile = ?, end level
		if ((LT_Player_State.tilecol_number == 11)||(LT_Keys[LT_ESC])){Scene = 1; game = 0;}

		//water palette animation
		LT_Cycle_palette(1,2);
		
		
		//if water or enemy, reset level
		if (LT_Player_State.tile_number == 102){
			if (LT_SFX_MODE == 1){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 2)sb_play_sample(5,11025);
			Scene = 1;
			game = 4;
			sleep(1);
			LT_Fade_out();
		}
		if (LT_Player_Col_Enemy()){dying = 1;/*LT_Set_Sprite_Animation(16,0);*/}
		
		if (dying){
			if (LT_SFX_MODE == 1){LT_Play_PC_Speaker_SFX(Speaker_Crash);}
			if (LT_SFX_MODE == 2)sb_play_sample(2,11025);
			Scene = 1;
			game = 4;
			sleep(1);
			LT_Fade_out();			
		}
		
		LT_Play_Music();
		LT_Update(1,8);
	}
	Clearkb();
}

void Load_Platform1(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/pre2.tmx");
	LT_Load_Tiles("GFX/pre2.bmp");
	
	//LT_Load_Sprite("GFX/player.bmp",16);
	LT_Load_Sprite("GFX/pre2s.bmp",16,Player_Animation_2);
	LT_Set_Sprite_Animation_Speed(16,4);
	LT_Load_Sprite("GFX/pre2e.bmp",17,Player_Animation_2);
	LT_Set_Sprite_Animation_Speed(17,8);
	
	if (LT_MUSIC_MODE == 2) LT_Load_Music("music/ADLIB/facespcm.imf",1);
	if (LT_MUSIC_MODE == 1) LT_Load_Music("music/ADLIB/faces.imf",0);
	LT_Clear_Samples();
	sb_load_sample("MUSIC/samples/drum.wav");
	sb_load_sample("MUSIC/samples/snare.wav");
	sb_load_sample("MUSIC/samples/explode.wav");
	sb_load_sample("MUSIC/samples/predie.wav");
	sb_load_sample("MUSIC/samples/boing.wav");
	sb_load_sample("MUSIC/samples/splash.wav");
	LT_Delete_Loading_Interrupt();
	LT_SetWindow("GFX/win_VGA.bmp");
}

void Run_Platform1(){
	extern unsigned char Enemies;
	
	int Speaker_Crash[16] = {69,3,120,32,39,200,20,60,16,106,12,87,8,70,32,60};
	int Speaker_Jump[16] = {30,35,40,43,44,45,46,47,48,49,50,51,52,53,54,55};
	int Speaker_Get_Item[16] = {69,70,71,72,69,64,58,40,30,12,12,16,20,70,32,60};
	int n;
	byte cards = 0;
	byte last_cards = 0;
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
			if (LT_SFX_MODE == 1)LT_Play_PC_Speaker_SFX(Speaker_Jump);
			if (LT_SFX_MODE == 2)sb_play_sample(4,11025);
		}
		
		if (LT_Player_Col_Enemy()) {
			if (LT_SFX_MODE == 1)LT_Play_PC_Speaker_SFX(Speaker_Crash);
			if (LT_SFX_MODE == 2)sb_play_sample(3,11025);
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
		LT_Parallax();
		LT_Play_Music();
		LT_Update(1,16);
	}
	Clearkb();
}

void Load_Puzzle(){
	
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/puz.tmx");
	LT_Load_Tiles("GFX/puz_VGA.bmp");
	LT_Load_Sprite("GFX/ball.bmp",8,Ball_Animation);
	LT_Load_Sprite("GFX/ball1.bmp",9,Enemy0_Animation);
	LT_Load_Music("music/ADLIB/puzzle.imf",0);
	
	LT_Delete_Loading_Interrupt();

	LT_SetWindow("GFX/win_VGA.bmp");

	LT_MODE = 2; //Physics mode
}

void Run_Puzzle(){

	Scene = 2;
	
	LT_Reset_Sprite_Stack();
	LT_Init_Sprite(8,4*16,4*16);
	LT_Set_AI_Sprites(9,9,0,0);
	LT_Set_Map(0);
	
	LT_SPRITE_MODE = 1;
	LT_IMAGE_MODE = 0;
	LT_ENDLESS_SIDESCROLL = 0;
	
	sprite[8].speed_x = 8*32;
	sprite[8].speed_y = 8*32;
	
	Print_Info_Wait_ENTER(1,1,36,3,"This is a puzzle game, the ball has symple physics, like bouncing and   falling slopes. PRESS ENTER");
	
	while(Scene == 2){
		//In mode 2, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		LT_Player_State = LT_move_player(8);

		//If collision tile = ?, end level
		if (LT_Player_State.tilecol_number == 11) {Scene = 1; game = 0;}
		if (LT_Keys[LT_ESC]) {Scene = 1; game = 0;} //esc exit
		LT_Cycle_palette(1,2);
		
		LT_Play_Music();
		LT_Update(1,8);
	}
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	LT_Load_Map("GFX/Shooter.tmx");
	LT_Load_Tiles("gfx/spa_VGA.bmp");
	LT_Reset_Sprite_Stack();
	LT_Load_Sprite("GFX/ship.bmp",16,Ship_Animation);
	LT_Load_Sprite("GFX/rocketb.bmp",17,Rocket_Animation);
	
	LT_Load_Music("music/ADLIB/shooter.imf",0);
	
	LT_Delete_Loading_Interrupt();

	LT_SetWindow("GFX/win_VGA.bmp");
}

void Run_Shooter(){
	int timer;
	int dying = 0;
	start:
	timer = 0;
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
			LT_Print_Window_Variable(32,SCR_X>>4);
		
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

