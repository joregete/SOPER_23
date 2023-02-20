/**
 * @file mrush.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Main program for the miner rush
 * @date 2023-02-14
 * 
 */

#include "miner.h"
#include "monitor.h"
void *wait4miner(void *pid);
void *wait4monitor(void *pid);


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
    int monitorPipe[2], minerPipe[2]; // monitorPipe: comunicates monitor with miner; minerPipe: comunicates miner with monitor
    int pipeStatus;

    if (argc != 4){
        printf("Usage: %s <TARGET> <ROUNDS> <NTHREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    target = atoi(argv[1]);
    if(target < 0 || target > POW_LIMIT ){
        fprintf(stderr, "TATGET_INI must be between 0 and %d\n", POW_LIMIT);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[2]);
    if (rounds <= 0){
        fprintf(stderr, "ROUNDS have to be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[3]);
    if (nthreads <= 0){
        fprintf(stderr, "The amount of miners(threads) have to be greater than 0\n");
        exit(EXIT_FAILURE);
    }
    if (nthreads > MAX_MINERS){
        fprintf(stderr, "MAX_MINERS have to be smaller than %d\n", MAX_MINERS);
        exit(EXIT_FAILURE);
    }

    pipeStatus = pipe(monitorPipe);  // read in position 0 and write in position 1
    if(pipeStatus < 0){
        perror("Error creating miner pipe\n");
        exit(EXIT_FAILURE);
    }

    pipeStatus = pipe(minerPipe);
    if(pipeStatus < 0){
        perror("Error creating miner pipe\n");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        exit(EXIT_FAILURE);
    }

    minerPID = fork();
    if (minerPID < 0){
        perror("Error creating the miner process\n");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        close(minerPipe[0]);
        close(minerPipe[1]);
        exit(EXIT_FAILURE);
    }
    
    if (minerPID == 0){
        close(monitorPipe[1]); //we close the write position of the monitor, we are not using it
        close(minerPipe[0]); //we close the read position of the miner, we are not using it
        if(miner(rounds, nthreads, target, monitorPipe[0], minerPipe[1]) == EXIT_FAILURE){
            perror("Error executing the miner process\n");
            exit(EXIT_FAILURE);
        }
    }

    monitorPID = fork();
    if (monitorPID < 0){
        perror("Error creating the monitor process\n");
        close(monitorPipe[0]);
        close(monitorPipe[1]);
        close(minerPipe[0]);
        close(minerPipe[1]);
        exit(EXIT_FAILURE);
    }
    
    if (monitorPID == 0){
        close(monitorPipe[0]); // we close the read position of the monitor, we are not using it
        close(minerPipe[1]); // we cose the write position of the miner, we are not using it
        if(monitor(monitorPipe[1], minerPipe[0]) == EXIT_FAILURE){
            perror("Error executing the monitor process\n");
            exit(EXIT_FAILURE);
        }
    }

    close(minerPipe[0]);
    close(minerPipe[1]);
    close(monitorPipe[0]);
    close(monitorPipe[1]);

    if(pthread_create(&minerThread, NULL, wait4miner, &minerPID)){
        perror("Error creating the miner thread\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&monitorThread, NULL, wait4monitor, &monitorPID)){
        perror("Error creating the monitor thread\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(minerThread, NULL)){
        perror("Error joining the miner thread\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(monitorThread, NULL)){
        perror("Error joining the monitor thread\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Wait for the miner process to finish
 * 
 * @param pid pid of the miner process
 * @return void
 */
void *wait4miner(void *pid){
    int status;
    waitpid(*(int *)pid, &status, 0);
    if(WIFEXITED(status))
        printf("Miner process finished with status %d, %s\n", WEXITSTATUS(status), (status)? "FAILURE" : "SUCCESS");
    else
        printf("Miner process finished abnormally \n");

    return NULL;
    // exit(EXIT_SUCCESS);
}

/**
 * @brief Wait for the monitor process to finish
 * 
 * @param pid pid of the monitor process
 * @return void
 */
void *wait4monitor(void *pid){
    int status;
    waitpid(*(int *)pid, &status, 0);
    if(WIFEXITED(status))
        printf("Monitor process finished with status %d, %s\n", WEXITSTATUS(status), (status)? "FAILURE" : "SUCCESS");
    else
        printf("Monitor process finished abnormally\n");
    
    return NULL;
    // exit(EXIT_SUCCESS);
}