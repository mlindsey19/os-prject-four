//mitch Lindsey
// cs 4760 p4
// 04 - 05 - 2019
//

#include <sys/shm.h>
#include <stdio.h>
#include "clock.h"

static int shmid;

char * getClockMem(){
    char * paddr;
    shmid = shmget ( SHMKEY, BUFF_SZ, 0777 | IPC_CREAT );

    if ( shmid == -1 )
        perror( "parent - error shmid" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}

void deleteClockMem( char * paddr ){
    int er;

    shmctl(shmid, IPC_RMID, NULL);
    if( ( er = shmdt( paddr ) ) == -1 ){
        perror("err shmdt clock:");
    }
}