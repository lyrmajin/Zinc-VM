#ifndef FRONTEND_H
#define FRONTEND_H

// Tokenizer

typedef enum
{
    INMEDIATE_TOKEN,
    SEPARATOR_TOKEN
} TOKENTYPE;

typedef struct
{
    TOKENTYPE tokentype;
    char *tokenstr;
    uint32_t line, column;
} TOKEN;

typedef struct
{
    TOKEN *tokens;
    uint8_t tokensCount;
    uint8_t listSize;
} TOKENLIST;

extern bool DoDebug;
extern bool DoInternalsDebug;

extern uint32_t LINE, COLUMN;

extern size_t bufferSize;

int tokenize(FILE *INPUT, TOKENLIST *tokenlist);

// Parser

typedef enum
{
    ROOT_NODE,
    DIRECTIVE_NODE,
    TAG_NODE,
    OPCODE_NODE,
    OPERAND_NODE,
    INMEDIATE_NODE
} ASTNODETYPE;

typedef struct ASTNODE
{
    ASTNODETYPE nodetype;
    char *nodestr;
    struct ASTNODE *parentdnode;
    struct ASTNODE **childnode;
    uint32_t childCount;
    uint32_t listSize;
} ASTNODE;

int parse(TOKENLIST *tokenlist, ASTNODE *astnode);

#endif