/**
* @file miner.c
* @brief Miner process
* @author Enmanuel, Jorge
* @version 0.1
* @date 2023-04-14
*
*/

#include "common.h"
#include "pow.h"

/**
 * @brief Generates a block with a target and a solution
 * @return block (Block struct)
 * 
*/
Block work(){
    Block block;
    long target, solution;

    for (target = 0; target < POW_LIMIT; target++){
        solution = pow_hash(target);
        if (solution == target){
            block.target = target;
            block.solution = solution;
            block.flag = 0;
            break;
        }
    }
    return block;
}

/**
 * @brief Miner process
 * @param argv[1] = rounds (int), number of rounds to mine
 * @param argv[2] = lag (int), time to wait between each message
 * @return 0 on success, -1 on failure
*/
int main(int argc, char *argv[]){
    Block block;
    MESSAGE msg;
    struct mq_attr attr;
    int rounds = 0, lag = 0, i = 0;
    mqd_t mq;

    if(argc != 3){
        printf("Usage: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Getting arguments
    rounds = atoi(argv[1]);
    lag = atoi(argv[2]);

    // Initialize the message queue
    attr = (struct mq_attr){
        .mq_flags = 0,
        .mq_maxmsg = MAX_MSG,
        .mq_msgsize = SIZE,
        .mq_curmsgs = 0
    };

    // Open the message queue
    if((mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t) -1){
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    // Generate blocks
    fprintf(stdout, "[%08d] Generating blocks...\n", getpid ());

    for(i = 0; i < rounds; i++){
        block = work();
        if(i == rounds-1)
            block.end = 1;

        msg.block = block;

        if(mq_send(mq, (char*)&msg, SIZE, 1) == -1){ // 1 is for the priority
            perror("mq_send");
            mq_close(mq); 
            mq_unlink(MQ_NAME);
            exit(EXIT_FAILURE);
        }
        usleep(lag*1000000);

    }

    // if(mq_send(mq, (char*)&msg, sizeof(msg), 1) == -1){
    //     perror("mq_send");
    //     mq_close(mq); 
    //     mq_unlink(MQ_NAME);
    //     exit(EXIT_FAILURE);
    // }
    fprintf(stdout, "[%08d] Finishing\n", getpid ());

    mq_close(mq);
    mq_unlink(MQ_NAME);
    exit(EXIT_SUCCESS);
}