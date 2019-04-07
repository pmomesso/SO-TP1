#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "tp1.h"

int main(void) {

    // sleep(1);

    char* arguments[3];
    arguments[0] = "/usr/bin/md5sum";
    arguments[2] = NULL;
    arguments[1] = malloc(sizeof(char)*256);

    ssize_t bytesRead;

    char* buff1 = NULL;
    ssize_t size = 0;

    // perror("antes del while\n");

    while((bytesRead = getline(&buff1, &size, stdin)) > 0) {
        int fd[2];
        if(pipe(fd) < 0) {
            perror("SLAVE.C: ERROR WHILE OPENING PIPE\n");
            perror(strerror(errno));
            exit(1);
        }
        // fprintf(stderr, "child of pid: %d; read: %s\n", getpid(), buff1);
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
        char buff2[256];
        int i = 0;
        while(read(fd[READ_END], buff2 + i, 1) > 0) {
            i++;
        }
        buff2[i - 1] = '\0';
        // fprintf(stderr, "%s\n", buff2);
        close(fd[READ_END]);
        printf("%s\n", buff2);

        // fprintf(stderr, "I read %d bytes\n", bytesRead);
        // fprintf(stderr, "I wrote %s %d\n", buff2, getpid());
        // perror(bytesRead);
        fflush(stdout);
        
    }

    // perror(bytesRead);

    // fprintf(stderr, "finish %d\n", getpid());

    free(buff1);
    free(arguments[1]);

}