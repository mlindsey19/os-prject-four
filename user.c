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
static void sighdl(int sig, siginfo_t *siginfo, void *context);
void receiveMessage();
void sendMessage();

ProcessControlBlock * pcb;
mqd_t mq;
SimClock * simClock;
char buffer[MAX_SIZE];
int slice;


int main(int argc, char * argv[])
{
    srand( time( NULL ) ^ getpid() );

    int shmidc = shmget(SHMKEY_clock,BUFF_clock, 0777);
    int shmidp = shmget(SHMKEY_pcb, BUFF_pcb, 0777);
    simClock =  ( shmat ( shmidc, 0, 0));
    pcb =  ( shmat ( shmidp, 0, 0));

    printf("sim clock: %i %i\n", simClock->sec, simClock->ns);
    printf("pcb: %i %i\n", pcb->pid, pcb->priority);


    mq = mq_open(QUEUE_NAME, O_RDWR, 0777);





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

    sigwait(&set,&sig );
    receiveMessage();

    sendMessage();
    exit(808);
}

static void sighdl(int sig, siginfo_t *siginfo, void *context)
{
    printf ("Sending PID: %ld, UID: %ld\n",
            (long)siginfo->si_pid, (long)siginfo->si_uid);
}

void receiveMessage() {
    ssize_t bytes_read;

    bytes_read = mq_receive(mq,( char * ) &slice, MAX_SIZE, 0);

    if (bytes_read >= 0) {
        printf("SERVER: Received message: %d\n", slice);
    } else {
        printf("SERVER: None \n");
    }
    fflush(stdout);
}
static void amendPCB(int percent){
    int timeUsed;

    timeUsed = ( 1 + percent ) * slice;
    pcb->cpu_used.ns += timeUsed;

}

void sendMessage() {
    int a,b,c,d;
    c = d = 0;
    a = rand() % 3;
    b = getpid();
    switch ( a ){
        case 0:
            break;
        case 1:
            amendPCB(99);
            break;
        case 2:
            c = rand() % 5;
            d = rand() % 1000; // wait time
            break;
        case 3:
            c = rand() % 99 ; // percent of quantum used
            d = ( 1 + c ) * slice;
            amendPCB(c);
            c = 0;
            break;
        default:;
    }
    sprintf(buffer, "%d %d %d %d", a, b, c, d);
    int s = mq_send( mq, buffer, MAX_SIZE, 0 );
    if (s != 0){
        perror( "message didnt send" );
    }
    fflush(stdout);
}

