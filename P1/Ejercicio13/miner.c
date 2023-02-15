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
    struct _miner_args* ma = (struct _miner_args*) args;
    while (ma->i < POW_LIMIT && ma->solution == -1){
        aux = ma->i++;
        if (ma->target == pow_hash(aux)) {
            ma->solution = aux;
            return NULL;
        }
    }
    return NULL;
}

int miner (int rounds, int nthreads, long target, int Psince, int Pwhere){
    int i, j, status;
    pthread_t* threads;
    struct _miner_args* ma;
    long solution = -1;

    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    ma = (struct _miner_args*) malloc(nthreads * sizeof(struct _miner_args));

    for (i = 0; i < rounds; i++){
        for (j = 0; j < nthreads; j++){
            ma[j].i = Psince + j;
            ma[j].target = target;
            ma[j].solution = -1;
            pthread_create(&threads[j], NULL, work, &ma[j]);
        }
        for (j = 0; j < nthreads; j++){
            pthread_join(threads[j], NULL);
            if (ma[j].solution != -1){
                solution = ma[j].solution;
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
    free(ma);
    return solution;
}

}