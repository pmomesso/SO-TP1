#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(void) {

    ssize_t bytesRead = 0;
    ssize_t n = 0;
    char** lineRead;
    *lineRead = NULL;
    while((bytesRead = getline(lineRead, &n, stdin)) > 0) { 
        int fd[2];
        pipe(fd);
        (*lineRead)[bytesRead-1] = '\0';
        printf("FROM CHILD: %s\n", *lineRead);
        if(fork() == 0) {
            //Nos interesa que md5sum escriba no a salida estandar sino al pipe que le proveemos para poder recuperar el hash
            close(1); 
            dup(fd[1]);
            close(fd[0]);
            execv("/usr/bin/md5sum", (char*[]){"/usr/bin/md5sum", *lineRead, NULL});
        }
        
        close(fd[1]); //Como padre no queremos escribir

        char buff[256];
        int i = 0;
        while((read(fd[0], buff + i, 1)) > 0) {
            printf("%c", buff[i]);
        }

    }

}