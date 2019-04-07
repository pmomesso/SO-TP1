#define _GNU_SOURCE
#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include "tp1.h"

#define NUM_SLAVES 5
#define INITIAL_LOAD 1
#define DEST_FILE_NAME "hashResults.txt"
#define MAX_ESTIMATED_FILES 1500
#define AVERAGE_ESTIMATED_HASH_LENGTH 40

int getHighestReadDescriptor(tSlave* slave, int numberOfSlaves);
size_t flushFileDescriptor(int sourceFileDescriptor, int* destFileDescriptors, int numDescriptors);

//Estructura de esclavo con dos pipes de comunicacion, un contador para la cantidad de archivos en procesamiento y un PID.
struct Slave {
    int fdParentToChild[2];
    int fdChildToParent[2];
    int jobs;
    pid_t pid;
};

int main(int argc, char* args[]) {

    if(argc - 1 > MAX_ESTIMATED_FILES) {
        fprintf(stderr, "AMOUNT OF FILES TOO LARGE\n");
        exit(1);
    }

    //Configuro la memoria compartida
    int shmcode = shmget(SHMEM_KEY, MAX_ESTIMATED_FILES*AVERAGE_ESTIMATED_HASH_LENGTH, IPC_CREAT | 0600);
    void* shmemStart;
    if((shmemStart = shmat(shmcode, NULL, 0)) == (void*)-1) {
        perror("MAIN.C: ERROR WHILE CONFIGURING SHARED MEMORY\n");
        exit(1);
    }
    *(int*)shmemStart = 0;
    *(int*)(shmemStart + 4) = 0;
    *(int*)(shmemStart + 8) = 0;

    //Configuro el semaforo para memoria compartida
    int semCodeShmem = semget(SEM_SHMEM_KEY, 1, IPC_CREAT | 0600);
    if(semCodeShmem < 0) {
        perror("MAIN.C: ERROR WHILE CONFIGURING SEMAPHORE\n");
        exit(1);
    }
    union semun {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCodeShmem, 0, SETVAL, arg);
    tSlave slaves[NUM_SLAVES];
    for(int i = 0; i < NUM_SLAVES; i++) {
        slaves[i].jobs = 0;
    }

    //Creo los pipes
    int argFile = 1;
    int numFiles = argc - 1;
    char buff[256];
    for(int i = 0; i < NUM_SLAVES; i++) {
        pipe(slaves[i].fdParentToChild);
        pipe(slaves[i].fdChildToParent);
        //Voy mandando las cargas iniciales
        for(int j = 0; j < INITIAL_LOAD && argFile <= numFiles; j++) {
            strcpy(buff, args[argFile]);
            strcat(buff, "\n");
            write(slaves[i].fdParentToChild[WRITE_END], buff, strlen(buff));
            slaves[i].jobs++;
            argFile++;
        }
    }

    /*Forkeo. Creo los esclavos y configuro sus pipes para que se comuniquen con la aplicacion.
    El padre les debe de mandar los nombres de los archivos. Los esclavos deben de mandar los hashes
    a medida que los calculan*/
    for(int i = 0; i < NUM_SLAVES; i++) {
        if((slaves[i].pid = fork()) == 0) {
            /*El esclavo no debe leer de stdin, sino del pipe Parent to Child*/
            close(STDIN_FILENO); 
            dup(slaves[i].fdParentToChild[READ_END]);
            close(slaves[i].fdParentToChild[READ_END]);
            /*El esclavo no debe escribir a stdout, sino al pipe Child to Parent*/
            close(STDOUT_FILENO);
            dup(slaves[i].fdChildToParent[WRITE_END]);
            close(slaves[i].fdChildToParent[WRITE_END]);

            /*El esclavo no esta interesado en leer de Child to Parent*/
            close(slaves[i].fdChildToParent[READ_END]);

            /*El esclavo no esta interesado en escribir a Parent to Child*/
            close(slaves[i].fdParentToChild[WRITE_END]);

            /*Por correctitud los esclavos solamente deben tener abiertos los pipes que usan y ninguno mas*/
            for(int j = 0; j < i; j++) {
                close(slaves[j].fdChildToParent[READ_END]);
                close(slaves[j].fdChildToParent[WRITE_END]);
                close(slaves[j].fdParentToChild[READ_END]);
                close(slaves[j].fdParentToChild[WRITE_END]);     
            }

            if(execv("./slaveProcess", (char*[]){"./slaveProcess", NULL}) < 0) {
                perror("MAIN.C: ERROR WHILE FORKING TO SLAVE\n");
                exit(1);
            }
        } else {
            /*El proceso aplicacion no esta interesado en leer de Parent to Child*/
            close(slaves[i].fdParentToChild[READ_END]);

            /*El proceso aplicacion no esta interesado en escribir en Child to Parent*/
            close(slaves[i].fdChildToParent[WRITE_END]);

            /*La lectura de pipe del hijo debe ser no bloqueante*/
            fcntl(slaves[i].fdChildToParent[READ_END], F_SETFL, O_NONBLOCK);
        }
    }

    //Estructuras para las operaciones de semaforo.
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

    //Obtengo el file descriptor mas alto para el select.
    int highestOne = getHighestReadDescriptor(slaves, NUM_SLAVES);

    int jobsDone = 0;
    fd_set readDescriptorSet;
    char a;
    int fileIndex = 1 + INITIAL_LOAD*NUM_SLAVES;
    char argument[256];
    //Punteros para la  memoria compartida.
    char* auxPointer = (char*) (shmemStart + 8);
    int* countPointer = (int*) (shmemStart + 4);
    int * auxStart;
    auxStart = (int*) shmemStart;
    int auxCounter = 0;

    //Creo o abro el archivo a escribir los hashes.
    int resultsFd = open(DEST_FILE_NAME,O_CREAT | O_RDWR ,0777);
    sleep(1);
    
    while(jobsDone < numFiles) {
        //Reinicio el set de descriptores.
        FD_ZERO(&readDescriptorSet);
        for(int i = 0; i < NUM_SLAVES; i++) {
            FD_SET(slaves[i].fdChildToParent[READ_END], &readDescriptorSet);
        }
        //Hago el select para obtener los pipes para leer.
        if(select(highestOne + 1, &readDescriptorSet, NULL, NULL, NULL) > 0) {
            for(int i = 0; i < NUM_SLAVES; i++) {
                if(FD_ISSET(slaves[i].fdChildToParent[READ_END], &readDescriptorSet)) {
                    while(read(slaves[i].fdChildToParent[READ_END], &a, 1) > 0) {
                        //Espero al semaforo y lo pido.
                        semop(semCodeShmem, &waitOp, 1);
                        semop(semCodeShmem, &getSemOp, 1);
                        //Escribo el caracter en la memoria compartida.
                        *auxPointer = a;
                        auxPointer++;
                        auxCounter++;
                        //Libero el semaforo
                        semop(semCodeShmem, &freeOp, 1);
                        //Escribo al archivo de texto.
                        write(resultsFd,&a,1);
                        if(a == '\n' || a == '\0') {
                            //Termino de leer un hash.
                            jobsDone++;
                            //Espero al semaforo y lo pido.
                            semop(semCodeShmem, &waitOp, 1);
                            semop(semCodeShmem, &getSemOp, 1);
                            //Actualizo en memoria compartida hasta donde leer.
                            *countPointer += auxCounter;
                            //Libero el semaforo.
                            semop(semCodeShmem, &freeOp, 1);
                            auxCounter = 0;
                            slaves[i].jobs--;
                            if(slaves[i].jobs == 0) {
                                //Envio un archivo que falte al esclavo si no tiene archivos por procesar.
                                if(fileIndex < argc) {
                                    strcpy(argument, args[fileIndex]);
                                    strcat(argument, "\n");
                                    write(slaves[i].fdParentToChild[WRITE_END], argument, strlen(argument));
                                    fileIndex++;
                                    slaves[i].jobs++;
                                }
                            }
                        }
                    }
                }
            }
       }
    }
    //Escribo el valor -1 al principio de la memoria compartida para informar al proceso vista que termino.
    *auxStart = -1;
    fflush(stdout);
    close(resultsFd);
    return 0;

}

int getHighestReadDescriptor(tSlave* slave, int numberOfSlaves) {
    int max = 0;
    for(int i = 0; i < numberOfSlaves; i++) {
        if(max < slave[i].fdChildToParent[READ_END]) {
            max = slave[i].fdChildToParent[READ_END];
        }
    }
    return max;
}