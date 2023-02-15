/**
 * @file miner.h
 * @author Enmanuel Abreu & Jorge Álvarez
 * @date 2023-02-14
 *
 */
#ifndef _MINER_H
#define _MINER_H

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

#include "pow.h"
#define MAX_MINERS 10

/**
 * @brief Computes the POW
 * @author Enmanuel Abreu & Jorge Álvarez
 * @param Psince value since where we're start reading the pipe
 * @param Pwhere value of the pipe where we're writing
 * @return 0 if everything goes correctly, 1 if error.
 */
int miner(int rounds, int nthreads, int Psince, int Pwhere);


#endif

