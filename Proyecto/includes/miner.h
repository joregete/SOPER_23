#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

/**
 * @brief Miner structure
 */
typedef struct _miner{
    pid_t pid; // pid of the miner
    short coins; // coins the miner owns
} Miner;

/**
 * @brief Block structure
 */
typedef struct _block{
    int id; // unique block id
    int target; // target to be solved
    int solution; // solution to the target
    int miner_pid; // pid of the miner that solved the POW
    List miners; // list of miners that voted for this block
    short total_votes; // total votes for this block
    short favorable_votes; // favorable votes for this block
} Block;

/**
 * @brief System structure
 */
typedef struct _system{
    List miners; // list of active miners
    Block last_block; // last block mined
    Block current_block; // current block being mined
    sem_t mutex; // mutex for the shared memory
    sem_t empty; // semaphore to protect shared memory
    sem_t fill; // semaphore to protect shared memory
} System;

/**
 * @brief Node structure
 */
typedef struct _Node {
    Miner *miner;
    struct _Node *next;
} Node;

/**
 * @brief List structure
 */
struct _List {
    Node *first;
};
