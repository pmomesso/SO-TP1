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
        perror("VIEW.C: PROBLEM WHILE TRYING TO CONFIGURE SEMAPHORE\n");
        exit(1);
    }

    //Obtengo la referencia a la memoria compartida
    int shmcode = shmget(SHMEM_KEY, 0, 0600);
    void* shmStart = shmat(shmcode, NULL, 0);
    if(shmStart == (void*)-1) {
        perror("VIEW.C: PROBLEM WHILE TRYING TO CONFIGURE SHARED MEMORY\n");
        exit(1);
    }

    // printf("hola");
    // return 0;

    struct sembuf waitOp;
    waitOp.sem_num = 0;
    waitOp.sem_flg = 0;
    waitOp.sem_op = 0;

    struct sembuf freeOp;
    freeOp.sem_num = 0;
    freeOp.sem_flg = 0;
    freeOp.sem_op = -1;

    struct sembuf getSemOp;
    getSemOp.sem_flg = 0;
    getSemOp.sem_num = 0;
    getSemOp.sem_op = 1;

    char* auxPointer = (char*)(shmStart + 8);
    int* countPointer = (int*)(shmStart + 4);
    int* auxStart = (int*)shmStart;
    int read = 0;
    int prevRead = 0;
    int toRead = 0;
    printf("before loop. Value at the start of shmem: %d\n", *auxStart);
    sleep(1);
    while(*auxStart != -1 || read < *countPointer) {
        // printf("inside the loop\n");
        toRead = *countPointer;
        // printf("%p countPointer: %d\n", countPointer, toRead);
        // printf("read: %d\n", read);
        // printf("entro\n");
        if(read < toRead) {
            // printf("view.c before wait semaphore value: %d\n", semctl(semCode, 0, GETVAL));
            // getSemaphore(semCode);
            semop(semCode, &waitOp, 1);
            semop(semCode, &getSemOp, 1);
            // printf("view.c after wait semaphore value: %d\n", semctl(semCode, 0, GETVAL));
            // printf(" -- ");
            // perror("hola\n");
            write(STDOUT_FILENO, auxPointer, toRead - read);
            fflush(stdout);
            // printf("hola\n");
            // printf("\ndone pointer\n");
            // freeSemaphore(semCode);
            semop(semCode, &freeOp, 1);
            // printf("view.c after wait semaphore value: %d\n", semctl(semCode, 0, GETVAL));
            read += toRead - read;
            auxPointer += toRead - prevRead;
            prevRead = read;
            // printf("view.c after free semaphore value: %d\n", semctl(semCode, 0, GETVAL));
            // sleep(1);
        }
        // sleep(1);
        // fprintf(stderr, "%d\n", *auxStart);
    }

    printf("finish\n");

}