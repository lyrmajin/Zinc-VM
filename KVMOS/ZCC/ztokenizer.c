#include "ztokenizer.h"

void alloctoken(TOKENLIST *tokenList, TOKEN token)
{
    tokenList->count++;
    tokenList->tokens = (TOKEN *)realloc(tokenList->tokens, tokenList->count * sizeof(TOKEN));
    if (!tokenList->tokens)
    {
        zerr("Internal error reallocating TokenList");
        exit(1);
    }
    tokenList->tokens[tokenList->count - 1].TYPE = token.TYPE;

    tokenList->tokens[tokenList->count - 1].tokenstr = malloc(strlen(token.tokenstr) + 1);
    if (tokenList->tokens[tokenList->count - 1].tokenstr == NULL)
    {
        zerr("Internla error allocating TokenStr");
        exit(1);
    }
    strcpy(tokenList->tokens[tokenList->count - 1].tokenstr, token.tokenstr);

    tokenList->tokens[tokenList->count - 1].line = token.line;
    tokenList->tokens[tokenList->count - 1].col = token.col;
    zdeb("[%s]\n", token.tokenstr, token.line, token.col);
}

void freetoken(TOKENLIST *tokenList)
{
    free(tokenList->tokens[tokenList->count - 1].tokenstr);
    tokenList->tokens[tokenList->count - 1].tokenstr = NULL;
    if (tokenList->count > 0)
        tokenList->count--;
    tokenList->tokens = (TOKEN *)realloc(tokenList->tokens, tokenList->count * sizeof(TOKEN));
}

void freealltoken(TOKENLIST *tokenList)
{
    while (tokenList->count > 0)
        freetoken(tokenList);
}

bool isSingleOperator(char c)
{
    switch (c)
    {
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case '+':
    case '-':
    case '*':
    case '/':
    case '<':
    case '>':
    case ';':
    case ':':
    case '%':
    case '~':
    case '!':
    case '&':
    case '^':
    case '|':
    case '=':
    case ',':
    case '\n':
        return true;
    default:
        return false;
    }
}

bool isDoubleOperator(char str[3])
{
    if (!strcmp(str, "=="))
        return true;
    else if (!strcmp(str, "+="))
        return true;
    else if (!strcmp(str, "-="))
        return true;
    else if (!strcmp(str, "*="))
        return true;
    else if (!strcmp(str, "/="))
        return true;
    else if (!strcmp(str, "|="))
        return true;
    else if (!strcmp(str, "&="))
        return true;
    else if (!strcmp(str, "^="))
        return true;
    else if (!strcmp(str, "!="))
        return true;
    else if (!strcmp(str, "~="))
        return true;
    else if (!strcmp(str, "%="))
        return true;
    else if (!strcmp(str, "//"))
        return true;
    else if (!strcmp(str, "/*"))
        return true;
    else
        return false;
}

bool isTripleOperator(char str[4])
{
    if (!strcmp(str, ">>="))
        return true;
    else if (!strcmp(str, "<<="))
        return true;
    else if (!strcmp(str, "..."))
        return true;
    else
        return false;
}

bool isConstantStartChar(char c)
{
    switch (c)
    {
    case '\'':
    case '"':
        return true;
    default:
        for (uint8_t i = 0; i < 10; i++)
            if (c == (i + '0'))
                return true;
        return false;
    }
}

bool isConstantEndChar(char c)
{
    switch (c)
    {
    case 'u':
    case 'U':
    case 'f':
    case 'F':
    case 'a':
    case '\\':
    case '\'':
    case '"':
        return true;
    default:
        for (uint8_t i = 0; i < 10; i++)
            if (c == (i + '0'))
                return true;
        return false;
    }
}

