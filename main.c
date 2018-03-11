/*
 * customproject.c
 *
 * Created: 2/27/2018 2:49:22 PM
 * Author : Charlie
 */ 
#include <avr/io.h>
#include <bit.h>
#include "io.c"
#include "keypad.h"
#include "seven_seg.h"
#include "ADC.h"
#include "CustomCharacters.h"
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <avr/eeprom.h>

//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;
//--------End Task scheduler data structure-----------------------------------

//--------Shared Variables----------------------------------------------------
unsigned char playerPosition = 16; //player column position
unsigned char enemyPosition = 17; //enemy column position
unsigned char playerAttack = 0; /* boolean flags */
unsigned char enemyAttack = 0; /*  for checking if there is a projectile out already */
unsigned char playerProjectilePosition = 0x00; //start from the player's position and move upward
unsigned char enemyProjectilePosition = 0x00; //start from the enemy's position and move downwards
unsigned char Score = 0x00; //score starts at 0, award 1 pt per hit
unsigned char lives = 2; //start with 2 lives
unsigned char enemyHP = 1; //1 hit to kill an enemy
unsigned char select = 0x00; //start game in Menu, and shoot while in game
unsigned char reset = 0x00; //soft reset
//custom icon arrays
unsigned char playerShip[8] = {
	0b10111,
	0b00000,
	0b01111,
	0b10000,
	0b10000,
	0b01111,
	0b00000,
	0b10111
};

unsigned char enemyShip[8] = {
	0b01000,
	0b00100,
	0b10010,
	0b10001,
	0b10001,
	0b10010,
	0b00100,
	0b01000
};

unsigned char playerBeam[8] = {
		0b00000,
		0b00000,
		0b00010,
		0b11111,
		0b11111,
		0b00010,
		0b00000,
		0b00000
};

unsigned char enemyBeam[8] = {
		0b11110,
		0b00010,
		0b00100,
		0b01111,
		0b00100,
		0b00010,
		0b00010,
		0b11110
};

//joystick threshholds from LEDs
unsigned short up = 800; //800
unsigned short down = 200; //200 
unsigned short left = 200; //200
unsigned short right = 800; //800

unsigned char inGame = 0x00; //check if still in game loop
unsigned short adc_resultX = 0x0000; //joystick input
unsigned short adc_resultY = 0x0000;
unsigned char menuCursor = 3;

//user-defined FSMs
enum gameStates {init, menu, gameLoop, /*showHighScores,*/ victory, defeat};
enum playerStates {move, updatePlayerLCD};
enum playerProjectile {updatePlayerProjectile};
enum enemyStates {spawn, calculateChoice, updateEnemyLCD};
enum enemyProjectile {updateEnemyProjectile};
	
