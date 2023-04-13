/**
 * @file minero.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Implementation of the miner funcionality
 * @date 2023-02-14
 * 
 */

#include "miner.h"

typedef struct _pipeData { 
    long target;
    long solution;
} PipeData;

typedef struct _minerData {
    long start;
    long end;
    long target;
} MinerData;

static PipeData pipeData;

int magicFlag = 0;
/**
 * @brief Private function that will execute the threads
 * 
 * @param args 
 * @return void* 
 */
void *work(void* args){
    long i, result;
    MinerData *minerData = (MinerData*) args;
    for(i = minerData->start; i < minerData->end; i++){
        if(magicFlag)
            return NULL;
        
        result = pow_hash(i);
        if(result == minerData->target){
            pipeData.solution = i;
            pipeData.target = result;
            magicFlag = 1;
            return NULL;
        }
    }
    return NULL;
}

int miner(int rounds, int nthreads, long target, int monitorPipe, int minerPipe){
    int i, j;
    pthread_t *threads;
    short resp;
    ssize_t nbytes;
    
    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    if(threads == NULL){
        perror("Error allocating memory for the threads");
        close(monitorPipe);
        close(minerPipe);
        exit(EXIT_FAILURE);
    }   

    MinerData *minerData = malloc(sizeof(MinerData)*nthreads);
    if(minerData == NULL){
        perror("Error allocating memory for the minerData");
        free(threads);
        close(monitorPipe);
        close(minerPipe);
        exit(EXIT_FAILURE);
    }

    for(i = 0; rounds <= 0 || i < rounds; i++){
        for(j = 0; j < nthreads; j++){
            minerData[j].start = j * ((POW_LIMIT -1 ) / nthreads);
            minerData[j].end = (j+1) * ((POW_LIMIT -1 ) / nthreads);
            minerData[j].target = target;
            if(pthread_create(&threads[j], NULL, work, &minerData[j])){
                perror("pthread_create");
                free(minerData);
                free(threads);
                close(monitorPipe);
                close(minerPipe);
                exit(EXIT_FAILURE);
            }
        }
        for(j = 0; j < nthreads; j++){
            if(pthread_join(threads[j], NULL)){
                perror("pthread_join");
                    free(minerData);
                    free(threads); 
                    close(monitorPipe);
                    close(minerPipe);
                    exit(EXIT_FAILURE);
                break;
            }
        } //TODO: controlar el retorno de write y read con ERRNO

        do{
            nbytes = write(minerPipe, &pipeData, sizeof(long)*2); //minerData is our direction and sizeof(long)*2 is the OFFSET
            if(nbytes < 0){
                perror("Error WRITING to the pipe in the miner");
                exit(EXIT_FAILURE);
            }
            nbytes = 0;
            nbytes = read(monitorPipe, &resp, sizeof(short)); //same here, but its blocking
            if(nbytes < 0){
                perror("Error READING from the pipe in the miner");
                exit(EXIT_FAILURE);
            }
                    
            if(!resp){
                printf("The solution has been invalidated\n");
                free(minerData);
                free(threads);
                exit(EXIT_FAILURE);
            }
        } while(nbytes < sizeof(short));

        target = pipeData.solution;
        magicFlag = 0;

    }
    free(minerData);
    free(threads);

    exit(EXIT_SUCCESS);
}