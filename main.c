/*
 * customproject.c
 *
 * Created: 2/27/2018 2:49:22 PM
 * Author : Charlie
 */ 
#include <avr/io.h>
#include <bit.h>
#include "io.c"
#include "seven_seg.h"
#include "ADC.h"
#include "CustomCharacters.h"
#include <avr/interrupt.h>
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
unsigned char oldEnemyPosition = 0x00;
unsigned char playerAttack = 0; /* boolean flags */
unsigned char enemyAttack = 0; /*  for checking if there is a projectile out already */
unsigned char playerProjectilePosition = 0x00; //start from the player's position and move upward
unsigned char enemyProjectilePosition = 0x00; //start from the enemy's position and move downwards
unsigned char Score = 0x00; //score starts at 0, award 1 pt per hit
unsigned char lives = 3; //start with 2 lives
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

//user-defined FSMs
enum gameStates {init, menu, gameLoop, victory, defeat};
enum playerStates {move, updatePlayerLCD};
enum playerProjectile {updatePlayerProjectile};
enum enemyStates {calculateChoice};
enum enemyProjectile {updateEnemyProjectile};
enum enemyLocation {updateEnemyLocation};	
	
int gameLogic(int state) {
	select = ~PINA & 0x10;
	reset = ~PINA & 0x20;
	adc_resultX = Read_ADC(2);
	adc_resultY = Read_ADC(3);
	switch (state) { //Transitions
		case init:
		break;
		case menu:
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
			else {
				state = menu;
			}
			
		break;
		case gameLoop:
		if (reset) {
			state = init;
			break;
		}
		if (Score >= 5) {
			state = victory;
			Write7Seg(Score);
			PORTC = ~PORTC;
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
			Write7Seg(Score);
			PORTC = ~PORTC;
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
		oldEnemyPosition = 0;
		playerProjectilePosition = 0x00; //start from the player's position and move upward
		enemyProjectilePosition = 0x00; //start from the enemy's position and move downwards
		playerAttack = 0;
		enemyAttack = 0;
		Score = 0x00; //high score starts at 0, award 1 pt per hit
		lives = 3; //3 lives
		select = 0x00;
		inGame = 0x00; //check if still in game loop
		adc_resultX = 0x0000; //joystick input horizontal
		adc_resultY = 0x0000; //joystick input vertical
		PORTB = 0x00;
		LCD_WriteCommand(0x0C);
		LCD_ClearScreen();
		LCD_DisplayString(1, "PRESS BTN 2 PLAY  HIGH SCORE:");
		uint8_t highestScore;
		highestScore = eeprom_read_byte((uint8_t*)10);
		LCD_Cursor(30);
		LCD_WriteData((char)highestScore + '0');
		Write7Seg(Score);
		PORTC = ~PORTC;
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
			++Score;
			Write7Seg(Score);
			PORTC = ~PORTC;
		}
		if (enemyProjectilePosition == playerPosition) {
			--lives;
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			enemyAttack = 0;
			enemyProjectilePosition = 0;
		}
		if (playerProjectilePosition == 17 || playerProjectilePosition <= 0) {
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(playerProjectilePosition + 1);
			LCD_WriteData(32);
			playerAttack = 0;
			playerProjectilePosition = 0;
		}
		if (enemyProjectilePosition >= 33 || enemyProjectilePosition == 16) {
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			enemyAttack = 0;
			enemyProjectilePosition = 0;
		}
		if (lives == 3) {
			PORTB = 0x07;
		}
		else if (lives == 2) {
			PORTB = 0x06;
		}
		else if (lives == 1) {
			PORTB = 0x04;
		}
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
		if (adc_resultY >= up && (playerPosition != 13 && playerPosition != 29)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			--playerPosition;
			LCD_Cursor(playerPosition);
			state = updatePlayerLCD;
		}
		else if (adc_resultY <= down && (playerPosition != 16 && playerPosition != 32)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			++playerPosition;
			LCD_Cursor(playerPosition);
			state = updatePlayerLCD;
		}
		else if (adc_resultX >= right && (playerPosition != 16 && playerPosition != 15 && playerPosition != 14 && playerPosition != 13)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			playerPosition -= 16;
			LCD_Cursor(playerPosition);
			state = updatePlayerLCD;
		}
		else if (adc_resultX <= left && (playerPosition != 32 && playerPosition != 31 && playerPosition != 30 && playerPosition != 29)) {
			LCD_Cursor(playerPosition);
			LCD_WriteData(32);
			playerPosition += 16;
			LCD_Cursor(playerPosition);
			state = updatePlayerLCD;
		}
		}
		break;
		case updatePlayerLCD:
		if (inGame) {
		LCD_Cursor(playerPosition);
		LCD_WriteData(0x00);
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
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(32);
			LCD_Cursor(playerProjectilePosition + 1);
			LCD_WriteData(32);
			--playerProjectilePosition;
			LCD_Cursor(playerProjectilePosition);
			LCD_WriteData(0x02);
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
		case calculateChoice:
		break;
		default:
		state = calculateChoice;
		break;
	}
	switch(state) {
		case calculateChoice:
		//random number between 0 and 100
		//if number is <40 and there's no projectile currently, then attack
		//if number is <40 and there's a projectile, move up or down (depending on where it is)
		//if number is >=40 and there's no projectile, either move or attack (50% chance for either)
		//if number is >=40 and there's a projectile, do below:
		//move to one of 3 surrounding spots on the following conditions:
		//if number % 3 == 0, move to spot top/bottom left/right
		//if number % 5 == 0, move to left or right
		//if the number fails to satisfy either condition, such as '49', then move 
		//left if possible, otherwise move right then move back
		if (inGame) {
				randomNumber = rand() % 101; //random number between 0 and 100
				if (randomNumber < 40 && enemyAttack == 0) {
					enemyAttack = 1;
					enemyProjectilePosition = enemyPosition + 2;
				}
				else if (randomNumber < 40 && enemyAttack) {
					if (enemyPosition == 1) {
						oldEnemyPosition = enemyPosition;
						enemyPosition += 16;
					}
					else if (enemyPosition == 2) {
						oldEnemyPosition = enemyPosition;
						enemyPosition += 16;
					}
					else if (enemyPosition == 17) {
						oldEnemyPosition = enemyPosition;
						enemyPosition -= 16;
					}
					else if (enemyPosition == 18) {
						oldEnemyPosition = enemyPosition;
						enemyPosition -= 16;
					}
				}
				else if (randomNumber >= 40 && enemyAttack == 0) {
					randomNumber = rand() % 2; //0 or 1
					if (randomNumber) {
						enemyAttack = 1;
						enemyProjectilePosition = enemyPosition + 2;
					} 
					else {
						if (randomNumber % 3 == 0 && randomNumber % 5 != 0) {
							if (enemyPosition == 17) {
								oldEnemyPosition = enemyPosition;
								enemyPosition = 2;
							}
							else if (enemyPosition == 18) {			
								oldEnemyPosition = enemyPosition;
								enemyPosition = 1;
							}
							else if (enemyPosition == 1) {		
								oldEnemyPosition = enemyPosition;
								enemyPosition = 18;
							}
							else if (enemyPosition == 2) {
								oldEnemyPosition = enemyPosition;
								enemyPosition = 17;
							}
						}
						else if (randomNumber % 3 != 0 && randomNumber % 5 == 0) {
							if (enemyPosition == 2 || enemyPosition == 18) {
								oldEnemyPosition = enemyPosition;
								--enemyPosition;
							}
							else if (enemyPosition == 1 || enemyPosition == 17) {								
								oldEnemyPosition = enemyPosition;
								++enemyPosition;
							}
						}
					}
					}
					else if (randomNumber >= 40 && enemyAttack) {
						if (randomNumber % 3 != 0 && randomNumber % 5 != 0) {
							if (enemyPosition >= 17) {
								oldEnemyPosition = enemyPosition;
								enemyPosition = enemyPosition - 16;
							}
							else if (enemyPosition <= 2) {
								oldEnemyPosition = enemyPosition;		
								enemyPosition = enemyPosition + 16;
							}
						}
						else if (randomNumber % 3 == 0 && randomNumber % 5 != 0) {
							if (enemyPosition == 17) {
								oldEnemyPosition = enemyPosition;
								enemyPosition = 2;
							}
							else if (enemyPosition == 18) {
								oldEnemyPosition = enemyPosition;				
								enemyPosition = 1;
							}
							else if (enemyPosition == 1) {
								oldEnemyPosition = enemyPosition;
								enemyPosition = 18;
							}
							else if (enemyPosition == 2) {
								oldEnemyPosition = enemyPosition;	
								enemyPosition = 17;
							}
						}
						else if (randomNumber % 3 != 0 && randomNumber % 5 == 0) {
							if (enemyPosition == 2 || enemyPosition == 18) {
								oldEnemyPosition = enemyPosition;					
								--enemyPosition;
							}
							else if (enemyPosition == 1 || enemyPosition == 17) {
								oldEnemyPosition = enemyPosition;
								++enemyPosition;
							}
						}
				}
		}
		break;
		default:
		state = calculateChoice;
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
		if (enemyAttack){
			LCD_Cursor(enemyProjectilePosition - 1);
			LCD_WriteData(32);
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(32);
			++enemyProjectilePosition;
			LCD_Cursor(enemyProjectilePosition);
			LCD_WriteData(0x03);
		}
		state = updateEnemyProjectile;
		}
		break;
		default:
		state = updateEnemyProjectile;
		break;
	}
	
	return state;
}

