/**
 * @file voting.c
 * @author Enmanuel de Abreu Gil, Jorge Álvarez Fernández
 * @brief archivo que contiene la implementación de la práctica 2
 * @version 0.1
 * @date 2023-03-13
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/**
 * 
 * Sintaxis: ./voting -n <número de procesos votantes> -t <tiempo de votación>
 * la función alarm() genera una señal SIGALRM pasados t segundos. 
 * 
 */

void _sigalrm_sigint_handler(){
    // hacer cosas cuando se reciba la señal SIGALRM o SIGINT
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
}

/**
 * @brief crea los procesos votantes...
 * @param n número de procesos votantes
 * @param t tiempo de votación
 */
int principal(int n, int t){
    short i = 0;
    pid_t pids[n], pid = 0;
    FILE *fd = fopen("pids.txt", "w");

    // manejar SIGALRM
    signal(SIGALRM, _sigalrm_sigint_handler);
    // manejar SIGINT
    signal(SIGINT, _sigalrm_sigint_handler);
    
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