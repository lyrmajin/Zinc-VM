#include "zparser.h"

uint8_t typecount = 3;

const char *type(uint8_t i)
{
    static const char *types[] = {"int", "char", "float"};
    if (i < typecount)
        return types[i];
    else
        zerr("Undefined identifier");
    return NULL;
}

void allocastnode(ASTNODE *parentastnode, ASTNODE *childastnode)
{
    parentastnode->childrenCount++;
    parentastnode->ASTchildren = (ASTNODE **)realloc(parentastnode->ASTchildren, parentastnode->childrenCount * sizeof(ASTNODE *));

    if (!parentastnode->ASTchildren)
    {
        zerr("Internal error reallocating AST nodes");
        exit(1);
    }

    parentastnode->ASTchildren[parentastnode->childrenCount - 1] = childastnode;
}

void parse(TOKENLIST *tokenList)
{

    ASTNODE *scopenode = &astnode;

    uint8_t i = 0;
    while (i < tokenList->count)
    {
        if (tokenList->tokens[i].TYPE == TOKEN_IDENTIFIER)
        {
            for (uint8_t j = 0; j < typecount; j++)
            {
                if (!strcmp(tokenList->tokens[i].tokenstr, type(j)))
                {
                    if ((i + 2) < tokenList->count)
                    {
                        if ((!strcmp(tokenList->tokens[i + 2].tokenstr, "=") || !strcmp(tokenList->tokens[i + 2].tokenstr, ";") || !strcmp(tokenList->tokens[i + 2].tokenstr, ",") || !strcmp(tokenList->tokens[i + 2].tokenstr, ")")) && tokenList->tokens[i + 1].TYPE == TOKEN_IDENTIFIER)
                        {
                            ASTNODEVARDEF *var_node = (ASTNODEVARDEF *)malloc(sizeof(ASTNODEVARDEF));
                            zdeb("Parent:%p\n", scopenode);
                            if ((i + 4) < tokenList->count)
                            {
                                if (!strcmp(tokenList->tokens[i + 2].tokenstr, "=") && (!strcmp(tokenList->tokens[i + 4].tokenstr, ";") || !strcmp(tokenList->tokens[i + 4].tokenstr, ",") || !strcmp(tokenList->tokens[i + 4].tokenstr, ")")))
                                {
                                    *var_node = (ASTNODEVARDEF){NODE_VAR_DECL, tokenList->tokens[i + 1].tokenstr, j, tokenList->tokens[i + 3].tokenstr};
                                    zdeb("%s %s %s %s%s\n", tokenList->tokens[i].tokenstr, tokenList->tokens[i + 1].tokenstr,
                                         tokenList->tokens[i + 2].tokenstr, tokenList->tokens[i + 3].tokenstr, tokenList->tokens[i + 4].tokenstr);
                                    i += 5;

                                    allocastnode(scopenode, (ASTNODE *)var_node);
                                    goto end_var_node; // skip the fallback
                                }
                            }

                            *var_node = (ASTNODEVARDEF){NODE_VAR_DECL, tokenList->tokens[i + 1].tokenstr, j, NULL};
                            allocastnode(scopenode, (ASTNODE *)var_node);
                            zdeb("%s %s%s\n", tokenList->tokens[i].tokenstr, tokenList->tokens[i + 1].tokenstr, tokenList->tokens[i + 2].tokenstr);
                            i += 3;

                        end_var_node:
                            if (scopenode->type == NODE_FUNC_DECL)
                                ((ASTNODEFUNDEF *)scopenode)->argumentsCount++;
                            break;
                        }
                        else if (!strcmp(tokenList->tokens[i + 2].tokenstr, "(") && tokenList->tokens[i + 1].TYPE == TOKEN_IDENTIFIER)
                        {
                            ASTNODEFUNDEF *var_node = (ASTNODEFUNDEF *)malloc(sizeof(ASTNODEFUNDEF));
                            *var_node = (ASTNODEFUNDEF){NODE_FUNC_DECL, tokenList->tokens[i + 1].tokenstr, NULL, 0, j, 0};
                            allocastnode(scopenode, (ASTNODE *)var_node);
                            zdeb("Parent:%p\n", scopenode);
                            scopenode = (ASTNODE *)var_node;
                            zdeb("%s %s%s\n", tokenList->tokens[i].tokenstr, tokenList->tokens[i + 1].tokenstr, tokenList->tokens[i + 2].tokenstr);
                            i += 3;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    else
                    {
                        zerr("Incorrect Definition");
                        i++;
                    }
                }
                else if (j == (typecount - 1))
                {
                    if (!strcmp(tokenList->tokens[i].tokenstr, "return"))
                    {
                        if (scopenode->type == NODE_BLOCK)
                        {
                            zdeb("Parent:%p\n", scopenode);
                            ASTNODERET *var_node = (ASTNODERET *)malloc(sizeof(ASTNODERET));
                            *var_node = (ASTNODERET){NODE_RETURN, NULL, NULL, 0, (ASTNODE *)(((ASTNODEBLOCK *)scopenode)->ASTparent), ((ASTNODEFUNDEF *)((ASTNODEBLOCK *)scopenode)->ASTparent)->def_type};
                            allocastnode(scopenode, (ASTNODE *)var_node);
                            zdeb("Parent:%p\n", scopenode);
                            scopenode = (ASTNODE *)var_node;
                            zdeb("%s\n", tokenList->tokens[i].tokenstr);
                        }
                        else
                            zerr("out of scope return");
                        i += 1;
                    }
                    else
                    {
                        zerr("Undefined type '%s'", tokenList->tokens[i].tokenstr);
                        i++;
                    }
                    continue;
                }
            }
        }
        else if (tokenList->tokens[i].TYPE == TOKEN_CONSTANT)
        {
            if ((!strcmp(tokenList->tokens[i + 1].tokenstr, ",") || !strcmp(tokenList->tokens[i + 1].tokenstr, ";") || !strcmp(tokenList->tokens[i + 1].tokenstr, ")") || !strcmp(tokenList->tokens[i + 1].tokenstr, "}")))
            {
                ASTNODEVARDEF *var_node = (ASTNODEVARDEF *)malloc(sizeof(ASTNODEVARDEF));
                zdeb("Parent:%p\n", scopenode);
                *var_node = (ASTNODEVARDEF){NODE_VAR_DECL, tokenList->tokens[i].tokenstr, VAR_AUTO, tokenList->tokens[i].tokenstr};
                zdeb("%s%s\n", tokenList->tokens[i].tokenstr, tokenList->tokens[i + 1].tokenstr);
                i += 2;

                allocastnode(scopenode, (ASTNODE *)var_node);
            }
            else
                i++;
        }
        else
        {
            zdeb("Parent:%p\n", scopenode);
            if (strcmp(tokenList->tokens[i].tokenstr, "\n"))
                zdeb("%s\n", tokenList->tokens[i].tokenstr);
            else
                zdeb("\\n\n");

            if (!strcmp(tokenList->tokens[i].tokenstr, "{"))
            {
                ASTNODEBLOCK *var_node = (ASTNODEBLOCK *)malloc(sizeof(ASTNODEBLOCK));
                *var_node = (ASTNODEBLOCK){NODE_BLOCK, NULL, NULL, 0, scopenode};
                allocastnode(scopenode, (ASTNODE *)var_node);
                scopenode = (ASTNODE *)var_node;
                i += 1;
            }
            if (!strcmp(tokenList->tokens[i].tokenstr, "}"))
            {
                scopenode = ((ASTNODEBLOCK*)scopenode)->ASTparent;
                zdeb("NewParent:%p\n", scopenode);
                i += 1;
            }
            else
            {
                i++;
            }
        }
    }
}