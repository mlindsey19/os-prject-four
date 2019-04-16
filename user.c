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


    mq_a = mq_open(QUEUE_A, O_RDWR | O_NONBLOCK, 0777);
    mq_b = mq_open(QUEUE_B, O_RDWR| O_NONBLOCK, 0777);
    mq_getattr(mq_a, &attr_a);
    mq_getattr(mq_b, &attr_b);


    struct sigaction action;
    memset (&action, 0, sizeof(action));
    action.sa_sigaction = sighdl;
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &action, NULL) != 0) {
        perror ("sigaction");
        return 1;
    }
    sigset_t set;
    int sig;
    if(sigaddset(&set, SIGUSR1) == -1) {
        perror("Sigaddset error");
    }
    ex = 1;
    while(ex) {
        sigwait(&set, &sig);
        receiveMessage();
        sendMessage();
    }
    pcb->sys_time_end.sec = simClock->sec;
    pcb->sys_time_end.ns = simClock->ns;
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
    pcb->last_burst_time = timeUsed;
    pcb->cpu_used.ns += timeUsed;

}

static void sendMessage() {
    int a,b,c,d;
    c = d = 0;
    a = rand() % 3;
    b = getpid();
    switch ( a ){
        case 0:
            ex = 1;
            break;
        case 1:
            amendPCB(99);
            break;
        case 2:
            c = rand() % 5;
            d = rand() % 1000; // wait time
            pcb->waitingTill.sec = c + simClock->sec;
            pcb->waitingTill.ns = d + simClock->ns;
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
    printf("user: sending %s - %i\n", buffer, attr_b);
    if (s != 0){
        perror( "message didnt send" );
    }

    fflush(stdout);
}

