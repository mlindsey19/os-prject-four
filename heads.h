//
// Created by mlind on 4/7/2019.
//

#ifndef PROJECT_FOUR_HEADS_H
#define PROJECT_FOUR_HEADS_H

#include <unistd.h>

typedef struct {
    unsigned int sec;
    unsigned int ns;
}SimClock;


#define BUFF_clock sizeof( SimClock )
#define SHMKEY_clock 862521 //shared memory key
#define SHMKEY_pcb 862523 //shared memory key
#define SHMKEY_mockQ 5475623 //shared memory key


#define PLIMIT 20
#define secWorthNancSec 1000000000

typedef struct {
    int priority;

    int pid;

    SimClock cpu_used;
    SimClock sys_time_begin;
    SimClock sys_time_end;
    SimClock waitingTill;
    unsigned int last_burst_time;
    int run;

}ProcessControlBlock;
#define BUFF_pcb sizeof( ProcessControlBlock ) * 18

typedef struct {
    int realQLen; //length
    int highQLen;
    int medQLen;
    int lowQLen;
    int realQsi; // start index
    int highQsi;
    int medQsi;
    int lowQsi;
    int waitQLen;
    int waitQsi;
}QueueCount;
#define NUMOFPCB 18
typedef struct {
    int realQpids[ NUMOFPCB ];
    int highQpids[ NUMOFPCB ];
    int medQpids[ NUMOFPCB ];
    int lowQpids[ NUMOFPCB ];
    int waitQpids [NUMOFPCB ];
} QueueArrays;
#define MAX_SIZE 32
#define NBUFS 5
typedef struct {
    int messageTo;
    int slice;
    int si;
    int len;
    char buffer0[ MAX_SIZE ];
    char buffer2[ MAX_SIZE ];
    char buffer3[ MAX_SIZE ];
    char buffer4[ MAX_SIZE ];
    char buffer5[ MAX_SIZE ];
    char buffer1[ MAX_SIZE ];

}MockQueue;

#define BUFF_mockQ sizeof( MockQueue )


#define MSG_STOP "exit"


#endif //PROJECT_FOUR_HEADS_H
