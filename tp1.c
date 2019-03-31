#include "tp1.h"
#include <sys/sem.h>
#include <sys/ipc.h>

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