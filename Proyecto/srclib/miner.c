#include "../includes/miner.h"

void init_block(Block *block, short last_id, int target) {
    block->id = last_id + 1;
    block->target = target;
    block->solution = 0;
    block->winner = 0;
    block->total_votes = 0;
    block->favorable_votes = 0;
    block->num_voters = 0;
}

uint8_t find_miner_index(Miner *miners, uint8_t num_miners, pid_t pid) {
    uint8_t i;
    for (i = 0; i < num_miners; i++) {
        if (miners[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void delete_miner(Miner *miners, uint8_t *num_miners, pid_t pid) {
    uint8_t index = find_miner_index(miners, *num_miners, pid);
    if (index != -1) {
        for (uint8_t i = index; i < *num_miners - 1; i++) {
            miners[i] = miners[i + 1]; // rearrange the array
        }
        (*num_miners)--;
    }
}

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
        dprintf(fd, "Id:\t\t\t%04d\nWinner:\t\t%d\nTarget:\t\t%ld\nSolution:\t%08ld\nVotes:\t\t%d/%d",
                    _block.id, _block.winner, _block.target, _block.solution, _block.favorable_votes, num_miners);
        _block.favorable_votes == num_miners ? dprintf(fd, "\t(validated) WidePeepoHappy") : dprintf(fd, "\t(rejected) pepeHands");
        dprintf(fd, "\nWallets:");
        for(i = 0; i < num_miners; i++)
            dprintf(fd, "\t%d:%02d", _block.miners[i].pid, _block.miners[i].coins);
        dprintf(fd, "\n-----------------------\n");
    }
    close(fd);
    exit(0);
}

void check_args(int argc, char *argv[], uint8_t *n_sec, uint8_t *nthreads){
    if (argc != 3){
        fprintf(stdout, "Usage: %s <NSECONDS> <NTHREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *n_sec = atoi(argv[1]);
    if (*n_sec <= 0 || *n_sec > 60){
        fprintf(stdout, "NSECONDS must be a value between 0 and 60\n");
        exit(EXIT_FAILURE);
    }
    *nthreads = atoi(argv[2]);
    if (*nthreads <= 0 || *nthreads > 9){
        fprintf(stdout, "NTHREADS must be a value between 0 and 9\n");
        exit(EXIT_FAILURE);
    }
}

System* create_system(){
    System *system;
    int fd_shm;
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
    system->monitor_up = 0;

    return system;
}