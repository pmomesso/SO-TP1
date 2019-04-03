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

int getHighestReadDescriptor(tSlave* slave, int numberOfSlaves);

struct Slave {
    int fdPtoC[2];
    int fdCtoP[2];
    pid_t pid;
};

int main(int argc, char* argv[]) {
   
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
  
    tSlave slaves[NUM_SLAVES];

    //Creo los pipes
    int argFile = 1;
    int numFiles = argc - 1;
    char buff[256] = {0};
    for(int i = 0; i < NUM_SLAVES; i++) {
        //Creo el pipe del padre al hijo y voy mandando las cargas iniciales
        pipe(slaves[i].fdPtoC);
        for(int j = 0; j < INITIAL_LOAD && argFile <= numFiles; j++) {
            strcpy(buff, argv[argFile]);
            strcat(buff, "\n");
            write(slaves[i].fdPtoC[WRITE_END], buff, strlen(buff));
            argFile++;
            
        }
        //Creo el pipe del hijo al padre.
        pipe(slaves[i].fdCtoP);
    }

   

    //Forkeo
    for(int i = 0; i < NUM_SLAVES; i++) {
        if((slaves[i].pid = fork()) == 0) {
           //Cambio la entrada estandar por el pipe del padre al hijo.
            close(STDIN_FILENO);
            dup(slaves[i].fdPtoC[READ_END]);
            close(slaves[i].fdPtoC[READ_END]);
            //Cambio la salida estandar por el pipe del hijo al padre.
            close(STDOUT_FILENO);
            dup(slaves[i].fdCtoP[WRITE_END]);
            close(slaves[i].fdCtoP[WRITE_END]);
            //Cierro el extremo de escritura del pipe del padre al hijo.
            close(slaves[i].fdPtoC[WRITE_END]);
            //Cierro el extremo de lectura del pipe del hijo al padre.
            close(slaves[i].fdCtoP[READ_END]);

            if(execv("./slaveProcess", (char*[]){"./slaveProcess", NULL}) < 0) {
                printf("%s\n", strerror(errno));
            }
        }
        //Ciero el extremo de lectura del pipe del padre al hijo.
        close(slaves[i].fdPtoC[READ_END]);
        //Cierro el extremo de escritura del pipe del hijo al padre.
        close(slaves[i].fdCtoP[WRITE_END]);
    }
    
    fd_set set;
    int maxfd = getHighestReadDescriptor(slaves,NUM_SLAVES);
    char readBuffer[256] = {0};
    char writeBuffer[256] = {0};
    int recievedFiles = 0;
    while(recievedFiles < numFiles ){
        //Creo el set de pipes para leer.
        FD_ZERO(&set);
        for(int i = 0; i < NUM_SLAVES; i++) 
            FD_SET(slaves[i].fdCtoP[READ_END], &set);

        //Hago el select para ver que pipes tienen para leer.
        if(select(maxfd + 1,&set,NULL,NULL,NULL) > 0){
            for(int i = 0; i < NUM_SLAVES; i++){
                //Me fijo si el pipe esta listo para leer.
                if(FD_ISSET(slaves[i].fdCtoP[READ_END],&set)){
                    int j = 0;
                    //Almaceno en readBuffer hasta encontrar un '\0' o hasta que no haya mas para leer.
                    while(read(slaves[i].fdCtoP[READ_END],readBuffer + j,1) > 0 && readBuffer[j] != '\0')
                         j++;
                    printf("%s\n",readBuffer);
                    recievedFiles++;
                    memset(readBuffer,0,j);
                    //Escribo al hijo que acabo de mandar el hash si quedan archivos.
                    if(argFile <= numFiles){
                        strcpy(writeBuffer, argv[argFile++]);
                        strcat(writeBuffer, "\n");
                        write(slaves[i].fdPtoC[WRITE_END], writeBuffer, strlen(writeBuffer));
                        memset(writeBuffer,0,strlen(writeBuffer));
                     }
                }
            }
        }
    }
    return 0;

}

int getHighestReadDescriptor(tSlave* slave, int numberOfSlaves) {
    int max = 0;
    for(int i = 0; i < numberOfSlaves; i++) {
        if(max < slave[i].fdCtoP[READ_END]) {
            max = slave[i].fdCtoP[READ_END];
        }
    }
    return max;
}