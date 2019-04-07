#ifndef TP1H
#define TP1H

/*---------------------*/
/*Funciones de utilidad*/
/*---------------------*/

#define READ_END 0
#define WRITE_END 1

#define SEM_SHMEM_KEY 1234
#define SHMEM_KEY 0x1234
#define SHM_SIZE 1

#define APPLICATION_FINISHED -1

/*Para uso en main.c*/
typedef struct Slave tSlave;

/*Para uso de semaforos*/

/*Coloca el valor del semaforo en 1*/
void getSemaphore(int semCode);

/*Coloca el valor del semaforo en 0*/
void freeSemaphore(int semCode);

#endif