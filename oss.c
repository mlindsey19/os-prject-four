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
#include <memory.h>

int errno;

#define BUFF_out_sz 32
#define QUANTUM 50000
int slice;

static void increment();
static void sigChild();
static void sigHandle( int );
static void cleanSHM();
static void assignPCB();
static SimClock nextProcTime();
static void receiveMessage();
static void sendMessage();

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

int processLimit = 6;
int activeLimit = 3;
int active = 0;
int total = 0;
char * clockaddr;
char * pcbpaddr;
char output[BUFF_out_sz] = "input.txt";
SimClock gentime;
ProcessControlBlock * pcb;
SimClock * simClock;
SimClock goTime;

struct mq_attr attr_a, attr_b;

pid_t pids[ PLIMIT ];
int bitv[NUMOFPCB];
mqd_t mq_a, mq_b;
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
    signal(SIGCONT, sigHandle );
    initQueCount();
    checkArgs( output, argc, argv, &processLimit, &activeLimit );

    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    pcbpaddr = getPCBMem();
    pcb = ( ProcessControlBlock * ) pcbpaddr;


    attr_a.mq_flags = 0;
    attr_a.mq_maxmsg = 10;
    attr_a.mq_msgsize = MAX_SIZE;
    attr_a.mq_curmsgs = 0;
    attr_b.mq_flags = 0;
    attr_b.mq_maxmsg = 10;
    attr_b.mq_msgsize = MAX_SIZE;
    attr_b.mq_curmsgs = 0;
    mq_a = mq_open( QUEUE_A, O_CREAT | O_RDWR | O_NONBLOCK, 0777, &attr_a );
    mq_b = mq_open( QUEUE_B, O_CREAT | O_RDWR | O_NONBLOCK, 0777, &attr_b );
    struct sigevent sigevent;
    sigevent.sigev_notify = SIGEV_SIGNAL;
    sigevent.sigev_signo = SIGCONT;
   // mq_notify(mq_b, &sigevent);

    gentime = nextProcTime();
    goTime.sec = 0;
    goTime.ns = 0;
    int k = 1;
//    while (1){
//
//        increment( simClock );
//        checkWaitQueue();
//
//        if ( simClock->sec > gentime.sec ||
//             ( simClock->sec >= gentime.sec && simClock->ns > gentime.ns ) ) {
//            generateProc();
//            gentime = nextProcTime();
//        }
//        for (total = 0 ; total < processLimit ; ) {
//            pid_t npid;
//            if ( simClock->sec > goTime.sec ||
//                 ( simClock->sec >= goTime.sec && simClock->ns >= goTime.ns ) ) {
//                goTime.sec = simClock->sec;
//                goTime.ns = simClock->ns;
//                npid = getNext();
//                if(npid == 0)
//                    continue;
//                sendMessage();
//                sigNextProc(npid);
//                sleep(1);
//                receiveMessage();
//            }
//            if( active >= activeLimit )
//                sigChild();
//            if ( total == processLimit && active <= 0 )
//                break;
//        }

    int a,b,c;
    a = b =c = 0;
    while(k<10000000){
        increment( simClock );
        mq_getattr(mq_b, &attr_b);
        checkWaitQueue();
        if( attr_b.mq_curmsgs > 0 )
           receiveMessage();

        if (a == 0) {
            generateProc();
            a++;
        }

        pid_t npid ;
        npid = getNext();
        if( npid > 0 ){
            sendMessage();
            sleep( 1 );
            sigNextProc( npid );
            k = k + 100;
        }
        k++;
    }

    sigChild();
    cleanSHM();


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
static void assignPCB(){
    pcb->priority = rand() % 100 < 12 ? 0 : 1;
    pcb->pid = pids[ total ];
    pcb->cpu_used.sec = 0;
    pcb->cpu_used.ns = 0;
    pcb->sys_time_begin.sec = simClock->sec;
    pcb->sys_time_begin.ns = simClock->ns;
    pcb->sys_time_end.sec = 0;
    pcb->sys_time_end.ns = 0;
    pcb->waitingTill.ns = 0;
    pcb->waitingTill.sec = 0;
}

