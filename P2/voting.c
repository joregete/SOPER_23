/**
 * @file voting.c
 * @brief implements the functionality of the voting system
 * @author Enmanuel Abreu & Jorge Álvarez
 * 
*/

#define _GNU_SOURCE // for ualarm
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "pids"
#define FILESEM "file_sem"
#define VOTINGSEM "vote_sem"
#define VOTINGFILE "voting_file"

/**
* @brief loop that distributes candidates and votes
* @param f file descriptor
* @return 0 if the function finishes correctly, 1 otherwise
*/
int loop(int f);

// definition of the Vote struct
typedef struct{
    pid_t pid;
    char vote;
}Vote;

pid_t *pid; // voters PIDs
int ppid; // parent PID
volatile int sig_recieved = 0;
int n_procs; // number of processes
sem_t *sem_en; // controls the access to the file
sem_t *sem_candida; // controls the permission to be a candidate

/**
 * @brief this function handles the signals SIGINT, SIGALRM and SIGUSR1
 * @param sig the signal received by the process
 * @author Enmanuel Abreu & Jorge Álvarez
*/
void handler(int sig){
    if (sig == SIGINT){
        if (ppid == getpid()){
            printf("finishing by signal\n");
            for (int i = 0; i < n_procs; i++){
                kill(pid[i], SIGINT);
                waitpid(pid[i], NULL, 0);
            }
            free(pid);
            sem_close(sem_en);
            sem_close(sem_candida);
            exit(EXIT_SUCCESS);
            }
        sig_recieved = SIGINT;
    }
    if (sig == SIGALRM){
        if (ppid != getpid()){
            return;
        }
        printf("finishing by alarm\n");
        for (int i = 0; i < n_procs; i++){
            kill(pid[i], SIGINT);
            waitpid(pid[i], NULL, 0);
        }
        free(pid);
        sem_close(sem_en);
        sem_close(sem_candida);
        exit(EXIT_SUCCESS);
    }
    if (sig == SIGUSR1 && sig_recieved == 0){
        sig_recieved = SIGUSR1;
    }
}

/**
 * @brief checks if the signal received is SIGINT
 * @author Enmanuel Abreu & Jorge Álvarez
*/
void check_sigint(){
    if (sig_recieved == SIGINT){
        free(pid);
        sem_close(sem_en);
        sem_close(sem_candida);
        exit(EXIT_SUCCESS);
    }
}

/**
 * @brief in charge of the voting process
 * @param f file descriptor
 * @return 0 if the program finishes correctly, 1 otherwise (short)
*/
short vote(int f){
    srand(rand());
    Vote vote;
    memset(&vote, 0, sizeof(vote));
    vote.pid = getpid();
    vote.vote = rand() % 2 ? 'Y' : 'N';
    sem_wait(sem_en);
    check_sigint();
    f = open(VOTINGFILE, O_WRONLY | O_APPEND);
    if (f == -1){
        perror("open");
        free(pid);
        return 1;
    }

    if(write(f, &vote, sizeof(vote)) != sizeof(vote)){
        perror("write");
        close(f);
        free(pid);
        return 1;
    }
    close(f);
    sem_post(sem_en);
    pause();
    loop(f);
    return 0;
}

