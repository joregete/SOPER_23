/**
* @file facepulls.c
* @brief Comprobador/Monitor
* @author Enmanuel, Jorge
* @version 1.0
* @date 2023-04-14
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "../includes/miner.h"
#include "../includes/common.h"

#define BUFFER_LENGTH 10
#define SHM_NAME "/facepulls_shm"

/* ----------------------------------------- GLOBALS ---------------------------------------- */

struct timespec delay;
volatile sig_atomic_t shutdown = 0;

typedef struct _sharedMemory{
    Block blocks[BUFFER_LENGTH];
    sem_t gym_mutex;
    sem_t gym_empty;
    sem_t gym_fill;
    uint8_t using;
    uint8_t reading;
    uint8_t writing;
} SharedMemory;


/* ----------------------------------------- FUNCTIONS ---------------------------------------*/

/**
 * @brief Comprobador is called when the shared memory does not exist, it creates it and
 *        starts listening to the MQ for messages from the miners and updates the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void comprobador(){
    int fd_shm, index, fd_sys;
    SharedMemory *shmem = NULL;
    Block msg;
    mqd_t mq;
    struct mq_attr attr;
    System *_system;
    
    //open shared memory
    if((fd_shm = shm_open (SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize of the mmeory segment
    if(ftruncate(fd_shm, sizeof(SharedMemory)) == -1){
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        // close_unlink();
        exit(EXIT_FAILURE);
    }

    // Mapping of the memory segment
    shmem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if(shmem == MAP_FAILED){
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Initialize the shared memory
    SharedMemory initialize = {
        .using = 0,
        .reading = 0,
        .writing = 0
    };
    memcpy(shmem, &initialize, sizeof(SharedMemory));

    // Initialize the semaphores
    if (sem_init(&(shmem->gym_mutex), 1, 1) == -1){
        perror("sem_init");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if(sem_init(&(shmem->gym_empty), 1, BUFFER_LENGTH) == -1){
        perror("sem_init");
        sem_destroy(&(shmem->gym_mutex));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if(sem_init(&shmem->gym_fill, 1, 0) == -1){
        perror("sem_init");
        sem_destroy(&(shmem->gym_mutex));
        sem_destroy(&(shmem->gym_empty));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    //set message queue attributes
    attr = (struct mq_attr){
        .mq_flags = 0,
        .mq_maxmsg = MAX_MSG,
        .mq_msgsize = SIZE,
        .mq_curmsgs = 0
    };
    if((mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t)-1){
        perror("mq_open");
        if(shmem->using == 1){
            sem_destroy(&(shmem->gym_mutex));
            sem_destroy(&(shmem->gym_empty));
            sem_destroy(&(shmem->gym_fill));
            shm_unlink(SHM_NAME);
        }
        shmem->using--;
        munmap(shmem, sizeof(SharedMemory));
        exit(EXIT_FAILURE);
    }

    //NOTIFY MINERS THAT MONITOR IS UP
    fd_sys = shm_open("/deadlift_shm", O_RDWR, 0666);
    if(fd_sys == -1){
        do {
            sleep(1);
            if(errno == EINTR){
                shm_unlink(SHM_NAME);
                mq_unlink(MQ_NAME);
                exit(EXIT_FAILURE);
            }
            fd_sys = shm_open("/deadlift_shm", O_RDWR, 0666);
        } while (fd_sys == -1);
    }
    _system = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE, MAP_SHARED, fd_sys, 0);
    close(fd_sys);
    if(_system == MAP_FAILED){
        perror("mmap");
        shm_unlink("/deadlift_shm");
        exit(EXIT_FAILURE);
    }
    sem_wait(&(_system->mutex));
    _system->monitor_up = 1;
    sem_post(&(_system->mutex));

    while(!shutdown){
        // receive message from MQ
        if(mq_receive(mq, (char *)&msg, SIZE, NULL) == -1){
            perror("mq_receive");
            if(shmem->using == 1){
                sem_destroy(&(shmem->gym_mutex));
                sem_destroy(&(shmem->gym_empty));
                sem_destroy(&(shmem->gym_fill));
                shm_unlink(SHM_NAME);
                mq_unlink(MQ_NAME);
                shm_unlink("/deadlift_shm");
            }
            shmem->using--;
            mq_close(mq);
            munmap(shmem, sizeof(SharedMemory));
            exit(EXIT_FAILURE);
        }

        sem_wait(&(shmem->gym_empty));
        sem_wait(&(shmem->gym_mutex));

        /* ----------- Protected ----------- */
        index = shmem->writing % BUFFER_LENGTH;
        shmem->writing++;
        shmem->blocks[index] = msg;
        /* ------------- end prot --------------- */
        
        sem_post(&(shmem->gym_mutex));
        sem_post(&(shmem->gym_fill));
    }
    sem_wait(&(_system->mutex));
    _system->monitor_up = 0;
    sem_post(&(_system->mutex));
    mq_close(mq);
    mq_unlink(MQ_NAME);
    shm_unlink("/deadlift_shm");
}

/**
 * @brief Monitor is called when the shared memory exists, it reads it and prints the info
 * @param fd_shm (int) file descriptor of the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void monitor(){
    SharedMemory *shmem = NULL;
    int aux_reading = 0, fd_shm = -1;
    uint8_t num_miners = 0, i;
    Block read_block;

    do {
        fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
    } while(fd_shm == -1 && errno == ENOENT);

    // Mapping of the memory segment
    shmem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if(shmem == MAP_FAILED){
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    sem_wait(&(shmem->gym_mutex));
    /* ----------- Protected ----------- */

    shmem->using++;

    /* ------------- end prot --------------- */
    sem_post(&(shmem->gym_mutex));

    fprintf(stdout,"[%08d] Printing blocks...\n", getpid ());
    while(!shutdown){
        sem_wait(&(shmem->gym_fill));
        sem_wait(&(shmem->gym_mutex));
        /* ----------- Protected ----------- */

        aux_reading = shmem->reading % BUFFER_LENGTH;
        shmem->reading++;
        read_block = shmem->blocks[aux_reading];
        sem_post(&(shmem->gym_mutex));
        sem_post(&(shmem->gym_empty));
        
        /* ------------- end prot --------------- */

        if(read_block.favorable_votes == read_block.total_votes)
        num_miners = read_block.total_votes;
        fprintf(stdout, "Id:\t\t%04d\nWinner:\t\t%d\nTarget:\t\t%ld\nSolution:\t%08ld\nVotes:\t\t%d/%d",
                    read_block.id, read_block.winner, read_block.target, read_block.solution, read_block.favorable_votes, num_miners);
        read_block.favorable_votes == num_miners ? fprintf(stdout, "\t(accepted) WidePeepoHappy") : fprintf(stdout, "\t(rejected) pepeHands");
        fprintf(stdout, "\nWallets:");
        for(i = 0; i < num_miners; i++)
            fprintf(stdout, "\t%d:%02d", read_block.miners[i].pid, read_block.miners[i].coins);
        fprintf(stdout, "\n-----------------------\n");
    }
}


/* -----------------------------------------   MAIN   --------------------------------------- */

int main(int argc, char *argv[]){
    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid != 0){ // parent
        comprobador();
        wait(NULL);
    }
    else
        monitor();

    shm_unlink(SHM_NAME);
    mq_unlink(MQ_NAME); 
    return 0;
}