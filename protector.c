/**
 * 
 * LUDUM DARE 46 - PROTECTOR
 * By Khopa
 * 
 * Using Shiru's NESLIB and Famitone2
 * 
 * Diclaimer :
 * - Code is messy, due to time limit and platform constraints.
 * - I'm relying on multiline macros instead of functions whenever i can, to save CPU cycle on function calls.
 * - Some static variable are shared to avoid defining too many of them.
 * 
 */

// NES Library
#include "neslib.h"

// Nametables data
#include "title.h"
#include "scenario.h"
#include "instructions.h"
#include "gameover.h"
#include "map1.h"

// Constants definition
#define PLAYER_ONE 0x00

// Game states
#define ST_MENU 0x00
#define ST_GAME 0x01
#define ST_INSTRUCTIONS 0x02
#define ST_SCOREBOARD 0x03
#define ST_SCENARIO 0x04

// Screen Size
#define SCREEN_X_SIZE 256
#define SCREEN_Y_SIZE 240
#define TILE_SIZE 8
#define TILE_SIZE_DOUBLE 16
#define TILE_SIZE_HALF 4

#define TOP 8*TILE_SIZE
#define BOT 27*TILE_SIZE
#define LEF 1*TILE_SIZE
#define RIG 30*TILE_SIZE

#define NPC_TOP 12*TILE_SIZE
#define NPC_BOT 22*TILE_SIZE
#define NPC_LEF 6*TILE_SIZE
#define NPC_RIG 25*TILE_SIZE

#define BULLET_SPEED 3
#define ENEMY_MAX 10
#define BULLET_MAX 5
#define BLOOD_MAX 9
#define SCENARIO_LENGTH 8

// Direction const
#define DIR_UP 0
#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_DOWN 3

// Palette for sprites
#define PLAYER_BULLET_PAL 0
#define BLOOD_PAL 3
#define BLOOD_EFFECT_DURATION 12

// Sprites addresses in CHR bank
#define SPRITE_EMPTY 0x00
#define SPRITE_MUSIC_ON 0x4D
#define SPRITE_MUSIC_OFF 0x5D
#define SPRITE_PLAYER_UP 0x48
#define SPRITE_PLAYER_DOWN 0x46
#define SPRITE_PLAYER_RIGHT 0x47
#define SPRITE_NPC_UP 0x6B
#define SPRITE_NPC_DOWN 0x69
#define SPRITE_NPC_RIGHT 0x6A
#define SPRITE_GAUGE_0_FILLED 0x7C
#define SPRITE_GAUGE_1_FILLED 0x7D
#define SPRITE_GAUGE_2_FILLED 0x7E
#define SPRITE_GAUGE_0_EMPTY 0x6C
#define SPRITE_GAUGE_1_EMPTY 0x6D
#define SPRITE_GAUGE_2_EMPTY 0x6E
#define SPRITE_BLOOD_EFFECT_0 0x43
#define SPRITE_BLOOD_EFFECT_1 0x44
#define SPRITE_BLOOD_EFFECT_2 0x53
#define SPRITE_BLOOD_EFFECT_3 0x54

// ZEROPAGE variables (frequently accessed)
// Note to self : Do not try to initialize zeropage variables.
#pragma bssseg(push,"ZEROPAGE")
static unsigned char PLAYER_X_POS;
static unsigned char PLAYER_Y_POS;
static unsigned char PLAYER_HP;
static unsigned int SCORE;
static unsigned char BULLET_X[BULLET_MAX];
static unsigned char BULLET_Y[BULLET_MAX];
#pragma bssseg(pop)

// Game State
static unsigned int STATE;

// PAD Management
static unsigned char pad;

// PLAYER OBJECT
static unsigned char   PLAYER_DIRECTION = DIR_DOWN;
static unsigned char   PLAYER_FLAGS = 0;
static unsigned char   PLAYER_PALETTE = 0;

// NPC OBJECT
static unsigned char   NPC_X_POS = 0;
static unsigned char   NPC_Y_POS = 0;
static unsigned char   NPC_HP;
static unsigned char   NPC_DIRECTION = DIR_DOWN;
static unsigned char   NPC_FLAGS = 0;
static unsigned char   NPC_PALETTE = 1;

// PLAYER BULLETS
static char BULLET_DX[BULLET_MAX];
static char BULLET_DY[BULLET_MAX];
static unsigned char BULLET_ALIVE[BULLET_MAX];

