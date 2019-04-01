#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include "tp1.h"

int main(void) {

    //Obtengo la referencia al semaforo
    int semCode = semget(SEM_SHMEM_KEY, 0, 0200);
    if(semCode < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    //Obtengo la referencia a la memoria compartida
    int shmcode = shmget(SEM_SHMEM_KEY, SHM_SIZE, 0200);
    void* shmStart = shmat(shmcode, NULL, 0);
    if(shmStart == NULL) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

}