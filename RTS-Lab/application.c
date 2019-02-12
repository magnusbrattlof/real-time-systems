#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>


char *DAC_OUTPUT = (char *) 0x4000741C;

int freqind[32] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
int periods[25] = {2024, 1908, 1805, 1701, 1608, 1515, 1433, 1351, 1276, 1205, 1136, 1073, 1012, 956, 903, 852, 804, 759, 716, 676, 638, 602, 568, 536, 506};

typedef struct {
    Object super;
    int count;
    int myNum;
	int runSum;
	char c;
	char inpStr[50];
	char bufNum[50];
	char bufSum[50];
} App;

typedef struct {
	Object super;
	int period;
	int volume;
	int mute;
    int background_loop_range;
    int deadline;
    int const_deadline;
} Sound;

App app = { initObject(), 0, 0, 0, '\0' };

Sound s = { initObject(), 500, 5, 0, 0, 100, 100 };
Sound b = { initObject(), 1300, 5, 0, 1000, 1300, 1300 };

void tone_generator(Sound *self, int unused);
void background_generator(Sound *self, int unused);
void load_control(Sound *self, char inp);
void deadline_control(Sound *self, char unused);
void volume_control(Sound *self, char inp);
void print_periods(int key);
void reader(App*, int);
void receiver(App*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    //SCI_WRITE(&sci0, "Rcv: \'");
	SCI_WRITECHAR(&sci0, c);
	if(c == 'e'){

		self->inpStr[self->count] = '\0';
		self->myNum = atoi(self->inpStr);
		self->runSum = self->runSum + self->myNum;
		snprintf(self->bufNum, 50, "%d\n", self->myNum);
		snprintf(self->bufSum, 50, "%d\n", self->runSum);

		SCI_WRITE(&sci0, "\n");

		SCI_WRITE(&sci0,"The entered number is: ");
		SCI_WRITE(&sci0, self->bufNum);
		SCI_WRITE(&sci0,"\n");

		//SCI_WRITE(&sci0,"The running number is: ");
		SCI_WRITE(&sci0,self->bufSum);
		//SCI_WRITE(&sci0,"\n");

		int key = atoi(self->bufSum);
		print_periods(key);
		self->count = 0;
	}

	else if(c == 'F') {
		self->runSum = 0;
		SCI_WRITE(&sci0, "<The running sum is 0\n");
	}
	else if(c == 'u' || c == 'd' || c == 'm') {
		volume_control(&s, c);
	}

	else if(c == 'b' || c == 'v') {
		load_control(&b, c);
	}
    else if (c == 'd') {
        deadline_control(&s, c);
        deadline_control(&b, c);
    }

	else {
		self->inpStr[self->count] = c;
		self->count++;
	}
}

void volume_control(Sound *self, char inp) {

	switch(inp){
        // UP - increment the volume by one
		case 'u':
			if(self->volume < 20)
				self->volume += 1;
			break;
        // DOWN - decrement the volume by one
		case 'd':
			if(self->volume > 5)
				self->volume -= 1;
			break;
        // MUTE or UNMUTE - mute the sound
		case 'm':
			if(!self->volume)
				self->volume = 5;
			else
				self->volume = 0;
			break;

		default:
			SCI_WRITE(&sci0, "Enter another character.\n");
		}
	}

void load_control(Sound *self, char inp) {
	char new_blr[50];

    switch(inp) {
        // Increases the background_loop with 500
		case 'b':
			self->background_loop_range += 500;
			snprintf(new_blr, 50, "%d\n", self->background_loop_range);
			SCI_WRITE(&sci0, new_blr);
			SCI_WRITE(&sci0, "\n");
			break;
        // Decreases the background_loop with 500
		case 'v':
			self->background_loop_range -= 500;
			snprintf(new_blr, 50, "%d\n", self->background_loop_range);
			SCI_WRITE(&sci0, new_blr);
			SCI_WRITE(&sci0, "\n");
			break;

		default:
			SCI_WRITE(&sci0, "Enter another character.\n");
		}
	}

void deadline_control(Sound *self, int unused) {
	if(self->deadline) {
        self->deadline = 0;
    }
	else {
        self->deadline = self->const_deadline;
    }
}

void tone_generator(Sound *self, int unused) {
	if(*DAC_OUTPUT) {
		*DAC_OUTPUT = 0;
	}
	else {
		*DAC_OUTPUT = self->volume;
	}

	SEND(USEC(self->period), USEC(self->deadline), self, tone_generator, 0);
}

void background_generator(Sound *self, int unused) {
	for(int i = 0; i <= self->background_loop_range; i++) {

	}
	SEND(USEC(self->period), USEC(self->deadline), self, background_generator, 0);
}

void benchmark_background() {
    int i, j;
    Time t1, t2, result;
    Time max = 0;
    Time average = 0;

    for(i = 0; i < 500; i++) {
        t1 = CURRENT_OFFSET();
        for(j = 0; j < 1000; j++) {
            // EMPTY LOOP
        }
        t2 = CURRENT_OFFSET();
        result = t2 - t1;
        average += result;

        if(result > max) {
            max = result;
        }
    }
    average = average / 500;
    
}

void benchmark_tone() {
    int i;
    for(i = 0; i < 500; i++) {

    }
}



void print_periods(int key) {

	int i;
	int new_num;
	char number[50];
	char bufIndex[50];
	key += 10;

	for(i = 0; i <= 31; i++) {
		new_num = periods[freqind[i] + key];
		snprintf(number, 50, "%d\t", new_num);
		snprintf(bufIndex, 50, "%d\n", freqind[i]);

		SCI_WRITE(&sci0, bufIndex);
		SCI_WRITE(&sci0,  number);

	}
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");


	tone_generator(&s, 0);
	background_generator(&b, 0);

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
