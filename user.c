//
// Created by mlind on 4/7/2019.
//
#include <stdlib.h>
#include "user.h"
#include "panamaCityBeach.c"
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "string.h"
#include <memory.h>

static char * getbuf();

static void receiveMessage();
static void sendMessage();

ProcessControlBlock * pcb;
ProcessControlBlock * thisPcb;
SimClock * simClock;
int slice;
int ex;
MockQueue * mockQueue;
struct timespec ts;
int main(int argc, char * argv[])
{

    int shmidc = shmget(SHMKEY_clock,BUFF_clock, 0777);
    int shmidp = shmget(SHMKEY_pcb, BUFF_pcb, 0777);
    int shmidmq = shmget(SHMKEY_mockQ, BUFF_mockQ, 0777);


    simClock =  ( shmat ( shmidc, 0, 0));
    pcb =  ( shmat ( shmidp, 0, 0));
    mockQueue =  ( shmat ( shmidmq, 0, 0));

    printf("user mockq -> msto %i  -- slice %i \n", mockQueue->messageTo, mockQueue->slice );


    int k;
    for (k=0; k < NUMOFPCB; k++){
        if(getpid() == pcb[ k ].pid ) {
            thisPcb = &pcb[k];
            break;
        }
    }
    printf("user pdb -> pid %u  -- pri %i - run %u \n",thisPcb->pid++, thisPcb->priority,thisPcb->run );

    ex = 1;
    while(1) {
        if (ex == 1000) break;
        usleep(1000);
        while (thisPcb->run != 1);
        thisPcb-> run = -1;
        receiveMessage();
        sendMessage();
    }




    thisPcb->sys_time_end.sec = simClock->sec;
    thisPcb->sys_time_end.ns = simClock->ns;
    printf("user pid %u -> BYE\n", getpid());
    exit(808);
}

static void receiveMessage() {

    if ( mockQueue->messageTo  == getpid() ) {
        slice = mockQueue->slice;
        mockQueue->messageTo = 0;
        printf("user %u: Received slice: %d\n",getpid(), slice);
    } else {
        printf("user %u: no message \n", getpid());
    }
}
static void amendPCB(int percent){
    int timeUsed;

    timeUsed = ( 1 + percent ) * slice;
    thisPcb->last_burst_time = timeUsed;
    thisPcb->cpu_used.ns += timeUsed;

}

static void sendMessage() {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec ^ getpid() );

    int a,b,c,d;
    c = d = 0;
    a = rand() % 9;
    b = getpid();
    switch ( a ){
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 0:
            a=0;
            ex = 1000;
            pcb->last_burst_time = 0;
            break;
        case 1:
            amendPCB(99);
            d = slice;
            break;
        case 2:
            c = rand() % 5;
            d = rand() % 1000; // wait time
            thisPcb->waitingTill.sec = c + simClock->sec;
            thisPcb->waitingTill.ns = d + simClock->ns;
            break;
        case 3:
            c = rand() % 99 ; // percent of quantum used
            d = ( 1 + c ) * slice;
            amendPCB(c);
            c = 0;
            break;
        default:;
    }
    char * buffer;
    buffer = getbuf();
    memset( buffer,0, MAX_SIZE );
    sprintf(buffer, " %d %d %d %d ", a, b, c, d);
    mockQueue->len++;
    printf("user: sending %s \n", buffer );

}

static char * getbuf(){
    int ind = mockQueue->len % NBUFS;
    switch (ind){
        case 0:
            return mockQueue->buffer0;
        case 1:
            return mockQueue->buffer1;
        case 2:
            return mockQueue->buffer2;
        case 3:
            return mockQueue->buffer3;
        case 4:
            return mockQueue->buffer4;
        case 5:
            return mockQueue->buffer5;

    }
}
