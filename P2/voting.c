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

volatile sig_atomic_t sigusr1_received = 0; // 0 = false, 1 = true
volatile sig_atomic_t sigusr2_received = 0; // 0 = false, 1 = true
short responses = 0; // número de hijos que han respondido
pid_t parent_pid = 0; // pid del padre

/**
 * @brief versión segura de sem_wait que controla que no se interrumpa 
 * la ejecución por una señal
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
 * @brief envia SIGTERM a todos los votante, espera a que terminen y libera los recursos del sistema
 */
void _sigalrm_sigint_handler(){
    kill(0, SIGTERM);
}


//HANDLERS USADOS POR VOTANTES

void _sigusr1_handler(){
    sigusr1_received = 1;
}

void _sigusr2_handler(){
    if(getpid() == parent_pid) // si es el padre
        responses++;
        return;
    sigusr2_received = 1; // si es un hijo
}

void _sigterm_handler(){
    printf("\n%d finishing by signal", getpid());
    exit(0);
}

int votante(sem_t *sem, FILE *voting_file){
    // Cuando el proceso votante reciba la señal SIGUSR1, mostrar el mensaje "Voting"
    // Cuando el proceso votante reciba la señal SIGTERM, liberar los recursos del sistema 
    // y terminar su ejecucion mostrando el mensaje "Finishing by signal"
    sigset_t mask, oldmask;

    // lo que nos recomendó Eduardo para esperar a SIGUSR1
    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    while (!sigusr1_received)
        sigsuspend (&oldmask);
    sigprocmask (SIG_UNBLOCK, &mask, NULL);

    // comprobar si éste es el primer proceso que llega
    if(!sigusr2_received){
        kill(0, SIGUSR2); // éste proceso es el Candidato, avisa a los demás
        // esperar en el semáforo
        _sem_wait_safe(sem);

        fprintf(voting_file, "%d\n", getpid());

        // liberar el semáforo
        sem_post(sem);
    }
    else{ // éste proceso es un votante
        // esperar en el semáforo
        _sem_wait_safe(sem);

        char response = (rand() % 2) ? 'Y' : 'N';
        fprintf(voting_file, "%c\n", response);

        // liberar el semáforo
        sem_post(sem);
    }
}

/**
 * @brief crea los procesos votantes...
 * @param n número de procesos votantes
 * @param t tiempo de votación
 */
int main(int argc, char * argv[]){
    FILE *voting_file;
    int shm_fd, nprocs = 0, nsecs = 0, i = 0, votes = 0;
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

    // crear semaforo
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED)
        return EXIT_FAILURE;
    

    parent_pid = getpid(); // guardar el pid del padre

    voting_file = fopen(FILEVOTES, "w");
    if(voting_file == NULL){
        sem_close(sem);
        sem_unlink(SEM_NAME);
        return EXIT_FAILURE;
    }

    // Crear n procesos votantes
    for(i=0;i<nprocs;i++) {
        pid = fork();
        if(pid == 0){ // proceso hijo
            signal(SIGINT, SIG_IGN);
            signal(SIGUSR1, _sigusr1_handler);
            signal(SIGUSR2, _sigusr2_handler);
            signal(SIGTERM, _sigterm_handler);
            votante(sem, voting_file);
        }
    }
    alarm(nsecs);
    signal(SIGUSR1, SIG_IGN); // ignorar SIGUSR1 para que kill(0, SIGUSR1) no se envíe a sí mismo
    signal(SIGTERM, SIG_IGN); // ignorar SIGTERM para que kill(0, SIGTERM) no se envíe a sí mismo
    signal(SIGALRM, _sigalrm_sigint_handler); // capturar SIGALRM
    signal(SIGINT, _sigalrm_sigint_handler); // capturar SIGINT
    signal(SIGUSR2, _sigusr2_handler); // capturar SIGUSR2 

    // enviar la señal SIGUSR1 a todos los procesos votantes
    kill(0, SIGUSR1);

    // esperar a que todos los procesos votantes terminen
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

    sem_close(sem);
    sem_unlink(SEM_NAME);
    fclose(voting_file);

    return 0;
}