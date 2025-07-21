#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h> 
#include <fcntl.h> 

#include "parser.h"
#include "builtins.h"
#include "process.h"
#include "shell.h"
#include "variable.h"
#include "expand.h" 

static sigjmp_buf hdJmp;  

static void heredocInt(int signal)
{

    (void)signal;
    siglongjmp(hdJmp, 1);

}

void collectHeredocs(node_t* node)
{

    if (!node) return;

    if (node->type == N_CMD && node->redir) 
    {

        for (redir_t* r = node->redir; r; r = r->next) 
        {

            if (r->type != R_HEREDOC) continue;

            struct sigaction old_sa, sa = { .sa_handler = heredocInt };

            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, &old_sa);

            if (sigsetjmp(hdJmp, 1) != 0) 
            {
                
                printf("\n");                      
                sigaction(SIGINT, &old_sa, NULL);
                r->data = strdup("");               
                continue;                           

            }

            size_t capacity = 0;
            size_t len = 0;
            size_t nread = 0;
            char* buffer = NULL;
            char* line = NULL;

            printf("> ");

            while (getline(&line, &nread, stdin) != -1) 
            {

                if (strcmp(line, r->target) == 0 || (strlen(line)-1 == strlen(r->target) &&
                     strncmp(line, r->target, strlen(r->target)) == 0))
                    break;

                if (len + strlen(line) + 1 > capacity) 
                {

                    capacity = capacity ? capacity * 2 : 256;
                    buffer = realloc(buffer, capacity);

                    if (!buffer) { perror("realloc"); break; }

                }

                strcpy(buffer + len, line);
                len += strlen(line);
                printf("> ");

            }

            free(line);
            sigaction(SIGINT, &old_sa, NULL);       
            r->data = buffer ? buffer : strdup("");

        }

    }

    collectHeredocs(node->left);
    collectHeredocs(node->right);

}

static int setupRedir(const redir_t* r)
{
    
    int fd = -1;

    switch (r->type) 
    {

    case R_IN:
    {

        fd = open(r->target, O_RDONLY);
        break;

    }
    case R_OUT:
    {

        fd = open(r->target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        break;

    }
    case R_APPEND:
    {

        fd = open(r->target, O_WRONLY | O_CREAT | O_APPEND, 0644);
        break;

    }
    case R_HEREDOC:
    {

        int sharedData[2]; pipe(sharedData);
        write(sharedData[1], r->data, strlen(r->data));
        close(sharedData[1]);
        fd = sharedData[0];
        break;

    }

    }

    return fd;

}

static int execSimple(node_t* command)
{

    char** argv = command->arguments;
    if (!argv || !argv[0] || argv[0][0] == '\0') return 0;

    for (int i = 0; argv[i]; ++i) 
    {

        char* exp = expandWord(argv[i]);
        free(argv[i]);
        argv[i] = exp;

    }

    if (executeBuiltin(argv)) return 0;
        
    pid_t pid = fork();

    if (pid == 0) 
    {                          
    
        if (getpgrp() == shell_pgid) setpgid(0, 0);
            
        signal(SIGINT,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        for (redir_t* r = command->redir; r; r = r->next) 
        {

            int fd = setupRedir(r);
            if (fd == -1) { perror(r->target); _exit(1); }

            if (r->type == R_IN || r->type == R_HEREDOC) dup2(fd, STDIN_FILENO);
            else dup2(fd, STDOUT_FILENO);
                
            close(fd);

        }

        char** envp = varsBuildEnviron();     
        execvpe(argv[0], argv, envp);           
        perror(argv[0]);                       
        for (char **exprPosition = envp; *exprPosition; ++exprPosition) free(*exprPosition);
        free(envp);
        exit(127);

    }

    pid_t mygrp = getpgrp();
    if (mygrp == shell_pgid) 
    {
        
        setpgid(pid, pid);              
        tcsetpgrp(STDIN_FILENO, pid);   
        
    }         
    else setpgid(pid, mygrp);   
         
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;

}

int run(node_t* node)
{

    switch (node->type) 
    {
    
    case N_CMD:
    {

        return execSimple(node);

    }
    case N_PIPE: 
    {

        int sharedData[2];
        pipe(sharedData);

        pid_t pgid = 0;
        pid_t left = fork();

        if (left == 0) 
        {      

            setpgid(0, 0);                
            pgid = getpid();

            signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            dup2(sharedData[1], STDOUT_FILENO);
            close(sharedData[0]); close(sharedData[1]);
            exit(run(node->left));

        }   

        setpgid(left, left);             
        pgid = left;
        pid_t right = fork();

        if (right == 0) 
        {    

            setpgid(0, pgid);

            signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            dup2(sharedData[0], STDIN_FILENO);
            close(sharedData[0]); close(sharedData[1]);
            exit(run(node->right));

        }

        setpgid(right, pgid);
        close(sharedData[0]); close(sharedData[1]);
        tcsetpgrp(STDIN_FILENO, pgid);    

        int status;
   
        while (waitpid(-pgid, &status, WUNTRACED) > 0)
        {

            if (WIFSTOPPED(status))  break;     
               
        }

        tcsetpgrp(STDIN_FILENO, shell_pgid);   
        return (WIFEXITED(status) ? WEXITSTATUS(status) : 1);

    }
    case N_BG:
    {

        pid_t pid = fork();
        if (pid == 0) 
        {

            setpgid(0, 0);              
            exit(run(node->left));     

        }

        setpgid(pid, pid);              
        const char* command = "<bg>";

        if (node->left && node->left->type == N_CMD && node->left->arguments[0]) command = node->left->arguments[0];

        addProcess(pid, command);           
        printf("[background] %d\n", pid);
        return 0;

    }
    case N_AND:
    {

        return (run(node->left) == 0) ? run(node->right) : 1;

    }
    case N_OR:
    {

        return (run(node->left) != 0) ? run(node->right) : 0;

    }
    case N_SEQ:
    {

        run(node->left);
        return run(node->right);

    }

    }

    return 1;                              

}
