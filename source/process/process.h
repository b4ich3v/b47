#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

typedef struct process
{

    pid_t pid;
    char command[256];
    int isRunning;
    
} process;

int jobsCount(void);
pid_t jobPgid(int index);
void addProcess(pid_t pid, const char* command);
void removeProcess(int index);
void printJobs(void);
void sigChldHandler(int signal);
void updateProcess(pid_t pid, int status);

#endif 
