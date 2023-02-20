/**
 * @file minero.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Implementation of the miner funcionality
 * @date 2023-02-14
 * 
 */

#include "miner.h"

typedef struct _minerData { //todos los hilos comparten esta estructura
    long target;
    long solution;
    volatile int i; //comunicar hilos, empieza a calcular desde la i del otro hilo
    // volatile obliga a volver a mirar el valor de la variable antes de usarla, evitando que 2 hilos usen el mismo valor evitando el uso de semaforos
    } MinerData;

static MinerData minerData;

/**
 * @brief Function that will execute the threads
 * 
 * @param args 
 * @return void* 
 */
void *work(void* args){
    long aux;
    MinerData *minerData = (MinerData*) args;
    while(minerData->i < POW_LIMIT && minerData->solution == -1){
        aux = minerData->i;
        minerData->i++;
        if(minerData->target == pow_hash(aux)){
            minerData->solution = aux;
        // printf("AUX:%d\n",aux);
            return NULL;
        }
    }
    return NULL;
}

int miner(int rounds, int nthreads, long target, int monitorPipe, int minerPipe){
    int i, j;
    pthread_t *threads;
    char resp;

    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    if(threads == NULL){
        perror("Error allocating memory for the threads");
        close(monitorPipe);
        close(minerPipe);
        exit(EXIT_FAILURE);
    }
    //minerData = (struct _minerData*) malloc(nthreads * sizeof(_minerData));

    minerData.solution = target;
    for(i = 0; rounds <= 0 || i < rounds; i++){
        minerData.i = 0;
        minerData.target = minerData.solution;
        minerData.solution = -1;

        for(j = 0; j < nthreads; j++){
            if(pthread_create(&threads[j], NULL, work, &minerData)){
                perror("pthread_create");
                break;
            }
        }

        for(j = 0; j < nthreads; j++){
            if(pthread_join(threads[j], NULL)){
                perror("pthread_join");
                free(threads);
                break;
            }
        }

        write(minerPipe, &minerData, sizeof(long)*2); //no hace falta poner minerData[0] ya que minerData es nuestra direccion y sizeof(long)*2 es el OFFSET
        read(monitorPipe, &resp, sizeof(char)); //same here, pero es bloqueante
        if(!resp){
            printf("The solution has been invalidated\n");
            free(threads);
            exit(EXIT_FAILURE);
        }
    }
    
    free(threads);
    exit(EXIT_SUCCESS);
}