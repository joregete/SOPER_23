/**
 * @file miner.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief program that a miner process will execute
 * @date 2023-04-18
 * 
 * @copyright Copyright (c) 2023
 */

#include "../includes/miner.h"
#include "../includes/pow.h"

volatile sig_atomic_t sigusr2_received = 0; // indicates SIGUSR2 reception
volatile sig_atomic_t sigusr1_received = 0; // indicates SIGUSR1 reception
volatile sig_atomic_t magic_flag = 0; // indicates mining solution was found
volatile sig_atomic_t shutdown = 0; // indicates system has to shutdown
mqd_t mq = -2; // message queue
struct mq_attr attr; // message queue attributes
long _solution = 0; // solution to the target

/**
 * @brief interruption-safe sem_wait
 * @param sem_id 
 * @return int 
 */
int _sem_wait_safe(sem_t *sem) {
  while (sem_wait(sem))
    if (errno == EINTR) errno = 0;
    else return -1;
  return 0;
}

/**
 * @brief private function to handle signals
 * @param sig signal received
 */
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("miner %d finishing by interrupt...\n", getpid());
        shutdown = 1;
    }
    if (sig == SIGUSR2) {
        sigusr2_received = 1;
    }
    if (sig == SIGALRM) {
        printf("miner %d finishing by alarm...\n", getpid());
        shutdown = 1;
    }
    if(sig == SIGUSR1){
        sigusr1_received = 1;
    }
}

/**
 * @brief private function that the miner threads will execute
 * @param args 
 * @return void* 
 */
void *work(void* args){
    long i, result;
    MinerData *miner_data = (MinerData*) args;
    for(i = miner_data->start; i < miner_data->end; i++){
        if(magic_flag)
            return NULL;
        
        result = pow_hash(i);
        if(result == miner_data->target){
            _solution = i;
            magic_flag = 1;
            return NULL;
        }
    }
    return NULL;
}

/**
 * @brief checks for the existance of a message queue. if it exists, send Block
 * if it does not, do nothing
 * @param _block 
 */
void send_queue(Block *_block){
    if(mq == -2) { // queue needs to be initialized again or for the first time
        // Initialize the queue attributes
        attr = (struct mq_attr){
            .mq_flags = 0,
            .mq_maxmsg = MAX_MSG,
            .mq_msgsize = SIZE,
            .mq_curmsgs = 0
        };
        // Open the message queue
        if((mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr)) == (mqd_t) -1){
            perror("mq_open");
            return;
        }
    }
    // send message
    if(mq_send(mq, (char*)_block, sizeof(Block), 2) == -1) {
        perror("mq_send");
        return;
    }
}

/**
 * @brief Main function for the miner process
 * @return 0 Exit success or 1 Exit failure
 */
