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

/**
 * @brief Miner process
 * @param argv[1] = rounds (int), number of rounds to mine
 * @param argv[2] = lag (int), time to wait between each message
 * @return 0 on success, -1 on failure
*/
int main(int argc, char *argv[]){
    int msg[2]; // msg[0] = target, msg[1] = solution
    struct mq_attr attr;
    int rounds = 0, lag = 0, i = 0;
    mqd_t mq;

    if(argc != 3){
        printf("Usage: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[1]);
    lag = atoi(argv[2]);

    // Initialize the message queue
    attr = (struct mq_attr){
        .mq_flags = 0,
        .mq_maxmsg = MAX_MSG,
        .mq_msgsize = sizeof(long)*2,
        .mq_curmsgs = 0
    };

    // Open the message queue
    if((mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t) -1){
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    // Generate blocks
    fprintf(stdout, "[%08d] Generating blocks...\n", getpid ());

    msg[SOLUTION] = 0;
    msg[TARGET] = 0;
    for(i = 0; i < rounds; i++){
        msg[SOLUTION] = pow_hash(msg[TARGET]);
        if(mq_send(mq, (char*)&msg, sizeof(msg), 1) == -1){ // 1 is the priority
            mq_close(mq); 
            mq_unlink(MQ_NAME);
            exit(EXIT_FAILURE);
        }
        sleep(lag);
    
        msg[TARGET] = msg[SOLUTION]; // the new target is the solution of the previous block
    }

    
    msg[TARGET] = -1;
    msg[SOLUTION] = -1;
    if(mq_send(mq, (char*)&msg, sizeof(msg), 1) == -1){
        perror("mq_send");
        mq_close(mq); 
        mq_unlink(MQ_NAME);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "[%08d] Finishing\n", getpid ());

    mq_close(mq);
    mq_unlink(MQ_NAME);
    exit(EXIT_SUCCESS);
}