#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "/Users/antho_000/Documents/Atmel Studio/7.0/header files/timer.h"

typedef struct task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
};

#define led_output PORTA

ennum Buttton_State 
int Busson_Sm ()

enum LED_States{LED_Off, LED_On};

int LED_SM(int state) {
	switch(state) {
		case LED_On:
    		led_output = 0x01;
    		state = LED_Off;
    		break;
		case LED_Off:
		default:
    		led_output = 0x00;
    		state = LED_On;
    		break;
	}
	return state;
}

int main() {
	DDRA = 0xFF; PORTA = 0x00;
	
	const unsigned long timerGCD = 50;
	const unsigned char numTask = 1;
	unsigned char i;
	
	TimerSet(timerGCD);
	TimerOn();
	
	struct task *taskArry[numTask];
	
	struct task task1 = (struct task){LED_On, 500, 0, &LED_SM};
	taskArry[0] = &task1;
	
	while(1){
		for(i = 0; i < numTask; ++i){
			if(taskArry[i]->elapsedTime >= taskArry[i]->period){
				taskArry[i]->state = taskArry[i]->TickFct(taskArry[i]->state);
				taskArry[i]->elapsedTime = 0;
			}
			taskArry[i]->elapsedTime += timerGCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
}