int gameLogic(int state) {
	select = ~PINA & 0x10;
	reset = ~PINA & 0x20;
	adc_resultX = Read_ADC(2);
	adc_resultY = Read_ADC(3);
	switch (state) { //Transitions
		case init:
		break;
		case menu:
		Write7Seg('0' + '0');
			if (reset) {
				state = init;
				eeprom_write_byte((uint8_t*)10, (uint8_t) 0);
				break;
			}
						
			if (select) {
				state = gameLoop;
				LCD_ClearScreen();
				inGame = 1;
				LCD_Cursor(playerPosition);
				LCD_WriteData(0x00);
				break;
			}
			/*
			else if (select && menuCursor == 19) {
				state = showHighScores;
				LCD_ClearScreen();
				LCD_DisplayString(1, "SHOWING HIGH SCORES");
				menuCursor = 3;
				break;
			}
			*/
			else {
				state = menu;
			}
			/*
			if (adc_resultX >= right) {
			menuCursor = 3;
			LCD_Cursor(menuCursor);
			}
			else if (adc_resultX <= left) {
			menuCursor = 19;
			LCD_Cursor(menuCursor);
			}
			*/
		break;
		case gameLoop:
		if (reset) {
			state = init;
			break;
		}
		if (Score >= 3) {
			state = victory;
			playerAttack = 0;
			enemyAttack = 0;
			LCD_ClearScreen();
			LCD_DisplayString(1, "    VICTORY!      PLAY AGAIN?");
			inGame = 0;
			break;
		}
		if (lives <= 0) {
			PORTB = 0x00;
			state = defeat;
			playerAttack = 0;
			enemyAttack = 0;
			LCD_ClearScreen();
			LCD_DisplayString(1, "    DEFEAT!!      PLAY AGAIN?");
			lives = 0;
			inGame = 0;
			break;
		}
		else {
		state = gameLoop;
		}
		break;
		/*
		case showHighScores:
		if (select) {
			state = init;
		}
		*/
		break;
		case victory:
		if (select || reset) {
			if (Score >= eeprom_read_byte((uint8_t*)10)) {
				eeprom_write_byte((uint8_t*)10, (uint8_t) Score);
			}
			state = init;
		}
		else {
			if (Score >= eeprom_read_byte((uint8_t*)10)) {
				eeprom_write_byte((uint8_t*)10, (uint8_t) Score);
			}			
			state = victory;
		}
		break;
		case defeat:
		if (select || reset) {
			if (Score >= eeprom_read_byte((uint8_t*)10)) {
				eeprom_write_byte((uint8_t*)10, (uint8_t) Score);
			}		
			state = init;
		}
		else {
			if (Score >= eeprom_read_byte((uint8_t*)10)) {
				eeprom_write_byte((uint8_t*)10, (uint8_t) Score);
			}	
			state = victory;
		}
		break;
		default:
		state = init;
		break;
	}
	switch (state) { //Actions
		case init:
		playerPosition = 16; //player column position
		enemyPosition = 17; //enemy column position
		playerProjectilePosition = 0x00; //start from the player's position and move upward
		enemyProjectilePosition = 0x00; //start from the enemy's position and move downwards
		playerAttack = 0;
		enemyAttack = 0;
		Score = 0x00; //high score starts at 0, award 1 pt per hit
		lives = 2; //start with 2 lives
		enemyHP = 1; //1 hit to kill an enemy
		select = 0x00;
		inGame = 0x00; //check if still in game loop
		adc_resultX = 0x0000; //joystick input horizontal
		adc_resultY = 0x0000; //joystick input vertical
		PORTB = 0x00;
		LCD_ClearScreen();
		LCD_DisplayString(1, "   START GAME     HIGH SCORE:");
		uint8_t highestScore;
		highestScore = eeprom_read_byte((uint8_t*)10);
		LCD_Cursor(30);
		LCD_WriteData((char)highestScore + '0');
		LCD_Cursor(40);
		state = menu;
		break;
		case menu:
		break;
		case gameLoop:
		if (playerProjectilePosition == enemyPosition) {
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(32);
			playerAttack = 0;
			playerProjectilePosition = 0;
			--enemyHP;
			++Score;
		}
		if (enemyProjectilePosition == playerPosition) {
			--lives;
			LCD_Cursor(enemyProjectilePosition + 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 2);
			LCD_WriteData(32);
			enemyAttack = 0;
			enemyProjectilePosition = 0;
		}
		if (playerProjectilePosition == 17 || playerProjectilePosition <= 0) {
			LCD_Cursor(playerProjectilePosition + 1);
			LCD_WriteData(32);
			playerAttack = 0;
			playerProjectilePosition = 0;
			LCD_Cursor(40);
		}
		if (enemyProjectilePosition >= 33 || enemyProjectilePosition == 17) {
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 2);
			LCD_WriteData(32);
			enemyAttack = 0;
			enemyProjectilePosition = 0;
			LCD_Cursor(40);
		}
		if (lives == 2) {
			PORTB = 0x03;
		}
		if (lives == 1) {
			PORTB = 0x01;
		}
		Write7Seg(Score);
		break;
		/*
		case showHighScores:
		break;
		*/
		case victory:
		break;
		case defeat:
		break;
		default:
		state = init;
		break;
	}
	return state;
}

int playerLogic(int state) {
	select = ~PINA & 0x10;
	adc_resultX = Read_ADC(2);
	adc_resultY = Read_ADC(3);
	switch (state) { //Transitions
		case move:
		state = move;
		break;
		case updatePlayerLCD:
		break;
		default:
		state = move;
		break;
	}
	switch (state) { //Actions
		case move:
		if (inGame) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(0x00);
			LCD_Cursor(40);
		if (adc_resultY >= up && (playerPosition != 14 && playerPosition != 30)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			LCD_Cursor(playerPosition + 1);
			LCD_WriteData(32);
			--playerPosition;
			LCD_Cursor(playerPosition);
			LCD_Cursor(40);
			state = updatePlayerLCD;
		}
		else if (adc_resultY <= down && (playerPosition != 16 && playerPosition != 32)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			LCD_Cursor(playerPosition + 1);
			LCD_WriteData(32);
			++playerPosition;
			LCD_Cursor(playerPosition);
			LCD_Cursor(40);
			state = updatePlayerLCD;
		}
		else if (adc_resultX >= right && (playerPosition != 16 && playerPosition != 15 && playerPosition != 14)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			LCD_Cursor(playerPosition + 1);
			LCD_WriteData(32);
			playerPosition -= 16;
			LCD_Cursor(playerPosition);
			LCD_Cursor(40);
			state = updatePlayerLCD;
		}
		else if (adc_resultX <= left && (playerPosition != 32 && playerPosition != 31 && playerPosition != 30)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			LCD_Cursor(playerPosition + 1);
			LCD_WriteData(32);
			playerPosition += 16;
			LCD_Cursor(playerPosition);
			LCD_Cursor(40);
			state = updatePlayerLCD;
		}
		}
		break;
		case updatePlayerLCD:
		if (inGame) {
		LCD_Cursor(playerPosition);
		LCD_WriteData(0x00);
		LCD_Cursor(40);
		}
		state = move;
		break;
		default:
		state = move;
		break;
	}
	return state;
}

