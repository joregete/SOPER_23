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
short responses = 0; // number of children that have responded
pid_t parent_pid = 0; // pid of the parent
FILE* voting_file = NULL; // file where the votes are stored
sem_t *sem; // semaphore

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

void _sigalrm_handler(){
    printf("\nfinishing by alarm");

    fclose(voting_file); // close the voting file
    sem_close(sem); // close the semaphore
    kill(0, SIGTERM);
}

void _sigint_handler(){
    printf("\nfinishing by signal");

    fclose(voting_file); // close the voting file
    sem_close(sem); // close the semaphore
    kill(0, SIGTERM);
}

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
    if(getpid() != parent_pid){
        fclose(voting_file); // each child must close its file stream
        sem_close(sem); // each child must close its semaphore
        exit(0);
    }
}

int votante(){
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
        kill(0, SIGUSR2); // this process is the Candidate, notify the others including parent
        fprintf(stdout, "%d es el candidato!!! FeelsGoodMan\n", getpid()); // ELIMINAR MAS TARDE
        fflush(stdout);

        // wait on semaphore
        if(_sem_wait_safe(sem) == -1){
            printf("Error: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        }

        // write the candidate pid as the first line of the file
        voting_file = fopen(FILEVOTES, "a");
        fprintf(voting_file, "%d\n", getpid());
        fclose(voting_file);

        // release semaphore
        sem_post(sem);
    }
    else{ // this process is a voter
        fprintf(stdout, "%d es votante Sadge\n", getpid()); // ELIMINAR MAS TARDE
        fflush(stdout);

        // wait on semaphore
        if(_sem_wait_safe(sem) == -1){
            printf("Error: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        }

        char response = (rand() % 2) ? 'Y' : 'N';
        voting_file = fopen(FILEVOTES, "a");
        fprintf(voting_file, "%c\n", response);
        fclose(voting_file);

        // release semaphore
        sem_post(sem);

        kill(parent_pid, SIGUSR2); // notify the parent that this process has finished
    }
    sigusr1_received = 0; // reset the flag
    sigusr2_received = 0; // reset the flag
    return 0;
}

/**
 * @brief creates the voter processes...
 */
int main(int argc, char * argv[]){
    int nprocs = 0, // number of voter processes
        nsecs = 0, // voting time
        votes = 0, // results of the voting
        i = 0; // loop variable
    char output[100] = "Candidate", // output string
        vote; // vote of each voter
    pid_t pid = 0, *pids = NULL; // variables to store the pids of the children
    FILE *aux = NULL; // auxiliary file pointer
    
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

    // Create n voting processes
    for(i=0;i<nprocs;i++) {
        pid = fork();
        if(pid == 0){ // child process
            signal(SIGINT, SIG_IGN);
            signal(SIGUSR1, _sigusr1_handler); // capture SIGUSR1
            signal(SIGUSR2, _sigusr2_handler); // capture SIGUSR2
            signal(SIGTERM, _sigterm_handler); // capture SIGTERM
            fprintf(stdout, "%d declaró sus manejadores. WideBoris\n", getpid()); // ELIMINAR MAS TARDE
            fflush(stdout);
            while(1)
                votante();
            exit(0);
        }
        else{ // parent process
            pids = (pid_t *) realloc(pids, (i+1)*sizeof(pid_t));
            pids[i] = pid;
        }
    }
    // sleep(0.25); // give children some time to declare handlers
    usleep(250000);
    signal(SIGALRM, _sigalrm_handler); // capture SIGALRM
    signal(SIGINT, _sigint_handler); // capture SIGINT
    signal(SIGUSR2, _sigusr2_handler); // capture SIGUSR2
    signal(SIGTERM, _sigterm_handler); // capture SIGTERM
    alarm(nsecs); // set alarm
    fprintf(stdout, "Progenitor %d declaró sus manejadores. WideBorpaSpin\n", getpid()); // ELIMINAR MAS TARDE
    fflush(stdout);

    while(1){
        if (sigusr1_received || responses >= nprocs)
            break;
        
        // send the SIGUSR1 signal to all voter processes
        fprintf(stdout, "Mandando SIGUSR1 a todos los hijos... PauseChamp\n"); // ELIMINAR MAS TARDE
        fflush(stdout);
        for(i = 0; i < nprocs; i++)
            kill(pids[i], SIGUSR1);
        
        // wait for all voter processes to finish
        while(responses < nprocs){
            sleep(1);
            printf("waiting... PauseChamp\n"); // ELIMINAR MAS TARDE
        }   

        // read the file and print the results
        voting_file = fopen(FILEVOTES, "r");
        if(voting_file == NULL){
            printf("Error: File could not be opened.\n");
            return EXIT_FAILURE;
        }
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
        printf("%s", output); // print the results
        sleep(0.25); // sleep for 0.25 seconds
        aux = fopen(FILEVOTES, "w"); // clear the file
        fclose(aux);
    }

    return 0;
}