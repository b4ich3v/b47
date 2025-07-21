#ifndef PARSER_H
#define PARSER_H

#include <stddef.h> 
#include "lexer.h"

typedef enum 
{

    N_CMD,          
    N_PIPE,         
    N_AND,          
    N_OR,           
    N_SEQ,
    N_BG
    
} ntype_t;

typedef enum 
{

    R_IN, 
    R_OUT, 
    R_APPEND, 
    R_HEREDOC 

} redir_k;

typedef struct redir
{

    redir_k type;
    char* target;   
    char* data;   
    struct redir* next;

} redir_t;

typedef struct node 
{

    ntype_t type;
    struct node* left;
    struct node* right;
    char** arguments;
    redir_t* redir;

} node_t;

node_t* parseLine(const char* line);
void freeTree(node_t* node);

#endif 
