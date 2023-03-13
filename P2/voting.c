/**
 * @file voting.c
 * @author Enmanuel de Abreu Gil, Jorge Álvarez Fernández
 * @brief archivo que contiene la implementación de la práctica 2
 * @version 0.1
 * @date 2023-03-13
 * 
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
void _sigalrm_sigint_handler(){
    short i = 0;
    pid_t pid = 0;
    FILE *fd = fopen("pids.txt", "r");

    while(fscanf(fd, "%ld\n", &pid) != EOF)
        kill(pid, SIGTERM);
}

void _sigusr1_handler(){
    // hacer cosas cuando se reciba la señal SIGUSR1
}

void _sigterm_handler(){
    // hacer cosas cuando se reciba la señal SIGTERM
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
int principal(int n, int t){
    short i = 0;
    pid_t pids[n], pid = 0;
    sem_t *sem_id = NULL;
    FILE *fd = NULL;

    if(n < 1 || t < 1){
        fprintf(stdout, "ERROR: principal() recibió argumentos inválidos.\n");
        exit(EXIT_FAILURE);
    }

    fd = fopen("pids.txt", "w");

    // manejar SIGALRM
    signal(SIGALRM, _sigalrm_sigint_handler);
    // manejar SIGINT
    signal(SIGINT, _sigalrm_sigint_handler);

    sem_id = sem_open("sem", O_CREAT, 0644, 1);

    // fijar una alarma para dentro de t segundos
    alarm(t);
    
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