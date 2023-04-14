#ifndef _COMMON_H
#define _COMMON_H

#include <mqueue.h>
#include <unistd.h>
#include "pow.h"

typedef struct _message{
    long target;
    long solution;
} MESSAGE;

#define MAX_MSG 7
#define MAX_MSG_BODY 1024
#define MQ_NAME "/mq_facepulls"

#endif