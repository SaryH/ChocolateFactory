#ifndef _LOCAL_H_
#define _LOCAL_H_
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h> 
#include <unistd.h>
#include <string.h>

//read file
#define INPUT_LINES 18
#define MAX_BUF 50

struct emp_info{
	int line_id;
	int emp_id;
	char chocolate_type[4];
};

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#endif
