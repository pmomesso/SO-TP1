#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "tp1.h"

#define MAX_LENGTH 512

int main(void) {

    int hashWriteFd;
    size_t bytesRead;
    char* filePath = NULL;
    ssize_t size = 0;

    //accedo a named pipe solo para escritura
    hashWriteFd = open(FIFO_FILE, O_WRONLY);

    //Obtengo la referencia al semaforo para pipe
    int semCodePipe = semget(SEM_KEY, 0, 0200);
    if(semCodePipe < 0) {
        perror("PROBLEM DURING EXECUTION\n");
        exit(1);
    }

    FILE *cmdPipe = NULL;
    char *hashBuff = calloc(MAX_LENGTH,sizeof(char));
    char *cmd = calloc(MAX_LENGTH,sizeof(char));
    while((bytesRead = getline(&filePath, &size, stdin)) > 0) {
        //Creo el comando.
        strcat(cmd,"md5sum");
        strcat(cmd,filePath);
        //Abre el pipe a la shell para leer el resultado.
        cmdPipe = popen(cmd,"r");
        //Leo respuesta.
        int length = 0;
        int c;
        while((c = fgetc(cmdPipe) != EOF) && length < MAX_LENGTH - 1)
            hashBuff[length++] = (char) c;
        hashBuff[length] = '\0';
        //Escribo al padre.
        waitSemaphore(semCodePipe);
        getSemaphore(semCodePipe);
        write(hashWriteFd,hashBuff,length);
        freeSemaphore(semCodePipe);
        //printf("%s\n", buff2);
        pclose(cmdPipe);
        //Limpia los buffers del hash y el comando poniendolos en 0.
        memset(hashBuff,0,length);
        memset(cmd,0,MAX_LENGTH);
    }
    free(hashBuff);
    free(cmd);
    free(filePath);
}