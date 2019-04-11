//Mitch Lindsey
//cs 4760
//04-02-19
//



#include <stdio.h>
#include <bits/signum.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/shm.h>
#include <wait.h>
#include <mqueue.h>
#include "clock.h"
#include "panamaCityBeach.h"
#include "checkargs.h"

#define BUFF_out_sz 32


void increment( SimClock * );
void sigChild();
void sigHandle( int );
void cleanSHM();
void assignPCB( ProcessControlBlock * , int );


unsigned int maxTimeBetweenNewProcsNS;
unsigned int maxTimeBetweenNewProcsSecs;

int processLimit = 6;
int activeLimit = 3;
int active = 0;
int total = 0;
char * clockaddr;
char * pcbpaddr;
char output[BUFF_out_sz] = "input.txt";
const int quantum = 10000;


pid_t pids[ PLIMIT ];

//alarm(3);

int main(int argc, char ** argv) {
    signal( SIGINT, sigHandle );
    signal( SIGALRM, sigHandle );
    checkArgs( output, argc, argv, &processLimit, &activeLimit );

    char user[] = "user";
    char path[] = "./user";

    SimClock * simClock;
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    ProcessControlBlock * pcb;
    pcbpaddr = getPCBMem();
    pcb = ( ProcessControlBlock * ) pcbpaddr;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;
    mqd_t mq;
    ssize_t bytes_read;

    mq = mq_open(QUEUE_NAME, O_CREAT, 0755, &attr);


//    if ( total == processLimit && active <= 0 )
//            break;

    increment( simClock );


    for (total = 0 ; total < processLimit ; ) {

        if ( ( pids[ total ] = fork() ) < 0 ) {
            perror("error forking new process");
            return 1;
        }
        if ( pids[ total ] == 0 ) {
            execl( path, user, NULL );
        }

        assignPCB( &pcb[ total ],  total  );
        total++, active++;

        if( active >= activeLimit )
            sigChild();
    }

    while( ( total < processLimit ) && ( active != 0 ) );

    cleanSHM();


    return 0;
}
void assignPCB(ProcessControlBlock * pcb, int total ){
    pcb->priority = rand() % 100 < 12 ? 0 : 1;
    pcb->pid = pids[ total ];
    pcb->cpu_used.sec = 0;
    pcb->cpu_used.ns = 0;
    pcb->sys_time.sec = 0;
    pcb->sys_time.ns = 0;
}

void increment(SimClock * simClock){
    simClock->sec++;
    simClock->ns += rand() % 1000;
    if (simClock->ns > secWorthNancSec ){
        simClock->sec++;
        simClock->ns -= secWorthNancSec;
        assert( simClock->ns <= secWorthNancSec && "too many nanoseconds");
    }
}

void sigHandle(int cc){
    cleanSHM();
}
void deleteMemory() {
    deleteClockMem(clockaddr);
    deletePCBMemory(pcbpaddr);
    mq_unlink(QUEUE_NAME);
}
void cleanSHM(){
    int i;
    for (i =0; i < processLimit; i++) {
        if (pids[i] > 0) {

            kill(pids[i], SIGTERM);
        }
    }
    deleteMemory();
    //   fclose(ofpt);

}

void sigChild() {
    pid_t pid;
    int i, status;
    for (i = 0; i < processLimit; i++) {
        if (pids[i] > 0) {
            pid = waitpid(pids[i], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 808 && pid != 0 ) {
                pids[i] = 0;
                printf( "term pid:%u\n", pid );
                active--;
            }
        }
    }
}



