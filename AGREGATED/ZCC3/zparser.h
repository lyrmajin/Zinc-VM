#ifndef ZPARSER_H
#define ZPARSER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "zerr.h"
#include "utils.h"
#include "ztokenizer.h"

typedef enum {
    NODE_ROOT,
    NODE_FUNC_DECL,
    NODE_VAR_DECL,
    NODE_RETURN,
    NODE_BLOCK
}NODETYPE;

typedef enum {
    VAR_INT,
    VAR_CHAR,
    VAR_FLOAT,
    VAR_AUTO
}DEFTYPE;

typedef struct ASTNODE{
    NODETYPE type;
    char *identifier;
    struct ASTNODE **ASTchildren;
    uint16_t childrenCount;
}ASTNODE;

typedef struct{
    NODETYPE node_type;
    const char *identifier;
    DEFTYPE def_type;
    char *values;
}ASTNODEVARDEF;

typedef struct{
    NODETYPE type;
    char *identifier;
    struct ASTNODE **ASTchildren;
    uint16_t childrenCount;
    DEFTYPE def_type;
    uint8_t argumentsCount;
}ASTNODEFUNDEF;

typedef struct{
    NODETYPE type;
    char *identifier;
    struct ASTNODE **ASTchildren;
    uint16_t childrenCount;
    struct ASTNODE *ASTparent;
    DEFTYPE def_type;
}ASTNODERET;

typedef struct{
    NODETYPE type;
    char *identifier;
    struct ASTNODE **ASTchildren;
    uint16_t childrenCount;
    struct ASTNODE *ASTparent;
}ASTNODEBLOCK;

extern ASTNODE astnode;

void parse(TOKENLIST *tokenList);

#endif