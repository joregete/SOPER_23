/**
    * @file miner.c
    * @brief Miner process
    * @author Enmanuel, Jorge
    * @version 0.1
    * @date 2023-04-14
    *
*/
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "pow.h"

int main(int argc, char *argv[]){
    MESSAGE msg;
    struct mq_attr attr;
    int rounds = 0, lag = 0, 
        i = 0;
    mqd_t mq;

    if(argc != 3){
        printf("Usage: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[1]);
    lag = atoi(argv[2]);

    attr.mq_maxmsg = MAX_MSG; // max number of messages in queue
    attr.mq_msgsize = MAX_MSG_BODY; // max size of message
    msg.target = 0;

    if((mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t) -1){
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < rounds; i++){
        msg.solution = pow_hash(msg.target);
        if(mq_send(mq, (char*)&msg, sizeof(msg), 1) == -1){ // 1 is the priority
            mq_close(mq); 
            mq_unlink(MQ_NAME);
            exit(EXIT_FAILURE);
        }
        sleep(lag);
        msg.target = msg.solution;
    }
    fprintf(stdout, "\nfinishing...\n");
    msg.solution = -1;
    msg.target = -1;
    if(mq_send(mq, (char*)&msg, sizeof(msg), 1) == -1){
        mq_close(mq); 
        mq_unlink(MQ_NAME);
        exit(EXIT_FAILURE);
    }
    mq_close(mq);
    mq_unlink(MQ_NAME);
    exit(EXIT_SUCCESS);
}