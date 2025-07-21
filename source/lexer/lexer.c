#define MAX_TOKEN_LEN 256
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <string.h>

#include "lexer.h"

static const char* source;          
static const char* exprPosition;          
static char textForLastToken[MAX_TOKEN_LEN];  

void lexerInit(const char* line)
{
    
    source = exprPosition = line; 
    textForLastToken[0] = '\0';

}

static void scanQuoted(char quote, char** out)
{

    exprPosition += 1;                      
    
    while (*exprPosition && *exprPosition != quote) 
    {
        
        if (quote == '"' && *exprPosition == '\\' && exprPosition[1]) *(*out)++ = *++exprPosition; 
        else *(*out)++ = *exprPosition;
            
        exprPosition += 1;

    }

    if (*exprPosition == quote) exprPosition += 1;  

}

static void skipWhiteSpaces(void)
{

    while (*exprPosition && isspace(*exprPosition)) exprPosition += 1; 

}

static char nextSymbol(void)
{

    return *exprPosition ? *exprPosition++ : '\0'; 

}

token_t lexerGet(void)
{

    skipWhiteSpaces();

    if (*exprPosition == '\0')
    {

        textForLastToken[0] = '\0';
        return TOK_EOF;


    }

    if (*exprPosition == '&' && *(exprPosition + 1) == '&') { exprPosition += 2; strcpy(textForLastToken, "&&"); return TOK_AND; }
    if (*exprPosition == '&') { nextSymbol(); strcpy(textForLastToken, "&"); return TOK_AMP; }
    if (*exprPosition == '|' && *(exprPosition + 1) == '|') { exprPosition += 2; strcpy(textForLastToken, "||"); return TOK_OR; }
    if (*exprPosition == ';') { nextSymbol(); strcpy(textForLastToken, ";");  return TOK_SEMI; }
    if (*exprPosition == '(') { nextSymbol(); strcpy(textForLastToken, "(");  return TOK_LPAR; }
    if (*exprPosition == ')') { nextSymbol(); strcpy(textForLastToken, ")");  return TOK_RPAR; }
    if (*exprPosition == '|') { nextSymbol(); strcpy(textForLastToken, "|");  return TOK_PIPE; }
    if (*exprPosition == '>' && *(exprPosition + 1) == '>') { exprPosition += 2; strcpy(textForLastToken, ">>"); return TOK_GTGT; }
    if (*exprPosition == '>') { nextSymbol(); strcpy(textForLastToken, ">");  return TOK_GT; }
    if (*exprPosition == '<' && *(exprPosition + 1) == '<') { exprPosition += 2; strcpy(textForLastToken, "<<"); return TOK_HEREDOC; }
    if (*exprPosition == '<') { nextSymbol(); strcpy(textForLastToken, "<"); return TOK_LT; }
    
    char *out = textForLastToken;

    while (*exprPosition) 
    {

        if (isspace(*exprPosition) || strchr("&|;()<>", *exprPosition)) break;
        
        if (*exprPosition == '\'' || *exprPosition == '"') 
        {

            scanQuoted(*exprPosition, &out);
            continue;

        }

        if (*exprPosition == '\\' && exprPosition[1]) ++exprPosition;
        *out++ = *exprPosition++;    
        
    }

    *out = '\0';
    return TOK_WORD;

}

token_t lexerPeek(void)
{

    const char* savedCurrent = exprPosition;
    char saved_lex[256]; strcpy(saved_lex, textForLastToken);

    token_t token = lexerGet();
    exprPosition = savedCurrent;
    strcpy(textForLastToken, saved_lex);

    return token;

}

const char* lexerText(void)
{ 

    return textForLastToken; 

}
