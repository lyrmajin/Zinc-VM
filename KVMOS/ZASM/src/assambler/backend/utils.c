// GLobal dependences
#include "../dependences.h"

// Internal dependences
#include "utils.h"

bool isSeparatorChar(char c)
{
    switch (c)
    {
    case ' ':
    case ',':
    case '.':
    case '#':
    case ':':
        return true;
    default:
        return false;
    }
}

int emittoken(char *line, size_t *j, TOKENLIST *tokenlist, TOKENTYPE tokentype, int (*allocateToken)(TOKENLIST*, TOKEN))
{
    if (line[0] != '\0')
    {                                 // sanity check to about creating empty tokens
        char *newline = strdup(line); // allocate and copy the string
        if (!newline)
        {
            emiterr("Unable to allocate memory for token str");
            if (DoInternalsDebug)
                emitdebug("Failed to allocate string \"%s\", skipping Token string\n", line);
            COLUMN++;
            *j = 0;
            line[*j] = '\0';
            return -1;
        }
        if (DoInternalsDebug)
            emitdebug("sucessfully allocated string \"%s\"\n", line);
        allocateToken(tokenlist, (TOKEN){tokentype, newline, LINE, COLUMN});
    }
    COLUMN++;
    *j = 0;
    line[*j] = '\0';
    return 0;
}

char *strtolower(char *str){
    char *tmp = (char*)malloc(sizeof(char) * strlen(str));
    for(uint64_t i = 0; i < strlen(str); i++){
        tmp[i] = tolower(str[i]);
    }
    return tmp;
}

void print_help(void) {
    printf("Usage: nzasm [options] <inputfile>\n\n");
    printf("Options:\n");
    printf("  -o <file>            Set output file name\n");
    printf("  --buffer <N[K|M|G]>  Set buffer size\n");
    printf("  --batch <N[K|M|G]>   Set batch size (min 4)\n");
    printf("  --err <file>         Redirect internal error log\n");
    printf("  --warn <file>        Redirect warnings log\n");
    printf("  --debug-file <file>  Redirect debug output\n");
    printf("  -d, --debug          Enable debug mode\n");
    printf("  -dp, --dump          Enable dump on flush\n");
    printf("  -D, --internal-debug Enable internal debug mode\n");
    printf("  --help, -h           Show this help message\n");
}