// BLOOD EFFECTS
static unsigned char BLOOD_X[BLOOD_MAX];
static unsigned char BLOOD_Y[BLOOD_MAX];
static char BLOOD_DX[BLOOD_MAX];
static char BLOOD_DY[BLOOD_MAX];
static unsigned char BLOOD_SPRITE[BLOOD_MAX];
static unsigned char BLOOD_LIVE[BULLET_MAX];

// ENEMY OBJECTS
static int ENEMY_X[ENEMY_MAX];
static int ENEMY_Y[ENEMY_MAX];
static int ENEMY_DX[ENEMY_MAX];
static int ENEMY_DY[ENEMY_MAX];
static unsigned char ENEMY_HP[ENEMY_MAX];
static unsigned char MUSIC_ON = 1;
static unsigned char PAUSED = 0;

// 'PRESS START' vram update buffer
static unsigned char uListPressStart[17] ={
	MSB(NTADR_A(10,14))|NT_UPD_HORZ,LSB(NTADR_A(10,14)),13,0,0,0,0,0,0,0,0,0,0,0,0,0,NT_UPD_EOF
};

// 'Scenario TEXT' vram update buffer
static unsigned char uScenarioText[31] ={
	MSB(NTADR_A(2,25))|NT_UPD_HORZ,LSB(NTADR_A(2,25)),27,
	0,0,0,0,0,
	0,0,0,0,0,
	0,0,0,0,0,
	0,0,0,0,0,
	0,0,0,0,0,0,0,
	NT_UPD_EOF
};

// Game Over 'HUD' vram update buffer
static unsigned char uListGameOver[14] ={
	MSB(NTADR_A(11,14))|NT_UPD_HORZ,LSB(NTADR_A(11,14)),10,  // AT BG position 11,14, UPDATE 10 TILES HORIZONTALLY
	0x33,0x23,0x2F,0x32,0x25,0x1A,  // 'S' 'C' 'O' 'R' 'E' ':'
	0x00,0x10,0x10,0x10,  // '0' '0' '0'
	NT_UPD_EOF
};

// Game 'HUD' vram update buffer
static unsigned char uListHUD[28] ={
	MSB(NTADR_A(2,2))|NT_UPD_HORZ,LSB(NTADR_A(2,2)),24,  // AT BG position 2,2, UPDATE 24 TILES HORIZONTALLY
	SPRITE_PLAYER_DOWN,SPRITE_EMPTY,SPRITE_GAUGE_0_FILLED,SPRITE_GAUGE_1_FILLED,SPRITE_GAUGE_2_FILLED,SPRITE_EMPTY,        // PLAYER HEAD (46), 3 TILE GAUGE
	SPRITE_NPC_DOWN,SPRITE_EMPTY,SPRITE_GAUGE_0_FILLED,SPRITE_GAUGE_1_FILLED,SPRITE_GAUGE_2_FILLED,SPRITE_EMPTY,        // PLAYER HEAD (46), 3 TILE GAUGE
	0x33,0x23,0x2F,0x32,0x25,0x1A,  // 'S' 'C' 'O' 'R' 'E' ':'
	SPRITE_EMPTY,0x10,0x10,0x10,SPRITE_EMPTY,SPRITE_MUSIC_ON,  // '0' '0' '0'  ' ' 'MUSIC_SPRITE'
	NT_UPD_EOF
};

// PALETTES
const unsigned char bgPalette[16]={ 0x0f,0x1b,0x2c,0x0c,0x0f,0x2a,0x1a,0x09,0x0f,0x2d,0x36,0x30,0x0f,0x30,0x37,0x07 };
const unsigned char spritePalette[16]={ 0x0f,0x20,0x36,0x07,0x0f,0x30,0x36,0x26,0x0f,0x28,0x36,0x15,0x0f,0x26,0x15,0x06 };

const static char *sentences[] = {
	"   THESE ARE DARK TIMES   ",
	"THE WORLD IS UNDER ATTACK ",
	"AN INVISIBLE ENEMY IS HERE",
	"   THAT YOU ONLY CAN SEE  ",
	"       THE VIRUS...       ",
	"    PROTECT THE NURSES    ",
	" WITH YOUR ANTIVIRUS GUN  ",
	"  YOU ARE OUR ONLY HOPE   ",
	"  GO AND ERADICATE THEM   "
};

/**
 * Put a String in the update buffer
 * str : string to put
 */
