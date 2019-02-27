#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

#define START 0
#define STOP 1
#define KEY 2
#define TEMPO 3
#define VOLUME 4

char *DAC_OUTPUT = (char *) 0x4000741C;
int LEADER = 0;
int freqind[32] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
int periods[25] = {2024, 1908, 1805, 1701, 1608, 1515, 1433, 1351, 1276, 1205, 1136, 1073, 1012, 956, 903, 852, 804, 759, 716, 676, 638, 602, 568, 536, 506};
float beats[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 0.5, 0.5, 0.5, 0.5, 1, 1, 0.5, 0.5, 0.5, 0.5, 1, 1, 1, 1, 2, 1, 1, 2};

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
	int killed;
	int count;
	int key;
} Sound;

typedef struct {
	Object super;
    int tempo;
	int count;
	int killed;
	int started;
} Melody;

App app = { initObject(), 0, 0, 0, '\0' };

Sound s = { initObject(), 1136, 10, 0, 0, 100, 100, 0 };
Sound b = { initObject(), 1300, 5, 0, 1000, 1300, 1300 };
Melody m = { initObject(), 120, 0};

void tone_generator(Sound *self, int unused);
void background_generator(Sound *self, int unused);
void load_control(Sound *self, char inp);
void deadline_control(Sound *self, int unused);
void volume_control(Sound *self, char inp);
void key_control(Sound *self, char inp);
void tempo_control(Melody *self, int tempo);
void print_periods(int key);
void reader(App*, int);
void receiver(App*, int);
void kill_tone(Sound *self, int unused);
void set_tempo(Melody *self, int t);
void set_key(Sound *self, int key);
void unkill(Sound *self, int unused);
void unkill_player(Melody *self, int unused);
void kill_player(Melody *self, int unused);
void melody_player(Melody *self, int unused);
void prepare_can_msg(CANMsg *self, int value);
void start_player(Melody *self, int unused);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

void start_player(Melody *self, int unused) {
	unkill_player(self, 0);
	if(self->started) {
		ASYNC(self, melody_player, 0);
	}
	else {
		ASYNC(&s, tone_generator, 0);
		ASYNC(self, melody_player, 0);
	}
}

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
	if(!LEADER) {
		if(msg.msgId == START) {
			start_player(&m, 0);
		}
		else if(msg.msgId == STOP) {
			kill_player(&m, 0);
		}
		else if(msg.msgId == KEY) {
			int key = msg.buff[0];
			set_key(&s, key);
		}
		else if(msg.msgId == TEMPO) {
			int tempo = msg.buff[0];
			set_tempo(&m, tempo);
		}
		else if(msg.msgId == VOLUME) {
			switch(msg.buff[0]) {
				case 0:
					volume_control(&s, 'd');
				case 1:
					volume_control(&s, 'u');
				default:
					SCI_WRITE(&sci0, "Blabla ");
			}
			SCI_WRITE(&sci0, "Can msg received: ");
			SCI_WRITECHAR(&sci0, msg.msgId);
			SCI_WRITE(&sci0,"\n");
		}
	}
}

