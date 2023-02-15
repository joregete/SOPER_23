/**
 * @file mrush.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Main program for the miner rush
 * @date 2023-02-14
 * 
 */

#include "miner.h"

int main(int argc, char *argv[]){
    int minerPID, monitorPID, rounds, nthreads, target;;
    pthread_t minerThread, monitorThread;
    int monitorPipe[2], minerPipe[2];
    int pipeStatus, threadStatus;

    if (argc != 4){
        printf("Usage: %s <TARGET> <ROUNDS> <NTHREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    target = atoi(argv[1]);
    if(target < 0 || target > POW_LIMIT ){
        fprintf(stderr, "TATGET_INI must be between 0 and %d", POW_LIMIT);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[2]);
    if (rounds <= 0){
        fprintf(stderr, "ROUNDS have to be greater than 0");
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[3]);
    if (nthreads <= 0){
        fprintf(stderr, "The amount of miners(threads) have to be greater than 0");
        exit(EXIT_FAILURE);
    }
    if (nthreads > MAX_MINERS){
        fprintf(stderr, "MAX_MINERS have to be smaller than %d", MAX_MINERS);
        exit(EXIT_FAILURE);
    }

    pipeStatus = pipe(monitorPipe);
    if(pipeStatus == -1){
        perror("Error creating the pipe");
        exit(EXIT_FAILURE);
    }

}