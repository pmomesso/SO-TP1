#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include "tp1.h"

#define SLAVES 5 //constante para cantidad de esclavos, si llegamos seria bueno cambiarla por una funcion

int int main(int argc, char const *argv[])
{
    

    int hashReaderFd:
    //Configuro la memoria compartida
    int shmcode = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0200);
    void* shmemStart;
    if((shmemStart = shmat(shmcode, NULL, 0)) == NULL) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    //Configuro el semaforo para shared memory
    int semCodeSh = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    if(semCodeSh < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }
    union semun {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCodeSh, 0, SETVAL, arg);

    //Configuro el semaforo para named pipe
    int semCodePipe = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    if(semCodePipe < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }
    union semun {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCodePipe, 0, SETVAL, arg);

    //configuro el named pipe por el que slaves mandan hashes el app lee
    mkfifo(FIFO_FILE, S_IFIFO|0640);
    //solo quiero que lea
    hashReaderfd = open(FIFO_FILE, O_RDONLY);

    //Comienzo el proceso vista (DE PRUEBA, NO PARA EL TP). El proceso vista deberia de escribir a la memoria el alfabeto
    if(fork() == 0) {
        execv("./view", (char*[]){"./view", NULL});
    }

    sleep(1);

    slave_t * slaves = createSlaves(SLAVES);


    fd_set wfds;
    int selectRet;
    int fileNumber = 0;
    int slaveAmmount = SLAVES; //aca iria la funcion que determine cuantos
    int maxFd = getMaxFd(slaves,slaveAmmount); //maximo fd ocupado para select
    int files = argc;

    //inicializacion del set para select
    FD_ZERO(&wfds);
    for(int i = 0; i < slaveAmmount; i++)
    {
        FD_SET(slaves[i].fd[WRITE_END], &wfds); //agrego los fd a monitorear
    }
    while(fileNumber < files) //la idea es que corte cuando ya se terminaron los archivos, no se si tiene sentido asi
    {
        selectRet = select(maxFd,NULL,&wfds,NULL,NULL);

        if(selectRet == -1)
        {
            perror("select()");
        }
        else if(selectRet) //hay alguno disponible para escribir
        {
            for(i = 0; i < slaveAmmount && fileNumber < files;i++)
            {
                //si el fd esta en el set, puedo escribir
                if(FD_ISSET(slaves[i].fd[WRITE_END], &wfds)) 
                {
                    //mando el archivo de una
                    char * file = argv[fileNumber];
                    int fileLength = strlen(file);
                    char * fileToSend = malloc(fileLength * sizeof(char)+1);
                    for(int j = 0; j < fileLength; j++)
                    {
                        fileToSend[j] = file[j];
                    }
                    fileToSend[j] = '\n';
                    write(slave[i].fd[WRITE_END], fileToSend, fileLength+1);
                    free(fileToSend);
                    fileNumber++;
                }
            }
        }
        //a partir de aca ver el named pipe para lectura
        waitSemaphore(semCodePipe);
        getSemaphore(semCodePipe);
        //el read
        printf("\n");
        freeSemaphore(semCodePipe);
    }
// lo del select a partir de lo visto en ejemplo de man



    //Pruebo la memoria compartida y el semaforo. Deberia de imprimirse el alfabeto
    waitSemaphore(semCodeSh);
    getSemaphore(semCodeSh);
    char* auxShmStart = (char*) shmemStart;
    for(char a = 'a'; a <= 'z'; a++) {
        printf("%c ", *auxShmStart);
        auxShmStart++;
    }
    printf("\n");
    freeSemaphore(semCodeSh);

    return 0;

}