/**
 * @file miner.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief program that a miner process will execute
 * @date 2023-04-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../includes/miner.h"
#include "../includes/pow.h"
#include "../includes/list.h"

#define SYSTEM_SHM "/deadlift_shm"
#define MAX_MINERS 100

short list_isEmpty(List list) {
    return (list.first == NULL ? 1 : 0);
}

void list_append(List list, Miner *miner) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL)
        return;
    
    newNode->miner = miner;
    newNode->next = NULL;

    if (list.first == NULL) {
        list.first = newNode;
    } else {
        Node* current = list.first;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

void list_delete(List list, Miner* miner) {
    Node* current = list.first;
    Node* previous = NULL;

    while (current != NULL) {
        if (current->miner == miner) {
            if (previous == NULL) {
                list.first = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            break;
        }
        previous = current;
        current = current->next;
    }
}

void _register(int pipe_read);

/**
 * @brief Main function for the miner rush
 * 
 * @param argc 
 * @param argv 
 * @return 0 Exit success or 1 Exit failure
 */
int main(int argc, char *argv[]){
    pid_t pid;
    int rounds, nthreads, target, fd_shm;
    pthread_t minerThread, monitorThread;
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
        //open shared memory
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
    }
    // Mapping of the memory segment
    system = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if(system == MAP_FAILED){
        perror("mmap");
        shm_unlink(SYSTEM_SHM);
        exit(EXIT_FAILURE);
    }

}

void _register(int pipe_read){
    return;
}