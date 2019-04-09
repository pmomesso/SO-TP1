#define _GNU_SOURCE
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include "tp1.h"

int main(void) {

    //Obtengo la referencia a la memoria compartida
    int shmcode = shmget(SHMEM_KEY, 0, 0600);
    while(shmcode < 0) {
        shmcode = shmget(SHMEM_KEY, 0, 0600);
    }

    //Obtengo la referencia al semaforo
    int semCode = semget(SEM_SHMEM_KEY, 0, 0600);
    while(semCode < 0) {
        semCode = semget(SEM_SHMEM_KEY, 0, 0600);
    }

    void* shmStart = shmat(shmcode, NULL, 0);
    while(shmStart == (void*)-1) {
        perror("VIEW.C: PROBLEM WHILE TRYING TO CONFIGURE SHARED MEMORY\n");
        exit(1);
    }

    //Estructuras para las operaciones de semaforo.
    struct sembuf waitOp[2];
    waitOp[0].sem_num = 0;
    waitOp[0].sem_flg = 0;
    waitOp[0].sem_op = 0;

    waitOp[1].sem_num = 0;
    waitOp[1].sem_flg = 0;
    waitOp[1].sem_op = 1;

    struct sembuf freeOp;
    freeOp.sem_num = 0;
    freeOp.sem_flg = 0;
    freeOp.sem_op = -1;

    char* auxPointer = (char*)(shmStart + 12);
    int* countPointer = (int*)(shmStart + 8);
    int* pidPointer = (int*)(shmStart + 4);
    int* auxStart = (int*)shmStart;
    semop(semCode, waitOp, 2);
    *pidPointer = getpid();
    semop(semCode, &freeOp, 1);
    int read = 0;
    int prevRead = 0;

    //Sigo hasta que lea -1 en el principio de la memoria y hasta que ya no haya mas hashes para leer en el buffer.
    while(*auxStart != APPLICATION_FINISHED || read < *countPointer) {
        int toRead = 0;
        //Obtengo hasta donde leer.
        semop(semCode, waitOp, 2);
        toRead = *countPointer;
        if(read < toRead) {
            //Espero y pido el semaforo.
            //Escribo por salida estandar el caracter que esta en la memoria.
            write(STDOUT_FILENO, auxPointer, toRead - read);
            read += toRead - read;
            auxPointer += toRead - prevRead;
            prevRead = read;
        }
        semop(semCode, &freeOp, 1);
    }
    
    shmdt(shmStart);

    return 0;
}