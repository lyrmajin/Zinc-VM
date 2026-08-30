// GLobal dependences
#include "../dependences.h"

// Internal dependences
#include "frontend.h"
#include "../backend/utils.h"
#include <unistd.h>

const char *typesstr[6] = {"ROOT_NODE",
                           "DIRECTIVE_NODE",
                           "TAG_NODE",
                           "OPCODE_NODE",
                           "OPERAND_NODE",
                           "INMEDIATE_NODE"};

int allocateASTNode(ASTNODE *parentastnode, ASTNODE *childastnode)
{
    if (parentastnode->listSize > parentastnode->childCount)
    {
        parentastnode->childnode[parentastnode->childCount] = childastnode;
        parentastnode->childCount++;
    }
    else
    {
        ASTNODE **tmp = NULL;
        for (uint8_t i = 0; i < 6; i++)
        {
            if (DoInternalsDebug)
                emitdebug("Attempting to reallocate nodes,(Data Before) nodesCount %u, listSize %u\n", parentastnode->childCount, parentastnode->listSize);
            parentastnode->listSize += 10;                                                        // adding 10 elements to the array
            tmp = realloc(parentastnode->childnode, sizeof(ASTNODE *) * parentastnode->listSize); // resizing astnodes to be listSize
            if (!tmp)
            { // checking reallocation errors
                emiterr("Unable to reallocate Tokens");
                if (DoInternalsDebug)
                    emitdebug("Unable to reallocate Tokens,(Data Midprocess) tokensCount %u, listSize %u\n", parentastnode->childCount, parentastnode->listSize);
                parentastnode->listSize -= 10; // if fail, restore original parenastnode listSize
                sleep(10);
                if (i >= 5)
                {
                    printf("Memory allocation still failing. Keep trying for another minute? [y/N]: ");
                    int c = getchar();
                    if (c == 'n' || c == 'N')
                        exit(-1);
                    else
                        i = 0;
                }
            }
            else
                i = 6;
        }
        parentastnode->childnode = tmp;
        parentastnode->childnode[parentastnode->childCount] = childastnode;
        parentastnode->childCount++;
    }
    if (DoDebug)
        emitdebug("Emited ASTNode \"%s\" type \"%s\"\n", parentastnode->childnode[parentastnode->childCount - 1]->nodestr, typesstr[childastnode->nodetype]);
    if (DoInternalsDebug)
        emitdebug("ASTNODES updated successfully,(Data After) nodesCount %u, listSize %u\nNode parent %p and string \"%s\" and type %u\n", parentastnode->childCount, parentastnode->listSize, childastnode->parentdnode, parentastnode->childnode[parentastnode->childCount - 1]->nodestr, childastnode->nodetype);
    return 0;
}

