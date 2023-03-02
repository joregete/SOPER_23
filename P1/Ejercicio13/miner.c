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
        write(minerPipe, &pipeData, sizeof(long)*2); //minerData is our direction and sizeof(long)*2 is the OFFSET
        read(monitorPipe, &resp, sizeof(short)); //same here, but its blocking

        target = pipeData.solution;
        magicFlag = 0;
        
        if(!resp){
            printf("The solution has been invalidated\n");
            free(threads);
            exit(EXIT_FAILURE);
        }
    }
    
    free(threads);
    free(minerData);

    exit(EXIT_SUCCESS);
}