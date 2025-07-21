#define _GNU_SOURCE
#define MAX_EXPAND_DEPTH 256
#include <ctype.h>
#include <readline/history.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"
#include "variable.h"

const char* exprPosition;                     
long parseFactor(void);
long parseTerm(void);
long parseExpr(void);
void skipWhiteSpaces(void);

char* preprocessLine(const char* source)
{

    char* histExpression = expandHistory(source);
    char* varExpression = expandVarsTilde(histExpression); free(histExpression);
    char* braceExpression = expandBrace(varExpression); free(varExpression);
    char* cmdExpression = expandCommandSubst(braceExpression); free(braceExpression);   
    char* arithExp = expandArith(cmdExpression); free(cmdExpression);   

    return arithExp;                           

}

long parseFactor(void)
{

    skipWhiteSpaces();

    if (isalpha((unsigned char)*exprPosition) || *exprPosition == '_') 
    {

        const char* start = exprPosition;
        while (isalnum((unsigned char)*exprPosition) || *exprPosition == '_') exprPosition++;

        char name[64];
        size_t n = exprPosition - start;

        if (n >= sizeof(name)) n = sizeof(name)-1;
        memcpy(name, start, n); name[n] = '\0';

        const char* value = varGet(name);
        long v = value ? strtol(value, NULL, 10) : 0;   
        skipWhiteSpaces();

        return v;

    }  

    if (*exprPosition == '(') 
    {

        exprPosition++;                                
        long v = parseExpr();             
        skipWhiteSpaces();

        if (*exprPosition == ')') exprPosition++;                 

        return v;

    }

    if (*exprPosition == '+' || *exprPosition == '-') 
    {

        int sign = (*exprPosition == '-') ? -1 : 1;
        exprPosition++;

        return sign * parseFactor();

    }

    long v = strtol(exprPosition, (char**)&exprPosition, 10);
    skipWhiteSpaces();

    return v;

}

long parseTerm(void)
{

    long v = parseFactor();

    while(true)
    {

        if (*exprPosition == '*') { exprPosition++; v *= parseFactor(); }
        else if (*exprPosition == '/') { exprPosition++; long r=parseFactor(); v /= r; }
        else if (*exprPosition == '%') { exprPosition++; long r=parseFactor(); v %= r; }
        else break;

    }

    return v;

}

long parseExpr(void)
{

    long v = parseTerm();

    while(true) 
    {

        if (*exprPosition == '+') { exprPosition++; v += parseTerm(); }
        else if (*exprPosition == '-') { exprPosition++; v -= parseTerm(); }
        else break;

    }

    return v;

}

long evalExpr(const char* s)
{

    exprPosition = s;
    return parseExpr();

}

void skipWhiteSpaces(void)
{ 

    while (isspace(*exprPosition)) exprPosition++; 

}

char* expandHistory(const char* source)
{
    
    if (strchr(source, '!') == NULL) return strdup(source);

    size_t len = strlen(source) * 2 + 1;
    char* out = malloc(len), *o = out;
    const char* current = source;

    while (*current) 
    {

        if (*current == '!' && *(current+1)) 
        {

            if (*(current+1) == '!') 
            {

                HIST_ENTRY* he = history_get(history_length);
                if (he) o += sprintf(o, "%s", he->line);
                current += 2;

            } 
            else if (isdigit((unsigned char)*(current+1))) 
            {

                int n = atoi(current + 1);
                HIST_ENTRY *he = history_get(n);

                if (he) o += sprintf(o, "%s", he->line);
                while (isdigit((unsigned char)*++current));

            } 
            else
            {                                     

                const char* pat = ++current;
                size_t patlen = strcspn(pat, " \t");

                for (int i = history_length; i > 0; i--) 
                {

                    HIST_ENTRY* he = history_get(i);

                    if (he && strncmp(he->line, pat, patlen) == 0) 
                    {

                        o += sprintf(o, "%s", he->line);
                        break;

                    }

                }

                current += patlen;

            }

            continue;

        }

        *o++ = *current++;

    }

    *o = '\0';
    return out;

}

