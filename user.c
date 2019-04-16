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

static void sighdl(int sig, siginfo_t *siginfo, void *context);
static void receiveMessage();
static void sendMessage();

ProcessControlBlock * pcb;
ProcessControlBlock * thisPcb;
mqd_t mq_a,mq_b;
SimClock * simClock;
char buffer[MAX_SIZE];
int slice;
int ex;
struct mq_attr attr_a, attr_b;


int main(int argc, char * argv[])
{
    srand( time( NULL ) ^ getpid() );

    int shmidc = shmget(SHMKEY_clock,BUFF_clock, 0777);
    int shmidp = shmget(SHMKEY_pcb, BUFF_pcb, 0777);
    simClock =  ( shmat ( shmidc, 0, 0));
    pcb =  ( shmat ( shmidp, 0, 0));
    int k;
    for (k=0; k< NUMOFPCB; k++){
        if(getpid() == pcb[ k ].pid ) {
            thisPcb = &pcb[k];
            break;
        }
    }


    mq_a = mq_open(QUEUE_A, O_RDWR , 0777);
    mq_b = mq_open(QUEUE_B, O_RDWR| O_NONBLOCK, 0777);
    mq_getattr(mq_a, &attr_a);
    mq_getattr(mq_b, &attr_b);

    ex = 1;
    while(1) {
        if (ex == 0) break;
        while (thisPcb->run == 0);
        thisPcb-> run =0;
        receiveMessage();
        usleep(1000);
        sendMessage();
    }
    thisPcb->sys_time_end.sec = simClock->sec;
    thisPcb->sys_time_end.ns = simClock->ns;
    printf("user pid %u -> BYE\n", getpid());
    exit(808);
}

static void sighdl(int sig, siginfo_t *siginfo, void *context){}

static void receiveMessage() {
    ssize_t bytes_read;

    bytes_read = mq_receive(mq_a,( char * ) &slice, MAX_SIZE, 0);
    if (bytes_read >= 0) {
        printf("user %u: Received slice: %d\n",getpid(), slice);
    } else {
        printf("user %u: no message \n", getpid());
    }
    fflush(stdout);
}
static void amendPCB(int percent){
    int timeUsed;

    timeUsed = ( 1 + percent ) * slice;
    thisPcb->last_burst_time = timeUsed;
    thisPcb->cpu_used.ns += timeUsed;

}

static void sendMessage() {
    srand( time( NULL ) ^ getpid() );
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
            ex = 0;
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
    memset( buffer,0, sizeof( buffer ) );
    sprintf(buffer, " %d %d %d %d ", a, b, c, d);
    int s = mq_send( mq_b, buffer, MAX_SIZE, 0 );
    mq_getattr(mq_b, &attr_b);
    printf("user: sending %s - %i\n", buffer, attr_b.mq_curmsgs);
    if (s != 0){
        perror( "message didnt send" );
    }

    fflush(stdout);
}

