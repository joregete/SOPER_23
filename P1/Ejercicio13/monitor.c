/**
 * @file monitor.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Funcionality of the monitor process
 * @date 2023-02-14
 * 
 */

#include "monitor.h"

int monitor(int monitorPipe, int minerPipe){
    long datos[2];
    char respuesta;
    ssize_t nbytes;
    while((nbytes = read(minerPipe, &datos, sizeof(long)*2))){
    // printf("%ld , %ld\n", datos[0], datos[1]);
        if (pow_hash(datos[1]) == datos[0]){
            respuesta = 1;
            printf("Solution accepted : %08d --> %08d\n", datos[0], datos[1]);
        }
        else{
            respuesta = 0;
            printf("Solution rejected: %08d !-> %08d\n", datos[0], datos[1]);
        }
        write(monitorPipe, &respuesta, sizeof(char));
    }
    
    if (nbytes < 0)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}