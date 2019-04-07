#define _GNU_SOURCE
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

    //Estructuras para las operaciones con semaforos.
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
    sleep(1);
    //Sigo hasta que lea -1 en el principio de la memoria y hasta que ya no haya mas hashes para leer en el buffer.
    while(*auxStart != APPLICATION_FINISHED || read < *countPointer) {
        //Obtengo hasta donde leer.
        toRead = *countPointer;
        if(read < toRead) {
            //Espero y pido el semaforo.
            semop(semCode, &waitOp, 1);
            semop(semCode, &getSemOp, 1);
           //Escribo por salida estandar el caracter que esta en la memoria.
            write(STDOUT_FILENO, auxPointer, toRead - read);
            fflush(stdout);
            // Libero el semaforo.
            semop(semCode, &freeOp, 1);
            read += toRead - read;
            auxPointer += toRead - prevRead;
            prevRead = read;
        } else {
            sleep(1);
        }
    }

}