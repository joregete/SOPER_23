/**
 * @file monitor.c
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief Funcionality of the monitor process
 * @date 2023-02-14
 * 
 */

#include "monitor.h"

int monitor(int monitorPipe, int minerPipe){
    long readData[2];
    ssize_t leido;
    short resp;
    while((leido = read(minerPipe, &readData, sizeof(long)*2))){
        if (leido < 0){
            perror("Error reading from the pipe in the monitor");
            exit(EXIT_FAILURE);
        }
        if (pow_hash(readData[1]) == readData[0]){
            resp = 1;
            printf("Solution accepted : %08ld --> %08ld\n", readData[0], readData[1]);
        }
        else{
            resp = 0;
            printf("Solution rejected: %08ld !-> %08ld\n", readData[0], readData[1]);
        }
        write(monitorPipe, &resp, sizeof(short));
    }
    exit(EXIT_SUCCESS);
}