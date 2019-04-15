//Mitch Lindsey
//cs 4760
//04-02-19
//

#include <stdio.h>
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
#include <errno.h>
#include <time.h>

int errno;

#define BUFF_out_sz 32
#define QUANTUM 50000
int slice;

void increment( SimClock * );
void sigChild();
void sigHandle( int );
void cleanSHM();
void assignPCB( ProcessControlBlock * );
SimClock nextProcTime();

void checkWaitQueue();
void assignToQueue(pid_t);
void aggregateStats();
void dispatchTime();
void addTogotime(int ,int );
void generateProc();
const unsigned int maxTimeBetweenNewProcsNS = 0;
const unsigned int maxTimeBetweenNewProcsSecs = 2;
void sigNextProc(pid_t);

void processMessage(int pid, int fl, int s, int ns);

int processLimit = 6;
int activeLimit = 3;
int active = 0;
int total = 0;
char * clockaddr;
char * pcbpaddr;
char output[BUFF_out_sz] = "input.txt";

char buffer[MAX_SIZE];
ProcessControlBlock * pcb;
SimClock * simClock;
SimClock goTime;

pid_t pids[ PLIMIT ];
int bitv[NUMOFPCB];
mqd_t mq;
char user[] = "user";
char path[] = "./user";
//alarm(3);
QueueCount queueCount;
QueueArrays queueArrays;
static void initQueCount();


int main(int argc, char ** argv) {
    signal( SIGINT, sigHandle );
    signal( SIGALRM, sigHandle );
    initQueCount();
    checkArgs( output, argc, argv, &processLimit, &activeLimit );

    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    pcbpaddr = getPCBMem();
    pcb = ( ProcessControlBlock * ) pcbpaddr;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0777, &attr);

    SimClock gentime = nextProcTime();
    goTime.sec = 0;
    goTime.ns = 0;

    while (1){

        slice = QUANTUM;

        increment( simClock );
        checkWaitQueue();
        assignToQueue();

        if ( simClock->sec > gentime.sec ||
             ( simClock->sec >= gentime.sec && simClock->ns > gentime.ns ) )
            generateProc();

        // for (total = 0 ; total < processLimit ; ) {
        pid_t npid;
        if ( simClock->sec > goTime.sec ||
             ( simClock->sec >= goTime.sec && simClock->ns > goTime.ns ) ) {


            sigNextProc(npid);
        }
        if( active >= activeLimit )
            sigChild();
        if ( total == processLimit && active <= 0 )
            break;
    }


    cleanSHM();


    return 0;
}
void assignPCB(ProcessControlBlock * pcb){
    pcb->priority = rand() % 100 < 12 ? 0 : 1;
    pcb->pid = pids[ total ];
    pcb->cpu_used.sec = 0;
    pcb->cpu_used.ns = 0;
    pcb->sys_time_begin.sec = simClock->sec;
    pcb->sys_time_begin.ns = simClock->ns;
    pcb->sys_time_end.sec = 0;
    pcb->sys_time_end.ns =0;
    pcb->waitingTill.ns=0;
    pcb->waitingTill.sec=0;
}

