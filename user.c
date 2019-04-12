//
// Created by mlind on 4/7/2019.
//
#include <stdlib.h>
#include "user.h"
#include "panamaCityBeach.c"
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include "string.h"
static void sighdl(int sig, siginfo_t *siginfo, void *context);


ProcessControlBlock * pcb;
mqd_t mq_r, mq_h, mq_m, mq_l;


int main(int argc, char * argv[])
{
    printf("hi - pid %i\n", getpid());

    SimClock * simClock;
    int shmidc = shmget(SHMKEY_clock,BUFF_clock, 0777);
    int shmidp = shmget(SHMKEY_pcb, BUFF_pcb, 0777);
    simClock =  ( shmat ( shmidc, 0, 0));
    pcb =  ( shmat ( shmidp, 0, 0));


    printf("simclock: %i %i", simClock->sec, simClock->ns);
    printf("pcb: %i %i", pcb->pid, pcb->priority);

    char buffer[MAX_SIZE];

    mq_r = mq_open(QUEUE_REAL, O_RDWR, 0755);
//    mq_h = mq_open(QUEUE_HIGH, O_RDWR, 0755);
//    mq_m = mq_open(QUEUE_MED, O_RDWR, 0755);
//    mq_l = mq_open(QUEUE_LOW, O_RDWR, 0755);

//
//    struct sigaction action, oldaction;
//    memset (&action, 0, sizeof(action));
//    memset (&action, 0, sizeof(oldaction));
//
//    action.sa_sigaction = sighdl;
//    action.sa_flags = SA_SIGINFO;
//
//    if (sigaction(SIGUSR1, &action, &oldaction) != 0) {
//        perror ("sigaction");
//        return 1;
//    }
    sigset_t set;
    int sig;
    if(sigaddset(&set, SIGUSR1) == -1) {
        perror("Sigaddset error");
    }
    sigwait(&set,&sig );
    sleep(2);
    printf("hi - after sigwait\n");

    exit(808);
}

static void sighdl(int sig, siginfo_t *siginfo, void *context)
{
    printf ("Sending PID: %ld, UID: %ld\n",
            (long)siginfo->si_pid, (long)siginfo->si_uid);
}