int loop(int f){
    check_sigint();
    sig_recieved = 0;
    errno = 0;
    sem_trywait(sem_candida);
    if (errno == EAGAIN){
        check_sigint();
        if (sig_recieved == SIGUSR1){
            vote(f);
        }
        else {
            pause();
            check_sigint();
            vote(f);
        }
    }
    else{
        check_sigint();
        sem_wait(sem_en);
        check_sigint();
        srand(rand());
        Vote vote = {getpid(), rand()%2 ? 'Y' : 'N'};
        f = open(VOTINGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (f == -1){
            perror("open");
            free(pid);
            return 1;
        }

        if(write(f, &vote, sizeof(Vote)) != sizeof(Vote)){
            perror("write");
            close(f);
            free(pid);
            return 1;
        }
        close(f);
        sem_post(sem_en);
        for (int i = 0; i < n_procs; i++){
            kill(pid[i], SIGUSR1);
        }
        sigset_t a;
        sigfillset(&a);
        sigdelset(&a, SIGINT);
        sigdelset(&a, SIGALRM);
        check_sigint();
        while(1){
            ualarm(1000, 0);
            sigsuspend(&a);
            check_sigint();
            struct stat file;
            sem_wait(sem_en);
            stat(VOTINGFILE, &file);
            if (file.st_size == n_procs*sizeof(Vote)){
                printf("Candidate %d => [ ", getpid());
                Vote *votes = (Vote*)calloc(n_procs, sizeof(Vote));
                if (!votes){
                    perror("calloc");
                    return 1;
                }
                f = open(VOTINGFILE, O_RDONLY);
                if (f == -1){
                    perror("open");
                    free(pid);
                    free(votes);
                    return 1;
                }
                if(read(f, votes, n_procs*sizeof(Vote)) != n_procs*sizeof(Vote)){
                    perror("read");
                    close(f);
                    free(pid);
                    free(votes);
                    return 1;
                }
                close(f);
                float count = 0;
                for(int i = 0; i < n_procs; i++){
                    printf("%c ", votes[i].vote);
                    if (votes[i].vote == 'Y') count++;
                }
                free(votes);
                printf("] => %s\n", count/n_procs > 0.5 ? "Accepted" : "Rejected");
            }
            sem_post(sem_en);
            ualarm(250000, 0);
            sigsuspend(&a);
            sem_post(sem_candida);
            for (int i = 0; i < n_procs; i++){
                kill(pid[i], SIGUSR1);
            }
            loop(f);
        }
    }
    return 0;
}

/**
* @brief in charge of the voting process
* @param f file descriptor
* @param act struct sigaction
* @return 0 if the program finishes correctly, 1 otherwise (short)
*/
short work(int f, struct sigaction act){
    sigdelset(&(act.sa_mask), SIGINT);
    sigdelset(&(act.sa_mask), SIGUSR1);
    check_sigint();
    sem_wait(sem_en);
    check_sigint();
    f = open(FILENAME, O_RDONLY);
    if (f == -1){
        perror("open");
        free(pid);
        return 1;
    }
    if(read(f, pid, n_procs*sizeof(pid_t)) != n_procs*sizeof(pid_t)){
        perror("read");
        close(f);
        free(pid);
        return 1;
    }
    close(f);
    sem_post(sem_en);
    loop(f);
    return 0;
}

/**
 * @brief checks the paramaters, forks and sends each child to work
 * @param argc number of arguments
 * @param argv arguments
 * @return 0 if the program finishes correctly, 1 otherwise (short)
 */
int main(int argc, char * argv[]){
    int f;
    if (argc != 3){
        printf("Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
        return 1;
    }
    int n_secs;

    if ((n_procs = atoi(argv[1])) < 1){
        printf("N_PROCS must be positive\n");
        return 1;
    }
    n_secs = atoi(argv[2]);
    ppid = getpid();
    struct sigaction act;
    
    act.sa_handler = handler;
    act.sa_flags = 0;

    sigfillset(&(act.sa_mask));
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

    pid = (pid_t *)calloc(n_procs, sizeof(pid_t));
    if (!pid){
        perror("calloc");
        return 1;
    }
    if ((sem_en = sem_open(FILESEM, O_CREAT | O_EXCL , S_IRUSR | S_IWUSR, 0) ) == SEM_FAILED) {
        perror (" sem_open ") ;
        return 1;
    }
    sem_unlink(FILESEM);

    if ((sem_candida = sem_open(VOTINGSEM, O_CREAT | O_EXCL , S_IRUSR | S_IWUSR, 1) ) == SEM_FAILED) {
        perror (" sem_open ") ;
        return 1;
    }
    sem_unlink(VOTINGSEM);


    for (int i = 0; i < n_procs; i++){
        if ((pid[i] = fork()) == 0){
            work(f, act);
        }
        if (pid[i]<0){
            perror("fork");
            for (i--;i >= 0; i--){
                kill(pid[i], SIGINT);
                waitpid(pid[i], NULL, 0);
            }
            free(pid);
            return 1;
        }
    }

    f = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f == -1){
        perror("open");
        free(pid);
        return 1;
    }

    if(write(f, pid, n_procs*sizeof(pid_t)) != n_procs*sizeof(pid_t)){
        perror("write");
        close(f);
        free(pid);
        return 1;
    }
    close(f);

    sigdelset(&(act.sa_mask), SIGINT);
    sigdelset(&(act.sa_mask), SIGALRM);
    sem_post(sem_en);

    if (n_secs > 0){
        alarm(n_secs);
    }
    while(pause());
}