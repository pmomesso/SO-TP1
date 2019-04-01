#include "tp1.h"
#include <sys/sem.h>
#include <sys/ipc.h>
//el que tenga docker fijese que header hay que incluir para usar select
struct Slave {
    pid_t pid;
    int fd[2];
};

void getSemaphore(int semCode) {
    union a {
        int val;
    } arg;
    arg.val = 1;
    semctl(semCode, 0, SETVAL, arg);
    return;
}

void freeSemaphore(int semCode) {
    union a {
        int val;
    } arg;
    arg.val = 0;
    semctl(semCode, 0, SETVAL, arg);
    return;
}

void waitSemaphore(int semCode) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 0;
    sop.sem_flg = 0;
    semop(semCode, &sop, 1);
    return;
}

tSlave * createSlaves(int slaveAmmount)
{
    tSlave * PtoCpipes;  //un pipe por cada app-->slave por cada uno

    for(int i = 0;i < slaveAmmount;i++)
    {
        pipe(PtoCpipes.fd);
        if(fork() == 0)
        {
            //Nos interesa que el hijo lea no de entrada estandar sino del pipe que le proveemos
            close(STDIN_FILENO);
            dup(PtoCpipes.fd[READ_END]);
            close(PtoCpipes.fd[WRITE_END]); //El hijo no debe de escribir a fdParentToChild
            execv("./slave", (char*[]){"./slave", NULL});
        }
        close(PtoCpipes.fd[READ_END]);
    }
    return PtoCpipes;
}

int getMaxFd(tSlave * slaves,int quantity) //la necesito para el select
{
    int max = 0;
    for(int i = 0;i < quantity;i++)
    {
        if(max < slaves[i].fd[WRITE_END])
        {
            max = slaves[i].fd[WRITE_END];
        }
    }
    return max;
}