#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define READ_END 0
#define WRITE_END 1

int main(void) {

    int fdPtoC[2];
    int fdCtoP[2];
    pipe(fdPtoC);
    pipe(fdCtoP);


    if(fork() == 0) {
        //Nos interesa que el hijo lea no de entrada estandar sino del pipe que le proveemos
        close(STDIN_FILENO);
        dup(fdPtoC[READ_END]);
        close(fdPtoC[WRITE_END]); //El hijo no debe de escribir a fdParentToChild

        //Nos interesa que el hijo escriba no a salida estandar sino al pipe que le proveemos
        close(STDOUT_FILENO);
        dup(fdCtoP[WRITE_END]);
        close(fdCtoP[READ_END]); //El hijo no debe de leer de fdChildToParent

        execv("./slave", (char*[]){"./slave", NULL});
    }

    char arg[256] = "main.c\n";
    write(fdPtoC[WRITE_END], arg, strlen(arg));
    strcpy(arg, "slave.c\n");
    write(fdPtoC[WRITE_END], arg, strlen(arg));
    close(fdPtoC[WRITE_END]);

    close(fdCtoP[WRITE_END]); //El padre no debe de escribir a fdChildToParent

    //Recupero el hash del hijo
    char buff[256];
    int i = 0;
    while((read(fdCtoP[READ_END], buff + i, 1)) > 0) {
        printf("%c", buff[i]);
        i++;
    }

}