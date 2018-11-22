/***********************
*  LITTLE GAME ENGINE  *
************************/

/*##########################################################################
	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program.                     
	Remember to compile in the LARGE memory model!                        
	
	All code is 8086 / 8088 compatible
	
	Please feel free to copy this source code.                            
	
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

SPRITE font, pointer, sprite_cursor, sprite_player, sprite_ball, *sprite_enemy, sprite_ship;
COLORCYCLE cycle_water;

int i = 0;
int j = 0;

LT_Col LT_Player_Col;

int Scene = 0;
int menu_option = 0;
int menu_pos[10] = {7.3*16,2*16,7.3*16,3*16,7.3*16,4*16,7.3*16,5*16,7.3*16,6*16};
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
		if (LT_Keys[1]) {SCR_Y= 200; goto exit;}//esc exit
	}
	sleep(1);
	exit:
}

void Load_Menu(){
	LT_Set_Loading_Interrupt(); 
	
	Load_map("GFX/menu.tmx");
	load_tiles("GFX/menutil.bmp");
	LT_Load_Sprite("GFX/cursor.bmp",&sprite_cursor,16);
	//LT_Load_Music("music/menu2.imf");
	//LT_Start_Music(70);
	LT_LoadMOD("MUSIC/MOD/dyna.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	PlayMOD(0);
	
	MCGA_SplitScreen(63); 
	MCGA_Scroll(SCR_X,144); //Scroll tittle
	LT_Set_Map(0,0);
	
	i = 0;
	LT_MODE = 0;
}

void Run_Menu(){
	int delay = 0; //wait between key press
	Scene = 1;
	game = 0;
	menu_option = 0;
	LT_Set_Sprite_Animation(&sprite_cursor,0,8,6);
	while(Scene == 1){
		MCGA_Scroll(SCR_X,144); //Scroll tittle
		SCR_X = LT_SIN[i]>>1;
		
		sprite_cursor.pos_x = menu_pos[menu_option] + (LT_COS[j]>>3);
		sprite_cursor.pos_y = menu_pos[menu_option+1];
		
		LT_Restore_Sprite_BKG(&sprite_cursor);
		LT_Draw_Sprite(&sprite_cursor);
		
		if (i > 360) i = 0;
		i+=2;
		if (j > 356) {j = 0; sprite_cursor.anim_counter = 0;}
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
	sprite_enemy = farcalloc(10,sizeof(SPRITE));
	LT_Load_Sprite("GFX/enemy2.bmp",&sprite_enemy[0],16);
	LT_Clone_Sprite(&sprite_enemy[1],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[2],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[3],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[4],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[5],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[6],&sprite_enemy[0]);
	
	LT_Load_Music("music/adlib/top_down.imf");
	//LoadMOD("MUSIC/MOD/Beach.mod"); 
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	//PlayMOD(1);
	
	//animate colours
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_Set_Map(0,0);
	LT_MODE = 0;
}

void Run_TopDown(){
	int n;
	Scene = 2;
	
	sprite_player.pos_x = 8*16;
	sprite_player.pos_y = 4*16;
	
	//Place enemies manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	LT_Set_Enemy(&sprite_enemy[0],13,7,1,0);
	LT_Set_Enemy(&sprite_enemy[1],20,17,0,-1);
	LT_Set_Enemy(&sprite_enemy[2],33,18,0,1);
	LT_Set_Enemy(&sprite_enemy[3],39,9,-1,0);
	LT_Set_Enemy(&sprite_enemy[4],70,16,0,1);
	LT_Set_Enemy(&sprite_enemy[5],54,4,1,0);
	LT_Set_Enemy(&sprite_enemy[6],36,25,0,-1);
	
	for (n = 0; n != 7; n++) LT_Set_Sprite_Animation(&sprite_enemy[n],0,2,16);
	
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_follow(&sprite_player);
		
		//Restore BKG
		for (n = 6; n != -1; n--) LT_Restore_Enemy_BKG(&sprite_enemy[n]);
		LT_Restore_Sprite_BKG(&sprite_player);
		
		//Draw sprites first to avoid garbage
		LT_Draw_Sprite(&sprite_player);
		for (n = 0; n != 7; n++)LT_Draw_Enemy(&sprite_enemy[n]);
		
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		LT_gprint(LT_Player_Col.tile_number,240,160);
		
		cycle_palette(&cycle_water,4);
		
		//In this mode sprite is controlled using U D L R
		LT_Player_Col = LT_move_player(&sprite_player);
		
		//Player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,6,4);
		else if (LT_Keys[LT_UP]) LT_Set_Sprite_Animation(&sprite_player,12,6,4);
		else if (LT_Keys[LT_DOWN]) LT_Set_Sprite_Animation(&sprite_player,18,6,4);
		else sprite_player.animate = 0;
		
		//Move the enemy
		for (n = 0; n != 7; n++) LT_Enemy_walker(&sprite_enemy[n],0);
		
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
	}
	//StopMOD(1);
	LT_unload_sprite(&sprite_player); //manually free sprites
	farfree(sprite_enemy); sprite_enemy = NULL; 
}

void Load_Platform(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/Platform.tmx");
	load_tiles("GFX/Platil.bmp");
	//LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/player.bmp",&sprite_player,16);
	sprite_enemy = farcalloc(10,sizeof(SPRITE));
	LT_Load_Sprite("GFX/enemy.bmp",&sprite_enemy[0],16);
	LT_Clone_Sprite(&sprite_enemy[1],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[2],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[3],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[4],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[5],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[6],&sprite_enemy[0]);
	LT_Load_Music("music/Adlib/platform.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	//animate water
	cycle_init(&cycle_water,palette_cycle_water);
	
	LT_MODE = 1;
}

void Run_Platform(){
	int n;
	startp:
	LT_Set_Map(0,0);
	sprite_player.pos_x = 4*16;
	sprite_player.pos_y = 4*16;
	
	//Place enemies manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	LT_Set_Enemy(&sprite_enemy[0],17,20,1,0);
	LT_Set_Enemy(&sprite_enemy[1],61,19,-1,0);
	LT_Set_Enemy(&sprite_enemy[2],61,13,-1,0);
	LT_Set_Enemy(&sprite_enemy[3],92,15,1,0);
	LT_Set_Enemy(&sprite_enemy[4],117,18,-1,0);
	LT_Set_Enemy(&sprite_enemy[5],163,20,1,0);
	LT_Set_Enemy(&sprite_enemy[6],192,12,-1,0);
	
	Scene = 2;
	
	while(Scene == 2){
		//SCR_X and SCR_Y are global variables predefined 
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_follow(&sprite_player);
		
		//Restore BKG
		for (n = 6; n != -1; n--) LT_Restore_Enemy_BKG(&sprite_enemy[n]);
		LT_Restore_Sprite_BKG(&sprite_player);
		
		//Draw sprites first to avoid garbage
		LT_Draw_Sprite(&sprite_player);
		for (n = 0; n != 7; n++)LT_Draw_Enemy(&sprite_enemy[n]);
		
		//scroll_map and draw borders
		LT_scroll_map();
		
		//In this mode sprite is controlled using L R and Jump
		LT_Player_Col = LT_move_player(&sprite_player);
		
		//set player animations
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,6,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,6,4);
		else sprite_player.animate = 0;
		
		//Move the enemy
		for (n = 0; n != 7; n++) LT_Enemy_walker(&sprite_enemy[n],1);
		
		//Flip
		for (n = 0; n != 7; n++){
			if (sprite_enemy[n].speed_x > 0) LT_Set_Sprite_Animation(&sprite_enemy[n],0,6,5);
			if (sprite_enemy[n].speed_x < 0) LT_Set_Sprite_Animation(&sprite_enemy[n],6,6,5);	
		}
		
		//if water, reset level
		if (LT_Player_Col.tile_number == 182) {
			MCGA_Fade_out(); 
			sprite_player.init = 0;
			goto startp;
		}
		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;

		//water palette animation
		cycle_palette(&cycle_water,2);

		//esc go to menu
		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
		
	}
	LT_unload_sprite(&sprite_player); //manually free sprites
	farfree(sprite_enemy); sprite_enemy = NULL; 
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
	
	sprite_player.pos_x = 4*16;
	sprite_player.pos_y = 3*16;
	
	//To simulate floats
	sprite_player.fpos_x = sprite_player.pos_x;
	sprite_player.fpos_y = sprite_player.pos_y;
	sprite_player.speed_x = 0;
	sprite_player.speed_y = 0;
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		MCGA_Scroll(SCR_X,SCR_Y);
		
		LT_Restore_Sprite_BKG(&pointer);
		LT_Restore_Sprite_BKG(&sprite_player);
		
		LT_scroll_follow(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		if (sprite_player.state == 0)LT_Draw_Sprite(&pointer);
		LT_gprint(LT_Player_Col.tilecol_number,240,160);
		//scroll_map update off screen tiles
		LT_scroll_map();
		
		//In mode 3, sprite is controlled using the speed.
		//Also there are physics using the collision tiles
		LT_Player_Col = LT_move_player(&sprite_player);
		
		if(rotate == 360) rotate = 0;
		if(rotate < 0) rotate = 360;
		pointer.pos_x = sprite_player.pos_x + 4 + (LT_COS[rotate]>>2);
		pointer.pos_y = sprite_player.pos_y + 4 + (LT_SIN[rotate]>>2);
		
		//ROTATE 
		if (LT_Keys[LT_RIGHT]) rotate++;
		if (LT_Keys[LT_LEFT]) rotate--;
		
		//HIT BALL
		if ((LT_Keys[LT_ACTION]) && (sprite_player.state == 0)){
			LT_Delete_Sprite(&pointer);
			sprite_player.state = 1; //Move
			//All this trouble to simulate floats :(
			sprite_player.speed_x = (LT_COS[rotate]/16)*80;
			sprite_player.speed_y = (LT_SIN[rotate]/16)*80;
		}

		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 11) Scene = 1;
		
		cycle_palette(&cycle_water,2);

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
	}
	
	LT_Unload_Sprite(&pointer);
	LT_unload_sprite(&sprite_player); //manually free sprites
}

void Load_Puzzle2(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/arkanoid.tmx");
	load_tiles("GFX/atiles.bmp");
	LT_load_font("GFX/font.bmp");
	
	LT_Load_Sprite("GFX/bar.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/aball.bmp",&sprite_ball,8);
	LT_Load_Music("music/ADLIB/top_down.imf");
	
	LT_Delete_Loading_Interrupt();
	LT_Start_Music(70);
	
	LT_Set_Map(0,0);
	LT_MODE = 0; 
}

void Run_Puzzle2(){
	int score = 0;
	Scene = 2;
	
	sprite_player.pos_x = 10*16;
	sprite_player.pos_y = 9*16;
	sprite_ball.pos_x = 10*16;
	sprite_ball.pos_y = 10*16;
	//To simulate floats
	sprite_ball.speed_x = 1;
	sprite_ball.speed_y = -1;
	while(Scene == 2){
		//SCR_X and SCR_Y are predefined global variables 
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(&sprite_ball);
		LT_Restore_Sprite_BKG(&sprite_player);
		LT_Draw_Sprite(&sprite_player);
		LT_Draw_Sprite(&sprite_ball);
		LT_gprint(score,240,160);

		//simple physics to bounce a ball
		LT_Player_Col = LT_Bounce_Ball(&sprite_ball);
		if (LT_Player_Col.col_x == 5) score++;
		if (LT_Player_Col.col_y == 5) score++;
		
		//ROTATE 
		if (LT_Keys[LT_RIGHT]) sprite_player.pos_x++;
		if (LT_Keys[LT_LEFT]) sprite_player.pos_x--;
		
		//HIT BALL
		if ((LT_Keys[LT_ACTION]) && (sprite_player.state == 0)){
			LT_Delete_Sprite(&pointer);
			sprite_ball.state = 1; //Move
			//All this trouble to simulate floats :(
			sprite_ball.speed_x = 1;
			sprite_ball.speed_y = -1;
		}

		//If collision tile = ?, end level
		if (LT_Player_Col.tilecol_number == 10) Scene = 1;

		if (LT_Keys[LT_ESC]) Scene = 1; //esc exit
	}
	LT_Unload_Sprite(&sprite_ball);
	LT_unload_sprite(&sprite_player); //manually free sprites
}

void Load_Shooter(){
	LT_Set_Loading_Interrupt(); 
	
	Scene = 2;
	load_map("GFX/Shooter.tmx");
	load_tiles("GFX/stiles.bmp");
	
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
		LT_Restore_Sprite_BKG(&sprite_player);
		LT_Restore_Sprite_BKG(&sprite_ship);
		LT_Draw_Sprite(&sprite_ship);
		LT_Draw_Sprite(&sprite_player);
		sprite_player.pos_x++;
		if (sprite_player.pos_x == sprite_ship.pos_x+2) Scene = 1; //esc exit
	}
	
	LT_Set_Sprite_Animation(&sprite_ship,1,4,12);
	while (Scene == 1){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(&sprite_ship);
		LT_Draw_Sprite(&sprite_ship);
		if (sprite_ship.frame == 4) Scene = 2; //esc exit
	}
	
	sleep(0.5);	
	sprite_ship.animate = 0;
	while (Scene == 2){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(&sprite_ship);
		LT_scroll_follow(&sprite_ship);
		LT_Draw_Sprite(&sprite_ship);
		
		sprite_ship.pos_y--;
		if (SCR_Y == 0) Scene = 3; 
		
		LT_scroll_map();
	}
	LT_Set_Sprite_Animation(&sprite_ship,4,4,5);
	while (Scene == 3){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(&sprite_ship);
		LT_Draw_Sprite(&sprite_ship);

		timer++;
		if (timer == 100) Scene = 4; 
	}
	
	LT_Start_Music(70);
	LT_MODE = 3; //SIDE SCROLL
	LT_Set_Sprite_Animation(&sprite_ship,8,4,4);
	while(Scene == 4){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_Restore_Sprite_BKG(&sprite_ship);
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
	LT_Init_GUS(12);
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
		Load_Puzzle2();
		Run_Puzzle2();
		goto menu;
	}
	if (game == 4){
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
	