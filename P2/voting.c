/**
 * @file voting.c
 * @author Enmanuel de Abreu Gil, Jorge Álvarez Fernández
 * @brief implements the exercise 10 of the second practice of SOPER
 * @version 1.0
 * @date 2023-03-13
 */

#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHM_NAME "/my_shm" // shared memory name
#define SEM_NAME "/my_sem" // semaphore name
#define FILENAME "pids.txt"


//GLOBAL VARIABLES
int *pids; // shared memory
sem_t *sem_id = NULL; // semaphore


/**
 * @brief versión segura de sem_wait que controla que no se interrumpa 
 * la ejecución por una señal
 */
int _sem_wait_safe(sem_t *sem_id) {
  while (sem_wait(sem_id))
    if (errno == EINTR) errno = 0;
    else return -1;
  return 0;
}

/**
 * @brief manda la señal SIGTERM a todos los procesos votantes
 * 
 */
// void _sigalrm_sigint_handler(){
//     short i = 0;
//     pid_t pid = 0;
//     FILE *fd = fopen("pids.txt", "r");

//     while(fscanf(fd, "%ld\n", &pid) != EOF)
//         kill(pid, SIGALRM);
    
//     fclose(fd);
//     remove("pids.txt");
// }

// void _sigusr1_handler(){
// }

// void _sigterm_handler(){
// }

void handle_signal(int sig) {
    switch(sig) {
        case SIGALRM:
            printf("Received SIGALRM\n");
            // do something with SIGALRM
            break;
        case SIGINT:
            printf("Received SIGINT\n");
            // send SIGTERM to all Votante processes
            kill(0, SIGTERM);
            break;
        case SIGUSR1:
            printf("Received SIGUSR1\n");
            // Cuando el sistema este listo, el proceso Principal enviará la señal SIGUSR1 a todos los procesos Votante

            break;
        case SIGTERM:
            printf("Received SIGTERM\n");
            // exit program
            munmap(pids, sizeof(int));
            if (shm_unlink(FILENAME) == -1) {
                perror("shm_unlink");
                exit(1);
            }
            printf("Finishing by signal\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            break;
    }
    
    // update pids.txt with current process ID
    if (sem_wait(sem) == -1) { // decrement semaphore
        perror("sem_wait");
        exit(1);
    }

    FILE* fp = fopen(FILENAME, "a"); //append mode
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }
    fprintf(fp, "%d\n", getpid());
    fclose(fp);

    if (sem_post(sem) == -1) { // increment semaphore
        perror("sem_post");
        exit(1);
    }
}


int votante(){
    // Cuando el proceso votante reciba la señal SIGUSR1, mostrar el mensaje "Voting"
    // Cuando el proceso votante reciba la señal SIGTERM, liberar los recursos del sistema 
    // y terminar su ejecucion mostrando el mensaje "Finishing by signal"
    sigset_t mask, oldmask;

    /* Set up the mask of signals to temporarily block. */
    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);

    /* Wait for a signal to arrive. */
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    while (!usr_interrupt)
    sigsuspend (&oldmask);
    sigprocmask (SIG_UNBLOCK, &mask, NULL);
}

/**
 * @brief crea los procesos votantes...
 * @param n número de procesos votantes
 * @param t tiempo de votación
 */
int main(int argc, char * argv[]){
    // short i = 0;
    // pid_t pids[n], pid = 0;
    FILE *fd = NULL;
    int shm_fd, nprocs = 0, nsecs = 0;
    

    if (argc != 3){
        printf("Usage: %s <N_PROCS> <N_SECS>\n <N_PROCS> es el número de procesos
                que participarán en la votación y\n <N_SECS> es el número máximo 
                de segundos que estará activo el sistema\n", argv[0]);
        return 1;
    }

    if(nprocs = atoi(agv[1]) <= 0){
        printf("Error: N_PROCS must be a positive integer.\n");
        return 1;
    }

    if(nsecs = atoi(agv[2]) <= 0){
        printf("Error: N_SECS must be a positive integer.\n");
        return 1;
    }

  // create shared memory segment
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    
    // attach shared memory segment
    pids = (int*) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (pids == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // set initial value in shared memory
    *pids = getpid();
    
    // create semaphore
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    // register signal handlers
    if (signal(SIGALRM, handle_signal) == SIG_ERR) {
        perror("signal SIGALRM");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("signal SIGINT");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGUSR1, handle_signal) == SIG_ERR) {
        perror("signal SIGUSR1");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTERM, handle_signal) == SIG_ERR) {
        perror("signal SIGTERM");
        exit(EXIT_FAILURE);
    }

    sem_id = sem_open("sem", O_CREAT, 0644, 1);
    if (sem_id == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // fijar una alarma para dentro de t segundos
    alarm(t); //no sé que coño hace esto xd
    
    if(fd == NULL){
        fprintf(stdout, "ERROR: principal() falló abriendo 'pids.txt'.\n");
        exit(EXIT_FAILURE);
    }
    // Crear n procesos votantes
    for(i=0;i<n;i++) {
        if(fork() == 0){
            // Almacenar los pids hijos en un fichero
            pid = getpid();
            fprintf(fd, "%d\n", pid);
            pids[i] = pid;
            votante();
        }
    }
    // enviar la señal SIGUSR1 a todos los procesos votantes
    for(i = 0; i < n; i++){
        kill(pids[i], SIGUSR1);
    }
    // en cuanto se reciba SIGINT o SIGALRM, hay que enviar SIGTERM
    // a todos los votante, esperar que terminen 
    for(i = 0; i < n; i++){
        wait(NULL);
    }
    // y liberar los recursos del sistema
}