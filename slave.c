#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "tp1.h"

int main(void) {

    //Argumentos para ejecutar el md5sum.
    char* arguments[3];
    arguments[0] = "/usr/bin/md5sum";
    arguments[2] = NULL;
    arguments[1] = malloc(sizeof(char)*256);

    ssize_t bytesRead;

    char* buff1 = NULL;
    size_t size = 0;

    //Leo hasta que  un end of file de la salida estandar.
    while((bytesRead = getline(&buff1, &size, stdin)) > 0) {
        int fd[2];
        //Abro el pipe para comunicarme con el proceso de md5sum.
        if(pipe(fd) < 0) {
            perror("SLAVE.C: ERROR WHILE OPENING PIPE\n");
            perror(strerror(errno));
            exit(1);
        }
        //Hago el fork con el proceso md5sum.
        if(fork() == 0) {
            close(1);
            dup(fd[WRITE_END]);
            strcpy(arguments[1], buff1);
            arguments[1][bytesRead - 1] = '\0';
            if(execv("/usr/bin/md5sum", arguments) < 0) {
                perror("SLAVE.C: ERROR WHILE FORKING\n");
                perror(strerror(errno));
                exit(1);
            }
        } else {
            close(fd[WRITE_END]);
        }
        //Leo del pipe del hijo el valor del hash.
        char buff2[256];
        int i = 0;
        while(read(fd[READ_END], buff2 + i, 1) > 0) {
            i++;
        }
        buff2[i - 1] = '\0';
        close(fd[READ_END]);
        //Imprimo por salida estandar el valor del hash.
        printf("%s\n", buff2);
        fflush(stdout);
        
    }
    //Libero la memoria de los buffer.
    free(buff1);
    free(arguments[1]);

}