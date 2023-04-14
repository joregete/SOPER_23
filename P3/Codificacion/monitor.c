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
#include "common.h"

/* ----------------------------------------- GLOBALS ---------------------------------------- */

#define SHM_NAME "/shm_facepulls"
#define MUTEX "/mutex_facepulls"
#define EMPTY "/sem_empty_facepulls"
#define FILL "/sem_fill_facepulls"

sem_t *gym_mutex = NULL;
sem_t *gym_empty = NULL;
sem_t *gym_fill = NULL;

typedef struct _block{
    short flag;
    long target;
    long solution;
} Block;


/* ------------------------------------- PRIVATE FUNCTIONS -----------------------------------*/
void close_unlink(){
    sem_close(gym_mutex);
    sem_close(gym_empty);
    sem_close(gym_fill);
    sem_unlink(MUTEX);
    sem_unlink(EMPTY);
    sem_unlink(FILL);
}

/* ----------------------------------------- FUNCTIONS ---------------------------------------*/
/**
 * @brief Comprobador is called when the shared memory does not exist, it creates it and
 *        starts listening to the MQ for messages from the miners and updates the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void comprobador(int lag){
    int fd_shm;
    short flag; // 0 = ERROR, 1 = OK, -1 = END
    mqd_t mq;
    struct mq_attr attr;
    MESSAGE msg;
    Block *block = NULL;
    // TODO: CHECK FOR THE CIRCULAR BUFFER IMPLEMENTATION

    printf(">> Comprobador\n");

    //open shared memory
    if((fd_shm = shm_open(SHM_NAME, O_RDONLY, 0)) == -1){
        perror("shm_open");
        close_unlink()
        exit(EXIT_FAILURE);
    }

    // Resize of the mmeory segment
    if(ftruncate(fd_shm, sizeof(Block)) == -1){
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        close_unlink()
        exit(EXIT_FAILURE);
    }

    // Mapping of the memory segment
    block = mmap(NULL, sizeof(Block), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if(block == MAP_FAILED){
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MAX_MSG_BODY;

    if((mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t) -1){
        close(fd_shm);
        shm_unlink(SHM_NAME);
        close_unlink()
        exit(EXIT_FAILURE);
    }

    mq_unlink(MQ_NAME); // as early as possible

    while(1){
        // receive message from MQ
        if(mq_receive(mq, (char *)&msg, sizeof(msg), NULL) == -1){
            perror("mq_receive");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            close_unlink()
            exit(EXIT_FAILURE);
        }
        if(msg.target == -1){
            // FINALIZAR PROGRAMA
            fprintf(stdout, "\nfinishing...\n");
            flag = -1; // flag to end monitor
            // TODO: NO SE SI AQUI TAMBIEN HAY QUE USAR SEMAFOROS, supongo que si
            memcpy(&(block->flag), &flag, sizeof(flag));
            mq_close(mq);
            close(fd_shm);
            shm_unlink(SHM_NAME);
            close_unlink()
            exit(EXIT_SUCCESS);
        }

        sem_wait(gym_empty);
        sem_wait(gym_mutex);
        // update shared memory   
            memcpy(&block->solution, &msg.solution, sizeof(msg.solution)); // TODO: check return value == NULL????
            memcpy(&block->target, &msg.target, sizeof(msg.target));
            if(msg.solution == pow_hash(msg.target)){
                flag = 1;
                memcpy(&block->flag, &flag, sizeof(flag)); // MINER WAS CORRECT
            }
            else{
                flag = 0;
                memcpy(&block->flag, &flag, sizeof(flag)); // MINER WAS WRONG
            }
        sem_post(gym_mutex);
        sem_post(gym_fill);
        sleep(lag);
    }
}

/**
 * @brief Monitor is called when the shared memory exists, it reads it and prints the info
 * @param fd_shm (int) file descriptor of the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void monitor(int fd_shm, int lag){
    Block *block = NULL;
    // Mapping of the memory segment
    block = mmap(NULL, sizeof(Block), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if(block == MAP_FAILED){
        perror("mmap");
        shm_unlink(SHM_NAME);
        close_unlink()
        close(fd_shm);
        exit(EXIT_FAILURE);
    }

    while(1){ //TODO: dónde va el fucking semaforo?
        if(block->flag == -1){
            // FINALIZAR PROGRAMA
            fprintf(stdout, "\nfinishing...\n");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            close_unlink()
            exit(EXIT_SUCCESS);
        }
        if(block->flag == 1)
            printf("Solution accepted: %08ld -> %08ld\n" , block->target, block->solution);
        else
            printf("Solution rejected: %08ld -> %08ld\n" , block->target, block->solution);
        
        sleep(lag);
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

    if(gym_mutex == sem_open(MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1) == SEM_FAILED){
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    if(gym_empty == sem_open(EMPTY, O_CREAT, S_IRUSR | S_IWUSR, 1) == SEM_FAILED){
        perror("sem_open");
        sem_close(gym_mutex);
        sem_unlink(MUTEX);
        exit(EXIT_FAILURE);
    }
    if(gym_fill == sem_open(FILL, O_CREAT, S_IRUSR | S_IWUSR, 1) == SEM_FAILED){
        perror("sem_open");
        sem_close(gym_mutex);
        sem_unlink(MUTEX);
        sem_close(gym_empty);
        sem_unlink(EMPTY);
        exit(EXIT_FAILURE);
    }

    // if shm exixts, calls monitor else calls comprobador
    fd_shm = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_shm == -1) // -1 shm does not exist
        comprobador(lag); // comprobador creates it
    else 
        monitor(fd_shm, lag); // monitor reads it
    
    close_unlink()

    return 0;
}