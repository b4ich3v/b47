#define _POSIX_C_SOURCE 200809L

#define MAX_PROCESSES 64
#include <signal.h>                 
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"

process processes[MAX_PROCESSES];
int processCount = 0;

int jobsCount(void)
{ 

    return processCount; 

}

pid_t jobPgid(int index)
{

    return (index >= 0 && index < processCount) ? processes[index].pid : -1; 

}

void updateProcess(pid_t pid, int status)
{

    for (int i = 0; i < processCount; i++)
    {

        if (processes[i].pid == pid) 
        {

            if (WIFSTOPPED(status)) processes[i].isRunning = 0;         
            else processes[i].isRunning = -1;         
                
            return;
            
        }

    }

}

void sigChldHandler(int signal)
{

    (void)signal;
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) 
    {

        updateProcess(pid, status);       

        if (WIFSTOPPED(status)) printf("\n[%d]  Stopped\n", pid);
        else if (WIFEXITED(status) || WIFSIGNALED(status)) printf("\n[%d]  Done\n", pid);
            
    }

}

const char* getProcessState(pid_t pid)
{

    static char state[32] = "Unknown";
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *filePath = fopen(path, "r");
    if (!filePath) return "Terminated";

    char line[128];

    while (fgets(line, sizeof(line), filePath)) 
    {

        if (strncmp(line, "State:", 6) == 0) 
        {

            char code;

            if (sscanf(line, "State:\t%c", &code) == 1) 
            {

                switch (code) 
                {

                case 'R': strcpy(state, "running"); break;
                case 'S': strcpy(state, "sleeping"); break;
                case 'D': strcpy(state, "waiting"); break;
                case 'Z': strcpy(state, "zombie"); break;
                case 'T': strcpy(state, "stopped"); break;
                case 'X': strcpy(state, "dead"); break;
                default:  strcpy(state, "unknown");

                }

                fclose(filePath);
                return state;

            }

        }

    }

    fclose(filePath);
    return "Unknown";

}

void addProcess(pid_t pid, const char* command)
{

    if (processCount < MAX_PROCESSES) 
    {

        processes[processCount].pid = pid;
        strncpy(processes[processCount].command, command, 256);
        processes[processCount].isRunning = 1;
        processCount += 1;

    }

}

void removeProcess(int index)
{

    if (index < 0 || index >= processCount) return;

    for (int i = index; i < processCount - 1; i++) 
    {

        processes[i] = processes[i + 1];

    }

    processCount -= 1;

}

void printJobs(void)
{

    printf("Jobs:\n");

    for (int i = 0; i < processCount;) 
    {

        pid_t result = waitpid(processes[i].pid, NULL, WNOHANG);

        if (result == processes[i].pid) 
        {

            removeProcess(i);
            continue;                      

        }

        if (result == -1) 
        {

            removeProcess(i);
            continue;

        }

        if (kill(processes[i].pid, 0) == 0) 
        {

            const char *state = getProcessState(processes[i].pid);
            printf("[%d] PID: %d CMD: %s STATUS: %s\n", i + 1, processes[i].pid, processes[i].command, state);
            i += 1;   

        } 
        else 
        {

            removeProcess(i);             

        }

    }

    if (processCount == 0) { puts("No jobs running"); return; }

}