void reader(App *self, int c) {
    //SCI_WRITE(&sci0, "Rcv: \'");
	SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0,"\n");
	CANMsg msg;
	
	if(c == 't') {
		self->inpStr[self->count] = '\0';
		int tempo = atoi(self->inpStr);
		self->count = 0;
		
		if(tempo > 59 && tempo < 100) {
			msg.buff[0] = self->inpStr[0];
			msg.buff[1] = self->inpStr[1];
			msg.buff[2] = 0;
			msg.length = 3;
		}
		else if(tempo >= 100 && tempo <= 250) {
			msg.buff[0] = self->inpStr[0];
			msg.buff[1] = self->inpStr[1];
			msg.buff[2] = self->inpStr[2];
			msg.buff[3] = 0;
			msg.length = 4;
		}
		
		msg.msgId = '3';
		msg.nodeId = 1;
		//SYNC(&m, tempo_control, tempo);
		CAN_SEND(&can0, &msg);
		
		if(LEADER) {
			set_tempo(&m, tempo);
		}
	}
	
	else if (c == 'k') {
		
		self->inpStr[self->count] = '\0';
		int key = atoi(self->inpStr);

		if(key < 0) {
			msg.buff[0] = self->inpStr[0];
			msg.buff[1] = self->inpStr[1];
			msg.buff[2] = 0;
			msg.length = 3;
		}
		else {
			msg.buff[0] = self->inpStr[0];
			msg.buff[1] = 0;
			msg.length = 2;
		}
		
		msg.msgId = '4';
		msg.nodeId = 1;
		self->count = 0;
		CAN_SEND(&can0, &msg);
		if(LEADER) {
			set_key(&s, key);
		}
	}
	else if(c == 'p') {
		
		msg.msgId = '5';
		msg.nodeId = 1;
		msg.length = 1;
		msg.buff[0] = 0;
		CAN_SEND(&can0, &msg);
		if(LEADER) {
			start_player(&m, 0);
		}
	}
	else if(c == 's') {

		msg.msgId = '6';
		msg.nodeId = 1;
		msg.length = 1;
		msg.buff[0] = 0;
		CAN_SEND(&can0, &msg);
		if(LEADER) {
			kill_player(&m, 0);
		}
	}

	else if(c == 'l') {
		if(LEADER) {
			LEADER = 0;
		}
		else {
			LEADER = 1;
		}
	}

	else if(c == 'F') {
		self->runSum = 0;
		SCI_WRITE(&sci0, "<The running sum is 0\n");
	}
	else if(c == 'u' || c == 'd' || c == 'm') {
		if(LEADER) {
			SYNC(&s, volume_control, c);
		}
		//volume_control(&s, c);
	}

	else if(c == 'b' || c == 'v') {
		SYNC(&b, load_control, c);
		//load_control(&b, c);
	}
    else if (c == 'x') {
		SYNC(&s, deadline_control, c);
		SYNC(&b, deadline_control, c);
        //deadline_control(&s, c);
        //deadline_control(&b, c);
    }

	else {
		self->inpStr[self->count] = c;
		self->count++;
	}
}

void volume_control(Sound *self, char inp) {

	switch(inp) {
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

void tempo_control(Melody *self, int tempo) {
	if(tempo > 0)
		set_tempo(self, tempo);
	else
		SCI_WRITE(&sci0, "Tempo out of bounds [60, 240]\n");
}

void background_generator(Sound *self, int unused) {
	for(int i = 0; i <= self->background_loop_range; i++) {

	}
	SEND(USEC(self->period), USEC(self->deadline), self, background_generator, 0);
}

void tone_generator(Sound *self, int unused) {
    // Check if tone is killed
	if(!self->killed) {
		if(*DAC_OUTPUT) {
			*DAC_OUTPUT = 0;
		}
		else {
			*DAC_OUTPUT = self->volume;
		}
	}
	AFTER(USEC(self->period), self, tone_generator, 0);
}
void set_period(Sound *self, int p){
	self->period = periods[freqind[p] + self->key + 10];
}
	
void set_key(Sound *self, int k){
	self->key = k - 5;
}

void set_tempo(Melody *self, int t) {
	self->tempo = t;
}

void melody_player(Melody *self, int unused) {

	self->started = 1;
	
	if(!self->killed) {
		
		// Disable killing of tone
		unkill(&s, 0);
		set_period(&s, self->count);
		
		// After the beat period - 50 kill the current tone
		AFTER(MSEC((60000/self->tempo) * beats[self->count] - 50), &s, kill_tone, 0);

	
		// After beat period call melody_player again but with new tone
		AFTER(MSEC((60000/self->tempo) * beats[self->count]), self, melody_player, 0);
		// Increment the counting of tone indices
		self->count = (self->count + 1) % 32;
	}
}

void unkill(Sound *self, int unused) {
	self->killed = 0;
}

void kill_tone(Sound *self, int unused) {
	self->killed = 1;
}

void kill_player(Melody *self, int unused) {
	self->killed = 1;
	self->count = 0;
	kill_tone(&s, 0);
}

void unkill_player(Melody *self, int unused) {
	self->killed = 0;
}

void startApp(App *self, int arg) {

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    //SCI_WRITE(&sci0, "Hello, hello...\n");

	//ASYNC(&m, melody_player, 0);
	//ASYNC(&s, tone_generator, 0);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
