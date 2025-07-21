#define _POSIX_C_SOURCE 200809L
#include <signal.h>  
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "monitor.h"
#include "process.h"
#include "shell.h"
#include "variable.h"

extern void printJobs();
extern void showSystemMonitor();

int shellFg(char** arguments);
int shellBg(char** arguments);
int shellCd(char** arguments);
int shellHelp(char** arguments);
int shellJobs(char** arguments);
int shellKill(char** arguments);
static int builtinSet (char** arguments);  
static int builtinUnset (char** arguments);
static int builtinExport(char** arguments);
static int builtinEnv (char** arguments);

int shellSysmon(char** arguments) 
{

    (void)arguments;
    showSystemMonitor();
    return 1;

}

char* builtinNames[] = 
{

    "cd", "help", "jobs", "kill", 
    "sysmon", "fg", "bg", "set", "unset", "export", "env"

};

int (*builtinHandlers[]) (char**) = 
{

    &shellCd, &shellHelp, &shellJobs, &shellKill,
    &shellSysmon, &shellFg, &shellBg, &builtinSet, 
    &builtinUnset, &builtinExport, &builtinEnv

};

int shellFg(char** arguments)
{

    if (!arguments[1]) { fputs("fg: usage fg <job|pid>\n", stderr); return 1; }

    pid_t pgid = (arguments[1][0] == '%') ? jobPgid(atoi(arguments[1] + 1) - 1) : atoi(arguments[1]);
    if (pgid <= 0) { fputs("fg: no such job\n", stderr); return 1; }

    setpgid(pgid, pgid);                       
    tcsetpgrp(STDIN_FILENO, shell_pgid);       

    if (tcsetpgrp(STDIN_FILENO, pgid) == -1)
    {

        perror("tcsetpgrp‑fg");
        return 1;

    }

    kill(-pgid, SIGCONT);
    int status;

    while (waitpid(-pgid, &status, WUNTRACED) > 0)
        if (WIFSTOPPED(status)) break;

    tcsetpgrp(STDIN_FILENO, shell_pgid);       
    return 1;

}

int shellBg(char** arguments) 
{

    if (arguments[1] == NULL) 
    {

        printf("bg: expected PID\n");
        return 1;

    }

    pid_t pid = atoi(arguments[1]);

    if (kill(pid, SIGCONT) == -1) 
    {

        perror("bg: SIGCONT failed");
        return 1;

    }

    printf("Process %d continued in background\n", pid);
    return 1;

}

int numBuiltins() 
{

    return sizeof(builtinNames) / sizeof(char*);

}

int shellCd(char** arguments) 
{
    
    if (arguments[1] == NULL) 
    {

        fprintf(stderr, "cd: expected argument\n");

    } 
    else 
    {

        if (chdir(arguments[1]) != 0) 
        {

            perror("cd error");

        }

    }

    return 1;

}

int shellHelp(char** arguments) 
{

    (void)arguments;
    printf("Built-in commands:\n");

    for (int i = 0; i < numBuiltins(); i++)
    {

        printf("  %s\n", builtinNames[i]);

    }

    printf("Use man pages for external commands.\n");
    return 1;

}

int shellJobs(char** arguments) 
{

    (void)arguments;
    printJobs();
    return 1;

}

static int builtinSet(char** arguments)
{
    if (!arguments[1] || !arguments[2] || arguments[3]) 
    {

        fputs("usage: set NAME VALUE\n", stderr);
        return 1;                  

    }

    varSet(arguments[1], arguments[2], false);
    return 1;                     
    
}

static int builtinUnset(char** arguments)
{

    if (!arguments[1] || arguments[2]) { fputs("usage: unset NAME\n", stderr); return 1; }

    varUnset(arguments[1]);
    return 1;

}

static int builtinExport(char** arguments)
{

    if (!arguments[1] || arguments[2]) { fputs("usage: export NAME\n", stderr); return 1; }
    const char* value = varGet(arguments[1]);
    if (!value) { fprintf(stderr, "export: %s not set\n", arguments[1]); return 1; }

    varSet(arguments[1], value, true);
    return 1;

}

static int builtinEnv(char** arguments)
{

    (void)arguments;
    char** envp = varsBuildEnviron();

    for (char** exprPosition = envp; *exprPosition; ++exprPosition) { puts(*exprPosition); free(*exprPosition); }
    free(envp);

    return 1;

}

static int sigFromName(const char* option)
{

    struct { const char* name; int signal; } signalTable[] = 
    {

        { "-HUP",  SIGHUP  }, { "-INT",  SIGINT  }, { "-TERM", SIGTERM },
        { "-KILL", SIGKILL }, { "-STOP", SIGSTOP }, { "-CONT", SIGCONT },
        { NULL, 0 }

    };

    for (int i = 0; signalTable[i].name; i++)
    {

        if (strcmp(option, signalTable[i].name) == 0) return signalTable[i].signal;

    }

    return -1;

}

int shellKill(char** arguments)
{

    if (!arguments[1]) { puts("kill: usage: kill [-SIGNAL] pid"); return 1; }

    int signal = SIGKILL;            
    int pidIndex = 1;

    if (arguments[1][0] == '-') 
    {

        signal = sigFromName(arguments[1]);
        if (signal == -1) { puts("kill: unknown signal"); return 1; }
        pidIndex = 2;

    }

    if (!arguments[pidIndex]) { puts("kill: expected PID"); return 1; }

    for (char *exprPosition = arguments[pidIndex]; *exprPosition; exprPosition++)
    {

        if (!isdigit((unsigned char)*exprPosition)) 
        {

            puts("kill: PID must be numeric"); return 1;

        }

    }

    pid_t pid = atoi(arguments[pidIndex]);
    if (pid == getpid()) { puts("Refuse to signal myself"); return 1; }

    if (kill(pid, signal) == -1) perror("kill");
    else printf("Signal %d sent to %d\n", signal, pid);
    return 1;

}

int executeBuiltin(char** arguments) 
{

    if (arguments[0] == NULL) 
    {

        return 1;

    }

    for (int i = 0; i < numBuiltins(); i++) 
    {

        if (strcmp(arguments[0], builtinNames[i]) == 0) 
        {

            return (*builtinHandlers[i])(arguments);

        }

    }

    return 0; 

}
