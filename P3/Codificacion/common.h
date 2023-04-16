#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include "pow.h"


typedef struct _block{
    long target;
    long solution;
    int flag;
    int end;
} Block;

typedef struct _message{
    Block block;
} MESSAGE;

#define MAX_MSG 7
#define MQ_NAME "/mq_facepulls"
#define SIZE sizeof(MESSAGE)

#endif