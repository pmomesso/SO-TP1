#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include "tp1.h"

int main(void) {

    //Configuro la memoria compartida
    int shmcode = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0200);
    void* shmemStart;
    if((shmemStart = shmat(shmcode, NULL, 0)) == NULL) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    //Configuro el semaforo
    int semCode = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    if(semCode < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }
    union semun {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCode, 0, SETVAL, arg);

    //Comienzo el proceso vista (DE PRUEBA, NO PARA EL TP)
    if(fork() == 0) {
        execv("./view", (char*[]){"./view", NULL});
    }

    sleep(1);

    //Pruebo la memoria compartida y el semaforo
    waitSemaphore(semCode);
    getSemaphore(semCode);
    char* auxShmStart = (char*) shmemStart;
    for(char a = 'a'; a <= 'z'; a++) {
        printf("%c ", *auxShmStart);
        auxShmStart++;
    }
    printf("\n");
    freeSemaphore(semCode);

    return 0;

}