void put_str_ubuffer(unsigned char buffer[],const char *str, char max){
	char x = 4;
	while(1){
		if(!*str || x-4 > max) break;
		buffer[x++]=*str++-0x20; //-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR sprite sheet
	}
}

/**
 * Set state to Menu
 */
void setMenuState(){
	// Clear any remaining sprite
	oam_clear();
	
	music_stop();

	// Turn off rendering
	ppu_off();

	STATE = ST_MENU;

	// Load nametable for title screen
	vram_adr(NAMETABLE_A);
	vram_unrle(title);

	// Set press start area as vram update
	set_vram_update(uListPressStart);
	
	// Set normal brightness
	pal_bright(4);

	// Load palette for sprite
	pal_spr(spritePalette);
	pal_bg(bgPalette);

	// Turn on rendering
	ppu_on_all();
}

/**
 * Set state to Scenario
 */
void setScenarioState(){
	// Clear any remaining sprite
	oam_clear();

	// Turn off rendering
	ppu_off();

	STATE = ST_SCENARIO;

	// Load nametable for title screen
	vram_adr(NAMETABLE_A);
	vram_unrle(scenario);

	// Set text area to update buffer
	set_vram_update(uScenarioText);
	
	// Set normal brightness
	pal_bright(4);

	// Load palette for sprites
	pal_bg(bgPalette);
	pal_spr(spritePalette);

	// Turn on rendering
	ppu_on_all();
}

/**
 * Set state to Instructions
 */
void setInstructionsState(){
	// Clear any remaining sprite
	oam_clear();

	// Turn off rendering
	ppu_off();

	STATE = ST_INSTRUCTIONS;

	// Load nametable for title screen
	vram_adr(NAMETABLE_A);
	vram_unrle(instructions);

	// Set text area to update buffer
	set_vram_update(NULL);
	
	// Set normal brightness
	pal_bright(4);
	
	// Load palette for sprites
	pal_bg(bgPalette);
	pal_spr(spritePalette);

	// Turn on rendering
	ppu_on_all();
}

/**
 * Set state to Menu
 */
void setGameState(){
	char i = 0;
	SCORE = 0;

	// Clear any remaining sprite
	oam_clear();
	for(i = 0; i < BULLET_MAX; i++){
		BULLET_ALIVE[i]=0;
	}
	for(i = 0; i < ENEMY_MAX; i++){
		ENEMY_HP[i]=0;
	}

	// Turn off rendering
	ppu_off();

	STATE = ST_GAME;
	PLAYER_X_POS = 14*8;
	PLAYER_Y_POS = 15*8;
	PLAYER_HP = 3;
	NPC_X_POS = 16*8;
	NPC_Y_POS = 15*8;
	NPC_HP = 3;
	PLAYER_DIRECTION = DIR_DOWN;

	if(MUSIC_ON){
		music_play(0);
	}

	// Load nametable for title screen
	vram_adr(NAMETABLE_A);
	vram_unrle(map1);

	// Set text area to update buffer
	set_vram_update(uListHUD);
	
	// Set normal brightness
	pal_bright(4);

	// Load palette for sprites
	pal_spr(spritePalette);
	pal_bg(bgPalette);

	// Turn on rendering
	ppu_on_all();
}

void setGameOverState(){

	// Turn off rendering
	ppu_off();
	
	STATE = ST_SCOREBOARD;
	music_stop();

	// Load nametable for title screen
	vram_adr(NAMETABLE_A);
	vram_unrle(gameover);

	// Set text area to update buffer
	set_vram_update(uListGameOver);
	
	// Set normal brightness
	pal_bright(4);

	// Load palette for sprites
	pal_spr(spritePalette);
	pal_bg(spritePalette);

	// Turn on rendering
	ppu_on_all();
}

#define TOGGLE_MUSIC(){\
	if(MUSIC_ON == 1){\
		MUSIC_ON = 0;\
		if(STATE == ST_GAME){\
			music_stop();\
		}\
	}else{\
		if(STATE == ST_GAME){\
			music_play(0);\
		}\
		MUSIC_ON = 1;\
	}\
}\