void secondpasstokenize(TOKENLIST *tokenList0, TOKENLIST *tokenList1)
{
    for (uint16_t i = 0; i < tokenList0->count; i++)
    {
        if (tokenList0->tokens[i].TYPE == TOKEN_UNDEF)
        {
            // zdeb("TOKEN:[%s]\n", tokenList0->tokens[i].tokenstr);

            uint16_t start = 0x100;
            uint16_t start2 = 0;
            uint16_t end = 0;
            uint16_t len = strlen(tokenList0->tokens[i].tokenstr);
            char *str = tokenList0->tokens[i].tokenstr;

            for (uint16_t j = 0; j < len; j++)
            {
                if (isConstantStartChar(str[j]) && ((j != 0) ? (str[j - 1] == ' ') : true) && start == 0x100)
                {
                    start = j;

                    if (start > start2)
                    {
                        char temp[256] = {0};
                        uint8_t k = 0;
                        for (uint8_t l = start2; l < start; l++)
                        {
                            if (str[l] != ' ')
                            {
                                temp[k++] = str[l];
                            }
                            else if (str[l] == ' ' && l < start && str[l + 1] != ' ' && k != 0)
                            {
                                temp[k] = '\0';

                                alloctoken(tokenList1, (TOKEN){
                                                           TOKEN_IDENTIFIER,
                                                           temp,
                                                           tokenList0->tokens[i].line,
                                                           start - tokenList0->tokens[i].col});

                                k = 0;
                            }
                            else
                            {
                                // zdeb("'%c','%c'(start:%u|l:%u|k:%u)\n", str[l], str[l + 1], start, l, k);
                            }
                        }
                        if (k > 0)
                        {
                            temp[k] = '\0';

                            alloctoken(tokenList1, (TOKEN){
                                                       TOKEN_IDENTIFIER,
                                                       temp,
                                                       tokenList0->tokens[i].line,
                                                       start - tokenList0->tokens[i].col});
                        }
                    }

                    end = start + 1;

                    if (str[j] == '"' || str[j] == '\'')
                    {
                        char quote = str[j];
                        while (end < len && str[end] != quote)
                            end++;
                        if (end < len)
                            end++;
                    }
                    else
                    {
                        while (end < len && isConstantEndChar(str[end]))
                            end++;
                    }

                    if (end > start)
                    {
                        char *temp = substr(str, start, end - start);
                        removeChar(temp, ' ');
                        alloctoken(tokenList1, (TOKEN){
                                                   TOKEN_CONSTANT,
                                                   temp,
                                                   tokenList0->tokens[i].line,
                                                   end - tokenList0->tokens[i].col});
                    }

                    start2 = end;
                    start = 0x100;
                    j = end - 1;
                }
            }

            if (start2 < len)
            {
                char temp[256] = {0};
                uint8_t k = 0;
                for (uint8_t l = start2; l < len; l++)
                {
                    if (str[l] != ' ')
                    {
                        temp[k++] = str[l];
                    }
                    else if (str[l] == ' ' && l < len && str[l + 1] != ' ' && k != 0)
                    {
                        temp[k] = '\0';

                        alloctoken(tokenList1, (TOKEN){
                                                   TOKEN_IDENTIFIER,
                                                   temp,
                                                   tokenList0->tokens[i].line,
                                                   start - tokenList0->tokens[i].col});

                        k = 0;
                    }
                    else
                    {
                        // zdeb("'%c','%c'(start2:%u|l:%u|k:%u)\n", str[l], str[l + 1], start2, l, k);
                    }
                }
                if (k > 0)
                {
                    temp[k] = '\0';

                    alloctoken(tokenList1, (TOKEN){
                                               TOKEN_IDENTIFIER,
                                               temp,
                                               tokenList0->tokens[i].line,
                                               start - tokenList0->tokens[i].col});
                }
            }
        }
        else
        {
            char *temp = tokenList0->tokens[i].tokenstr;
            removeChar(temp, ' ');
            alloctoken(tokenList1, (TOKEN){
                                       TOKEN_OPERATOR,
                                       temp,
                                       tokenList0->tokens[i].line,
                                       tokenList0->tokens[i].col});
        }
    }
    freealltoken(tokenList0);
}

void firstpasstokenize(FILE *file)
{
    char buffer[4] = {0};
    char line[256] = {0};
    uint16_t linenum = 0, colnum = 0;
    size_t n;
    long pos = ftell(file);

    while ((n = fread(buffer, sizeof(char), 3, file)) > 0)
    {
        buffer[n] = '\0';
        if (n < 3 && n != 0)
            clearerr(file);

        for (uint8_t i = 0; i < n; i++)
            if (buffer[i] == '\n')
            {
                linenum++;
                colnum = 0;
            }

        char triple[4] = {buffer[0], buffer[1], buffer[2], '\0'};
        char double_op[3] = {buffer[0], buffer[1], '\0'};

        if (n == 3 && isTripleOperator(triple))
        {
            if (line[0] != '\0')
            {
                alloctoken(&OnePassTokenList, (TOKEN){TOKEN_UNDEF, line, linenum, colnum});
                line[0] = '\0';
            }

            colnum += 3;
            alloctoken(&OnePassTokenList, (TOKEN){TOKEN_OPERATOR, triple, linenum, colnum});
        }
        else if (n >= 2 && isDoubleOperator(double_op))
        {
            if (!strcmp(double_op, "//"))
            {
                while ((n = fread(buffer, sizeof(char), 1, file)) == 1 && buffer[0] != '\n')
                    ;
                linenum++;
                colnum = 0;
                pos = ftell(file);
                continue;
            }
            else if (!strcmp(double_op, "/*"))
            {
                while ((n = fread(buffer, sizeof(char), 2, file)) == 2 && strcmp((char[]){buffer[0], buffer[1], '\0'}, "*/"))
                {
                    buffer[n] = '\0';
                    for (uint8_t i = 0; i < 2; i++)
                        if (buffer[i] == '\n')
                        {
                            linenum++;
                            colnum = 0;
                        }
                }
                pos = ftell(file);
                continue;
            }
            if (line[0] != '\0')
            {
                alloctoken(&OnePassTokenList, (TOKEN){TOKEN_UNDEF, line, linenum, colnum});
                line[0] = '\0';
            }

            colnum += 2;
            alloctoken(&OnePassTokenList, (TOKEN){TOKEN_OPERATOR, double_op, linenum, colnum});
            fseek(file, pos + 2, SEEK_SET);
        }
        else if (n >= 1 && isSingleOperator(buffer[0]))
        {
            if (line[0] != '\0')
            {
                alloctoken(&OnePassTokenList, (TOKEN){TOKEN_UNDEF, line, linenum, colnum});
                line[0] = '\0';
            }

            colnum++;
            alloctoken(&OnePassTokenList, (TOKEN){TOKEN_OPERATOR, (char[]){buffer[0], '\0'}, linenum, colnum});
            fseek(file, pos + 1, SEEK_SET);
        }
        else
        {
            size_t len = strlen(line);
            line[len] = buffer[0];
            line[len + 1] = '\0';
            colnum++;
            fseek(file, pos + 1, SEEK_SET);
        }
        pos = ftell(file);
    }
    zdeb("RETOKENIZING\n");
    secondpasstokenize(&OnePassTokenList, &SecondPassTokenList);
}