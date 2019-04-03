#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "tp1.h"

int main(void) {

    char* arguments[3];
    arguments[0] = "/usr/bin/md5sum";
    arguments[2] = NULL;
    arguments[1] = malloc(sizeof(char)*256);

    ssize_t bytesRead;

    char* buff1 = NULL;
    ssize_t size = 0;

    while((bytesRead = getline(&buff1, &size, stdin)) != -1) {

        int fd[2];
        if(pipe(fd) < 0) {
            printf("%s\n", strerror(errno));
        }
        if(fork() == 0) {
            close(STDOUT_FILENO);
            dup(fd[WRITE_END]);
            close(fd[WRITE_END]);
            strcpy(arguments[1], buff1);
            arguments[1][bytesRead - 1] = '\0';
            if(execv("/usr/bin/md5sum", arguments) < 0) {
                printf("%s\n", strerror(errno));
            }
        } else {
            close(fd[WRITE_END]);
        }
        char buff2[256] = {0};
        int i = 0;
        while(read(fd[READ_END], buff2 + i, 1) > 0) {
            i++;
        }
        buff2[i - 1] = '\0';
        close(fd[READ_END]);
        write(STDOUT_FILENO,buff2,i);

    }

    free(buff1);
    free(arguments[1]);

}