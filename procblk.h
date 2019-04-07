//
// Created by mlind on 4/5/2019.
//

#ifndef PROJECT_FOUR_PC_H
#define PROJECT_FOUR_PC_H

#define SHMKEY 862523 //shared memory key
#define PLIMIT 20
char * getPCBMem();
void deletePCBMemory(char *);

typedef struct {
    int priority;
    int pid;

    SimTime cpu_used;
    SimTime sys_time;
    SimTime burst_time;

}ProcessControlBlock;
#define BUFF_SZ2 sizeof( ProcessControlBlock ) * 18

#endif //PROJECT_FOUR_PC_H
