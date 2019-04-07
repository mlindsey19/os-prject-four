//
// Created by mlind on 4/5/2019.
//

#include "procblk.h"


char * getPCBMem(){
    char * paddr;
    shmid = shmget (SHMKEY2, BUFF_SZ, 0777 | IPC_CREAT);

    if (shmid == -1)
        perror("parent - error shmid");

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}

void deletePCBMemory(char * paddr){
    int er;

    shmctl(shmid, IPC_RMID, NULL);
    if((er = shmdt(paddr)) == -1){
        perror("err shmdt:");
    }
}