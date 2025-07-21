#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdbool.h>

typedef struct var 
{

    char* name;
    char* value;
    bool exported;   

} var_t;

typedef struct var_entry 
{

    char* name;
    char* value;
    bool exported;
    struct var_entry* next;

} var_entry;

void varsInit(void);
void varsDestroy(void);
const char* varGet(const char* name);
int varSet(const char* name, const char* value, bool exported); 
int varUnset(const char* name);   
char** varsBuildEnviron(void);   

#endif 