#define PLAYER_SHOOT(){\
	for(i = 0; i < BULLET_MAX; i++){\
		if(!BULLET_ALIVE[i]){\
			sfx_play(0,0);\
			switch(PLAYER_DIRECTION){\
				case DIR_UP:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS;\
					BULLET_DX[i] = 0;\
					BULLET_DY[i] = -BULLET_SPEED;\
					break;\
				case DIR_LEFT:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS + TILE_SIZE_HALF;\
					BULLET_DX[i] = -BULLET_SPEED;\
					BULLET_DY[i] = 0;\
					break;\
				case DIR_RIGHT:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS + TILE_SIZE_HALF;\
					BULLET_DX[i] = BULLET_SPEED;\
					BULLET_DY[i] = 0;\
					break;\
				case DIR_DOWN:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS;\
					BULLET_DX[i] = 0;\
					BULLET_DY[i] = BULLET_SPEED;\
					break;\
			}\
			BULLET_ALIVE[i] = 1;\
			break;\
		}\
	}\
}

/**
 * Spawn sprite for blood effect
 * X_POS Position X of sprite
 * Y_POS Position X of sprite
 */ 
#define BLOOD_EFFECT_AT(X_POS, Y_POS){\
	sfx_play(3,0);\
	tmp1 = 0;\
	tmp2 = 0;\
	for(tmp2 = 0; tmp2 < BLOOD_MAX; tmp2++){\
		if(BLOOD_LIVE[tmp2] == 0){\
			BLOOD_X[tmp2] = X_POS;\
			BLOOD_Y[tmp2] = Y_POS;\
			BLOOD_DX[tmp2] = rand8() > 128 ? -1 : 1;\
			BLOOD_DY[tmp2] = rand8() > 128 ? -1 : 1;\
			switch(rand8()%4){\
				case 0: BLOOD_SPRITE[tmp2] = SPRITE_BLOOD_EFFECT_0; break;\
				case 1: BLOOD_SPRITE[tmp2] = SPRITE_BLOOD_EFFECT_1; break;\
				case 2: BLOOD_SPRITE[tmp2] = SPRITE_BLOOD_EFFECT_2; break;\
				case 3: BLOOD_SPRITE[tmp2] = SPRITE_BLOOD_EFFECT_3; break;\
			}\
			BLOOD_LIVE[tmp2] = BLOOD_EFFECT_DURATION;\
			tmp1 = tmp1 + 1;\
			if(tmp1 == 4) break;\
		}\
	}\
};

/**
 * Spawn enemy
 */ 
#define SPAWN_ENEMY(){\
	sfx_play(4,0);\
	i = 0;\
	for(i = 0; i < ENEMY_MAX; i++){\
		if(ENEMY_HP[i] == 0){\
			tmp = rand8();\
			tmp2 = rand8();\
			if(tmp > 180){\
				ENEMY_X[i] = SCREEN_X_SIZE/2;\
				ENEMY_Y[i] = SCREEN_Y_SIZE - 16;\
				ENEMY_DX[i] = tmp2 > 128 ? -2 : 2;\
				ENEMY_DY[i] = -2;\
			} else if(tmp > 120){\
				ENEMY_X[i] = SCREEN_X_SIZE/2;\
				ENEMY_Y[i] = TOP + 8;\
				ENEMY_DX[i] = tmp2 > 128 ? -2 : 2;\
				ENEMY_DY[i] = 2;\
			} else if(tmp > 60){\
				ENEMY_X[i] = 8;\
				ENEMY_Y[i] = 18*8 + 4;\
				ENEMY_DX[i] = 2;\
				ENEMY_DY[i] = tmp2 > 128 ? -2 : 2;\
			} else {\
				ENEMY_X[i] = SCREEN_X_SIZE-16;\
				ENEMY_Y[i] = 18*8 + 4;\
				ENEMY_DX[i] = -2;\
				ENEMY_DY[i] = tmp2 > 128 ? -2 : 2;\
			}\
			ENEMY_HP[i] = 2;\
			break;\
		}\
	}\
};