char* expandVarsTilde(const char *source)
{

    size_t len = strlen(source) * 2 + 1;
    char* out = malloc(len), *o = out;
    const char* current = source;

    while (*current) 
    {

        if (*current == '~' && (current == source || isspace((unsigned char)*(current-1)))) 
        {

            const char *home = getenv("HOME");
            if (home) { strcpy(o, home); o += strlen(home); }
            current++;
            continue;

        }

        if (*current == '$') 
        {

            current++;
            
            if (*current == '{') 
            {

                const char* start = ++current;
                while (*current && *current != '}') current++;

                size_t n = current - start;
                char var[128]; strncpy(var, start, n); var[n] = 0;
                const char *value = varGet(var);
                if (!value) value = getenv(var);

                if (value) { strcpy(o, value); o += strlen(value); }
                if (*current == '}') current++;

                continue;

            } 
            else if (isalnum((unsigned char)*current) || *current == '_') 
            {

                const char *start = current;
                while (isalnum((unsigned char)*current) || *current == '_') current++;

                size_t n = current - start;
                char var[128]; strncpy(var, start, n); var[n] = 0;
                const char *value = varGet(var);
                if (!value) value = getenv(var);

                if (value) { strcpy(o, value); o += strlen(value); }
                continue;

            }
            
            *o++ = '$';
            continue;

        }

        *o++ = *current++;

    }

    *o = '\0';
    return out;

}

static char* expandBraceDepth(const char* source, int depth)
{

    if (depth >= MAX_EXPAND_DEPTH) 
    {

        fprintf(stderr, "brace expansion: nesting too deep (>%d levels)\n", MAX_EXPAND_DEPTH);
        return strdup(source);          

    }

    const char* b = strchr(source, '{');
    if (!b) return strdup(source);

    const char* e = strchr(b, '}');
    if (!e) return strdup(source);

    const char* t = b;
    while (t > source && !isspace((unsigned char) * (t - 1))) t--;

    char linePrefix[256];
    char tokenPrefix[128];
    snprintf(linePrefix, sizeof linePrefix, "%.*s", (int)(t - source), source);
    snprintf(tokenPrefix, sizeof tokenPrefix, "%.*s", (int)(b - t), t);

    const char* after_e = e + 1;
    while (*after_e && !isspace((unsigned char) * after_e)) after_e++;

    char tokenSuffix[128];
    char lineSuffix[256];
    snprintf(tokenSuffix, sizeof tokenSuffix, "%.*s", (int)(after_e - (e + 1)), e + 1);
    strcpy(lineSuffix, after_e);

    char inside[256];
    snprintf(inside, sizeof inside, "%.*s", (int)(e - b - 1), b + 1);

    size_t outSize = strlen(source) * 8;
    char* out = calloc(outSize, 1);
    strcat(out, linePrefix);    
    int needSpace = 0;

    void add_variant(const char* value)
    {

        size_t need = strlen(tokenPrefix) + strlen(value) + strlen(tokenSuffix) + strlen(lineSuffix)+2;
        char* temp = malloc(need + 1);

        sprintf(temp, "%s%s%s%s%s", tokenPrefix, value, tokenSuffix, *lineSuffix ? " " : "", lineSuffix);

        char* recursion = expandBraceDepth(temp, depth + 1); 
        if (needSpace) strcat(out, " ");
        needSpace = 1;
        strcat(out, recursion);

        free(recursion);
        free(temp);

    }

    char* dots = strstr(inside, "..");

    if (dots) 
    {

        char left[64];
        char right[64];
        snprintf(left,  sizeof left,  "%.*s", (int)(dots - inside), inside);
        snprintf(right, sizeof right, "%s", dots + 2);

        if (isdigit((unsigned char)left[0])) 
        {

            int s = atoi(left), f = atoi(right), step = (s <= f) ? 1 : -1;
            char fmt[8]; sprintf(fmt, "%%0%dd", (int)strlen(left));

            for (int i = s; ; i += step) 
            {

                char num[32]; sprintf(num, fmt, i);
                add_variant(num);

                if (i == f) break;

            }

        } 
        else
        {

            char s = left[0], f = right[0], step = (s <= f) ? 1 : -1;

            for (char c = s; ; c += step) 
            {

                char ch[2] = { c, 0 };
                add_variant(ch);

                if (c == f) break;

            }

        }

    } 
    else 
    {         

        char* save = NULL;
        char* tok = strtok_r(inside, ",", &save);

        while (tok) { add_variant(tok); tok = strtok_r(NULL, ",", &save); }

    }

    return out;

}

