//Mitch Lindsey
//cs 4760
//04-02-19
//



#include <stdio.h>
#include "clock.h"
#include "procblk.h"


void incrementClock(int * , int );

unsigned int maxTimeBetweenNewProcsNS;
unsigned int maxTimeBetweenNewProcsSecs;

int processLimit = 6;
int activeLimit = 3;
int active = 0;
int total = 0;
char * paddr;

pid_t pids[ PLIMIT ];

alarm(3);

int main() {
    signal( SIGINT, sigHandle );
    signal( SIGALRM, sigHandle );
    checkArgs( infile, output, argc, argv, &processLimit, &activeLimit );

    SimClock simClock;
    paddr = getClockMem();
    simClock = ( SimClock * ) paddr;
    simClock.sec = 0;
    simClock.ns = 0;


    if ( total == processLimit && active <= 0 )
            break;

        increment( simClock );

        if (total < maxEver && active < maxActive ) {
            if ( ( children[ total ] = fork()) < 0 ) {
                perror( "error forking new process" );
                return 1;
            }
            if ( getppid() == oss_pid )
                execl( "./user", "user", NULL );
            total++;
            active++;
        }


    return 0;
}




void increment(int * simClock){
    simClock.sec++;
    simCloack.ns += rand() % 1000;
    if (simCloack.ns > secWorthNancSec ){
        simCloack.sec++;
        simCloack.ns -= secWorthNancSec;
        assert( simCloack.ns <= secWorthNancSec && "too many nanoseconds");
    }
}