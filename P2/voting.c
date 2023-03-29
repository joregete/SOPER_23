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
#define FILEPIDS "txt/pids.txt"
#define SEM_NAME "/calf_raise_sem"
#define SEM_EN_NAME "/sem_en"

// ---------------------------- GLOBAL VARIABLES ----------------------------
volatile sig_atomic_t sigusr1_received = 0; // 0 = false, 1 = true
volatile sig_atomic_t sigusr2_received = 0; // 0 = false, 1 = true
short responses = 0; // number of children that have responded
pid_t parent_pid = 0; // pid of the parent
FILE* voting_file = NULL; // file where the votes are stored
sem_t *sem; // semaphore
sem_t *sem_en; // semaphore

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
 * @brief kills the system and releases the resources
 */
void _kill_system(){
    FILE *pids = NULL;
    pid_t pid = 0;
    short num_voters = 0;
    sem_close(sem); 
    sem_unlink(SEM_NAME);
    sem_close(sem_en); 
    sem_unlink(SEM_EN_NAME); 
    pids = fopen(FILEPIDS, "r"); 
    while(fscanf(pids, "%d\n", &pid) != EOF){ 
        kill(pid, SIGTERM); // send SIGTERM to all the children
        num_voters++;
    }
    fclose(pids);
    for(short i = 0; i < num_voters; i++) 
        wait(NULL); // wait for the processes to finish
    exit(EXIT_SUCCESS);
}

void _sig_handler(int signum){
    switch(signum){
        case SIGALRM:
            printf("\nfinishing by alarm\n");
            _kill_system();
            break;
        case SIGINT:
            if(getpid() != parent_pid) return; // only the parent can be interrupted by SIGINT
            printf("\nfinishing by signal\n");
            _kill_system();
            break;
        case SIGUSR1:
            sigusr1_received = 1;
            break;
        case SIGUSR2:
            if(getpid() == parent_pid){ // if it's the parent
                responses++;
                return;
            }
            sigusr2_received = 1; // if it's a child
            break;
        case SIGTERM:
            if(getpid() != parent_pid){ // only the parent can be interrupted by SIGTERM
                sem_close(sem); // each child must close its semaphore
                exit(0);
            }
            break;
    }
}

int votante(){
    FILE *pids = NULL;
    int sem_value = 0;
    sigset_t mask1, mask2, oldmask;
    pid_t pid = 0, own_pid = getpid();

    while(1){
        sigusr1_received = 0; // reset the flag
        sigusr2_received = 0; // reset the flag
        sigfillset(&mask1);
        sigdelset (&mask1, SIGUSR1);
        // save the old mask
        sigprocmask (SIG_SETMASK, &mask1, &oldmask);
        // wait until parent sends SIGUSR1
        while (!sigusr1_received)
            sigsuspend (&oldmask);
        sigprocmask (SIG_SETMASK, &oldmask, NULL);

        // check if this is the first process to arrive
        if(sem_getvalue(sem_en, &sem_value) == 0){ // CANDIDATE
            fprintf(stdout, "Candidate %d\n", getpid());
            fflush(stdout);
            sem_post(sem_en); // update so other processes know that there is a candidate
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

            // send SIGUSR2 to all other children
            pids = fopen(FILEPIDS, "r");
            while(fscanf(pids, "%d\n", &pid) != EOF)
                if(pid != own_pid) kill(pid, SIGUSR2);
            fclose(pids);
        }
        else{ // this process is a voter
            // wait until candidate sends SIGUSR2
            fprintf(stdout, "Voter %d\n", getpid());
            fflush(stdout);
            sigfillset(&mask2);
            sigdelset (&mask2, SIGUSR2);
            // save the old mask
            sigprocmask (SIG_SETMASK, &mask2, &oldmask);
            // wait until parent sends SIGUSR1
            while (!sigusr1_received)
                sigsuspend (&oldmask);
            sigprocmask (SIG_SETMASK, &oldmask, NULL);

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
            kill(parent_pid, SIGUSR2); // notify the parent that this voter has voted
        }
    }
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
    FILE *aux = NULL, // auxiliary file pointer
        *pids_file = NULL; // file where the pids of the children are stored
    struct sigaction sa; // structure to handle signals

    sa.sa_handler = _sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
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

    // create semaphores
    sem_unlink(SEM_NAME); // unlink the semaphore (just in case it was created before)
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED) {
        sem_close(sem);
        sem_unlink(SEM_NAME);
        perror("sem_open");
        return EXIT_FAILURE;
    }

    sem_unlink(SEM_EN_NAME); // unlink the semaphore (just in case it was created before)
    sem_en = sem_open(SEM_EN_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem_en == SEM_FAILED) {
        sem_close(sem_en);
        sem_unlink(SEM_EN_NAME);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        perror("sem_open");
        return EXIT_FAILURE;
    }

    parent_pid = getpid(); // save the parent's pid
    pids_file = fopen(FILEPIDS, "w"); // open the file where the pids of the children will be stored

    // Create n voting processes
    for(i=0;i<nprocs;i++) {
        pid = fork();
        if(pid == 0){ // child process
            fclose(pids_file); // close the file
            votante(); // GET TO WORK!!!
        }
        else{ // parent process
            fprintf(pids_file, "%d\n", pid); // save the pid of the child
            pids = (pid_t *) realloc(pids, (i+1)*sizeof(pid_t)); // allocate memory for the pid
            pids[i] = pid; // save the pid of the child
        }
    }
    fclose(pids_file); // close the file
    alarm(nsecs); // set alarm

    while(1){
        usleep(250000); // sleep for 0.25 seconds
        // clear files
        aux = fopen(FILEVOTES, "w"); 
        fclose(aux);
        aux = fopen(FILEPIDS, "w"); 
        fclose(aux);
        sem_wait(sem_en); // allow a new candidate
        responses = 0; // reset the number of responses
        // send the SIGUSR1 signal to all voter processes
        fprintf(stdout, "Mandando SIGUSR1 a todos los hijos... PauseChamp\n"); // ELIMINAR MAS TARDE
        fflush(stdout);
        for(i = 0; i < nprocs; i++)
            kill(pids[i], SIGUSR1);
        
        // wait for all voter processes to finish
        while(responses != nprocs-1){
            ;
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
        fclose(voting_file);

        strcat(output, "]");
        votes > 0 ? strcat(output, " => Accepted\n") : strcat(output, " => Rejected\n");
        printf("%s", output); // print the results
        strcpy(output, "Candidate"); // reset the output string
    }

    return 0;
}