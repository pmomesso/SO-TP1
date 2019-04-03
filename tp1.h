/*---------------------*/
/*Funciones de utilidad*/
/*---------------------*/

#define READ_END 0
#define WRITE_END 1

#define SEM_SHMEM_KEY 1234
#define SHM_SIZE 1


/*Para uso en main.c*/
typedef struct Slave tSlave;

/*Para uso de semaforos*/

/*Coloca el valor del semaforo en 1*/
void getSemaphore(int semCode);

/*Coloca el valor del semaforo en 0*/
void freeSemaphore(int semCode);

/*Deja el proceso en espera hasta que el valor del semaforo sea 0*/
void waitSemaphore(int semCode);