#define _POSIX_C_SOURCE 200809L 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "variable.h"

static bool looksLikeAssignment(const char* word)
{

    if (!isalpha((unsigned char)word[0]) && word[0] != '_') return false;

    const char* equalPosition = strchr(word, '=');

    if (!equalPosition) return false;
    if (equalPosition == word || equalPosition[1] == '\0') return false;
    
    for (const char* current = word; current < equalPosition; current++)
    {

        if (!isalnum((unsigned char)*current) && *current != '_') return false;

    }    

    return true;

}

static node_t* newNode(ntype_t nodeType, node_t* left, node_t* right)
{

    node_t* node = malloc(sizeof *node);
    node->type = nodeType; node->left = left; node->right = right; node->arguments = NULL;

    return node;

}

static void expect(token_t want)
{

    token_t token = lexerGet();

    if (token != want) 
    {

        fprintf(stderr, "syntax error, expected %d got %d\node", want, token);
        exit(1);

    }

}

static node_t* parseSimple(void)
{

    token_t token = lexerPeek();

    if (token != TOK_WORD) 
    {

        fprintf(stderr, "syntax error: expected command\node");
        exit(1);

    }

    size_t capacity = 4;
    size_t countOfArguments = 0;
    char** arguments = malloc(capacity * sizeof(char*));

    while ((token = lexerPeek()) == TOK_WORD)
    {

        lexerGet();
        const char* raw = lexerText();

        if (looksLikeAssignment(raw)) 
        {

            char* equalPosition = strchr(raw, '=');
            *equalPosition = '\0';
            varSet(raw, equalPosition + 1, false);

            continue;

        }

        if (countOfArguments == capacity) 
        {

            capacity *= 2;
            arguments = realloc(arguments, capacity * sizeof(char*));

        }

        arguments[countOfArguments++] = strdup(raw);

    }

    arguments[countOfArguments] = NULL;
    node_t* node = newNode(N_CMD, NULL, NULL);
    node->arguments = arguments;

    redir_t* redirHead = NULL;
    redir_t** redirTail = &redirHead;

    while(true)
    {

        token_t token = lexerPeek();
        if (token != TOK_LT && token != TOK_GT && token != TOK_GTGT && token != TOK_HEREDOC) break;
        lexerGet();                                   

        if (lexerGet() != TOK_WORD) 
        {

            fprintf(stderr, "redir: expected word after redirection\n");
            exit(1);

        }

        char* word = strdup(lexerText());
        redir_t* redir = calloc(1, sizeof * redir);

        if (token == TOK_LT) redir->type = R_IN;
        else if (token == TOK_GT) redir->type = R_OUT;
        else if (token == TOK_GTGT) redir->type = R_APPEND;
        else redir->type = R_HEREDOC;   

        redir->target = word;
        *redirTail = redir; 
        redirTail = &redir->next;

    }

    node->redir = redirHead;
    return node;

}

static node_t* parseList(void);   

static node_t* parseSimpleOrGroup(void)
{

    if (lexerPeek() == TOK_LPAR) 
    {

        lexerGet();                      
        node_t* subtree = parseList();
        expect(TOK_RPAR);

        return subtree;

    }

    return parseSimple();

}

static node_t* parsePipeline(void)
{

    node_t* node = parseSimpleOrGroup();

    while (lexerPeek() == TOK_PIPE) 
    {

        lexerGet();
        node_t* right = parseSimpleOrGroup();
        node = newNode(N_PIPE, node, right);

    }

    return node;

}

static node_t* parseAndor(void)
{

    node_t* node = parsePipeline();

    while (lexerPeek() == TOK_AND || lexerPeek() == TOK_OR) 
    {

        token_t operator = lexerGet();
        node_t* right = parsePipeline();
        node = newNode(operator == TOK_AND ? N_AND : N_OR, node, right);

    }

    return node;

}

static node_t* parseList(void)
{

    node_t* node = parseAndor();

    if (lexerPeek() == TOK_AMP) 
    {

        lexerGet();                     
        node = newNode(N_BG, node, NULL);     

    }

    while (lexerPeek() == TOK_SEMI) 
    {

        lexerGet();
        node_t* right = parseAndor();
        node = newNode(N_SEQ, node, right);

    }

    return node;

}

node_t* parseLine(const char* line)
{

    lexerInit(line);
    node_t* tree = parseList();

    if (lexerGet() != TOK_EOF) 
    {

        fprintf(stderr, "syntax error: trailing tokens\node");
        exit(1);

    }

    return tree;

}

void freeTree(node_t* node)
{

    if (!node) return;

    if (node->type == N_CMD) 
    {

        if (node->arguments) 
        {

            for (char** currentArgument = node->arguments; *currentArgument; currentArgument++) free(*currentArgument);
            free(node->arguments);

        }

        redir_t* redir = node->redir;

        while (redir) 
        {

            redir_t* next = redir->next;
            free(redir->target);
            free(redir->data);   
            free(redir);
            redir = next;
            
        }

    } 
    else
    {

        freeTree(node->left);
        freeTree(node->right);

    }

    free(node);

}


