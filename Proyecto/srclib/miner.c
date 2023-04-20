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

#define SYSTEM_SHM "/deadlift_shm"

volatile sig_atomic_t sigusr2_received = 0; // indicates SIGUSR2 reception
volatile sig_atomic_t magic_flag = 0; // indicates mining solution was found
volatile sig_atomic_t shutdown = 0; // indicates system has to shutdown
long _solution = 0; // solution to the target

/**
 * @brief private function to handle signals
 * 
 * @param sig signal received
 */
void signal_handler(int sig) {
    if (sig == SIGINT) {
        shutdown = 1;
    }
    if (sig == SIGUSR2) {
        sigusr2_received = 1;
    }
    if (sig == SIGALRM) {
        printf("Miner %d finishing by alarm...\n", getpid());
        shutdown = 1;
        raise(SIGINT);
    }
}

/**
 * @brief private function that the miner threads will execute
 * 
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
 * @brief initialize a new block
 * 
 * @param block block to initialize
 * @param last_id id of the last block
 * @param target target to be solved
 * @param miner miner that created the block
 */
void init_block(Block *block, short last_id, int target, Miner *miner) {
    block->id = last_id + 1;
    block->target = target;
    block->solution = 0;
    block->winner = 0;
    block->total_votes = 0;
    block->favorable_votes = 0;
    block->miners[0] = *miner;
}

/**
 * @brief private function to find the index of a miner in the miners array
 * 
 * @param miners array
 * @param num_miners number of miners in the array
 * @param pid pid of the miner to find
 * @return uint8_t index of the miner in the array 
 */
