/**
 * 
 * LUDUM DARE 46 - AVOIDERS
 * By Khopa
 * 
 * Using Shiru's NESLIB
 * 
 */

// NES Library
#include "neslib.h"

// Sprite definitions
#include "sprites.h"

// RAW NAMETABLE DATA
#include "title.h"
#include "scenario.h"
#include "instructions.h"
#include "map1.h"

#define PLAYER_ONE 0x00

#define ST_MENU  0x00
#define ST_GAME  0x01
#define ST_INSTRUCTIONS  0x02
#define ST_SCOREBOARD 0x03
#define ST_SCENARIO 0x04

#define SCREEN_X_SIZE 256
#define SCREEN_Y_SIZE 240

#define TOP 8*8
#define BOT 27*8
#define LEF 1*8
#define RIG 30*8

#define NPC_TOP 12*8
#define NPC_BOT 22*8
#define NPC_LEF 6*8
#define NPC_RIG 25*8


// 64 sprites is the max the NES can handle
// -> 2 sprite for player
// -> 2 for NPC to protect
// -> 16 for enemy objects
// -> 10 for player gun bullets
// -> 1 for bonuses
// -> 9 for blood special effects
#define ENEMY_MAX 10
#define BULLET_MAX 4
#define BLOOD_MAX 9
#define SCENARIO_LENGTH 8

// Direction const
#define DIR_UP 0
#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_DOWN 3

#define PLAYER_BULLET_PAL 0

// PAD Management
static unsigned char pad;
static unsigned int STATE;

// SCORE 
static unsigned int SCORE = 0;

// PLAYER OBJECT
static unsigned char   PLAYER_X_POS = 0;
static unsigned char   PLAYER_Y_POS = 0;
static unsigned char   PLAYER_HP;
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
static unsigned char BULLET_X[BULLET_MAX];
static unsigned char BULLET_Y[BULLET_MAX];
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

// ENEMY
static int ENEMY_X[ENEMY_MAX];
static int ENEMY_Y[ENEMY_MAX];
static int ENEMY_DX[ENEMY_MAX];
static int ENEMY_DY[ENEMY_MAX];
static unsigned char ENEMY_HP[ENEMY_MAX];

// Score board nametable vram update list
const unsigned char updateListData[8] ={
	MSB(NTADR_A(12,2))|NT_UPD_HORZ,LSB(NTADR_A(12,2)),4,0,0,0,0,NT_UPD_EOF
};

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

