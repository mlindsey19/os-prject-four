//
// Created by mlind on 4/5/2019.
//

#ifndef PROJECT_FOUR_CLOCK_H
#define PROJECT_FOUR_CLOCK_H

#define SHMKEY 862521 //shared memory key
#define secWorthNancSec 1000000000
char * getClockMem();
void deleteClockMem(char *);
typedef struct {
    unsigned int sec;
    unsigned int ns;
}SimClock;

#define BUFF_SZ sizeof( SimTime )

#endif //PROJECT_FOUR_CLOCK_H
