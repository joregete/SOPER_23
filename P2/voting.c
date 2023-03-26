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
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define FILEVOTES "txt/voting.txt"
#define SEM_NAME "/calf_raise_sem"

// ---------------------------- GLOBAL VARIABLES ----------------------------
volatile sig_atomic_t sigusr1_received = 0; // 0 = false, 1 = true
volatile sig_atomic_t sigusr2_received = 0; // 0 = false, 1 = true
volatile sig_atomic_t sigalrm_pending = 0; // 0 = false, 1 = true
short responses = 0; // number of children that have responded
pid_t parent_pid = 0; // pid of the parent
FILE* voting_file = NULL;

/**
 * @brief safe version of sem_wait that controls that execution is not interrupted by a signal
 */
int _sem_wait_safe(sem_t *sem_id) {
  while (sem_wait(sem_id))
    if (errno == EINTR) 
        errno = 0;
    else 
        return -1;
  return 0;
}

/**
 * @brief sends SIGTERM to all voters, waits for them to finish and releases system resources
 */
void _sigalrm_sigint_handler(int sig){
    if(sig == SIGALRM)
        printf("\nfinishing by alarm");
    if(sig == SIGINT)
        printf("\nfinishing by signal");

    fclose(voting_file); // close the voting file
    kill(0, SIGTERM);
}

// handler for SIGALRM
void _sigalrm_handler(int sig){
    sigalrm_pending = 1;
}

//HANDLERS USED BY VOTERS

void _sigusr1_handler(){
    sigusr1_received = 1;
}

void _sigusr2_handler(){
    if(getpid() == parent_pid){ // if it's the parent
        responses++;
        return;
    }
    sigusr2_received = 1; // if it's a child
}

void _sigterm_handler(){
    fclose(voting_file); // each child must close its file stream
    exit(0);
}

int votante(sem_t *sem){
    // When the voter process receives the SIGUSR1 signal, show the message "Voting"
    // When the voter process receives the SIGTERM signal, release system resources 
    // and end its execution showing the message "Finishing by signal"
    sigset_t mask, oldmask;

    // what Eduardo recommended us to wait for SIGUSR1
    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    while (!sigusr1_received)
        sigsuspend (&oldmask);
    sigprocmask (SIG_UNBLOCK, &mask, NULL);

    // check if this is the first process to arrive
    if(!sigusr2_received){
        kill(0, SIGUSR2); // this process is the Candidate, notify the others
        // wait on semaphore
       if(_sem_wait_safe(sem) == -1){
            printf("Error: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        }
        
        fprintf(stdout, "%d es el candidato!!! FeelsGoodMan\n", getpid()); // ELIMINAR MAS TARDE
        fflush(stdout);
        fprintf(voting_file, "%d\n", getpid());

        // release semaphore
        sem_post(sem);
    }
    else{ // this process is a voter
        // wait on semaphore
        if(_sem_wait_safe(sem) == -1){
            printf("Error: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "%d es votante Sadge\n", getpid()); // ELIMINAR MAS TARDE
        fflush(stdout);


        char response = (rand() % 2) ? 'Y' : 'N';
        fprintf(voting_file, "%c\n", response);

        // release semaphore
        sem_post(sem);
    }
    return 0;
}

/**
 * @brief creates the voter processes...
 * @param n number of voter processes
 * @param t voting time
 */
int main(int argc, char * argv[]){
    int nprocs = 0, nsecs = 0, votes = 0, i;
    char output[100] = "Candidate", vote;
    pid_t pid = 0;
    sem_t *sem;
    
    if (argc != 3){
        printf("Error: Invalid number of arguments.\n");
        return EXIT_FAILURE;
    }

    nprocs = atoi(argv[1]);
    if(nprocs <= 0){
        printf("Error: N_PROCS must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    nsecs = atoi(argv[2]);
    if(nsecs <= 0){
        printf("Error: N_SECS must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    // create semaphore
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED) {
        sem_close(sem);
        sem_unlink(SEM_NAME);
        perror("sem_open");
        return EXIT_FAILURE;
}
    

    parent_pid = getpid(); // save the parent's pid

    // have the file already opened for the children
    voting_file = fopen(FILEVOTES, "w");
    if(voting_file == NULL){
        sem_close(sem);
        sem_unlink(SEM_NAME);
        return EXIT_FAILURE;
    }

    // Create n voter processes
    for(i=0;i<nprocs;i++) {
        pid = fork();
        if(pid == 0){ // child process
            signal(SIGINT, SIG_IGN);
            signal(SIGUSR1, _sigusr1_handler);
            signal(SIGUSR2, _sigusr2_handler);
            signal(SIGTERM, _sigterm_handler);
            fprintf(stdout, "%d declaró sus manejadores. WideBoris\n", getpid()); // ELIMINAR MAS TARDE
            fflush(stdout);
            // signal parent when done setting up
            kill(parent_pid, SIGUSR1);
            votante(sem);
        }
    }
    sleep(0.5); // give children some time to declare handlers
    alarm(nsecs);
    signal(SIGUSR1, SIG_IGN); // ignore SIGUSR1 so that kill(0, SIGUSR1) is not sent to itself
    signal(SIGTERM, SIG_IGN); // ignore SIGTERM so that kill(0, SIGTERM) is not sent to itself
    signal(SIGALRM, _sigalrm_sigint_handler); // capture SIGALRM
    signal(SIGINT, _sigalrm_sigint_handler); // capture SIGINT
    signal(SIGUSR2, _sigusr2_handler); // capture SIGUSR2
    fprintf(stdout, "Progenitor %d declaró sus manejadores. WideBorpaSpin\n", getpid()); // ELIMINAR MAS TARDE
    fflush(stdout);



    // send the SIGUSR1 signal to all voter processes
    fprintf(stdout, "Mandando SIGUSR1 a todos los hijos... PauseChamp\n"); // ELIMINAR MAS TARDE
    fflush(stdout);
    kill(0, SIGUSR1);

    // wait for all voter processes to finish
    while(responses < nprocs)
        sleep(1);

    fscanf(voting_file, "%d", &pid);
    sprintf(output, " %d", pid);
    strcat(output, " => [ ");
    while(fscanf(voting_file, "%c\n", &vote) != EOF){
        if(vote == 'Y'){
            strcat(output, "Y ");
            votes++;
        }
        else{
            strcat(output, "N ");
            votes--;
        }
    }

    strcat(output, "]");
    votes > 0 ? strcat(output, " => Accepted\n") : strcat(output, " => Rejected\n");
    printf("%s", output);

    for(i=0; i < nprocs; i++)
        wait(NULL);

    sem_close(sem);
    sem_unlink(SEM_NAME);
    fclose(voting_file);

    return 0;
}