int enemyLocationUpdate(int state) {
	switch (state) {
		case updateEnemyLocation:
		state = updateEnemyLocation;
		break;
		default:
		state = updateEnemyLocation;
		break;
	}
	
	switch (state) {
		case updateEnemyLocation:
		if (inGame) {
			if (oldEnemyPosition != enemyPosition) {
				LCD_Cursor(oldEnemyPosition);
				LCD_WriteData(32);
				LCD_Cursor(enemyPosition);
				LCD_WriteData(0x01);
			}
			state = updateEnemyLocation;
		}
		break;
		default:
		state = updateEnemyLocation;
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
unsigned long int player_calc = 25;
unsigned long int enemy_calc = 200;
unsigned long int playerProjectile_calc = 25;
unsigned long int enemyProjectile_calc = 25;
unsigned long int updateEnemyLocation_calc = 100;

unsigned long int tmpGCD = 1;
tmpGCD = findGCD(player_calc, enemy_calc);
tmpGCD = findGCD(tmpGCD, playerProjectile_calc);
tmpGCD = findGCD(tmpGCD, game_calc);
tmpGCD = findGCD(tmpGCD, enemyProjectile_calc);
tmpGCD = findGCD(tmpGCD, updateEnemyLocation_calc);

unsigned long int GCD = tmpGCD;

unsigned long int player_period = player_calc/GCD;
unsigned long int enemy_period = enemy_calc/GCD;
unsigned long int playerProjectile_period = playerProjectile_calc/GCD;
unsigned long int game_period = game_calc/GCD;
unsigned long int enemyProjectile_period = enemyProjectile_calc/GCD;
unsigned long int updateEnemyLocation_period = updateEnemyLocation_calc/GCD;

static task task1, task2, task3, task4, task5, task6;
task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6 };
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
task3.state = -1;
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

//update enemy location
task6.state = updateEnemyLocation;
task6.period = updateEnemyLocation_period;
task6.elapsedTime = 0;
task6.TickFct = &enemyLocationUpdate;

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