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
#include "mockQueue.h"
#include <errno.h>
#include <time.h>
#include <memory.h>

int errno;

#define BUFF_out_sz 32
#define QUANTUM 50000
int slice;

static void increment();
static void sigChild();
static void sigHandle( int );
static void cleanSHM();
static void assignPCB(ProcessControlBlock *);
static SimClock nextProcTime();
static void receiveMessage();
static void sendMessage(pid_t);

static void checkWaitQueue();
static void assignToQueue(pid_t);
static void aggregateStats(pid_t);
static void dispatchTime();
static void addToGoTime(int, int);
static void generateProc();
const unsigned int maxTimeBetweenNewProcsNS = 0;
const unsigned int maxTimeBetweenNewProcsSecs = 2;
static void sigNextProc(pid_t);
static void processMessage(int pid, int fl, int s, int ns);
static int getNext();
char * getbuf();

int processLimit = 1;
int activeLimit = 1;
int active = 0;
int total = 0;
char * clockaddr;
char * pcbpaddr;
char * mockQueueaddr;
char output[BUFF_out_sz] = "input.txt";
SimClock gentime;
ProcessControlBlock * pcb;
SimClock * simClock;
MockQueue * mockQueue;
struct timespec ts;


pid_t pids[ PLIMIT ];
int bitv[NUMOFPCB];
char user[] = "user";
char path[] = "./user";
//alarm(3);
QueueCount queueCount;
QueueArrays queueArrays;
static void initQueCount();


int main(int argc, char ** argv) {
    signal( SIGINT, sigHandle );
    signal( SIGALRM, sigHandle );
    signal( SIGSEGV, sigHandle );
    checkArgs( output, argc, argv, &processLimit, &activeLimit );
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    pcbpaddr = getPCBMem();
    pcb = ( ProcessControlBlock * ) pcbpaddr;

    mockQueueaddr = getMockQMem();
    mockQueue = ( MockQueue * ) mockQueueaddr;

    initQueCount();


    gentime = nextProcTime();
    while(1) {
        increment();
        if ((active < activeLimit) && ( total< processLimit ) &&
            (simClock->sec >= gentime.sec && simClock->ns > gentime.ns)) {
            generateProc();
            gentime = nextProcTime();
            sleep(1);
        }
        pid_t npid;
        npid = getNext();
        if (npid > 0) {
            sendMessage(npid);
            sigNextProc(npid);
        }

        receiveMessage();

        if(active == 0 && total == processLimit)
            break;
    }
    sigChild();
    return 0;
}
static int getNext(){
    if ( ( queueCount.realQLen - queueCount.realQsi ) > 0 ){
        slice = QUANTUM;
        return queueArrays.realQpids[ queueCount.realQsi++ % NUMOFPCB ];
    }
    if ( ( queueCount.highQLen - queueCount.highQsi ) > 0 ){
        slice = 2 * QUANTUM;
        return queueArrays.highQpids[ queueCount.highQsi++ % NUMOFPCB ];
    }
    if ( ( queueCount.medQLen - queueCount.medQsi ) > 0 ){
        slice = 4 * QUANTUM;
        return queueArrays.medQpids[ queueCount.medQsi++ % NUMOFPCB ];
    }
    if ( ( queueCount.lowQLen - queueCount.lowQsi ) > 0 ){
        slice = 8 * QUANTUM;
        return queueArrays.lowQpids[ queueCount.lowQsi++ % NUMOFPCB ];
    }
    return -1;
}
static void assignPCB(ProcessControlBlock * pcbt){
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  );
    pcbt->priority = rand() % 100 < 12 ? 0 : 1;
    pcbt->pid = pids[ total ];
    pcbt->cpu_used.sec = 0;
    pcbt->cpu_used.ns = 0;
    pcbt->sys_time_begin.sec = simClock->sec;
    pcbt->sys_time_begin.ns = simClock->ns;
    pcbt->sys_time_end.sec = 0;
    pcbt->sys_time_end.ns = 0;
    pcbt->waitingTill.ns = 0;
    pcbt->waitingTill.sec = 0;
    pcbt->run = 0;
}

static void increment(){
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec );    //simClock->sec++;
    simClock->ns += rand() % 100000;
    if (simClock->ns > secWorthNancSec ){
        simClock->sec++;
        simClock->ns -= secWorthNancSec;
        assert( simClock->ns <= secWorthNancSec && "too many nanoseconds");
    }
}

static void sigHandle(int cc){

    cleanSHM();
}
static void deleteMemory() {
    deleteClockMem(clockaddr);
    deletePCBMemory(pcbpaddr);
    deleteMockQMem(mockQueueaddr);
}
static void cleanSHM(){
    int i;
    for (i =0; i < processLimit; i++) {
        if (pids[i] > 0) {

            kill(pids[i], SIGTERM);
        }
    }
    deleteMemory();
    //   fclose(ofpt);

}

static void sigChild() {
    pid_t pid;
    int i, status;
    for (i = 0; i < processLimit; i++) {
        if (pids[i] > 0) {
            pid = waitpid(pids[i], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 808 && pid != 0 ) {
                pids[i] = 0;
                printf( "term pid:%u\n", pid );
                //  active--;
            }
        }
    }
}

static void receiveMessage() {
    char * buffer;
    int si = mockQueue->si;
    int pid, fl, s, ns;
    if ( ( mockQueue->len - si ) > 0 ) {
        buffer = getbuf();
        printf("OSS: Received message -> %s\n", buffer );
        sscanf(buffer, " %d %d %d %d ", &pid, &fl, &s, &ns );
        memset( buffer, 0, MAX_SIZE );
        mockQueue->si++;
        processMessage(pid, fl, s, ns);
    }
}

