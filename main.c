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

struct Slave {
    int fdParentToChild[2];
    int fdChildToParent[2];
    int jobs;
    pid_t pid;
};

/*
TODO
1) Arreglar escritura a hijos
2) Arreglar lectura de hijos
*/

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
    // return 0;

    int sharedMemoryDescriptor = shmcode;
    int destFileDescriptor = open(DEST_FILE_NAME, O_CREAT | O_WRONLY);

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

    // if(fork() == 0) {
    //     if(execv("./viewProcess", (char*[]){"./viewProcess", NULL}) < 0) {
    //         perror("");
    //     }
    // }

    // int* auxPointer = (unsigned int*) (shmemStart + 4);
    // int* auxStart = (int*) shmemStart;
    // *auxStart = 0;
    // for(int i = 0; i < 1024; i++) {
    //     // printf("%X%c", i, (i%20==0 && i != 0)?'\n':' ');
    //     // sleep(1);
    //     waitSemaphore(semCodeShmem);
    //     getSemaphore(semCodeShmem);
    //     // printf("main.c semaphore value: %d\n", semctl(semCodeShmem, 0, GETVAL));
    //     // sleep(1);
    //     *auxPointer = i;
    //     auxPointer += 4;
    //     *auxStart += 1;
    //     freeSemaphore(semCodeShmem);
    //     // printf("main.c semaphore value: %d\n", semctl(semCodeShmem, 0, GETVAL));
    //     sleep(1);
    //     // printf("%X", i);
    // }

    // waitSemaphore(semCodeShmem);
    // getSemaphore(semCodeShmem);
    // *auxStart = -1;
    // freeSemaphore(semCodeShmem);

    // return 0;

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
            // printf("wrote: %s to %d'th child. He now has %d jobs\n", buff, i, slaves[i].jobs);
            argFile++;
        }
        // write(slaves[i].fdParentToChild[WRITE_END], "", 1);
        // write(slaves[i].fdChildToParent[WRITE_END], "", 1);
    }

    /*Forkeo. Creo los esclavos y configuro sus pipes para que se comuniquen con la aplicacion.
    El padre les debe de mandar los nombres de los archivos. Los esclavos deben de mandar los hashes
    a medida que los calculan*/
    for(int i = 0; i < NUM_SLAVES; i++) {
        if((slaves[i].pid = fork()) == 0) {
            /*El esclavo no debe leer de stdin, sino del pipe Parent to Child*/
            close(STDIN_FILENO); 
            dup(slaves[i].fdParentToChild[READ_END]);

            /*El esclavo no debe escribir a stdout, sino al pipe Child to Parent*/
            close(STDOUT_FILENO);
            dup(slaves[i].fdChildToParent[WRITE_END]);

            /*El esclavo no esta interesado en leer de Child to Parent*/
            close(slaves[i].fdChildToParent[READ_END]);

            /*El esclavo no esta interesado en escribir a Parent to Child*/
            close(slaves[i].fdParentToChild[WRITE_END]);

            // for(int j = 0; j < i; j++) {
            //     close(slaves[j].fdParentToChild[READ_END]);
            //     close(slaves[j].fdParentToChild[WRITE_END]);
            //     close(slaves[j].fdChildToParent[READ_END]);
            //     close(slaves[j].fdChildToParent[WRITE_END]);
            // }

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

    int status;
    char characters[256];
    int highestOne = getHighestReadDescriptor(slaves, NUM_SLAVES);
    int closedPipe = 0;
    int jobsDone = 0;
    fd_set readDescriptorSet;
    char a;
    int fileIndex = 1 + INITIAL_LOAD*NUM_SLAVES;
    char argument[256];

    char* auxPointer = (char*) (shmemStart + 8);
    int* countPointer = (int*) (shmemStart + 4);
    unsigned int * auxStart;
    auxStart = (int*) shmemStart;
    int auxCounter = 0;
    int resultsFd = open("hashResults.txt",O_CREAT | O_RDWR ,0777);
    sleep(1);
    
    while(jobsDone < numFiles) {
        // sleep(1);
        FD_ZERO(&readDescriptorSet);
        for(int i = 0; i < NUM_SLAVES; i++) {
            FD_SET(slaves[i].fdChildToParent[READ_END], &readDescriptorSet);
        }
        if(select(highestOne + 1, &readDescriptorSet, NULL, NULL, NULL) > 0) {
            // printf("found descriptors\n");
            for(int i = 0; i < NUM_SLAVES; i++) {
                if(FD_ISSET(slaves[i].fdChildToParent[READ_END], &readDescriptorSet)) {
                    while(read(slaves[i].fdChildToParent[READ_END], &a, 1) > 0) {
                        // fprintf(stderr, "main.c Before semaphore %d\n", semctl(semCodeShmem, 0, GETVAL));
                        // waitSemaphore(semCodeShmem);
                        // getSemaphore(semCodeShmem);
                        semop(semCodeShmem, &waitOp, 1);
                        semop(semCodeShmem, &getSemOp, 1);
                        // fprintf(stderr, "main.c Got semaphore %d\n", semctl(semCodeShmem, 0, GETVAL));
                        // fprintf(stderr, "FREE\n");
                        // fprintf(stderr, "main.c before free %d\n", semctl(semCodeShmem, 0, GETVAL));
                        *auxPointer = a;
                        // fprintf(stderr, "main.c before free %d\n", semctl(semCodeShmem, 0, GETVAL));
                        // fprintf(stderr, "main.c before free %d\n", semctl(semCodeShmem, 0, GETVAL));
                        auxPointer++;
                        auxCounter++;
                        // fprintf(stderr, "main.c before free %d\n", semctl(semCodeShmem, 0, GETVAL));
                        // printf("%p\n", auxPointer);
                        // freeSemaphore(semCodeShmem);
                        semop(semCodeShmem, &freeOp, 1);
                        // fprintf(stderr, "main.c after semaphore %d\n", semctl(semCodeShmem, 0, GETVAL));
                        write(resultsFd,&a,1);
                        if(a == '\n' || a == '\0') {
                            jobsDone++;
                            // waitSemaphore(semCodeShmem);
                            // getSemaphore(semCodeShmem);
                            // fprintf(stderr, "main.c Before semaphore %d\n", semctl(semCodeShmem, 0, GETVAL));
                            semop(semCodeShmem, &waitOp, 1);
                            semop(semCodeShmem, &getSemOp, 1);
                            // fprintf(stderr, "main.c After semaphore %d\n", semctl(semCodeShmem, 0, GETVAL));
                            // printf("done it\n");
                            *countPointer += auxCounter;
                            // freeSemaphore(semCodeShmem);
                            semop(semCodeShmem, &freeOp, 1);
                            auxCounter = 0;
                            slaves[i].jobs--;
                            if(slaves[i].jobs == 0) {
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
        // printf("TERMINE\n");
        // fflush(stdout);
    }

    *auxStart = -1;

    fprintf(stderr,"TERMINE\n");
    fflush(stdout);
    close(resultsFd);
    return 0;

}

size_t flushFileDescriptor(int sourceFileDescriptor, int* destFileDescriptors, int numDescriptors) {
    char buff;
    size_t bytesWritten = 0;
    int fd = open(DEST_FILE_NAME, O_WRONLY);
    while(read(sourceFileDescriptor, &buff, 1) > 0) {
        // for(int fileDescriptorIndex = 0; fileDescriptorIndex < numDescriptors; fileDescriptorIndex++) {
        //     write(destFileDescriptors[fileDescriptorIndex], (void*)&buff, 1);
        // }
        printf("%c", buff);
        bytesWritten++;
    }
    return bytesWritten;
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