void increment(SimClock * simClock){
    srand( time( NULL ) ^ getpid() );
    //simClock->sec++;
    simClock->ns += rand() % 10000;
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
    mq_close(mq);
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

int receiveMessage() {
    ssize_t bytes_read;
    int pid, fl, s, ns;
    bytes_read = mq_receive( mq,( char * ) &slice, MAX_SIZE, 0 );
    if (bytes_read >= 0) {
        printf("SERVER: Received message: %s\n", buffer);
        sscanf(buffer, "%d %d %d %d", &pid, &fl, &s, &ns );
        processMessage(pid, fl, s, ns);
    } else
        printf("SERVER: None \n");
    fflush(stdout);
}

void processMessage(int pid, int fl, int s, int ns) {
    switch ( fl ){
        case 0:
            aggregateStats(pid);
            printf("OSS: Received pid: %u terminated. \n", pid );
            break;
        case 1:
            dispatchTime();
            addTogotime(s,ns);
            assignToQueue(pid);
            printf("OSS: Received pid: %u used %is %ins. \n", pid, s, ns );
            break;
        case 2:
            printf("OSS: Received pid: %u waiting %is %ins. \n", pid, s, ns );
            break;
        case 3:
            printf("OSS: Received pid: %u preempted after %is %ins. \n", pid, s, ns );
            dispatchTime();
            addTogotime(s,ns);
            assignToQueue(pid);
            break;
        default:
            ;
    }
}
void dispatchTime() {
    int d = (rand() % 9900) + 100;
    printf("OSS: Dispatch time %ins. \n", d);
    goTime.ns += d;
    if (goTime.ns > secWorthNancSec) {
        goTime.sec++;
        goTime.ns -= secWorthNancSec;
    }
}
void addTogotime(int s, int ns){
    goTime.ns += ns;
    goTime.sec += s;
    if (goTime.ns > secWorthNancSec ){
        goTime.sec++;
        goTime.ns -= secWorthNancSec;
    }
}
void aggregateStats( pid_t pid ){
    int i;
    for ( i = 0; i < NUMOFPCB; i++ ){
        if ( pid == pcb[i].pid )
            bitv[ i ] = 0;
    }
    //do something with stats

}

void sendMessage() {
    int s = mq_send( mq, ( char * ) &slice, MAX_SIZE, 0 );
    if (s != 0)
        perror( "message didnt send" );
    fflush(stdout);
}

SimClock nextProcTime(){
    int ss = maxTimeBetweenNewProcsSecs * secWorthNancSec + maxTimeBetweenNewProcsNS;
    SimClock x;
    int dx = ( rand() % ss ) + total;
    x.sec = dx / secWorthNancSec;
    x.ns = dx % secWorthNancSec;
    return x;
}
void generateProc() {
    if ( ( pids[ total ] = fork() ) < 0)
        perror("error forking new process");
    if ( pids[ total ] == 0 )
        execl( path, user, NULL );
    int i,j;
    j=-1;
    for(i=0; i< NUMOFPCB;i++) {
        if (bitv[i] == 0) {
            j = i;
            break;
        }
    }
    if(j >= 0){
        assignPCB( &pcb[ j ] );
        assignToQueue(pcb[j].pid);
        active++, total++;
    }
}
void sigNextProc(pid_t npid){
    if ( sigqueue( npid, SIGUSR1, ( union sigval ) 0 ) != 0 )
        perror( "sig not sent: " );
}


void checkWaitQueue(){
    int i ;
    for(i =0; i < NUMOFPCB;i++)
        if (queueArrays.waitQpids[i] > 0){
            SimClock t = pcb[queueArrays.waitQpids[i]].waitingTill;
            if ( simClock->sec > t.sec ||
                 ( simClock->sec >= t.sec && simClock->ns > t.ns ) ) {
                assignToQueue( queueArrays.waitQpids[ i ] );
                queueCount.waitQsi++;
                queueArrays.waitQpids[ i ] = 0 ;
            }
        }
}
void assignToQueue(pid_t pid){

    if( pcb[total].priority == 0 )
        queueArrays.realQpids[ ( queueCount.realQsi + queueCount.realQLen++ ) % NUMOFPCB ] = pid;
    else{
        if ( pcb->last_burst_time < QUANTUM )
            queueArrays.highQpids[ ( queueCount.highQsi + queueCount.highQLen++ ) % NUMOFPCB ] = pid;
        if ( pcb->last_burst_time < 2 * QUANTUM )
            queueArrays.medQpids[ ( queueCount.medQsi + queueCount.medQLen++ ) % NUMOFPCB ] = pid;
        if ( pcb->last_burst_time < 3 * QUANTUM )
            queueArrays.lowQpids[ ( queueCount.lowQsi + queueCount.lowQLen++ ) % NUMOFPCB ] = pid;
        else
            queueArrays.waitQpids[ ( queueCount.waitQsi + queueCount.waitQLen++ ) % NUMOFPCB ] = pid;
    }
}
static void initQueCount(){
    queueCount.highQLen = 0;
    queueCount.highQsi = 0;
    queueCount.lowQLen = 0;
    queueCount.lowQsi = 0;
    queueCount.medQLen =0;
    queueCount.medQsi = 0;
    queueCount.realQLen = 0;
    queueCount.realQsi = 0;
    queueCount.waitQLen = 0;
    queueCount.waitQsi = 0;
    int i;
    for (i=0; i< NUMOFPCB; i++)
        bitv[ i ] = 0;
}