int playerProjectileLogic(int state) {
	switch (state) {
		case updatePlayerProjectile:
		break;
		default:
		state = updatePlayerProjectile;
	}
	
	switch (state) {
		case updatePlayerProjectile:
		if (inGame) {
		if (select && playerAttack == 0) {
			playerAttack = 1;
			playerProjectilePosition = playerPosition - 1;
		}
		if (playerAttack) {
			LCD_Cursor(40);
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(playerProjectilePosition + 1);
			LCD_WriteData(32);
			LCD_Cursor(playerProjectilePosition + 2);
			LCD_WriteData(32);
			--playerProjectilePosition;
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(0x02);
			LCD_Cursor(40);
		}
		}
		break;
		default:
		state = updatePlayerProjectile;
		break;
	}
	return state;
}

int enemyLogic(int state) {
	unsigned char randomNumber = 0;
	switch(state) {
		case spawn:
		break;
		case calculateChoice:
		if (enemyHP <= 0) {
			state = spawn;
		}
		else {
			state = calculateChoice;
		}
		break;
		case updateEnemyLCD:
		break;
		default:
		state = updateEnemyLCD;
		break;
	}
	switch(state) {
		case spawn:
		if (inGame) {
		enemyHP = 1;
		if (enemyPosition == 17) {
			enemyPosition = 1;
		}
		else if (enemyPosition == 18) {
			enemyPosition = 2;
		}
		else if (enemyPosition == 1) {
			enemyPosition = 18;
		}
		else {
			enemyPosition = 17;
		}
		LCD_Cursor(enemyPosition);
		LCD_WriteData(0x01);
		LCD_Cursor(40);
		state = updateEnemyLCD;
		}
		break;
		case calculateChoice:
		//random number between 0 and 100
		//if number is <40 and there's no projectile currently, then attack
		//if number is <40 and there's a projectile, don't do anything
		//if number is >=40 and there's no projectile, either move or attack (50% chance for either)
		//if number is >=40 and there's a projectile, do below:
		//move to one of 3 surrounding spots on the following conditions:
		//if number % 2 == 0, move to spot above/below
		//if number % 3 == 0, move to spot top/bottom left/right
		//if number % 5 == 0, move to left or right
		//if the number fails to satisfy the condition, such as '49', then move 
		//left if possible, otherwise move right then move back
		if (inGame) {
			if (enemyHP <= 0) {
				state = spawn;
			}
			else {
				randomNumber = rand() % 101; //random number between 0 and 100
				if (randomNumber < 40 && enemyAttack == 0) {
					enemyAttack = 1;
					enemyProjectilePosition = enemyPosition + 2;
					state = updateEnemyLCD;
				}
				else if (randomNumber < 40 && enemyAttack) {
					state = updateEnemyLCD;
				}
				else if (randomNumber >= 40 && enemyAttack == 0) {
					randomNumber = rand() % 2; //0 or 1
					if (randomNumber) {
						enemyAttack = 1;
						enemyProjectilePosition = enemyPosition + 2;
						state = updateEnemyLCD;
					} 
					else {
						if (randomNumber % 2 == 0) {
							if (enemyPosition == 17 || enemyPosition == 18) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = enemyPosition - 16;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 1 || enemyPosition == 2) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);		
								enemyPosition = enemyPosition + 16;
								state = updateEnemyLCD;
							}
						}
						else if (randomNumber % 3 == 0) {
							if (enemyPosition == 17) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = 2;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 18) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								enemyPosition = 1;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 1) {			
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								enemyPosition = 18;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 2) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								enemyPosition = 17;
								state = updateEnemyLCD;
							}
						}
						else if (randomNumber % 5 == 0) {
							if (enemyPosition == 2 || enemyPosition == 18) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								--enemyPosition;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 1 || enemyPosition == 17) {								
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								++enemyPosition;
								state = updateEnemyLCD;
							}
						}
					}
					}
					else if (randomNumber >= 40 && enemyAttack) {
						if (randomNumber % 2 == 0) {
							if (enemyPosition >= 17) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = enemyPosition - 16;
								state = updateEnemyLCD;
							}
							else if (enemyPosition <= 2) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								enemyPosition = enemyPosition + 16;
								state = updateEnemyLCD;
							}
						}
						else if (randomNumber % 3 == 0) {
							if (enemyPosition == 17) {				
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = 2;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 18) {					
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = 1;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 1) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);
								enemyPosition = 18;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 2) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								enemyPosition = 17;
								state = updateEnemyLCD;
							}
						}
						else if (randomNumber % 5 == 0) {
							if (enemyPosition == 2 || enemyPosition == 18) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								--enemyPosition;
								state = updateEnemyLCD;
							}
							else if (enemyPosition == 1 || enemyPosition == 17) {
								LCD_Cursor(enemyPosition + 1);
								LCD_WriteData(32);
								LCD_Cursor(enemyPosition - 1);
								LCD_WriteData(32);								
								++enemyPosition;
								state = updateEnemyLCD;
							}
						}
				}
			}
		}
		state = updateEnemyLCD;
		break;
		
		case updateEnemyLCD:
		if (inGame) {
			LCD_Cursor(enemyPosition);
			LCD_WriteData(0x01);
			LCD_Cursor(40);
			if (enemyHP <= 0) {
				state = spawn;
			}
			else {
				state = calculateChoice;
			}
		}
		if (enemyAttack) {
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition + 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(0x03);
			LCD_Cursor(40);			
		}
		break;
		default:
		state = updateEnemyLCD;
		break;
	}
	return state;
}

