#define _GNU_SOURCE 
#define RM_TOK(position)  do { arguments[position] = NULL; if (arguments[(position)+1]) arguments[(position) + 1] = NULL; } while(0)
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <wordexp.h>

#include "builtins.h"
#include "executor.h"
#include "expand.h"
#include "parser.h"
#include "process.h"
#include "variable.h"

extern void collectHeredocs(node_t* node); 
pid_t shell_pgid;
static pid_t parent_pgid;

int main() 
{

    varsInit();

    parent_pgid = tcgetpgrp(STDIN_FILENO);

    shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);        
    tcsetpgrp(STDIN_FILENO, shell_pgid);    

    signal(SIGINT, SIG_IGN);               
    signal(SIGTSTP, SIG_IGN);               
    signal(SIGTTOU, SIG_IGN);               
    signal(SIGTTIN, SIG_IGN); 
    
    while(true) 
    {

        if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1 && errno != EPERM)
        {

            perror("tcsetpgrp‑prompt");

        }

        char* input = readline("\033[38;2;255;165;0mb47>\033[0m ");
        
        if (!input) 
        {

            if (errno == EIO) 
            {        

                clearerr(stdin);
                continue;

            }

            break;    
                                
        }                       
    
        char *expanded = preprocessLine(input);
        if (*expanded) add_history(expanded);

    
        char* q = expanded;
        while (*q && isspace((unsigned char)*q)) q++;

        if (*q == '\0') 
        {                        

            free(expanded);
            free(input);
            continue;

        }

        if (strcmp(expanded, "exit") == 0) 
        {

            free(expanded);
            free(input);
            break;

        }

        node_t* tree = parseLine(expanded);   
        collectHeredocs(tree); 
        run(tree);     

        freeTree(tree);
        free(expanded);
        free(input);

    }

    tcsetpgrp(STDIN_FILENO, shell_pgid);     
    if (tcsetpgrp(STDIN_FILENO, parent_pgid) == -1) perror("tcsetpgrp‑restore");
    varsDestroy();
    return 0;

}
