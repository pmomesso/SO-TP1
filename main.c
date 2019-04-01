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
#include <errno.h>
#include "tp1.h"

#define NUM_SLAVES 1
#define INITIAL_LOAD 2

int getHighestWriteDescriptor(tSlave* slave, int numberOfSlaves);

struct Slave {
    int fd[2];
    pid_t pid;
};

int main(int argc, char* args[]) {

    //Configuro la memoria compartida
    int shmcode = shmget(SEM_SHMEM_KEY, SHM_SIZE, IPC_CREAT | 0200);
    void* shmemStart;
    if((shmemStart = shmat(shmcode, NULL, 0)) == NULL) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    //Configuro el semaforo para memoria compartida
    int semCodeShmem = semget(SEM_SHMEM_KEY, 1, IPC_CREAT | 0600);
    if(semCodeShmem < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }
    union semun {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCodeShmem, 0, SETVAL, arg);

    //Configuro el semaforo para la named pipe
    int semCodePipe = semget(SEM_PIPE_KEY, 1, IPC_CREAT | 0600);
    if(semCodePipe < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }
    arg.val = 0;
    semctl(semCodePipe, 0, SETVAL, arg);

    //Configuro el named pipe por el que slaves mandan hashes el app lee
    mkfifo(FIFO_NAME, IPC_CREAT | 0640);
    int hashReaderfd = open(FIFO_NAME, O_RDONLY); //solo quiero que lea

    tSlave slaves[NUM_SLAVES];

    //Creo los pipes
    int argFile = 1;
    int numFiles = argc - 1;
    char buff[256];
    for(int i = 0; i < NUM_SLAVES; i++) {
        pipe(slaves[i].fd);
        //Voy mandando las cargas iniciales
        for(int j = 0; j < INITIAL_LOAD && argFile <= numFiles; j++) {
            strcpy(buff, args[argFile]);
            strcat(buff, '\n');
            printf("sent %s to child\n", args[argFile]);
            write(slaves[i].fd[WRITE_END], buff, strlen(buff));
            argFile++;
        }
    }

    //Variables para select
    fd_set set;
    FD_ZERO(&set);
    for(int i = 0; i < NUM_SLAVES; i++) {
        FD_SET(slaves[i].fd[WRITE_END], &set);
    }

    //Forkeo
    for(int i = 0; i < NUM_SLAVES; i++) {
        if((slaves[i].pid = fork()) == 0) {
            close(STDIN_FILENO);
            dup(slaves[i].fd[READ_END]);
            close(slaves[i].fd[WRITE_END]);
            if(execv("./slaveProcess", (char*[]){"./slaveProcess", NULL}) < 0) {
                printf("%s\n", strerror(errno));
            }
        }
        close(slaves[i].fd[READ_END]);
    }

    return 0;

}

int getHighestWriteDescriptor(tSlave* slave, int numberOfSlaves) {
    int max = 0;
    for(int i = 0; i < numberOfSlaves; i++) {
        if(max < slave[i].fd[WRITE_END]) {
            max = slave[i].fd[WRITE_END];
        }
    }
    return max;
}