uint8_t find_miner_index(Miner *miners, uint8_t num_miners, pid_t pid) {
    uint8_t i;
    for (i = 0; i < num_miners; i++) {
        if (miners[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief private function to give a coin to a miner
 * 
 * @param miners miners array
 * @param num_miners number of miners in the array
 * @param pid pid of the miner to give the coin to
 */
void give_coin(Miner *miners, uint8_t num_miners, pid_t pid) {
    uint8_t index = find_miner_index(miners, num_miners, pid);
    if (index != -1) {
        miners[index].coins++;
    }
}

/**
 * @brief private function to delete a miner from the miners array
 * 
 * @param miners miners array
 * @param num_miners number of miners in the array
 * @param pid pid of the miner to delete
 */
void delete_miner(Miner *miners, uint8_t *num_miners, pid_t pid) {
    uint8_t index = find_miner_index(miners, *num_miners, pid);
    if (index != -1) {
        for (uint8_t i = index; i < *num_miners - 1; i++) {
            miners[i] = miners[i + 1]; // rearrange the array
        }
        (*num_miners)--;
    }
}

/**
 * @brief funtionn that the child process will execute
 * 
 * @param pipe_read read end of the pipe, used to read the blocks
 */
void _register(int pipe_read){
    Block _block;
    uint8_t ret = 0, num_miners = 0, i = 0;
    char filename[15];
    int fd;
    sprintf(filename, "reg_%d.txt", getppid());
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0){
        perror("open");
        exit(EXIT_FAILURE);
    }
    while(1){ // read blocks until the pipe is closed
        ret = read(pipe_read, &_block, sizeof(Block));
        if(ret == 0)
            break;
        if(ret < 0){
            perror("read");
            exit(EXIT_FAILURE);
        }
        // print info into the file
        num_miners = _block.total_votes;
        dprintf(fd, "Id:\t\t\t%04d\nWinner:\t\t%d\nTarget:\t\t%ld\nSolution\t%08ld\nVotes:\t\t%d/%d",
                    _block.id, _block.winner, _block.target, _block.solution, _block.favorable_votes, num_miners);
        _block.favorable_votes == num_miners ? dprintf(fd, "\t(accepted) WidePeepoHappy") : dprintf(fd, "\t(rejected) pepeHands");
        dprintf(fd, "\nWallets:");
        for(i = 0; i < num_miners; i++)
            dprintf(fd, "\t%d:%02d", _block.miners[i].pid, _block.miners[i].coins);
        dprintf(fd, "\n-----------------------\n");
    }
    close(fd);
    exit(0);
}

/**
 * @brief Main function for the miner process
 * @return 0 Exit success or 1 Exit failure
 */
int main(int argc, char *argv[]){
    pid_t pid;
    pthread_t *threads;
    short n_sec, nthreads, fd_shm;
    uint8_t first_miner_flag = 0, i, j;
    int miner2register[2]; // pipe
    long target = 0;
    struct sigaction act;
    sigset_t a;
    struct timespec sleep_time;
    System *system; // structure representing the shared memory

    if (argc != 3){
        printf("Usage: %s <NSECONDS> <NTHREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    n_sec = atoi(argv[1]);
    if (n_sec <= 0){
        fprintf(stderr, "NSECONDS has to be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[2]);
    if (nthreads <= 0){
        fprintf(stderr, "NTHREADS has to be greater than 0\n");
        exit(EXIT_FAILURE);
    }

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
    // if shm exixts, open it else create it
    fd_shm = shm_open(SYSTEM_SHM, O_RDWR, 0666);
    if (fd_shm == -1){ // -1 shm does not exist
        printf("\nCreating shared memory\n");
        first_miner_flag = 1;
        //create shared memory
        if((fd_shm = shm_open (SYSTEM_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1){
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        // Resize of the memory segment
        if(ftruncate(fd_shm, sizeof(System)) == -1){
            perror("ftruncate");
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }

        // Mapping of the memory segment
        system = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        close(fd_shm);
        if(system == MAP_FAILED){
            perror("mmap");
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }

        // Initialize the semaphores
        if (sem_init(&(system->mutex), 1, 1) == -1){
            perror("sem_init");
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }
        if(sem_init(&(system->empty), 1, MAX_MINERS) == -1){
            perror("sem_init");
            sem_destroy(&(system->mutex));
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }
        if(sem_init(&system->fill, 1, 0) == -1){
            perror("sem_init");
            sem_destroy(&(system->mutex));
            sem_destroy(&(system->empty));
            shm_unlink(SYSTEM_SHM);
            exit(EXIT_FAILURE);
        }
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
            munmap(system, sizeof(System));
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

    sem_wait(&(system->empty));
    sem_wait(&(system->mutex));
    /* ----------- Protected ----------- */
    system->miners[system->num_miners] = this_miner;
    system->num_miners++;
    sem_post(&(system->mutex));
    sem_post(&(system->fill));
    /* ------------- end prot --------------- */
    printf("\nminer %d registered\n", this_miner.pid);
    // remove SIGUSR1 from auxiliar mask, for whenever this miner needs to be suspended
    sigfillset(&a);
    sigdelset(&a, SIGUSR1);
    sigdelset(&a, SIGINT);
    sigdelset(&a, SIGALRM);

    // FIRST ROUND MANAGEMENT
    if(first_miner_flag == 1){
        Block first_block;
        init_block(&first_block, -1, 0, &this_miner); // initialize first block ever
        sem_wait(&(system->empty));
        sem_wait(&(system->mutex));
        /* ----------- Protected ----------- */
        system->current_block = first_block; // first block ready to get mined
        // send sigusr1 to all miners except this one, trigger start of first round
        for(i = 0; i < system->num_miners; i++){
            if(system->miners[i].pid != this_miner.pid)
                kill(system->miners[i].pid, SIGUSR1);
        }
        sem_post(&(system->mutex));
        sem_post(&(system->fill));
        /* ------------- end prot --------------- */        
    } else
        sigsuspend(&a); // wait for SIGUSR1, wait for start of first round
    
    // allocate memory for threads
    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    if(threads == NULL){
        perror("malloc threads");
        exit(EXIT_FAILURE);
    }
    // unblock SIGUSR2, which is used to start voting
    sigdelset(&(act.sa_mask), SIGUSR2);
    MinerData *miner_data = (MinerData*) malloc(sizeof(MinerData)*nthreads);
    if(miner_data == NULL){
        perror("malloc minerData");
        exit(EXIT_FAILURE);
    }
    while(!shutdown){
        sigusr2_received = 0;
        magic_flag = 0;
        // get this round's target
        sem_wait(&(system->fill));
        sem_wait(&(system->mutex));
        /* ----------- Protected ----------- */
        target = system->current_block.target;
        sem_post(&(system->mutex));
        sem_post(&(system->empty));
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
                exit(EXIT_FAILURE);
            }
        }
        for(j = 0; j < nthreads; j++){
            if(pthread_join(threads[j], NULL)){
                perror("pthread_join");
                    free(miner_data);
                    free(threads);
                    exit(EXIT_FAILURE);
                break;
            }
        }
        // check if this dude is the first to finish
        if(sigusr2_received == 0){ // WINNER WINNER CHICKEN DINNER
            sem_wait(&(system->fill));
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            // update current block and send sigusr2 to all miners
            for(i = 0; i < system->num_miners; i++){
                if(system->miners[i].pid != this_miner.pid)
                    kill(system->miners[i].pid, SIGUSR2);
            }
            system->current_block.solution = _solution;
            system->current_block.winner = this_miner.pid;
            give_coin(system->miners, system->num_miners, this_miner.pid);
            sem_post(&(system->mutex));
            sem_post(&(system->empty));
            /* ------------- end prot --------------- */
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 250000000; // sleep for voting, 0.25s
            nanosleep(&sleep_time, NULL);
            // when voting is done, send block to register
            write(miner2register[1], &(system->current_block), sizeof(Block));
            // set last block to current block and start again
            sem_wait(&(system->empty));
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            system->last_block = system->current_block; // update System
            Block new_block; // create new block for the next round
            init_block(&new_block, system->last_block.id, system->last_block.solution, &this_miner);
            system->current_block = new_block;
            // send sigusr1 to all miners except this one, means start of next round
            for(i = 0; i < system->num_miners; i++){
                if(system->miners[i].pid != this_miner.pid)
                    kill(system->miners[i].pid, SIGUSR1);
            }
            sem_post(&(system->mutex));
            sem_post(&(system->fill));
        } else { // loser pepeHands
            sem_wait(&(system->empty));
            sem_wait(&(system->mutex));
            /* ----------- Protected ----------- */
            system->current_block.miners[system->current_block.total_votes] = this_miner;
            system->current_block.favorable_votes++;
            system->current_block.total_votes++;
            sem_post(&(system->mutex));
            sem_post(&(system->fill));
            /* ------------- end prot --------------- */
            sigsuspend(&a); // wait for SIGUSR1, means start of next round
        }
    }
    // SHUTDOWN
    close(miner2register[1]);
    free(threads);
    free(miner_data);
    // delete miner from shared memory
    sem_wait(&(system->empty));
    sem_wait(&(system->mutex));
    /* ----------- Protected ----------- */
    if(system->num_miners == 1){ // last miner
        printf("\nLast miner finished, deleting shared memory\n");
        sem_destroy(&(system->mutex));
        sem_destroy(&(system->empty));
        sem_destroy(&(system->fill));
        delete_miner(system->miners, &(system->num_miners), this_miner.pid);
        munmap(system, sizeof(System));
        shm_unlink(SYSTEM_SHM);
        return 0;
    }
    delete_miner(system->miners, &(system->num_miners), this_miner.pid);
    sem_post(&(system->mutex));
    sem_post(&(system->fill));
    munmap(system, sizeof(System));
    shm_unlink(SYSTEM_SHM);
    /* ------------- end prot --------------- */
    wait(NULL);
    return 0;
}