static void processMessage(int fl, int pid,  int s, int ns) {
    switch ( fl ){
        case 0:
            aggregateStats(pid);
            printf("OSS: Received pid: %u terminated. \n", pid );
            break;
        case 1:
            dispatchTime();
            assignToQueue(pid);
            printf("OSS: Received pid: %u used %is %ins. \n", pid, s, ns );
            break;
        case 2:
            printf("OSS: Received pid: %u waiting %is %ins. \n", pid, s, ns );
            break;
        case 3:
            printf("OSS: Received pid: %u preempted after %is %ins. \n", pid, s, ns );
            dispatchTime();
            assignToQueue(pid);
            break;
        default:
            ;
    }
}
static void dispatchTime() {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec );
    int d = (rand() % 9900) + 100;
    printf("OSS: Dispatch time %ins. \n", d);
}
static void aggregateStats( pid_t pid ){
    int i;
    for ( i = 0; i < NUMOFPCB; i++ ) {
        if ( pid == pcb[ i ].pid ) {
            bitv[i] = 0;
            active--;
            break;
        }
    }

    //do something with stats
}

static void sendMessage(pid_t npid) {
    while ( mockQueue->messageTo != -1 );
    printf("OSS: sending slice %i\n", slice);
    mockQueue->messageTo = npid;
    mockQueue->slice = slice;
}

static SimClock nextProcTime(){
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  );
    int ss = maxTimeBetweenNewProcsSecs * secWorthNancSec + maxTimeBetweenNewProcsNS;
    SimClock x;
    int dx = ( rand() % ss ) ;
    x.sec = ( dx / secWorthNancSec ) + gentime.sec;
    x.ns = ( dx % secWorthNancSec ) + gentime.ns;
    if (x.ns > secWorthNancSec ){
        x.sec++;
        x.ns -= secWorthNancSec;
        assert( x.ns <= secWorthNancSec && "x - too many nanoseconds");
    }
    printf("OSS: process index %i may generate after %is %ins\n",total, x.sec, x.ns);
    return x;
}
static void generateProc() {
    if ( ( pids[ total ] = fork() ) < 0)
        perror("error forking new process");
    if ( pids[ total ] == 0 )
        execl( path, user, NULL );
    int i,j;
    j=-1;
    for( i = 0; i < NUMOFPCB; i++ ) {
        if ( bitv[ i ] == 0 ) {
            j = i;
            bitv[ i ] = 1;
            break;
        }
    }
    if(j >= 0){
        assignPCB( &pcb[ j ] );
        assignToQueue( pcb[ j ].pid );
        active++, total++;
    }
}
static int getPCBindex(pid_t pid){
    int i;
    for (i = 0 ;i < NUMOFPCB; i++){
        if (pid == pcb[ i ].pid)
            return i;
    }
}
static void sigNextProc(pid_t npid){
    printf("OSS: sending signal to %u\n", npid);
    int k = getPCBindex(npid);
    pcb[ k ].run = 1;

}

static void checkWaitQueue(){
    if ( ( queueCount.waitQLen - queueCount.waitQsi ) == 0)
        return;
    int i, j;
    for(i = 0; i < NUMOFPCB; i++)
        if ( queueArrays.waitQpids[ i ] > 0 ){
            SimClock t;
            j= getPCBindex(queueArrays.waitQpids[ i ]);
            t.sec = pcb[ j ].waitingTill.sec;
            t.ns = pcb[ j ].waitingTill.ns;
            if ( simClock->sec > t.sec ||
                 ( simClock->sec >= t.sec && simClock->ns > t.ns ) ) {
                assignToQueue( queueArrays.waitQpids[ i ] );
                queueCount.waitQsi++;
                queueArrays.waitQpids[ i ] = 0 ;
            }
        }
}
static void assignToQueue(pid_t pid) {
    int j = getPCBindex(pid);
    if (j < 0) {
        printf("OSS: pid did not match any in table \n");
        return;
    }

    if( pcb[ j ].priority == 0 ) {
        queueArrays.realQpids[ ( queueCount.realQsi + queueCount.realQLen++ ) % NUMOFPCB] = pid;
        printf("OSS: Put PID %u in queue 0\n", pid);
    }
    else{
        if ( pcb[ j ].last_burst_time < ( 2 * QUANTUM ) ) {
            queueArrays.highQpids[ ( queueCount.highQsi + queueCount.highQLen++ ) % NUMOFPCB ] = pid;
            printf("OSS: Put PID %u in queue 1\n", pid);
        }else if ( pcb[ j ].last_burst_time < ( 4 * QUANTUM ) ) {
            queueArrays.medQpids[ ( queueCount.medQsi + queueCount.medQLen++ ) % NUMOFPCB] = pid;
            printf("OSS: Put PID %u in queue 2\n", pid);
        }else if ( pcb[ j ].last_burst_time < ( 8 * QUANTUM ) ) {
            queueArrays.lowQpids[ ( queueCount.lowQsi + queueCount.lowQLen++ ) % NUMOFPCB] = pid;
            printf("OSS: Put PID %u in queue 3\n", pid);
        }else {
            queueArrays.waitQpids[ ( queueCount.waitQsi + queueCount.waitQLen++ ) % NUMOFPCB] = pid;
            printf("OSS: Put PID %u in queue wait\n", pid);
        }    }
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
    mockQueue->si = 0;
    mockQueue->messageTo = -1;
    mockQueue->slice = 0;
}
char * getbuf(){
    int ind = mockQueue->si % NBUFS;
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
