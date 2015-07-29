/* 	This file contains declarations of functions for
 *      multi lookup.
 *  
 */

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "queue.h"
#include "util.h"

#define MAX_INPUT_FILES	10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define INET6_ADDRSTRLEN 46
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define MAX_FILE_NAME_LENGTH 255
#define QUEUE_SIZE 10
#define T_PER_CPU 2

/* Fuction run by request threads to read input file and add hostnames to queue
 */
void* inputRequest(void *fn);

/* Fuction run by resolver threads which executes dns lookup and writes result to file
 */
void* resolveRequests();

/* Main function which takes in arguments and creates necessary threads */
int main(int argc, char *argv[]);

#endif
