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
#include "common.h"

#define BUFFER_LENGTH 6
#define SHM_NAME "/shm_facepulls"

/* ----------------------------------------- GLOBALS ---------------------------------------- */

struct timespec delay;

typedef struct _sharedMemory{
    short flag[BUFFER_LENGTH];
    long target[BUFFER_LENGTH];
    long solution[BUFFER_LENGTH];
    sem_t gym_mutex;
    sem_t gym_empty;
    sem_t gym_fill;
    int using;
    int reading;
    int writing;
} SharedMemory;


/* ----------------------------------------- FUNCTIONS ---------------------------------------*/
/**
 * @brief Comprobador is called when the shared memory does not exist, it creates it and
 *        starts listening to the MQ for messages from the miners and updates the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void comprobador(int lag){
    int fd_shm, index;
    SharedMemory *shmem = NULL;
    MESSAGE msg;
    mqd_t mq;
    struct mq_attr attr;
    
    printf(">> Comprobador\n");

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
        .flag = {0},
        .target = {0},
        .solution = {0},
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

    mq_unlink(MQ_NAME); // as early as possible

    fprintf(stdout,"[%08d] Checking blocks...\n", getpid());
    while(1){
        // receive message from MQ
        if(mq_receive(mq, (char *)&msg, SIZE, NULL) == -1){
            perror("mq_receive");
            if(shmem->using == 1){
                sem_destroy(&(shmem->gym_mutex));
                sem_destroy(&(shmem->gym_empty));
                sem_destroy(&(shmem->gym_fill));
                shm_unlink(SHM_NAME);
            }
            shmem->using--;
            mq_close(mq);
            munmap(shmem, sizeof(SharedMemory));
            exit(EXIT_FAILURE);
        }

        if(msg.block.end == 1){
            fprintf(stdout, "[%08d] Finishing\n", getpid());
            // FINALIZAR PROGRAMA
            sem_wait(&(shmem->gym_empty));
            sem_wait(&(shmem->gym_mutex));

            /* ----------- Protected ----------- */
            index = shmem-> writing % BUFFER_LENGTH;
            shmem->writing++;
            shmem->target[index] = -1;
            shmem->solution[index] = -1;

            if(shmem->using == 1){
                sem_destroy(&(shmem->gym_mutex));
                sem_destroy(&(shmem->gym_empty));
                sem_destroy(&(shmem->gym_fill));
                shm_unlink(SHM_NAME);
            }
            else{
                shmem->writing++;
                shmem->using--;
            }
            sem_post(&(shmem->gym_mutex));
            sem_post(&(shmem->gym_fill));
            /* ------------- end prot --------------- */
            
            mq_close(mq);
            munmap(shmem, sizeof(SharedMemory));
            shm_unlink(SHM_NAME); //already done, but just in case, it wont hurt
            exit(EXIT_SUCCESS);
        }

        sem_wait(&(shmem->gym_empty));
        sem_wait(&(shmem->gym_mutex));

        /* ----------- Protected ----------- */ //si solution es igual a pow_hash(target) entonces flag = 1
        index = shmem->writing % BUFFER_LENGTH;
        shmem->writing++;
        shmem->target[index] = msg.block.target;
        shmem->solution[index] = msg.block.solution;
        shmem->flag[index] = (pow_hash(shmem->target[index]) == shmem->solution[index]);
        /* ------------- end prot --------------- */
        
        sem_post(&(shmem->gym_mutex));
        sem_post(&(shmem->gym_fill));

        delay.tv_sec = 0; // espera 0 segundos
        delay.tv_nsec = lag * 1000000; // espera 'lag' milisegundos
        nanosleep(&delay, NULL);
    }
}

/**
 * @brief Monitor is called when the shared memory exists, it reads it and prints the info
 * @param fd_shm (int) file descriptor of the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void monitor(int fd_shm, int lag){
    SharedMemory *shmem = NULL;
    int aux_reading = 0,
        target = 0,
        solution = 0;
    short aux_flag = 0;
    
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
    while(1){
        sem_wait(&(shmem->gym_fill));
        sem_wait(&(shmem->gym_mutex));
        /* ----------- Protected ----------- */

        aux_reading = shmem->reading % BUFFER_LENGTH;
        shmem->reading++;
        target = shmem->target[aux_reading];
        solution = shmem->solution[aux_reading];

        if(target == -1 && solution == -1){
            fprintf(stdout, "[%08d] Finishing\n", getpid());
            if(shmem->using == 1){
                sem_destroy(&(shmem->gym_mutex));
                sem_destroy(&(shmem->gym_empty));
                sem_destroy(&(shmem->gym_fill));
                shm_unlink(SHM_NAME);
            }
            else{
                shmem->using--;
                shmem->reading++;
            }
            sem_post(&(shmem->gym_mutex));
            sem_post(&(shmem->gym_empty));
        /* ------------- end prot--------------- */
        
            munmap(shmem, sizeof(SharedMemory));
            exit(EXIT_SUCCESS);
        }
        
        aux_flag = shmem->flag[aux_reading];
        sem_post(&(shmem->gym_mutex));
        sem_post(&(shmem->gym_empty));
        
        /* ------------- end prot --------------- */

        if(aux_flag == 1)
            printf("Solution accepted: %08d -> %08d\n", solution, target);
        else
            printf("Solution rejected: %08d -> %08d\n", solution, target);
        
        delay.tv_sec = 0; // espera 0 segundos
        delay.tv_nsec = lag * 1000000; // espera 'lag' milisegundos
        nanosleep(&delay, NULL);
    }
}


/* -----------------------------------------   MAIN   --------------------------------------- */

int main(int argc, char *argv[]){
    int lag = 0, fd_shm;
    if(argc != 2){
        printf("Usage: %s <lag>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    lag = atoi(argv[1]);

    // if shm exixts, calls monitor else calls comprobador
    fd_shm = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_shm == -1) // -1 shm does not exist
        comprobador(lag); // comprobador creates it
    else
        monitor(fd_shm, lag); // monitor reads it

    shm_unlink(SHM_NAME);
    mq_unlink(MQ_NAME); 

    return 0;
}