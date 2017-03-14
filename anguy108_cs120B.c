#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/io.c"
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/usart_ATmega1284.h"
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/bit.h"
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/keypad.h"
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/timer.h"


typedef struct task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
};

struct Bucket{
	unsigned char x_loc;
	unsigned char y_loc;
};

/*global variables
*
*/
const unsigned char score2win = 50;
unsigned char pressA;

//player's status
unsigned char score;
unsigned char lifepts;

//i.o.
unsigned char keypad_input;
unsigned char arduino_receive;

//position of the buckets
struct Dog bucket_array[4];

/*The following state machines corresponds to the Atmega
*communicating with the Arduino. 
*/

enum Send_States{send_start, send_release, send_press};


int Send_SM(int state){
	static unsigned char data;
	switch(state)
	{
		case send_release:
			if(keypad_input != '\0'){//if some sort of button is pressed on the keypad
				if(USART_IsSendReady(0)){//if there is not data in the buffer
					USART_Send(keypad_input, 0);//send keypad button
					state = send_press;			
				}	
			}
			break;
		case send_press:
			//if there is not button pressed, go back to send_start
			//purpose of this is to prevent glitch from happening
			state = (keypad_input != '\0') ? send_press : send_start;
			break;
		default:
		case send_start:
			//initial FSM
			state = send_release;
			break;
	}
		return state;
}


/*The following state machines correspond to the arduino communicating with the
*Atmega the condition of the objects. The conditions include:
*			-How many buckets jumped over the balls; thus incrementing accordingly
*			-How many buckets hit a ball; thus decremementing accordingly
*/

enum Receive_States{receive_start, receive_sample};

int Receive_SM(int state){
	switch(state){
		case receive_sample:
			if(USART_HasReceived(0)){
				arduino_receive = USART_Receive(0);//unsigned char
				USART_Flush(0);
				/*if(input == 0x01){
					//
				}*/
			}
			break;
		default:
		case receive_start:
			arduino_receive = 0x00;
			break;
	}
	switch(state){
		default:
		case receive_start:
		case receive_sample:
			state = receive_sample;
			break;
	}
	return state;
}


/*The following state machine corresponds to the keypad input.
*Menu: A = Start
*Game: D = Jump; 1st Dog: Left(1), Right(3)
*				 2nd Dog: Left(4), Right(6)
*				 3rd Dog: Left(7), Right(9)
*				 4th Dog: Left(), Right()
*	   C = Reset all buckets to default column
*/

unsigned char keypad_input;

enum Keypad_States{KeypadStart, KeypadWait};

int Keypad_SM(int state){
	//transitions
	switch(state)
	{
		case KeypadWait:
			keypad_input = GetKeypadKey();//keypad.h
			state = KeypadStart;//clock cycle to prevent glitch
			break;
		default:
		case KeypadStart:
			state = KeypadWait;
			break;
	}
	return state;
}

/*This following state machine corresponds to the rules of the actual game
*ga
*/

enum Game_States{Game_start, Game_menu, Game_play, Game_win, Game_lose};

int Game_SM(int state){
	switch(state){
		case Game_menu:
			/
			break;
		case Game_play:
			break;
		case Game_win:
			break;
		case Game_lose:
			break;
		default:
		case Game_start:
			state = Game_menu;
			break;
	}
	return state;
}


/*The following state machines will be corresponding to the
*following LED for whole game. The LEDs will light in in a pattern
*using the shift register. Once the game starts, the player will have a 
*total of 8 lives. Each time they lose a life, one LED would turn off
*if the user loses, nothing will happen and the LED will reset to the pattern
*on the start menu. If the user wins, then the the whole LED row will light up
*/

// Ensure DDRC is setup as output
void transmit_data ( unsigned short data ) {//function allows for simple use of shift registers
	int i;
	//change i = 9 if can get other shift register fixed
	for ( i = 9; i >= 0; --i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= (( data >> i ) & 0x0001);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x04;
	}
	// set RCLK = 1. Rising edge copies data from the “Shift” register to the “Storage” register
	PORTB |= 0x02;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}

//unsigned short led_pattern[] = {0, 129, 66, 36, 24}; // adding a vlue in [0] because of glitch
unsigned short led_pattern[] = {0, 513, 258, 132, 72, 48};//predefined values corresponding to LED
const unsigned char led_patternNum = 6; //---------------------------change to 6 if have 2nd shift register------------------------------
const unsigned short led_max = 1023;//255; 
const unsigned short led_min = 0;

enum LED_States{Led_Start, Led_Wait, Led_Game, Led_Win, Led_Lose};

