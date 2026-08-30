#include "zcodegen.h"

void codegen(ASTNODE *astnode, FILE *filestream)
{
    // uint16_t VheapCount = 0;
    // uint16_t VProgramCount = 0;
    ASTNODE *scopenode = astnode;
    for (uint16_t i = 0; i < scopenode->childrenCount; i++)
    {
        // zdeb("%p, i:%u, type:%u\n", scopenode, i, ((ASTNODE *)(scopenode->ASTchildren[i]))->type);
        if (((ASTNODE *)(scopenode->ASTchildren[i]))->type == NODE_FUNC_DECL)
        {
            zemit(filestream, "%s:\n", scopenode->ASTchildren[i]->identifier);
            codegen(scopenode->ASTchildren[i], filestream);
        }
        if (((ASTNODE *)(scopenode->ASTchildren[i]))->type == NODE_BLOCK)
        {
            codegen(scopenode->ASTchildren[i], filestream);
        }
        else if (((ASTNODE *)(scopenode->ASTchildren[i]))->type == NODE_RETURN)
        {

            codegen(scopenode->ASTchildren[i], filestream);
            zemit(filestream, "POP R3\n");
            zemit(filestream, "RET\n");
        }
        else if (((ASTNODE *)(scopenode->ASTchildren[i]))->type == NODE_VAR_DECL)
        {
            if (((ASTNODEVARDEF *)(scopenode->ASTchildren[i]))->def_type == VAR_AUTO)
            {
                zemit(filestream, "MVI R0, #0x%X\n", (isNumber(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values)) ? (unsigned int)strtol(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values, NULL, 10) : 0);
                zemit(filestream, "PUSH R0\n");
            }
            else if (((ASTNODEVARDEF *)(scopenode->ASTchildren[i]))->def_type == VAR_INT)
            {
                if (((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values != NULL)
                {
                    if (isNumber(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values))
                    {
                        zemit(filestream, "MVI R0, #0x%X\n", (unsigned int)strtol(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values, NULL, 10));
                        zemit(filestream, "PUSH R0\n");
                    }
                    else
                    {
                        for (uint8_t o = 0; o < strlen(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values); o++)
                        {
                            zemit(filestream, "MVI R0, #'%c'\n", ((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values[o]);
                            zemit(filestream, "PUSH R0\n");
                        }
                    }
                }
            }
            else if (((ASTNODEVARDEF *)(scopenode->ASTchildren[i]))->def_type == VAR_CHAR)
            {
                if (((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values != NULL)
                    for (uint8_t o = 0; o < strlen(((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values); o++)
                    {
                        zemit(filestream, "MVI R0, #'%c'\n", ((ASTNODEVARDEF *)scopenode->ASTchildren[i])->values[o]);
                        zemit(filestream, "PUSH R0\n");
                    }
            }
        }
    }
}