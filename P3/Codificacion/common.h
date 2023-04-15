#ifndef _COMMON_H
#define _COMMON_H

#include <mqueue.h>
#include <unistd.h>
#include <sys/mman.h>
#include "pow.h"

// typedef struct _message{
//     long target;
//     long solution;
// } MESSAGE;

#define MAX_MSG 7
#define MQ_NAME "/mq_facepulls"
#define TARGET 0
#define SOLUTION 1

#endif