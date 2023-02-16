/**
 * @file monitor.h
 * @author Enmanuel Abreu & Jorge Álvarez
 * @brief 
 * @date 2023-02-14
 * 
*/

#ifndef MONITOR_H
#define MONITOR_H

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


/**
 * @brief in this function we will monitor the process
 *        and we will print the information of the process
 *        and check if the solution is correct.
 * @param Psince value since where we're start reading the pipe
 * @param Pwhere value of the pipe where we're writing
 * @return 0 ok, 1 error
 */
int monitor (int Psince, int Pwhere);


#endif