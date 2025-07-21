#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>  
      
#include "variable.h"

static var_entry* head = NULL;

void varsInit(void)
{

    head = NULL;
    extern char** environ;

    for (char** current = environ; *current; ++current) 
    {

        char* eq = strchr(*current, '=');
        if (!eq) continue;

        char tmp[eq - *current + 1];
        memcpy(tmp, *current, eq - *current);
        tmp[eq - *current] = '\0';

        if (varSet(tmp, eq + 1, true) != 0) 
        {

            perror("varsInit");
            varsDestroy();          
            return;
        }

    }

}

void varsDestroy(void)
{
    var_entry* current = head;

    while (current) 
    {

        var_entry *nxt = current->next;

        free(current->name);
        free(current->value);
        free(current);

        current = nxt;

    }

    head = NULL;

}

static var_entry* findEntry(const char* name)
{

    for (var_entry* entry = head; entry; entry = entry->next)
    {

        if (strcmp(entry->name, name) == 0) return entry;

    }

    return NULL;

}

const char* varGet(const char* name)
{

    var_entry* entry = findEntry(name);
    return entry ? entry->value : NULL;

}

int varSet(const char* name, const char* value, bool exported)
{

    if (!name || !*name || strchr(name, '=')) return -1;
    var_entry* entry = findEntry(name);

    if (entry) 
    {

        char* newValue = strdup(value ? value : "");
        if (!newValue) return -1;

        free(entry->value);
        entry->value = newValue;
        if (exported) entry->exported = true;

        return 0;

    }

    var_entry* newEntry = malloc(sizeof(*newEntry));
    if (!newEntry) return -1;

    newEntry->name = strdup(name);
    if (!newEntry->name) { free(newEntry); return -1; }
    newEntry->value = strdup(value ? value : "");

    if (!newEntry->value) 
    {

        free(newEntry->name);
        free(newEntry);
        return -1;

    }

    newEntry->exported = exported;
    newEntry->next = head;
    head = newEntry;

    return 0;

}

int varUnset(const char* name)
{

    var_entry** linkPtr = &head;

    while (*linkPtr) 
    {

        var_entry *entry = *linkPtr;

        if (strcmp(entry->name, name) == 0) 
        {

            *linkPtr = entry->next;
            free(entry->name);
            free(entry->value);
            free(entry);

            return 0;
            
        }

        linkPtr = &(*linkPtr)->next;

    }

    return -1;   

}

char** varsBuildEnviron(void)
{
    
    size_t counter = 0;

    for (var_entry *entry = head; entry; entry = entry->next)
    {

        if (entry->exported) counter += 1;

    }

    char** newEnv = malloc((counter + 1) * sizeof(char*));
    if (!newEnv) return NULL;

    size_t index = 0;

    for (var_entry* entry = head; entry; entry = entry->next) 
    {
        
        if (!entry->exported) continue;
        size_t len = strlen(entry->name) + 1 + strlen(entry->value) + 1;
        newEnv[index] = malloc(len);

        if (!newEnv[index]) 
        {

            while (index) free(newEnv[--index]);
            free(newEnv);
            return NULL;

        }

        snprintf(newEnv[index], len, "%s=%s", entry->name, entry->value);
        index += 1;

    }

    newEnv[index] = NULL;
    return newEnv;

}
