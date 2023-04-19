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

int find_miner_index(Miner *miners, uint8_t num_miners, pid_t pid) {
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
            miners[i] = miners[i + 1];
        }
        (*num_miners)--;
    }
}

void _register(int pipe_read){
    exit(0);
}

/**
 * @brief Main function for the miner process
 * @return 0 Exit success or 1 Exit failure
 */
int main(int argc, char *argv[]){
    pid_t pid;
    int rounds, nthreads, fd_shm;
    int miner2register[2]; // pipe
    System *system;

    if (argc != 3){
        printf("Usage: %s <ROUNDS> <NTHREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[1]);
    if (rounds <= 0){
        fprintf(stderr, "ROUNDS has to be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[2]);
    if (nthreads <= 0){
        fprintf(stderr, "The amount of miners(threads) has to be greater than 0\n");
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
        close(miner2register[0]); // close write end
        _register(miner2register[1]);
    }

    // miner
    close(miner2register[1]); // close read end
    // if shm exixts, open it else create it
    fd_shm = shm_open(SYSTEM_SHM, O_RDWR, 0666);
    if (fd_shm == -1){ // -1 shm does not exist
        printf("\nCreating shared memory\n");
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
    }
    if(system->num_miners == MAX_MINERS){
        printf("\nsystem doesn't accept more miners\n");
        shm_unlink(SYSTEM_SHM);
        exit(EXIT_SUCCESS);
    }
    Miner new_miner; // new miner representing this process
    new_miner.pid = getpid();
    new_miner.coins = 0;
    sem_wait(&(system->empty));
    sem_wait(&(system->mutex));

    /* ----------- Protected ----------- */
    system->miners[system->num_miners] = new_miner;
    system->num_miners++;
    
    sem_post(&(system->mutex));
    sem_post(&(system->fill));
    /* ------------- end prot --------------- */
    printf("\nminer %d registered\n", new_miner.pid);
    sleep(5);
    printf("\nminer %d finished\n", new_miner.pid);
    sem_wait(&(system->empty));
    sem_wait(&(system->mutex));

    /* ----------- Protected ----------- */
    if(system->num_miners == 1){ // last miner
        printf("\nLast miner finished, deleting shared memory\n");
        sem_destroy(&(system->mutex));
        sem_destroy(&(system->empty));
        sem_destroy(&(system->fill));
        delete_miner(system->miners, &(system->num_miners), new_miner.pid);
        munmap(system, sizeof(System));
        shm_unlink(SYSTEM_SHM);
        return 0;
    }
    delete_miner(system->miners, &(system->num_miners), new_miner.pid);
    munmap(system, sizeof(System));
    shm_unlink(SYSTEM_SHM);
    sem_post(&(system->mutex));
    sem_post(&(system->fill));
    /* ------------- end prot --------------- */
    return 0;
}