int LED_SM(int state){
	static unsigned char led_index;
	static unsigned char dir;//used as a flag to simulate "direction" of the led shifting
	static unsigned short led_score;
	static unsigned char led_toggle;
	//unsigned char inputB = ~PIND & 0x0F; //-----------------------------------------------debugging purposes-------------------------------
	//action
	switch(state){
		case Led_Wait:
			if(dir){
				++led_index;
				if(led_index == led_patternNum){
					led_index -= 2; // -=2 so that LED doesn't "hold/freeze"
					dir = 0;
				}
			
			}
			else{
				--led_index;
				if(led_index == 0){
					led_index += 2; // +=2 so that LED doesn't "hold/freeze"
					dir = 1;
				}
			
			}
			transmit_data(led_pattern[led_index]);//function utilizing shift registers
			break;
		case Led_Game:
			if((arduino_receive >= 0x05) && (arduino_receive <= 0x08)){//any value between 5 and 8 is a key for decrementing life points
			//if(arduino_receive == 0x02){
				//masking the values so that values 5 through 8 can be valued 1 to 4.
				//the max lifepoints that can be decremented is 4 (because there is a max of 4 buckets)
				arduino_receive -= 4; 
				for(int i = 0; i < arduino_receive; ++i){
					--lifepts; //decrement lifepts
					led_score = led_score - (1 << lifepts);
					//add "invisible power-up"
					transmit_data(led_score);
					if(score > score2win){
						state = Led_Win;
						break;
					}
					else if(lifepts == 0){
						state = Led_Lose;
						break;
					}
				}
			//-------------------debugging purposes----------------------------------------------------------------------------------------------------
			//if(keypad_input == '2'){
				//--lifepts; 
			}
			//PORTD = 0X0F & PIND;
			break;
		case Led_Win:
			if(led_toggle % 10 == 0){//every 10 count turn off all LED
				transmit_data(led_min);
			}
			else if(led_toggle % 5 == 0){//every 5 count turn on all LED
				transmit_data(led_max);
			}
			++led_toggle;
			break;
		case Led_Lose:
			transmit_data(led_min);
			++led_toggle;
			break;
		default:
		case Led_Start:
			led_index = 0;
			dir = 1;
			led_score = led_max; //255
			led_toggle = 1;
			lifepts = 8; //will be set by the actual game sm
			break;
	}
	//transition
	switch(state){
		case Led_Wait:
			//------------------debugging purposes------------------------------------------------------------------------------------------------
			if(pressA){
				state = Led_Game;
				pressA = 0;
				//keypad_input = 0x00;
			}
			break;
		case Led_Game:
			if(lifepts == 0){
				//wait until LCD give signal to change to Led_Start
				state = Led_Lose;
			}
			break;
		case Led_Win:
		case Led_Lose:
			if(led_toggle > 40){
				state = Led_Start;
				led_toggle = 0;
			}
			break;
		default:
		case Led_Start:
			state = Led_Wait;
		break;
	}
	return state;
}


/*The following state machines will be used for the LCD display
*This will allow this setup the menu and give the user instruction
*to when the game will start. In addition, this will keep track of the
*score and display the power ups
*/
//-------------------------------------------------------need to do: power ups, game over!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
char str[33] = "                                 ";
char str_clear[33] = "                                 ";
const unsigned char welcome[] = "    Welcome!    ";
const unsigned char instruction[] = "  Press 'A' to Start ";
const unsigned char score_screen[] = "Score: ";
const unsigned char pause[] = "     Pause      ";
const unsigned char lose[] = "    Gameover    ";
const unsigned char win[] = "    You Win!    ";
//power ups
const unsigned char invis[] = " Power up: Invisible! Don't lose lives                ";
const unsigned char invisSize = 54;
const unsigned char d_points[] = " Power up: Double Points!                ";
const unsigned char d_pointsSize = 41;
//power downs
const unsigned char horizonalFlip[] = "Power down: Horizonal Flip                ";
const unsigned char horizonalFlipSize = 42;
const unsigned char verticalFlip[] = "Power down: Upside Down                ";
const unsigned char verticalFlipSize = 39;

enum String_States{Menu_Start, Menu_Wait, Menu_Game, Menu_GameUpdate, Menu_Gameover, Menu_Win};

