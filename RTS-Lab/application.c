#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

int freqind[32] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0}
int periodsforkey0[32] = {1136,1012,902,1136,1136,1012,902,1136,902,851,758,902,851,758,758,675,758,851,902,1136,758,675,758,851,902,1136,1136,1517,1136,1136,1517,1136};

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

App app = { initObject(), 0, 0, 0, '\0' };

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
    SCI_WRITE(&sci0, "Rcv: \'");
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
		
		SCI_WRITE(&sci0,"The running number is: ");
		SCI_WRITE(&sci0,self->bufSum);
		SCI_WRITE(&sci0,"\n");
		self->count = 0;
		}
		
	else if(c == 'F'){
		self->runSum = 0;
		SCI_WRITE(&sci0, "<The running sum is 0\n");
		}
	else{
		self->inpStr[self->count] = c;
		SCI_WRITE(&sci0, "\'\n");
		self->count++;
		}
}

/**********************************FUNCTION TO PRINT PERIODS****************************/
void printperiod(){
	}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");

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
