/**
 * @file miner.h
 * @author Enmanuel, Jorge
 * @brief Miner header file
 * @version 0.1
 * @date 2023-04-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

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
#include <mqueue.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_MINERS 100
#define MAX_MSG 9
#define MQ_NAME "/mq_facepulls"
#define SYSTEM_SHM "/deadlift_shm"

/**
 * @brief Miner structure
 */
typedef struct _miner{
    pid_t pid; // pid of the miner
    short coins; // coins the miner owns
} Miner;

/**
 * @brief Miner Data structure
 */
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
    Miner miners[MAX_MINERS]; // list of miners that mined for this block
    uint8_t num_voters; // number of miners that have to vote for this block
    uint8_t total_votes; // total votes for this block
    uint8_t favorable_votes; // favorable votes for this block
} Block;

#define SIZE sizeof(Block)

/**
 * @brief System structure, this is the shared memory
 */
typedef struct _system{
    Miner miners[MAX_MINERS]; // list of active miners in the system
    uint8_t num_miners; // number of active miners in the system
    Block last_block; // last block mined
    Block current_block; // current block being mined
    sem_t mutex; // protection for shared memory
    uint8_t monitor_up; // flag to check if the monitor is up
} System;

/**
 * @brief initialize a new block
 * 
 * @param block block to initialize
 * @param last_id id of the last block
 * @param target target to be solved
 * @param miner miner that created the block
 */
void init_block(Block *block, short last_id, int target);


/**
 * @brief function to find the index of a miner in the miners array
 * 
 * @param miners array
 * @param num_miners number of miners in the array
 * @param pid pid of the miner to find
 * @return uint8_t index of the miner in the array 
 */
uint8_t find_miner_index(Miner *miners, uint8_t num_miners, pid_t pid);

/**
 * @brief function to delete a miner from the miners array
 * 
 * @param miners miners array
 * @param num_miners number of miners in the array
 * @param pid pid of the miner to delete
 */
void delete_miner(Miner *miners, uint8_t *num_miners, pid_t pid);

/**
 * @brief funtion that the child process will execute
 * 
 * @param pipe_read read end of the pipe, used to read the blocks
 */
void _register(int pipe_read);

/**
 * @brief function that checks the parameters passed to the program
 * @param argc int
 * @param argv char**
 * @param n_sec uint8_t
 * @param nthreads uint8_t
 */
void check_args(int argc, char *argv[], uint8_t *n_sec, uint8_t *nthreads);

/**
 * @brief open memory segment for system, upload initial values
 * @return System* new system
 */
System* create_system();