char* expandBrace(const char* source)
{

    return expandBraceDepth(source, 0);

}

char* expandCommandSubst(const char* source)
{

    const char* current = source;
    
    while ((current = strstr(current, "$("))) 
    {

        if (*(current + 2) == '(') { current += 2; continue; }
        const char* start = current + 2;       

        int depth = 1;
        const char* q = start;

        while (*q && depth) 
        {

            if (*q == '(') depth++;
            else if (*q == ')') depth--;

            q++;

        }

        if (depth) break;               

        size_t lenOfCommand = (q - 1) - start;
        char command[512];
        snprintf(command, sizeof command, "%.*s", (int)lenOfCommand, start);

        FILE* filePath = popen(command, "r");
        if (!filePath) break;                  

        char buffer[1024] = {0};
        fread(buffer, 1, sizeof(buffer) - 1, filePath);
        pclose(filePath);

        size_t lenOfBuffer = strlen(buffer);
        if (lenOfBuffer && buffer[lenOfBuffer - 1] == '\n') buffer[lenOfBuffer - 1] = '\0';

        size_t newSize = strlen(source) - (q - current) + strlen(buffer) + 1;
        char* result = malloc(newSize);
        snprintf(result, newSize, "%.*s%s%s", (int)(current - source), source, buffer, q);                           

        char* final = expandCommandSubst(result);
        free(result);
        
        return final;

    }
    
    return strdup(source);

}

char* expandArith(const char* source)
{

    const char* current = strstr(source, "$((");
    if (!current) return strdup(source);

    const char* q = strstr(current + 3, "))");
    if (!q) return strdup(source);

    char expression[128];
    snprintf(expression, sizeof expression, "%.*s", (int)(q - current - 3), current + 3);
    long value = evalExpr(expression);

    size_t outSize = strlen(source) + 32;
    char* result = malloc(outSize);
    snprintf(result, outSize, "%.*s%ld%s", (int)(current - source), source, value, q + 2);

    return result;

}

char* expandWord(const char* source)
{

    size_t capacity = 32;
    size_t len = 0;
    char* result = malloc(capacity);
    if (!result) return NULL;

    while (*source) 
    {

        if (*source == '$' && isalpha((unsigned char)source[1])) 
        {
        
            const char* start = ++source;
            while (isalnum((unsigned char)*source) || *source == '_') source++;
            size_t newLen = source - start;

            char name[128];
            if (newLen >= sizeof(name)) newLen = sizeof(name)-1;
            memcpy(name, start, newLen);
            name[newLen] = '\0';

            const char* value = varGet(name);
            if (!value) value = "";

            size_t lenOfValue = strlen(value);
            while (len + lenOfValue + 1 > capacity) { capacity *= 2; result = realloc(result, capacity); }
            memcpy(result + len, value, lenOfValue);
            len += lenOfValue;

        } 
        else 
        {

            if (len + 2 > capacity) { capacity *= 2; result = realloc(result, capacity); }
            result[len++] = *source++;

        }

    }

    result[len] = '\0';
    return result;

}