#define HUD_UPDATE(){\
	switch(PLAYER_HP){\
		case 3:\
			uListHUD[5] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[6] = SPRITE_GAUGE_1_FILLED;\
			uListHUD[7] = SPRITE_GAUGE_2_FILLED;\
			break;\
		case 2:\
			uListHUD[5] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[6] = SPRITE_GAUGE_1_FILLED;\
			uListHUD[7] = SPRITE_GAUGE_2_EMPTY;\
			break;\
		case 1:\
			uListHUD[5] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[6] = SPRITE_GAUGE_1_EMPTY;\
			uListHUD[7] = SPRITE_GAUGE_2_EMPTY;\
			break;\
		default:\
			uListHUD[5] = SPRITE_GAUGE_0_EMPTY;\
			uListHUD[6] = SPRITE_GAUGE_1_EMPTY;\
			uListHUD[7] = SPRITE_GAUGE_2_EMPTY;\
	}\
	switch(NPC_HP){\
		case 3:\
			uListHUD[11] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[12] = SPRITE_GAUGE_1_FILLED;\
			uListHUD[13] = SPRITE_GAUGE_2_FILLED;\
			break;\
		case 2:\
			uListHUD[11] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[12] = SPRITE_GAUGE_1_FILLED;\
			uListHUD[13] = SPRITE_GAUGE_2_EMPTY;\
			break;\
		case 1:\
			uListHUD[11] = SPRITE_GAUGE_0_FILLED;\
			uListHUD[12] = SPRITE_GAUGE_1_EMPTY;\
			uListHUD[13] = SPRITE_GAUGE_2_EMPTY;\
			break;\
		default:\
			uListHUD[11] = SPRITE_GAUGE_0_EMPTY;\
			uListHUD[12] = SPRITE_GAUGE_1_EMPTY;\
			uListHUD[13] = SPRITE_GAUGE_2_EMPTY;\
	}\
	if(SCORE > 999) SCORE = 999;\
	uListHUD[22] = 0x10+SCORE/100;\
	uListHUD[23] = 0x10+SCORE/10%10;\
	uListHUD[24] = 0x10+SCORE%10;\
	if(MUSIC_ON){\
		uListHUD[26] = SPRITE_MUSIC_ON;\
	}else{\
		uListHUD[26] = SPRITE_MUSIC_OFF;\
	}\
};

/**
 * Main function
 */
