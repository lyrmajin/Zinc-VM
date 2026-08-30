#include <stdio.h>
#include "ztokenizer.h"
#include "zparser.h"
#include "zerr.h"
#include "zcodegen.h"

TOKENLIST OnePassTokenList;
TOKENLIST SecondPassTokenList;

ASTNODE astnode = {NODE_ROOT, NULL, NULL, 0};

char output[256] = "out";

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        zerr("Not file input");
        return 1;
    }
    for (uint16_t i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i - 1], "-o")){
            strcpy(output, argv[i]);
            continue;
        }
        if(!strcmp(argv[i], "-o"))
        continue;
        FILE *file = fopen(argv[i], "rb");
        if (!file)
        {
            zerr("Unable to open file %s", argv[i]);
            return 1;
        }
        firstpasstokenize(file);
        fclose(file);
    }
    zdeb("PARSING\n");
    parse(&SecondPassTokenList);
    zdeb("GENERATING CODE\n");
    codegen(&astnode, fopen(output, "w"));

    return 0;
}