int String_SM(int state){
	static unsigned char front; static unsigned char dir;
	static unsigned char i;
	unsigned short numDivide;
	unsigned short scoreTemp; //keep the updated value
	unsigned char scoreOut; 
	//action
	switch(state){
		case Menu_Game:
			/*if(arduino_receive >= 0x01 && arduino_receive <= 0x04){
				for(int i = 0; i < arduino_receive; ++i){
					score += 10; //increment score
				}*/
			//if(arduino_receive >= 0x01){
			//if(keypad_input == '1'){
			//score += 10;
			++score;
				//power-up flag
				if(score <= score2win)
				{
					scoreTemp = score; //store value into scoreTemp so that score isn't being changed  these statements
					numDivide = 100; //initialize val to be divided
					
					//set up the digits for each place (100s, 10s, 1s)
					for(int x = 0; x < 3; ++x){
						//if statement as safety bound
						if(numDivide > 0){
							scoreOut = (char)(scoreTemp / numDivide); //divide to get the the number for that placement
							str[x+7] = '0' + scoreOut; //store the character into the right placement
							scoreTemp %= numDivide; //update scoreTemp to be the remaining
							numDivide /= 10; //update numDivide to the next digit placement
						}
					}
				}
			//}
			break;
		case Menu_GameUpdate:
			break;
		case Menu_Gameover:
			if(i < 40){
				if(i % 10 == 0){
					for(int x = 0; x < 33; ++x){
						str[x] = str_clear[x];
					}
				}
				else if(i% 5 == 0){
					for(int x = 0; x < 8; ++x){
						str[x+4] = lose[x+4];
					}
				}
				++i;
			}
			else{
				state = Menu_Start;
			}
			break;
		case Menu_Win:
			if(i < 40){
				if(i % 10 == 0){
					for(int x = 0; x < 33; ++x){
						str[x] = str_clear[x];
					}
				}
				else if(i% 5 == 0){
					for(int x = 0; x < 8; ++x){
						str[x+4] = win[x+4];
					}
				}
				++i;
			}
			else{
				state = Menu_Start;
			}
			break;
		case Menu_Wait:
			if(dir){
				/* <6 because added more spaces to compensate that front is unsigned
				*/
				if(front < 6){
					for(int x = 0; x < 16; ++x){
						//17 because want to start on 2nd line
						str[x+16] = instruction[front + x];
					}
					++front;
				}
				else{dir = 0; front = 5;}
			}
			else{
				if(front > 0){
					for(int x = 0; x < 16; ++x){
						//17 because want to start on 2nd line
						str[x+16] = instruction[front + x];
					}
					--front;
				}
				else{dir = 1;front = 1;}
			}
			break;
		default:
		case Menu_Start:
			//**Initialize the beginning str**
			//insert 'Welcome!' into str
			for(int x = 0; x < 8; ++x){
				str[x+4] = welcome[x+4];
			}
			front = 1; dir = 1; i = 0; pressA = 0; score = 0;
			break;
	}
	//transition
	switch(state){
		case Menu_Game:
			//if the player's score pass the thereshold to win
			if(score > score2win){
				if(USART_IsSendReady(0)){
					USART_Send(0x09, 0); //send the flag for winning
				}
				if(i < 40){
					if(i % 10 == 0){
						for(int x = 0; x < 33; ++x){
							str[x] = str_clear[x];
						}
					}
					else if(i % 5 == 0){
						for(int x = 0; x < 8; ++x){
							str[x+4] = win[x+4];
						}
					}
					++i;
				}
				else{
					state = Menu_Start;
				}
			}
			else if(lifepts == 0){
				USART_Send(0x0A, 0); //send the flag for losing
				state = Menu_Gameover;
			}
			break;
		case Menu_Gameover:
		case Menu_Win:
			if(i >= 40){
				state = Menu_Start;
			}
			break;
		case Menu_Wait:
			//Press A to Start
			if(keypad_input == 'A'){
				pressA = 1;
				LCD_ClearScreen();
				//clear current str
				for(int x = 0; x < 33; ++x){
					str[x] = str_clear[x];
				}
				//copy score string
				for(int x = 0; x < 7; ++x){
					str[x] = score_screen[x];
				}
				//print intial score
				for(int x = 0; x < 3; ++x){
					str[x+7] = '0';
				}
				state = Menu_Game;
				//initialize location for dogs
				for(int x = 0; x < 4; ++x){
					dog_array[x].x_loc = 1;
					bucket_array[x].y_loc = (x*2) + 1;
				}
				score = 0;
			}
			else{
				state = Menu_Wait;
			}
			break;
		default:
		case Menu_Start:
			state = Menu_Wait;
			break;
	}
	return state;
}

enum LCD_States{LCDStart, LCD};

int LCD_SM(int state){
	
	//transition
	switch(state){
		case LCD:
			break;
		default:
		case LCDStart:
			state = LCD;
			break;
	}
	//action
	switch(state){
		case LCD:
			LCD_DisplayString(1, str);
			LCD_Cursor(0);
			break;
		default:
		case LCDStart:
			break;
	}
	return state;
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;
	const unsigned char numSize = 6;//7;
	const unsigned char periodGCD = 20;
	
	TimerSet(periodGCD);
	TimerOn();
	
	LCD_init();
	struct task *arry[numSize];
	
	initUSART(0);
	
	struct task task0 = (struct task){KeypadStart, 20, 0, &Keypad_SM};
	struct task task1 = (struct task){send_start, 40, 0, &Send_SM};
	struct task task2 = (struct task){receive_start, 40, 0, &Receive_SM};
	//struct task task3 = (struct task){Game_start,40, 0, &Game_SM};//This SM will be used to control the overall game
	struct task task3 = (struct task){Menu_Start, 400, 0, &String_SM};
	struct task task4 = (struct task){LCDStart, 400, 0, &LCD_SM};
	struct task task5 = (struct task){Led_Start, 200, 0, &LED_SM};
	
	arry[0] = &task0;
	arry[1] = &task1;
	arry[2] = &task2;
	arry[3] = &task3;
	arry[4] = &task4;
	arry[5] = &task5;
	//arry[6] = &task6;
	while(1){
		for(unsigned char i = 0; i < numSize; ++i){
			if(arry[i]->elapsedTime >= arry[i]->period){
				arry[i]->state = arry[i]->TickFct(arry[i]->state);
				arry[i]->elapsedTime = 0;
			}
			arry[i]->elapsedTime += periodGCD;
		}
		
		
		while(!TimerFlag);
		TimerFlag = 0;
	}
}