// Game 'HUD' vram update buffer
static unsigned char uListHUD[26] ={
	MSB(NTADR_A(2,2))|NT_UPD_HORZ,LSB(NTADR_A(2,2)),22,  // AT BG position 3,3, UPDATE 22 TILES
	0x46,0,0x7C,0x7D,0x7E,0,        // PLAYER HEAD (46), 3 TILE GAUGE
	0x69,0,0x7C,0x7D,0x7E,0,        // NPC HEAD (69), 3 TILE GAUGE
	0x33,0x23,0x2F,0x32,0x25,0x1A,  // 'S' 'C' 'O' 'R' 'E' ':'
	0x00,0x10,0x10,0x10,            // '0' '0' '0'
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
 * Put a String on the screen
 * adr : location in vram/screen
 * str : string to put
 */
void put_str(unsigned int adr,const char *str){
	vram_adr(adr);
	while(1)
	{
		if(!*str) break;
		vram_put((*str++)-0x20); //-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR sprite sheet
	}
}

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
	PLAYER_X_POS = 15*8;
	PLAYER_Y_POS = 15*8;
	PLAYER_HP = 3;
	NPC_X_POS = 16*8;
	NPC_Y_POS = 15*8;
	NPC_HP = 3;
	music_play(0);

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

#define PLAYER_SHOOT(){\
	for(i = 0; i < BULLET_MAX; i++){\
		if(!BULLET_ALIVE[i]){\
			sfx_play(3,0);\
			switch(PLAYER_DIRECTION){\
				case DIR_UP:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS;\
					BULLET_DX[i] = 0;\
					BULLET_DY[i] = -4;\
					break;\
				case DIR_LEFT:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS + 4;\
					BULLET_DX[i] = -4;\
					BULLET_DY[i] = 0;\
					break;\
				case DIR_RIGHT:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS + 4;\
					BULLET_DX[i] = 4;\
					BULLET_DY[i] = 0;\
					break;\
				case DIR_DOWN:\
					BULLET_X[i] = PLAYER_X_POS;\
					BULLET_Y[i] = PLAYER_Y_POS;\
					BULLET_DX[i] = 0;\
					BULLET_DY[i] = 4;\
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
			tmp = rand8();\
			BLOOD_DX[tmp2] = tmp > 128 ? -1 : 1;\
			BLOOD_DY[tmp2] = tmp > 128 ? -1 : 1;\
			BLOOD_SPRITE[tmp2] = 0x54;\
			BLOOD_LIVE[tmp2] = 8;\
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
			uListHUD[5] = 0x7C;\
			uListHUD[6] = 0x7D;\
			uListHUD[7] = 0x7E;\
			break;\
		case 2:\
			uListHUD[5] = 0x7C;\
			uListHUD[6] = 0x7D;\
			uListHUD[7] = 0x6E;\
			break;\
		case 1:\
			uListHUD[5] = 0x7C;\
			uListHUD[6] = 0x6D;\
			uListHUD[7] = 0x6E;\
			break;\
		default:\
			uListHUD[5] = 0x6C;\
			uListHUD[6] = 0x6D;\
			uListHUD[7] = 0x6E;\
	}\
	switch(NPC_HP){\
		case 3:\
			uListHUD[11] = 0x7C;\
			uListHUD[12] = 0x7D;\
			uListHUD[13] = 0x7E;\
			break;\
		case 2:\
			uListHUD[11] = 0x7C;\
			uListHUD[12] = 0x7D;\
			uListHUD[13] = 0x6E;\
			break;\
		case 1:\
			uListHUD[11] = 0x7C;\
			uListHUD[12] = 0x6D;\
			uListHUD[13] = 0x6E;\
			break;\
		default:\
			uListHUD[11] = 0x6C;\
			uListHUD[12] = 0x6D;\
			uListHUD[13] = 0x6E;\
	}\
	if(SCORE > 999) SCORE = 999;\
	uListHUD[22] = 0x10+SCORE/100;\
	uListHUD[23] = 0x10+SCORE/10%10;\
	uListHUD[24] = 0x10+SCORE%10;\
};

/**
 * Main function
 */
void main(void)
{
	// Frame counter for game logic
	unsigned int frame = 0;
	
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
						sfx_play(0,0);
						put_str_ubuffer(uListPressStart, "           ", 16);	
						setScenarioState();	
					}
				}else{
					input_dampener = 0;
				}

				// Flash press start
				if(frame%24 == 0){
					if(blink == 1){
						blink = 0;
					}else{
						blink = 1;
					}
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
				}else {
					input_dampener=0;
				}

				break;
				// Do instructions menu stuff (just wait for player input)
			case ST_SCENARIO:
				pad=pad_trigger(PLAYER_ONE); // PAD for player 1
				
				if(pad & PAD_START || pad & PAD_A){
					if(input_dampener == 0){
						if(currentText >= SCENARIO_LENGTH){
							currentText = 0;
							setInstructionsState();
						}else{
							currentText ++;
						}
					}
				}

				if(pad&PAD_START || pad & PAD_A){
					input_dampener=64;
				}else{
					input_dampener=0;
				}

				put_str_ubuffer(uScenarioText, sentences[currentText], 27);

				break;
			case ST_GAME:

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
								setMenuState();
							}
						}
					}else{
						bright_effect = 4;
						pal_bright(4);
					}
				}


				if(frame == 255){
					SPAWN_ENEMY();
				}

				if(input_dampener > 0) input_dampener--;
				pad=pad_poll(PLAYER_ONE); // PAD for player 1

				// Controlling Player
				tmp = 2;
				if(pad&PAD_A) tmp = 4;
				if(pad&PAD_LEFT){
					PLAYER_X_POS = PLAYER_X_POS-tmp; PLAYER_DIRECTION = DIR_LEFT;
				}
				if(pad&PAD_RIGHT){
					 PLAYER_X_POS = PLAYER_X_POS+tmp; PLAYER_DIRECTION = DIR_RIGHT;	
				}
				if(pad&PAD_UP){
					PLAYER_Y_POS = PLAYER_Y_POS-tmp; PLAYER_DIRECTION = DIR_UP;
				}    
				if(pad&PAD_DOWN){
					PLAYER_Y_POS = PLAYER_Y_POS+tmp; PLAYER_DIRECTION = DIR_DOWN;
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
						tmp = 0x48;
						break;
					case DIR_LEFT:
						tmp = 0x47;
						PLAYER_FLAGS = OAM_FLIP_H;
						break;
					case DIR_RIGHT:
						tmp = 0x47;
						break;
					case DIR_DOWN:
						tmp = 0x46;
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
							tmp = 0x6B;
							break;
						case DIR_LEFT:
							tmp = 0x6A;
							NPC_FLAGS = OAM_FLIP_H;
							break;
						case DIR_RIGHT:
							tmp = 0x6A;
							break;
						case DIR_DOWN:
							tmp = 0x69;
							break;
					}
					spr = oam_spr(NPC_X_POS, NPC_Y_POS, tmp, 4|NPC_FLAGS|NPC_PALETTE, spr);
					spr = oam_spr(NPC_X_POS, NPC_Y_POS+8, tmp+0x10, 4|NPC_FLAGS|(NPC_PALETTE+1), spr);
				}

				// BLOOD EFFECTS 
				i = 0;
				for(i = 0; i < BLOOD_MAX; i++){
					if(BLOOD_LIVE[i] > 0){
						BLOOD_X[i] = BLOOD_X[i] + BLOOD_DX[i];
						BLOOD_Y[i] = BLOOD_Y[i] + BLOOD_DY[i];
						BLOOD_LIVE[i] = BLOOD_LIVE[i] - 1;
						spr = oam_spr(BLOOD_X[i], BLOOD_Y[i], BLOOD_SPRITE[i], 4|1, spr);
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
						}else if(BULLET_Y[i] > BOT){
							BULLET_ALIVE[i] = 0;
						}

						// Check collision with NPC
						if(NPC_HP > 0 && BULLET_X[i]+4 > NPC_X_POS && BULLET_X[i]+4 < NPC_X_POS+8){
							if(BULLET_Y[i]+4 > NPC_Y_POS+8 && BULLET_Y[i]+4 < NPC_Y_POS+16){
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
		if(frame > 0xFF){
			frame = 0x00;
		}
	}
}
