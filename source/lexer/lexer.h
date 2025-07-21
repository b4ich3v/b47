#ifndef LEXER_H
#define LEXER_H

typedef enum 
{

    TOK_WORD,           
    TOK_PIPE,           
    TOK_GT, 
    TOK_LT,     
    TOK_AND,            
    TOK_OR,             
    TOK_SEMI,           
    TOK_LPAR, 
    TOK_RPAR, 
    TOK_AMP,  
    TOK_HEREDOC,        
    TOK_GTGT,  
    TOK_EOF

} token_t;

void lexerInit(const char* line);
token_t lexerPeek(void);            
token_t lexerGet(void);             
const char* lexerText(void);        

#endif 