int parse(TOKENLIST *tokenlist, ASTNODE *astnode)
{
    int64_t i = 0;
    ASTNODE *scopenode = astnode->parentdnode;
    if(!scopenode)
    {
        emiterr("Unable to open Scope node.");
    }
    while (i < tokenlist->tokensCount)
    {
        if (DoInternalsDebug)
            emitdebug("Token:%s\n", tokenlist->tokens[i].tokenstr);

        if (tokenlist->tokens[i].tokentype == SEPARATOR_TOKEN)
        {
            if (!strcmp(tokenlist->tokens[i].tokenstr, "."))
            {
                if ((i + 1) < tokenlist->tokensCount)
                {
                    if (tokenlist->tokens[i + 1].tokentype == INMEDIATE_TOKEN)
                    {
                        ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                        if (!tmp)
                        {
                            emiterr("Unable to allocate tmp astnode");
                            i++;
                            continue;
                        }
                        *tmp = (ASTNODE){DIRECTIVE_NODE, tokenlist->tokens[i + 1].tokenstr, scopenode, NULL, 0, 0};
                        allocateASTNode(scopenode, tmp);
                        scopenode = tmp;
                        i += 2;
                    }
                    else
                    {
                        emiterr("Undefined directive \"%s\"", tokenlist->tokens[i + 1].tokenstr);
                        i++;
                    }
                }
                else
                {
                    emiterr("No directive was found after '.'");
                    i++;
                }
            }
            else if (!strcmp(tokenlist->tokens[i].tokenstr, ","))
            {
                if ((i + 1) < tokenlist->tokensCount)
                {
                    if (tokenlist->tokens[i + 1].tokentype == INMEDIATE_TOKEN || (tokenlist->tokens[i + 1].tokentype == SEPARATOR_TOKEN && !strcmp(tokenlist->tokens[i + 1].tokenstr, "#")))
                    {
                        ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                        if (!tmp)
                        {
                            emiterr("Unable to allocate tmp astnode");
                            i++;
                            continue;
                        }
                        *tmp = (ASTNODE){OPERAND_NODE, tokenlist->tokens[i + 1].tokenstr, scopenode, NULL, 0, 0};
                        allocateASTNode(scopenode, tmp);
                        i += ((tokenlist->tokens[i + 1].tokentype == SEPARATOR_TOKEN && !strcmp(tokenlist->tokens[i + 1].tokenstr, "#"))) ? 1 : 2;
                    }
                    else
                    {
                        emiterr("Undefined operator \"%s\"", tokenlist->tokens[i + 1].tokenstr);
                        i++;
                    }
                }
                else
                {
                    emiterr("No operand was found after ','");
                    i++;
                }
            }
            else if (!strcmp(tokenlist->tokens[i].tokenstr, "#"))
            {
                if ((i + 1) < tokenlist->tokensCount)
                {
                    if (tokenlist->tokens[i + 1].tokentype == INMEDIATE_TOKEN)
                    {
                        ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                        if (!tmp)
                        {
                            emiterr("Unable to allocate tmp astnode");
                            i++;
                            continue;
                        }
                        *tmp = (ASTNODE){INMEDIATE_NODE, tokenlist->tokens[i + 1].tokenstr, scopenode, NULL, 0, 0};
                        allocateASTNode(scopenode, tmp);
                        i += 2;
                    }
                    else
                    {
                        emiterr("Undefined operator \"%s\"", tokenlist->tokens[i + 1].tokenstr);
                        i++;
                    }
                }
                else
                {
                    emiterr("No inmediate was found after '#'");
                    i++;
                }
            }
            else
            {
                emitwarn("Undefined separator behaviour");
                i++;
            }
        }
        else
        {
            if ((i + 1) < tokenlist->tokensCount)
            {
                if (tokenlist->tokens[i + 1].tokentype == SEPARATOR_TOKEN)
                {
                    if (!strcmp(tokenlist->tokens[i + 1].tokenstr, ":"))
                    {
                        ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                        if (!tmp)
                        {
                            emiterr("Unable to allocate tmp astnode");
                            i++;
                            continue;
                        }
                        *tmp = (ASTNODE){TAG_NODE, tokenlist->tokens[i].tokenstr, scopenode, NULL, 0, 0};
                        allocateASTNode(scopenode, tmp);
                        scopenode = tmp;
                        i += 2;
                    }
                    else if (!strcmp(tokenlist->tokens[i + 1].tokenstr, ","))
                    {
                        ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                        if (!tmp)
                        {
                            emiterr("Unable to allocate tmp astnode");
                            i++;
                            continue;
                        }
                        *tmp = (ASTNODE){OPERAND_NODE, tokenlist->tokens[i].tokenstr, scopenode, NULL, 0, 0};
                        allocateASTNode(scopenode, tmp);
                        i++;
                    }
                    else
                    {
                        if (DoInternalsDebug)
                            emitdebug("Unable to find any reference to \"%s\"\n", tokenlist->tokens[i].tokenstr);
                        i++;
                    }
                }
                else
                {
                    ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                    if (!tmp)
                    {
                        emiterr("Unable to allocate tmp astnode");
                        i++;
                        continue;
                    }
                    *tmp = (ASTNODE){OPCODE_NODE, tokenlist->tokens[i].tokenstr, scopenode, NULL, 0, 0};
                    allocateASTNode(scopenode, tmp);
                    i++;
                }
            }
            else
            {
                ASTNODE *tmp = (ASTNODE *)malloc(sizeof(ASTNODE));
                if (!tmp)
                {
                    emiterr("Unable to allocate tmp astnode");
                    i++;
                    continue;
                }
                *tmp = (ASTNODE){OPCODE_NODE, tokenlist->tokens[i].tokenstr, scopenode, NULL, 0, 0};
                allocateASTNode(scopenode, tmp);
                i++;
            }
        }
    }
    return 0;
}