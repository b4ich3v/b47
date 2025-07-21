#ifndef EXPAND_H
#define EXPAND_H

char* preprocessLine(const char* source);
char* expandHistory(const char* source);
char* expandVarsTilde(const char* source);
char* expandBrace(const char* source);
char* expandCommandSubst(const char* source);
char* expandArith(const char* source);
char* expandWord(const char* source);

#endif 
