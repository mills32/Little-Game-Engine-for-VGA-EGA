/***********************
*  LITTLE GAME ENGINE  *
*	TEST
************************/
#include "lt__eng.h"

SPRITE sprite_player, *sprite_enemy;

int Scene = 0;

LT_Col LT_Player_Col;

void Load_Platform(){
	LT_Set_Loading_Interrupt(); 

	Scene = 2;
	load_map("GFX/mario.tmx");
	load_tiles("GFX/mariotil.bmp");
	sprite_enemy = farcalloc(10,sizeof(SPRITE));
	LT_Load_Sprite("GFX/s_mario.bmp",&sprite_player,16);
	LT_Load_Sprite("GFX/goomba.bmp",&sprite_enemy[0],16);
	LT_Clone_Sprite(&sprite_enemy[1],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[2],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[3],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[4],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[5],&sprite_enemy[0]);
	LT_Clone_Sprite(&sprite_enemy[6],&sprite_enemy[0]);
	LT_Load_Music("music/mario2.imf");
	
	LT_Delete_Loading_Interrupt();
	
	LT_Start_Music(70);
	
	LT_MODE = 1;//platform
}

void Run_Platform(){
	int n;
	startp:
	Scene = 2;
	sprite_player.pos_x = 16*10;
	sprite_player.pos_y = 0;
	
	//Place enemies manually on map in tile units (x16)
	//They will be drawn only if they are inside viewport.
	LT_Set_Enemy(&sprite_enemy[0],22,10,-1,0);
	LT_Set_Enemy(&sprite_enemy[1],32,14,1,0);
	LT_Set_Enemy(&sprite_enemy[2],53,14,-1,0);
	LT_Set_Enemy(&sprite_enemy[3],78,10,-1,0);
	LT_Set_Enemy(&sprite_enemy[4],103,14,1,0);
	LT_Set_Enemy(&sprite_enemy[5],171,10,-1,0);
	LT_Set_Enemy(&sprite_enemy[6],201,14,1,0);
	
	LT_Set_Map(0,0);
	
	while(Scene == 2){
		MCGA_Scroll(SCR_X,SCR_Y);
		LT_scroll_follow(&sprite_player);
		
		//Restore BKG
		for (n = 6; n != -1; n--) LT_Restore_Enemy_BKG(&sprite_enemy[n]);
		LT_Restore_Sprite_BKG(&sprite_player);
		
		//Draw sprites first to avoid garbage
		LT_Draw_Sprite(&sprite_player);
		for (n = 0; n != 7; n++)LT_Draw_Enemy(&sprite_enemy[n]);
		LT_gprint(LT_Player_Col.tile_number,268,164);
		
		//
		LT_scroll_map();
		
		//In this mode sprite is controlled using L R and Jump
		LT_Player_Col = LT_move_player(&sprite_player);
		
		if (LT_Keys[LT_RIGHT]) LT_Set_Sprite_Animation(&sprite_player,0,4,4);
		else if (LT_Keys[LT_LEFT]) LT_Set_Sprite_Animation(&sprite_player,6,4,4);
		else sprite_player.animate = 0;
		
		//if (sprite_player.ground == 0) sprite_player.frame = 4;
		if (LT_Keys[LT_ESC])  Scene = -1; //esc exit
		
		//Move the enemy
		for (n = 0; n != 7; n++) LT_Enemy_walker(&sprite_enemy[n],1);
		
		//Flip
		for (n = 0; n != 7; n++){
			if (sprite_enemy[n].speed_x > 0) LT_Set_Sprite_Animation(&sprite_enemy[n],2,2,5);
			if (sprite_enemy[n].speed_x < 0) LT_Set_Sprite_Animation(&sprite_enemy[n],0,2,5);	
		}
		
		//fall
		if (sprite_player.pos_y > 16<<4) { MCGA_Fade_out(); sprite_player.init = 0; goto startp;}
		
		//Found castle
		if (LT_Player_Col.tilecol_number == 11) {
			gotoxy(0,23);
			printf("BYE BYE");
			sleep(4);
			Scene = -1;
		}
	}
	
	LT_unload_sprite(&sprite_player); //manually free sprites
	farfree(sprite_enemy); sprite_enemy = NULL;
}


void main(){
	
	system("cls");
	LT_Init();
	
	LT_load_font("GFX/font.bmp");
	
	gotoxy(17,13);
	printf("LOADING");
	gotoxy(0,0);
	
	//You can use a custom loading animation for the Loading_Interrupt
	LT_Load_Animation("GFX/loading.bmp",&LT_Loading_Animation,32);
	LT_Set_Animation(&LT_Loading_Animation,0,4,6);
	
	Load_Platform();
	Run_Platform();
	
	LT_ExitDOS(); //frees map, tileset, font and music.
	
	return;
}