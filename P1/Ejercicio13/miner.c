/**
 * @file minero.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Implementation of the miner funcionality
 * @date 2023-02-14
 * 
 */

#include "miner.h"

struct _miner_args {
    int i;
    long target;
    long solution;
};

struct _miner_args miner_args;

void* work(void* args) {
    int aux;
    struct _miner_args* minerArgs = (struct _miner_args*) args;
    while (minerArgs->i < POW_LIMIT && minerArgs->solution == -1){
        aux = minerArgs->i++;
        if (minerArgs->target == pow_hash(aux)) {
            minerArgs->solution = aux;
            return NULL;
        }
    }
    return NULL;
}

int miner (int rounds, int nthreads, long target, int Psince, int Pwhere){
    int i, j, status;
    pthread_t* threads;
    struct _miner_args* minerArgs;
    long solution = -1;

    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    minerArgs = (struct _miner_args*) malloc(nthreads * sizeof(struct _miner_args));

    for (i = 0; i < rounds; i++){
        for (j = 0; j < nthreads; j++){
            minerArgs[j].i = Psince + j;
            minerArgs[j].target = target;
            minerArgs[j].solution = -1;
            if(pthread_create(&threads[j], NULL, work, &minerArgs[j])){
                perror("Error creating the thread");
                exit(EXIT_FAILURE);
            }
        }
        for (j = 0; j < nthreads; j++){
            if(pthread_join(threads[j], NULL) != 0){
                perror("Error joining the thread");
                exit(EXIT_FAILURE);
            }
            if (minerArgs[j].solution != -1){
                solution = minerArgs[j].solution;
                break;
            }
        }
        if (solution != -1){
            break;
        }
        for (j = 0; j < nthreads; j++){
            Psince += nthreads;
            write(Pwhere, &Psince, sizeof(int));
            read(Psince, &Psince, sizeof(int));
        }
    }
    free(threads);
    free(minerArgs);
    exit(EXIT_SUCCESS);
}