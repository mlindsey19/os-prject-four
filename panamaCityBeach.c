//
// Created by mlind on 4/5/2019.
//

#include <sys/shm.h>
#include <stdio.h>
#include "panamaCityBeach.h"

static int shmid;

char * getPCBMem(){
    char * paddr;
    shmid = shmget (SHMKEY_pcb, BUFF_pcb, 0777 | IPC_CREAT);

    if (shmid == -1)
        perror("parent - error shmid");

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}

void deletePCBMemory(char * paddr){
    int er;

    shmctl(shmid, IPC_RMID, NULL);
    if((er = shmdt(paddr)) == -1){
        perror("err shmdt pcb:");
    }
}