int enemyProjectileLogic (int state) {
	switch (state) {
		case updateEnemyProjectile:
		break;
		default:
		state = updateEnemyProjectile;
		break;
	}
	
	switch (state) {
		case updateEnemyProjectile:
		if (inGame) {
		if (enemyAttack) {
			//LCD_Cursor(40);
			//LCD_Cursor(enemyProjectilePosition);
			//LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 2);
			LCD_WriteData(32);
			++enemyProjectilePosition;
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(0x03);
			LCD_Cursor(40);
		}
		}
		break;
		default:
		state = updateEnemyProjectile;
		break;
	}
	
	return state;
}

int main() {
DDRA = 0x03; PORTA = 0xFC; // A0/A1 for LCD, rest for joystick
DDRB = 0xFF; PORTB = 0x00; // programmer
DDRC = 0xFF; PORTC = 0x00; // 7 seg
DDRD = 0xFF; PORTD = 0x00; // LCD data lines

unsigned long int game_calc = 10;
unsigned long int player_calc = 100;
unsigned long int enemy_calc = 50;
unsigned long int playerProjectile_calc = 50;
unsigned long int enemyProjectile_calc = 100;

unsigned long int tmpGCD = 1;
tmpGCD = findGCD(player_calc, enemy_calc);
tmpGCD = findGCD(tmpGCD, playerProjectile_calc);
tmpGCD = findGCD(tmpGCD, game_calc);
tmpGCD = findGCD(tmpGCD, enemyProjectile_calc);

unsigned long int GCD = tmpGCD;

unsigned long int player_period = player_calc/GCD;
unsigned long int enemy_period = enemy_calc/GCD;
unsigned long int playerProjectile_period = playerProjectile_calc/GCD;
unsigned long int game_period = game_calc/GCD;
unsigned long int enemyProjectile_period = enemyProjectile_calc/GCD;

static task task1, task2, task3, task4, task5;
task *tasks[] = { &task1, &task2, &task3, &task4, &task5 };
const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

//main menu / game logic
task1.state = init;
task1.period = game_period;
task1.elapsedTime = 0;
task1.TickFct = &gameLogic;

//player
task2.state = move;
task2.period = player_period;
task2.elapsedTime = 0;
task2.TickFct = &playerLogic;

//enemy
task3.state = spawn;
task3.period = enemy_period;
task3.elapsedTime = 0;
task3.TickFct = &enemyLogic;

//playerProjectile
task4.state = updatePlayerProjectile;
task4.period = playerProjectile_period;
task4.elapsedTime = 0;
task4.TickFct = &playerProjectileLogic;

//enemyProjectile
task5.state = updateEnemyProjectile;
task5.period = enemyProjectile_period;
task5.elapsedTime = 0;
task5.TickFct = &enemyProjectileLogic;

//load sprites / custom images
CustomCharacter(0x00, playerShip);
CustomCharacter(0x01, enemyShip);
CustomCharacter(0x02, playerBeam);
CustomCharacter(0x03, enemyBeam);

TimerSet(GCD);
TimerOn();
LCD_init();
ADC_init();
unsigned short i;
while (1) {
	for (i = 0; i < numTasks; i++) {
		if (tasks[i]->elapsedTime == tasks[i]->period) {
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += 1;
	}
	while (!TimerFlag);
	TimerFlag = 0;
}
return 0;
}
