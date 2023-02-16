/**
 * @file mrush.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Main program for the miner rush
 * @date 2023-02-14
 * 
 */

#include "miner.h"
void wait4miner(void *pid);
void wait4monitor(void *pid);


/**
 * @brief Main function for the miner rush
 * 
 * @param argc 
 * @param argv 
 * @return 0 Exit success or 1 Exit failure
 */
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

    pipeStatus = pipe(minerPipe);
    if(pipeStatus == -1){
        perror("Error creating the pipe");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        exit(EXIT_FAILURE);
    }

    minerPID = fork();
    if (minerPID < 0){
        perror("Error creating the miner process");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        close(minerPipe[0]);
        close(minerPipe[1]);
        exit(EXIT_FAILURE);
    }

    monitorPID = fork();
    if (monitorPID < 0){
        perror("Error creating the monitor process");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        exit(EXIT_FAILURE);
    }
    
    if (monitorPID == 0){
        close(monitorPipe[1]);
        close(minerPipe[0]);
        close(minerPipe[1]);
        monitor(monitorPipe[0]);
        exit(EXIT_SUCCESS);
    }

    close(monitorPipe[0]);
    close(monitorPipe[1]);
    close(minerPipe[0]);
    close(minerPipe[1]);

    if(pthread_create(&minerThread, NULL, wait4miner, NULL) != 0){
        perror("Error creating the miner thread");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&monitorThread, NULL, wait4monitor, NULL) != 0){
        perror("Error creating the monitor thread");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(minerThread, NULL) != 0){
        perror("Error joining the miner thread");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(monitorThread, NULL) != 0){
        perror("Error joining the monitor thread");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/**
 * @brief Wait for the miner process to finish
 * 
 * @param pid pid of the miner process
 * @return void
 */
void wait4miner(void *pid){
    int status;
    waitpid(*(int *)pid, &status, 0);
    if(WIFEXITED(status)){
        printf("Miner process finished with status %d \n", WEXITSTATUS(status));
    }
    else{
        printf("Miner process finished abnormally \n");
    }
    exit(EXIT_SUCCESS);
}

/**
 * @brief Wait for the monitor process to finish
 * 
 * @param pid pid of the monitor process
 * @return void
 */
void wait4monitor(void *pid){
    int status;
    waitpid(*(int *)pid, &status, 0);
    if(WIFEXITED(status)){
        printf("Monitor process finished with status %d \n", WEXITSTATUS(status));
    }
    else{
        printf("Monitor process finished abnormally \n");
    }
    exit(EXIT_SUCCESS);
}