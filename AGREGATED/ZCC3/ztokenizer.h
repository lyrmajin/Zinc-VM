#ifndef ZTOKENIZER_H
#define ZTOKENIZER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "zerr.h"
#include "utils.h"

typedef enum{
    TOKEN_UNDEF,
    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT,
    TOKEN_OPERATOR
}TOKENTYPE;

typedef struct{
    TOKENTYPE TYPE;
    char *tokenstr;
    uint16_t line, col;
}TOKEN;

typedef struct{
    TOKEN *tokens;
    uint16_t count;
}TOKENLIST;

extern TOKENLIST OnePassTokenList;
extern TOKENLIST SecondPassTokenList;

/*
bool isSingleOperator(char c);
bool isDoubleOperator(char str[3]);
bool isTripleOperator(char str[4]);
*/

void firstpasstokenize(FILE *file);

#endif