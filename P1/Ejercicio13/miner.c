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