int main(int argc, char *argv[]){
    pid_t pid;
    pthread_t *threads;
    uint8_t first_miner_flag = 0, i, j, n_sec, nthreads;
    int miner2register[2], ret = -2, fd_shm;
    long target = 0;
    struct sigaction act;
    sigset_t a, old_a;
    struct timespec sleep_time;
    System *system; // structure representing the shared memory

    check_args(argc, argv, &n_sec, &nthreads);

    if(pipe(miner2register) < 0){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if(pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){ // register
        close(miner2register[1]); // close write end
        _register(miner2register[0]);
    }
    // miner
    close(miner2register[0]); // close read end
    // if shm exixts, open it. else create it
    fd_shm = shm_open(SYSTEM_SHM, O_RDWR, 0666);
    if (fd_shm == -1){ // shm does not exist
        first_miner_flag = 1;
        // create shared memory and initialize system
        system = create_system();
    } else { // shm exists
        // mapping of the memory segment
        system = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        close(fd_shm);
        if(system == MAP_FAILED){
            perror("mmap");
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }
        if(system->num_miners == MAX_MINERS){
            printf("\nsystem doesn't accept more miners\n");
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_SUCCESS);
        }
    }
    act.sa_handler = signal_handler; // assign signal handler
    act.sa_flags = 0;
    sigfillset(&(act.sa_mask)); // start with a full mask
    // choose signals upon which the handler'll be called
    if(sigaction(SIGINT, &act, NULL) < 0){
        perror("sigaction");
        return 1;
    }
    if(sigaction(SIGUSR1, &act, NULL) < 0){
        perror("sigaction");
        return 1;
    }
    if(sigaction(SIGUSR2, &act, NULL) < 0){
        perror("sigaction");
        return 1;
    }
    if(sigaction(SIGALRM, &act, NULL) < 0){
        perror("sigaction");
        return 1;
    }
    sigdelset(&(act.sa_mask), SIGINT); // unblock SIGINT
    sigdelset(&(act.sa_mask), SIGALRM); // unblock SIGALRM
    alarm(n_sec); // set alarm for the end of the process

    // log new miner representing this process into system
    Miner this_miner; 
    this_miner.pid = getpid();
    this_miner.coins = 0;

    sem_wait(&(system->mutex));
    /* ----------- Protected ----------- */
    system->miners[system->num_miners] = this_miner;
    system->num_miners++;
    sem_post(&(system->mutex));
    /* ------------- end prot --------------- */
    printf("\nminer %d registered\n", this_miner.pid);
    // remove SIGUSR1 from auxiliar mask, for whenever this miner needs to be suspended
    sigfillset(&a);
    sigdelset(&a, SIGUSR1);
    sigdelset(&a, SIGINT);
    sigdelset(&a, SIGALRM);
    // sigemptyset(&a);
    // sigaddset(&a, SIGUSR1);

    // FIRST ROUND MANAGEMENT
    if(first_miner_flag == 1){
        Block first_block;
        init_block(&first_block, -1, 0); // initialize first block ever
        sem_wait(&(system->mutex));
        /* ----------- Protected ----------- */
        system->current_block = first_block; // first block ready to get mined
        // send sigusr1 to all miners except this one, trigger start of first round
        for(i = 0; i < system->num_miners; i++){
            if(system->miners[i].pid != this_miner.pid)
                kill(system->miners[i].pid, SIGUSR1);
        }
        sem_post(&(system->mutex));
        /* ------------- end prot --------------- */        
    } else{
        // sigprocmask(SIG_BLOCK, &a, &old_a); // block SIGUSR1
        // while(!sigusr1_received) // wait for SIGUSR1
        //     sigsuspend(&old_a);
        // sigprocmask(SIG_UNBLOCK, &a, NULL); // unblock SIGUSR1
        // sigusr1_received = 0; // reset flag
        // NO NEED TO PROTECT THIS SIGSUSPEND BECAUSE IT'S THE FIRST ROUND.
        // IF FIRST MINER ALREADY SENT SIGUSR1, IT'LL MINE ONE BLOCK
        // ALONE, SEND SIGUSR1 FOR NEXT ROUND AND THIS PROCESS WILL 
        // EXIT SIGSUSPEND
        sigsuspend(&a); // wait for SIGUSR1, wait for start of first round
    }
        
    // allocate memory for threads
    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    if(threads == NULL){
        perror("malloc threads");
        sem_destroy(&(system->mutex));
        exit(EXIT_FAILURE);
    }
    // unblock SIGUSR2, which is used to start voting
    sigdelset(&(act.sa_mask), SIGUSR2);
    MinerData *miner_data = (MinerData*) malloc(sizeof(MinerData)*nthreads);
    if(miner_data == NULL){
        perror("malloc minerData");
        free(threads);
        sem_destroy(&(system->mutex));
        exit(EXIT_FAILURE);
    }
    // initialitation ended, time to start mining
    while(!shutdown){
        sigusr2_received = 0;
        magic_flag = 0;
        // get this rounds target
        target = system->current_block.target;
        // this miner will mine current block, so it's a voter
        sem_wait(&(system->mutex));
        /* ----------- Protected ----------- */
        system->current_block.miners[system->current_block.num_voters] = this_miner;
        system->current_block.num_voters++;
        sem_post(&(system->mutex));
        /* ------------- end prot --------------- */
        // start mining
        for(j = 0; j < nthreads; j++){
            miner_data[j].start = j * ((POW_LIMIT -1 ) / nthreads);
            miner_data[j].end = (j+1) * ((POW_LIMIT -1 ) / nthreads);
            miner_data[j].target = target;
            if(pthread_create(&threads[j], NULL, work, &miner_data[j])){
                perror("pthread_create");
                free(miner_data);
                free(threads);
                sem_destroy(&(system->mutex));
                exit(EXIT_FAILURE);
            }
        }
        for(j = 0; j < nthreads; j++){
            if(pthread_join(threads[j], NULL)){
                perror("pthread_join");
                    free(miner_data);
                    free(threads);
                    sem_destroy(&(system->mutex));
                    exit(EXIT_FAILURE);
                break;
            }
        }
        // check if this dude is the first to finish
        if(sigusr2_received == 0){ // WINNER WINNER CHICKEN DINNER
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            // publish solution
            system->current_block.solution = _solution; 
            // send SIGUSR2 to all miners in mined block except this one, triggering start of voting
            for(i = 0; i < system->current_block.num_voters; i++){
                if(system->current_block.miners[i].pid != this_miner.pid)
                    kill(system->current_block.miners[i].pid, SIGUSR2);
            }
            // this miner votes for itself
            system->current_block.total_votes++;
            system->current_block.favorable_votes++;
            sem_post(&(system->mutex));
            /* ------------- end prot --------------- */
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 100000000; // 0.1 seconds
            // wait until all miners have voted
            while(system->current_block.total_votes != system->current_block.num_voters){
                ret = nanosleep(&sleep_time, NULL);
                if(ret == -1){
                    free(miner_data);
                    free(threads);
                    sem_destroy(&(system->mutex));
                    if(errno != EINTR)
                        perror("nanosleep");                    
                    exit(EXIT_FAILURE);
                }
            }
            // when voting is done, check if the solution got accepted
            if(system->current_block.total_votes == system->current_block.favorable_votes){
                system->current_block.winner = this_miner.pid;
                this_miner.coins++;
            }
            // send block to register and monitor
            ret = write(miner2register[1], &(system->current_block), sizeof(Block));
            if(ret < 0){
                perror("write");
                sem_destroy(&(system->mutex));
                free(miner_data);
                free(threads);
                exit(EXIT_FAILURE);
            }
            if(system->monitor_up == 1) // check if monitor is up
                send_queue(&(system->current_block));
            else mq = -2;
            // set last block to current block and start again
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            system->last_block = system->current_block; // update System
            Block new_block; // create new block for the next round
            init_block(&new_block, system->last_block.id, system->last_block.solution);
            system->current_block = new_block;
            // send sigusr1 to all miners except this one, means start of next round
            for(i = 0; i < system->num_miners; i++){
                if(system->miners[i].pid != this_miner.pid)
                    kill(system->miners[i].pid, SIGUSR1);
            }
            sem_post(&(system->mutex));
        } else { // loser pepeHands
            // vote for the solution that potential winner posted
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            if(system->current_block.solution == _solution) // if solution is correct in this miner's opinion
                system->current_block.favorable_votes++;
            system->current_block.total_votes++;
            sem_post(&(system->mutex));
            /* ------------- end prot --------------- */
            sigsuspend(&a); // wait for SIGUSR1, means start of next round
            // sigprocmask(SIG_BLOCK, &a, &old_a); // block SIGUSR1
            // while(!sigusr1_received) // wait for SIGUSR1
            //     sigsuspend(&old_a);
            // sigprocmask(SIG_UNBLOCK, &a, NULL); // unblock SIGUSR1
            // sigusr1_received = 0; // reset flag
        }
    }
    // SHUTDOWN
    close(miner2register[1]);
    free(threads);
    free(miner_data);
    // delete miner from shared memory
    sem_wait(&(system->mutex));
    /* ----------- Protected ----------- */
    if(system->num_miners == 1){ // last miner
        printf("\nlast miner finished, deleting shared memory\n");
        sem_destroy(&(system->mutex));
        delete_miner(system->miners, &(system->num_miners), this_miner.pid);
        munmap(system, sizeof(System));
        shm_unlink(SYSTEM_SHM);
        return 0;
    }
    delete_miner(system->miners, &(system->num_miners), this_miner.pid);
    sem_post(&(system->mutex));
    munmap(system, sizeof(System));
    shm_unlink(SYSTEM_SHM);
    /* ------------- end prot --------------- */
    wait(NULL);
    return 0;
}