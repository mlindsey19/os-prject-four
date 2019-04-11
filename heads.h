//
// Created by mlind on 4/7/2019.
//

#ifndef PROJECT_FOUR_HEADS_H
#define PROJECT_FOUR_HEADS_H
typedef struct {
    unsigned int sec;
    unsigned int ns;
}SimClock;
#define BUFF_SZ sizeof( SimClock )
#define SHMKEY 862521 //shared memory key
#define secWorthNancSec 1000000000
#define SHMKEY2 862523 //shared memory key
#define PLIMIT 20

typedef struct {
    int priority;
    int pid;

    SimClock cpu_used;
    SimClock sys_time;
    SimClock burst_time;

}ProcessControlBlock;
#define BUFF_SZ2 sizeof( ProcessControlBlock ) * 18


#define QUEUE_NAME  "/test_queue"
#define MAX_SIZE    1024
#define MSG_STOP    "exit"


#endif //PROJECT_FOUR_HEADS_H
