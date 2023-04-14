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

#define SHM_NAME "/shm_facepulls"

typedef struct _block{
    short flag;
    long target;
    long solution;
} Block;

/**
 * @brief Comprobador is called when the shared memory does not exist, it creates it and
 *        starts listening to the MQ for messages from the miners and updates the shared memory
 * @param lag (int) time to wait between each message
 * @return void
*/
void comprobador(int lag){
    int fd_shm;
    short flag;
    mqd_t mq;
    struct mq_attr attr;
    MESSAGE msg;
    Block *block = NULL;

    printf(">> Comprobador\n");

    //open shared memory
    if((fd_shm = shm_open(SHM_NAME, O_RDONLY, 0)) == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize of the mmeory segment
    if(ftruncate(fd_shm, sizeof(Block)) == -1){
        perror("ftruncate");
        shm_unlink(SHM_NAME);
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
        exit(EXIT_FAILURE);
    }

    mq_unlink(MQ_NAME); // as early as possible

    while(1){
        // receive message from MQ
        if(mq_receive(mq, (char *)&msg, sizeof(msg), NULL) == -1){
            perror("mq_receive");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        if(msg.target == -1){
            // FINALIZAR PROGRAMA
            fprintf(stdout, "\nfinishing...\n");
            flag = -1;
            memcpy(&(block->flag), &flag, sizeof(flag));
            mq_close(mq);
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_SUCCESS);
        }
        // cuando el comprobador reciba mensaje del minero hacer:
        memcpy(&block->solution, &msg.solution, sizeof(msg.solution));
        memcpy(&block->target, &msg.target, sizeof(msg.target));
        if(msg.solution == pow_hash(msg.target)){
            flag = 1;
            memcpy(&block->flag, &flag, sizeof(flag)); // MINER WAS CORRECT
        }
        else{
            flag = 0;
            memcpy(&block->flag, &flag, sizeof(flag)); // MINER WAS WRONG
        }
        
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
        close(fd_shm);
        exit(EXIT_FAILURE);
    }
    while(1){
        if(block->flag == -1){
            // FINALIZAR PROGRAMA
            fprintf(stdout, "\nfinishing...\n");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_SUCCESS);
        }
        if(block->flag == 1)
            printf("Solution accepted: %08ld -> %08ld\n" , block->target, block->solution);
        else
            printf("Solution rejected: %08ld -> %08ld\n" , block->target, block->solution);
        
        sleep(lag);
    }
}

int main(int argc, char *argv[]){
    int lag = 0, fd_shm;
    if(argc != 2){
        printf("Usage: %s <lag>\n", argv[0]);
        exit(1);
    }
    lag = atoi(argv[1]);

    // if shm exixts, calls monitor else calls comprobador
    fd_shm = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_shm == -1) // -1 shm does not exist
        comprobador(lag); // comprobador creates it
    else 
        monitor(fd_shm, lag); //either way, monitor reads it
    
    return 0;
}