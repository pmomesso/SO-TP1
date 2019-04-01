#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "tp1.h"
#include <sys/sem.h>
#include <sys/ipc.h>

int main(void) {

    int hashWriteFd
    char* arguments[3];
    arguments[0] = "/usr/bin/md5sum";
    arguments[2] = NULL;
    arguments[1] = malloc(sizeof(char)*256);

    size_t bytesRead;

    char* buff1 = NULL;
    ssize_t size = 0;

    //accedo a named pipe solo para escritura
    hashWriteFd = open(FIFO_FILE, O_WRONLY);

    //Obtengo la referencia al semaforo para pipe
    int semCodePipe = semget(SEM_KEY, 0, 0200);
    if(semCodePipe < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    while((bytesRead = getline(&buff1, &size, stdin)) > 0) {
        int fd[2];
        if(pipe(fd) < 0) {
            printf("%s\n", strerror(errno));
        }
        if(fork() == 0) {
            close(1);
            dup(fd[WRITE_END]);
            strcpy(arguments[1], buff1);
            arguments[1][bytesRead - 1] = '\0';
            if(execv("/usr/bin/md5sum", arguments) < 0) {
                printf("%s\n", strerror(errno));
            }
        } else {
            close(fd[WRITE_END]);
        }

        char buff2[256];
        int i = 0;
        while(read(fd[READ_END], buff2 + i, 1) > 0) {
            i++;
        }
        buff2[i - 1] = '\0';
        close(fd[READ_END]);
        waitSemaphore(semCodePipe);
        getSemaphore(semCodePipe);
        write(hashWriteFd,buff2,i);
        freeSemaphore(semCodePipe);
        //printf("%s\n", buff2);
    }

    free(buff1);
    free(arguments[1]);
}