void main(void)
{
	// Frame counter for game logic
	unsigned char frame = 0;
	
	// Shared iterative register I
	unsigned char i = 0;

	// Shared iterative register J
	unsigned char j = 0;

	// Sprite counters
	char spr = 0;
	unsigned char maxSpr = 0;

	// Blink
	unsigned char blink = 0;

	// Current text for instructions
	unsigned char currentText = 0;

	// Input buffer 
	unsigned char input_dampener = 0;

	// TMP shared RAM char for various ops
	unsigned char tmp = 0;
	unsigned char tmp1 = 0;
	unsigned char tmp2 = 0;
	unsigned int intmp = 0;

	unsigned char bright_effect = 4;
	unsigned char fsprite = 0x74;

	// Initialize Menu
	setMenuState();

	// Enable PPU
	ppu_on_all();


	while(1){

		// Wait next PPU frame
		ppu_wait_nmi();
		spr = 0;

		switch(STATE){

			// Do menu stuff (just wait for player input)
			case ST_MENU:
				pad=pad_trigger(PLAYER_ONE); // PAD for player 1
				if(pad&PAD_START){
					if(input_dampener == 0){
						input_dampener = 64;
						sfx_play(1,0);
						put_str_ubuffer(uListPressStart, "           ", 16);	
						setScenarioState();	
					}
				}else{
					input_dampener = 0;
				}

				// Flash press start
				if(frame%24 == 0){
					blink^=1;
				}

				if(blink){
					put_str_ubuffer(uListPressStart, "PRESS START", 16);
				}else{
					put_str_ubuffer(uListPressStart, "           ", 16);		
				}

				break;

			// Do instructions menu stuff (just wait for player input)
			case ST_INSTRUCTIONS:
				pad=pad_trigger(PLAYER_ONE); // PAD for player 1
				
				if(pad & PAD_START){
					setGameState();
					sfx_play(1,0);
					input_dampener=24;
				}else if(pad&PAD_SELECT){
					TOGGLE_MUSIC();
				}else {
					input_dampener=0;
				}

				break;
			// Scenario Screen :	
			case ST_SCENARIO:
				pad=pad_trigger(PLAYER_ONE); // PAD for player 1
				
				if(pad & PAD_START || pad & PAD_A){
					if(input_dampener == 0){
						if(currentText >= SCENARIO_LENGTH){
							currentText = 0;
							sfx_play(1,0);
							setInstructionsState();
						}else{
							sfx_play(2,0);
							currentText ++;
						}
					}
				}

				if(pad&PAD_START || pad & PAD_A){
					input_dampener=64;
				}else if(pad&PAD_SELECT && input_dampener == 0){
					input_dampener=64;
					TOGGLE_MUSIC();
				}else{
					input_dampener=0;
				}

				

				put_str_ubuffer(uScenarioText, sentences[currentText], 27);

				break;
			// Game over screen :		
			case ST_SCOREBOARD:
				pad=pad_trigger(PLAYER_ONE); // PAD for player 1
				
				if(intmp < SCORE){
					sfx_play(2,0);
					if(intmp < 15){
						if(frame%6 == 0){
							intmp++;
						}
					}else if(intmp < 40){
						if(frame%4 == 0){
							intmp++;
						}
					}else if(intmp < 80){
						if(frame%2 == 0){
							intmp++;
						}
					}else{
						intmp+=2;
						if(intmp > SCORE){
							intmp = SCORE;
						}
					}
				}else{
					if(pad & PAD_START){
						setMenuState();
					}else {
						input_dampener=0;
					}
				}

				if(pad&PAD_SELECT){
					TOGGLE_MUSIC();
				}

				uListGameOver[10] = 0x10+intmp/100;
				uListGameOver[11] = 0x10+intmp/10%10;
				uListGameOver[12] = 0x10+intmp%10;
				break;
			// Game loop logic below :		
			case ST_GAME:

				pal_spr(spritePalette);
				pal_bg(bgPalette);

				if(PAUSED){
					pad=pad_poll(PLAYER_ONE); // PAD for player 1
					pal_bright(3);
					if(pad&PAD_START && input_dampener == 0){
						PAUSED = 0;
						input_dampener = 12;
						pal_bright(4);
						if(MUSIC_ON == 1){
							music_play(0);
						}
					}else{
						if(input_dampener > 0){
							input_dampener--;
						}
						break;
					}
				}

				if(frame%6 == 0){
					if(fsprite == 0x74){
						fsprite = 0x75;
					}else{
						fsprite = 0x74;
					}
				}
				
				if(frame%18==0 || bright_effect == 1){
					if(bright_effect < 4){
						bright_effect++;
						pal_bright(bright_effect);
						if(bright_effect == 4){
							if(NPC_HP == 0 || PLAYER_HP == 0){
								intmp = 0;
								setGameOverState();
							}
						}
					}else{
						bright_effect = 4;
						pal_bright(4);
					}
				}


				if(frame == 255){
					SPAWN_ENEMY();
				} else if(SCORE > 15){
					if(frame == 128){
						SPAWN_ENEMY();
					}
				} else if(SCORE > 35){
					if(frame == 64){
						SPAWN_ENEMY();
					}
				} else if(SCORE > 50){
					if(frame == 172){
						SPAWN_ENEMY();
					}
				}	

				if(input_dampener > 0) input_dampener--;
				pad=pad_poll(PLAYER_ONE); // PAD for player 1

				// Controlling Player
				tmp = 2;
				tmp2 = PLAYER_DIRECTION;
				if(pad&PAD_LEFT){
					PLAYER_X_POS = PLAYER_X_POS-tmp; tmp2 = DIR_LEFT;
				}
				if(pad&PAD_RIGHT){
					 PLAYER_X_POS = PLAYER_X_POS+tmp; tmp2 = DIR_RIGHT;	
				}
				if(pad&PAD_UP){
					PLAYER_Y_POS = PLAYER_Y_POS-tmp; tmp2 = DIR_UP;
				}    
				if(pad&PAD_DOWN){
					PLAYER_Y_POS = PLAYER_Y_POS+tmp; tmp2 = DIR_DOWN;
				}
				if(!(pad&PAD_A)){
					PLAYER_DIRECTION = tmp2;
				}

				if(pad&PAD_SELECT && input_dampener == 0){
					TOGGLE_MUSIC();
					input_dampener = 12;
				}

				if(pad&PAD_START && input_dampener == 0){
					PAUSED = 1;
					input_dampener = 12;
					sfx_play(2,0);
					if(MUSIC_ON){
						music_stop();
					}
				}

				// Fire
				if(pad&PAD_B && input_dampener == 0){
					PLAYER_SHOOT();
					input_dampener = 12;
				}

				// Wall colision
				if(PLAYER_X_POS < LEF){
					PLAYER_X_POS = LEF;
				}else if(PLAYER_X_POS > RIG){
					PLAYER_X_POS = RIG;
				}
				if(PLAYER_Y_POS < TOP){
					PLAYER_Y_POS = TOP;
				}else if(PLAYER_Y_POS > BOT){
					PLAYER_Y_POS = BOT;
				}

				// NPC "AI"
				if(NPC_HP > 0){
					tmp = rand8();
					if(tmp > 235 ){
						if(tmp < 240){
							NPC_X_POS = NPC_X_POS-3; NPC_DIRECTION = DIR_LEFT;
						} else if(tmp < 245){
								NPC_X_POS = NPC_X_POS+3; NPC_DIRECTION = DIR_RIGHT;	
						} else if(tmp < 250){
							NPC_Y_POS = NPC_Y_POS-3; NPC_DIRECTION = DIR_UP;
						} else {
							NPC_Y_POS = NPC_Y_POS+3; NPC_DIRECTION = DIR_DOWN;
						}
					}

					// NPC colision
					if(NPC_X_POS < NPC_LEF){
						NPC_X_POS = NPC_LEF;
					}else if(NPC_X_POS > NPC_RIG){
						NPC_X_POS = NPC_RIG;
					}
					if(NPC_Y_POS < NPC_TOP){
						NPC_Y_POS = NPC_TOP;
					}else if(NPC_Y_POS > NPC_BOT){
						NPC_Y_POS = NPC_BOT;
					}
				}

				// Display Player
				tmp = 0;
				PLAYER_FLAGS = 0;
				switch(PLAYER_DIRECTION){
					case DIR_UP:
						tmp = SPRITE_PLAYER_UP;
						break;
					case DIR_LEFT:
						tmp = SPRITE_PLAYER_RIGHT;
						PLAYER_FLAGS = OAM_FLIP_H;
						break;
					case DIR_RIGHT:
						tmp = SPRITE_PLAYER_RIGHT;
						break;
					case DIR_DOWN:
						tmp = SPRITE_PLAYER_DOWN;
						break;
				}

				// Sprites buffer ops
				spr = 0;
				spr = oam_spr(PLAYER_X_POS, PLAYER_Y_POS, tmp, 4|PLAYER_FLAGS|PLAYER_PALETTE, spr);
				spr = oam_spr(PLAYER_X_POS, PLAYER_Y_POS+8, tmp+0x10, 4|PLAYER_FLAGS|(PLAYER_PALETTE), spr);

				if(NPC_HP > 0){
					tmp = 0;
					NPC_FLAGS = 0;
					switch(NPC_DIRECTION){
						case DIR_UP:
							tmp = SPRITE_NPC_UP;
							break;
						case DIR_LEFT:
							tmp = SPRITE_NPC_RIGHT;
							NPC_FLAGS = OAM_FLIP_H;
							break;
						case DIR_RIGHT:
							tmp = SPRITE_NPC_RIGHT;
							break;
						case DIR_DOWN:
							tmp = SPRITE_NPC_DOWN;
							break;
					}
					spr = oam_spr(NPC_X_POS, NPC_Y_POS, tmp, 4|NPC_FLAGS|NPC_PALETTE, spr);
					spr = oam_spr(NPC_X_POS, NPC_Y_POS+8, tmp+0x10, 4|NPC_FLAGS|(NPC_PALETTE+1), spr);
				}

				// BLOOD EFFECTS 
				i = 0;
				tmp2 = frame%2;
				for(i = 0; i < BLOOD_MAX; i++){
					if(BLOOD_LIVE[i] > 0){
						BLOOD_X[i] = BLOOD_X[i] + BLOOD_DX[i];
						BLOOD_Y[i] = BLOOD_Y[i] + BLOOD_DY[i];
						BLOOD_LIVE[i] = BLOOD_LIVE[i] - 1;
						if(tmp2){
							spr = oam_spr(BLOOD_X[i], BLOOD_Y[i], BLOOD_SPRITE[i], 4|3, spr);
						}else{
							spr = oam_spr(BLOOD_X[i], BLOOD_Y[i], BLOOD_SPRITE[i], 4|3|OAM_FLIP_H, spr);
						}
					}
				}

				// PLAYER PROJECTILES
				i = 0;
				for(i = 0; i < BULLET_MAX; i++){
					if(BULLET_ALIVE[i] > 0){
						BULLET_X[i] = BULLET_X[i] + BULLET_DX[i];
						BULLET_Y[i] = BULLET_Y[i] + BULLET_DY[i];
						if(BULLET_X[i] < LEF){
							BULLET_ALIVE[i] = 0;
						}else if(BULLET_X[i] > RIG){
							BULLET_ALIVE[i] = 0;
						}
						if(BULLET_Y[i] < TOP){
							BULLET_ALIVE[i] = 0;
						}else if(BULLET_Y[i] > BOT + TILE_SIZE){
							BULLET_ALIVE[i] = 0;
						}

						// Check collision with NPC
						if(NPC_HP > 0 && BULLET_X[i]+TILE_SIZE_HALF > NPC_X_POS && BULLET_X[i]+TILE_SIZE_HALF < NPC_X_POS+TILE_SIZE){
							if(BULLET_Y[i]+TILE_SIZE_HALF > NPC_Y_POS+TILE_SIZE && BULLET_Y[i]+TILE_SIZE_HALF < NPC_Y_POS+TILE_SIZE_DOUBLE){
								sfx_play(1,1);
								NPC_HP--;
								bright_effect = 1;
								BULLET_ALIVE[i] = 0;
								BLOOD_EFFECT_AT(NPC_X_POS, NPC_Y_POS);
							}
						}

						j = 0;
						for(j = 0; j < ENEMY_MAX; j++){
							if(ENEMY_HP[j] > 0 && BULLET_X[i]+4 > ENEMY_X[j] -4 && BULLET_X[i]+4 < ENEMY_X[j]+12){
								if(BULLET_Y[i]+4 > ENEMY_Y[j]-4 && BULLET_Y[i]+4 < ENEMY_Y[j]+12){
									sfx_play(1,1);
									BULLET_ALIVE[i] = 0;
									ENEMY_HP[j] = 0;
									ENEMY_DX[j] = 0;
									ENEMY_DY[j] = 0;
									SCORE ++;
									BLOOD_EFFECT_AT(ENEMY_X[j], ENEMY_Y[j]);
								}
							}
						}

						// DISPLAY
						spr = oam_spr(BULLET_X[i], BULLET_Y[i], 0x45, 4|3, spr);
					}
				}

				// ENEMYS
				i = 0;
				tmp = rand8();
				for(i = 0; i < ENEMY_MAX; i++){
					if(ENEMY_HP[i] > 0){
						
						if(ENEMY_X[i] < LEF){
							ENEMY_X[i] = LEF;
							ENEMY_DX[i] = 2;
						}else if(ENEMY_X[i] > RIG){
							ENEMY_X[i] = RIG;
							ENEMY_DX[i] = -1;
						}
						if(ENEMY_Y[i] < TOP){
							ENEMY_Y[i] = TOP;
							ENEMY_DY[i] = tmp > 128 ? 2 : -1;
						}else if(ENEMY_Y[i] > BOT + 8){
							ENEMY_Y[i] = BOT + 8;
							ENEMY_DY[i] = tmp > 128 ? -2 : -1;
						}
						
						ENEMY_X[i] = ENEMY_X[i] + ENEMY_DX[i];
						ENEMY_Y[i] = ENEMY_Y[i] + ENEMY_DY[i];
						spr = oam_spr(ENEMY_X[i], ENEMY_Y[i], fsprite, 4|3, spr);

						// Check collision with NPC
						if(NPC_HP > 0 && ENEMY_X[i]+4 > NPC_X_POS && ENEMY_X[i]+4 < NPC_X_POS+8){
							if(ENEMY_Y[i]+4 > NPC_Y_POS+8 && ENEMY_Y[i]+4 < NPC_Y_POS+16){
								sfx_play(1,1);
								NPC_HP--;
								bright_effect = 1;
								ENEMY_HP[i] = 0;
								BLOOD_EFFECT_AT(NPC_X_POS, NPC_Y_POS);
							}
						}

						// Check collision with PLAYER
						if(PLAYER_HP > 0 && ENEMY_X[i]+4 > PLAYER_X_POS && ENEMY_X[i]+4 < PLAYER_X_POS+8){
							if(ENEMY_Y[i]+4 > PLAYER_Y_POS+8 && ENEMY_Y[i]+4 < PLAYER_Y_POS+16){
								sfx_play(1,1);
								PLAYER_HP--;
								bright_effect = 1;
								ENEMY_HP[i] = 0;
								BLOOD_EFFECT_AT(PLAYER_X_POS, PLAYER_Y_POS);
							}
						}

					}

					

				}

				HUD_UPDATE();
				break;

		}

		// Sprite cleaner
		if(spr > maxSpr){
			maxSpr = spr;
		}else{
			// Clear sprites
			while(spr<maxSpr){
				spr = oam_spr(0,0,0,2,spr);
			}
		}

		frame++;
	}
}
