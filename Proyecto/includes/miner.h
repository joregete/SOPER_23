#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_MINERS 100

/**
 * @brief Miner structure
 */
typedef struct _miner{
    pid_t pid; // pid of the miner
    short coins; // coins the miner owns
} Miner;

typedef struct _minerData {
    long start;
    long end;
    long target;
} MinerData;

/**
 * @brief Block structure
 */
typedef struct _block{
    short id; // unique block id
    long target; // target to be solved
    long solution; // solution to the target
    pid_t winner; // pid of the miner that solved the POW
    Miner miners[MAX_MINERS]; // list of miners that voted for this block
    uint8_t total_votes; // total votes for this block
    uint8_t favorable_votes; // favorable votes for this block
} Block;

/**
 * @brief System structure, this is the shared memory
 */
typedef struct _system{
    Miner miners[MAX_MINERS]; // list of active miners in the system
    uint8_t num_miners; // number of active miners in the system
    Block last_block; // last block mined
    Block current_block; // current block being mined
    sem_t mutex; // protection for shared memory
    uint8_t monitor_up; // flag to indicate if the monitor is up
} System;