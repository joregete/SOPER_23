/**
 * @file monitor.c
 * @author your name (you@domain.com)
 * @brief Funcionality of the monitor process
 * @date 2023-02-14
 * 
 */

#include "monitor.h"

int monitor(int Psince, int Pwhere){
    int datos[2];
    char respuesta;
    ssize_t nbytes;
    while((nbytes = read(Psince, &datos, sizeof(int)*2))){
        if (pow_hash(datos[1]) == datos[0]){
            respuesta = 1;
            printf("Solution accepted : %08d --> %08d\n", datos[0], datos[1]);
        }
        else{
            respuesta = 0;
            printf("Solution rejected: %08d !-> %08d\n", datos[0], datos[1]);
        }
        write(Pwhere, &respuesta, sizeof(char));
    }
    if (nbytes < 0)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}