static void increment(){
    srand( time( NULL ) ^ getpid() );
    //simClock->sec++;
    simClock->ns += rand() % 100000;
    if (simClock->ns > secWorthNancSec ){
        simClock->sec++;
        simClock->ns -= secWorthNancSec;
        assert( simClock->ns <= secWorthNancSec && "too many nanoseconds");
    }
}

static void sigHandle(int cc){
    if(cc == SIGUSR2)
        receiveMessage();
    else
        cleanSHM();
}
static void deleteMemory() {
    deleteClockMem(clockaddr);
    deletePCBMemory(pcbpaddr);
    mq_close(mq_a);
    mq_unlink(QUEUE_A);
    mq_close(mq_b);
    mq_unlink(QUEUE_B);
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
    ssize_t bytes_read;
    char buffer[MAX_SIZE];
    memset( buffer,0, sizeof( buffer ) );
    int pid, fl, s, ns;
    bytes_read = mq_receive( mq_b, buffer, MAX_SIZE, 0 );
    if (bytes_read > 0) {
        printf("OSS: Received message -> %s\n", buffer);
        sscanf(buffer, " %d %d %d %d ", &pid, &fl, &s, &ns );
        processMessage(pid, fl, s, ns);
    } else
        printf("OSS: No message \n");
    fflush(stdout);
}

static void processMessage(int fl, int pid,  int s, int ns) {
    switch ( fl ){
        case 0:
            aggregateStats(pid);
            printf("OSS: Received pid: %u terminated. \n", pid );
            break;
        case 1:
            dispatchTime();
            addToGoTime(s, ns);
            assignToQueue(pid);
            printf("OSS: Received pid: %u used %is %ins. \n", pid, s, ns );
            break;
        case 2:
            printf("OSS: Received pid: %u waiting %is %ins. \n", pid, s, ns );
            break;
        case 3:
            printf("OSS: Received pid: %u preempted after %is %ins. \n", pid, s, ns );
            dispatchTime();
            addToGoTime(s, ns);
            assignToQueue(pid);
            break;
        default:
            ;
    }
}
static void dispatchTime() {
    int d = (rand() % 9900) + 100;
    printf("OSS: Dispatch time %ins. \n", d);
    goTime.ns += d;
    if (goTime.ns > secWorthNancSec) {
        goTime.sec++;
        goTime.ns -= secWorthNancSec;
    }
}
static void addToGoTime(int s, int ns){
    goTime.ns += ns;
    goTime.sec += s;
    if (goTime.ns > secWorthNancSec ){
        goTime.sec++;
        goTime.ns -= secWorthNancSec;
    }
}
static void aggregateStats( pid_t pid ){
    int i;
    for ( i = 0; i < NUMOFPCB; i++ ) {
        if (pid == pcb[i].pid) {
            bitv[i] = 0;
            active--;
            break;
        }
    }

    //do something with stats

}

static void sendMessage() {
    printf("OSS: sending slice %i\n", slice);
    int s = mq_send( mq_a, ( char * ) &slice, MAX_SIZE, 0 );
    if (s != 0)
        perror( "Parent - message didnt send" );
    fflush(stdout);
}

static SimClock nextProcTime(){
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
static void sigNextProc(pid_t npid){
    if ( sigqueue( npid, SIGUSR1, ( union sigval ) 0 ) != 0 )
        perror( "sig not sent: " );
}


static void checkWaitQueue(){
    if ( ( queueCount.waitQLen - queueCount.waitQsi ) == 0)
        return;
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
static void assignToQueue(pid_t pid) {
    int i, j;
    for (i = 0, j = -1; i < NUMOFPCB; i++) {
        if (pcb[i].pid == pid)
            j = i;
    }
    if (j < 0) {
        printf("OSS: pid did not match any in table");
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
}
