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

/*********************************** Function and method delcaration ***********************************/
void tone_generator(Sound *self, int unused);
void background_generator(Sound *self, int unused);
void load_control(Sound *self, char inp);
void deadline_control(Sound *self, int unused);
void volume_control(Sound *self, char inp);
void key_control(Sound *self, char inp);
void print_periods(int key);
void reader(App*, int);
void receiver(App*, int);
void kill_tone(Sound *self, int unused);
void set_tempo(Melody *self, int t);
void set_key(Sound *self, int key);
void set_period(Sound *self, int p);
void unkill(Sound *self, int unused);
void unkill_player(Melody *self, int unused);
void kill_player(Melody *self, int unused);
void melody_player(Melody *self, int unused);
void prepare_can_msg(CANMsg *self, int value);
void start_player(Melody *self, int unused);
void canon_controler(Melody *self, int unused);
void canon_send(CANMsg *self, int unused);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

/* The recevier method for the CAN bus */
void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
	
	if(!LEADER) {
		if(msg.nodeId == 2 || msg.nodeId == 0) {
			if(msg.msgId == START) {
				SYNC(&m, start_player, 0);
			}
			else if(msg.msgId == STOP) {
				SYNC(&m, kill_player, 0);
			}
			else if(msg.msgId == KEY) {
				int key = msg.buff[0];
				SYNC(&s, set_key, key);
			}
			else if(msg.msgId == TEMPO) {
				int tempo = msg.buff[0];
				SYNC(&m, set_tempo, tempo);
			}
			else if(msg.msgId == VOLUME) {
				switch(msg.buff[0]) {
					case 0:
						SYNC(&s, volume_control, 'd');
					case 1:
						SYNC(&s, volume_control, 'u');
					default:
						SCI_WRITE(&sci0, "Default ");
				}
				SCI_WRITE(&sci0, "Can msg received: ");
				SCI_WRITECHAR(&sci0, msg.msgId);
				SCI_WRITE(&sci0,"\n");
			}
		}
	}
}

/*********************************** Reader for controlling the keyboad ***********************************/

/* This is the function that handles keyboard inputs */
void reader(App *self, int c) {

	SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0,"\n");
	CANMsg msg;
	
	// Controlling the tempo
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
		CAN_SEND(&can0, &msg);
		
		if(LEADER) {
			SYNC(&m, set_tempo, tempo);
		}
	}
	
	// Controlling the key 
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
			SYNC(&s, set_key, key);
		}
	}
	else if(c == 'p') {
		if(LEADER) {
			SYNC(&m, start_player, 0);
			SYNC(&m, canon_controler, 0);
		}
	}
	
	// Controlling the stop
	else if(c == 's') {

		msg.msgId = '6';
		msg.nodeId = 1;
		msg.length = 1;
		msg.buff[0] = 0;
		CAN_SEND(&can0, &msg);
		if(LEADER) {
			SYNC(&m, kill_player, 0);
		}
	}

	// Set the leader mode
	else if(c == 'l') {
		if(LEADER) {
			LEADER = 0;
		}
		else {
			LEADER = 1;
		}
	}
	
/****************************** Old controlling functions ******************************/
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

/*********************************** Melody and tone generators ***********************************/

/* This is the tone gernerator mehod */
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
	// After the correct period we call this tone_generator again
	AFTER(USEC(self->period), self, tone_generator, 0);
}

/* This is the melody player that controls the tone_generator, giving it the correct beat */
void melody_player(Melody *self, int unused) {

	// Used for starting and stopping the player
	self->started = 1;
	
	// If user has pressed the stop button, won't play anymore
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

/*********************************** Methods for controlling ***********************************/

void start_player(Melody *self, int unused) {
	SYNC(self, unkill_player, 0);
	
	if(self->started) {
		SYNC(self, melody_player, 0);
	}
	else {
		SYNC(&s, tone_generator, 0);
		SYNC(self, melody_player, 0);
	}
}

void set_period(Sound *self, int p) {
	self->period = periods[freqind[p] + self->key + 10];
}
	
void set_key(Sound *self, int k){
	self->key = k - 5;
}

void set_tempo(Melody *self, int t) {
	self->tempo = t;
}

/* This was ment to control the canon of the system */
void canon_controler(Melody *self, int unused) {
	// Initializing the variables for delays and can msg
	int delay_board1;
	int delay_board2;
	CANMsg msg;

	msg.msgId = 0;
	msg.nodeId = 1;
	msg.length = 1;
	msg.buff[0] = 0;

	// Calculate the delays for the other slave boards
	delay_board1 = (60000/self->tempo) * (beats[self->count] * 8) - 50;
	delay_board2 = (60000/self->tempo) * (beats[self->count] * 4) - 50;
	
	// Start the tone generator on the other boards
	AFTER(MSEC(delay_board1), &msg, canon_send, 0);
	AFTER(MSEC(delay_board2), &msg, canon_send, 0);
}

/* Method for sending out can message */
void canon_send(CANMsg *self, int unused) {
	CAN_SEND(&can0, self);
}

/* Method for unkilling the tone generator */
void unkill(Sound *self, int unused) {
	self->killed = 0;
}

/* Method for killing the tone generator */
void kill_tone(Sound *self, int unused) {
	self->killed = 1;
}

/* Method for killing the melody player */
void kill_player(Melody *self, int unused) {
	self->killed = 1;
	self->count = 0;
	kill_tone(&s, 0);
}

/* Method for unkilling the melody player */
void unkill_player(Melody *self, int unused) {
	self->killed = 0;
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

/*********************************** Functions and methods for background and old stuff ***********************************/

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

void background_generator(Sound *self, int unused) {
	for(int i = 0; i <= self->background_loop_range; i++) {

	}
	SEND(USEC(self->period), USEC(self->deadline), self, background_generator, 0);
}

/*********************************** Starting methods for the system ***********************************/

/* Main method for starting the system */
void startApp(App *self, int arg) {

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
	
    //SCI_WRITE(&sci0, "Hello, hello...\n");
	//ASYNC(&m, melody_player, 0);
	//ASYNC(&s, tone_generator, 0);
}

/* Main function for installing interrupt handerls and starting TinyTimber*/
int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
