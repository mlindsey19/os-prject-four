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
void recieveMessage();

ProcessControlBlock * pcb;
mqd_t mq_r, mq_h, mq_m, mq_l;
SimClock * simClock;
char buffer[MAX_SIZE];


int main(int argc, char * argv[])
{
    printf("hi - pid %i\n", getpid());

    int shmidc = shmget(SHMKEY_clock,BUFF_clock, 0777);
    int shmidp = shmget(SHMKEY_pcb, BUFF_pcb, 0777);
    simClock =  ( shmat ( shmidc, 0, 0));
    pcb =  ( shmat ( shmidp, 0, 0));

    printf("sim clock: %i %i\n", simClock->sec, simClock->ns);
    printf("pcb: %i %i\n", pcb->pid, pcb->priority);


    mq_r = mq_open(QUEUE_REAL, O_RDWR, 0755);
//    mq_h = mq_open(QUEUE_HIGH, O_RDWR, 0755);
//    mq_m = mq_open(QUEUE_MED, O_RDWR, 0755);
//    mq_l = mq_open(QUEUE_LOW, O_RDWR, 0755);




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
    printf("hi - after sigwait\n");
    recieveMessage()
    exit(808);
}

static void sighdl(int sig, siginfo_t *siginfo, void *context)
{
    printf ("Sending PID: %ld, UID: %ld\n",
            (long)siginfo->si_pid, (long)siginfo->si_uid);
}

void recieveMessage() {
    ssize_t bytes_read;

    bytes_read = mq_receive(mq_r, buffer, MAX_SIZE, NULL);

    if (bytes_read >= 0) {
        printf("SERVER: Received message: %s\n", buffer);
    } else {
        printf("SERVER: None \n");
    }
    fflush(stdout);
}