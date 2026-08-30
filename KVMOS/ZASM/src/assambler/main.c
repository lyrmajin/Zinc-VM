// GLobal dependences
#include "dependences.h"

// Internal Headers for Backend and Frontend. See reference in documentation
#include "frontend/frontend.h"
#include "backend/backend.h"

// Global variables

bool GLOBALERRORONALLOCATION = false;

// From dependences
FILE *INTERNALERR = NULL;
FILE *INTERNALWARN = NULL;
FILE *INTERNALDEBUG = NULL;

uint32_t ERR_COUNT = 0, WARN_COUNT = 0;

TOKENLIST tokenlist;
size_t bufferSize = (16 * 1024);

ASTNODE astnode;

size_t batchSize = (4 * 1024);

// For Compiler/Assambler

FILE *OUTPUT = NULL;
FILE *INPUT = NULL;

char *INFILENAME = "";
char *OUTFILENAME;

uint32_t LINE = 0, COLUMN = 0;

bool DoDump = false;
bool DoDebug = false;
bool DoInternalsDebug = false;

int main(int argc, char **argv)
{
    INTERNALERR = stderr;
    INTERNALWARN = stderr;
    INTERNALDEBUG = stdout;

    // check trough the arguments for any flag. else, tokenize.

    int i = 1;
    while (i < argc)
    {
        if (!strcmp(argv[i], "-o")) // Ouput file name
        {
            if ((i + 1) < argc)
            {
                if (OUTFILENAME)
                {
                    emitwarn("Output file already set to %s, ignoring new value %s", OUTFILENAME, argv[i + 1]);
                    i += 2;
                    continue;
                }
                OUTFILENAME = argv[i + 1];
                if(strcmp(OUTFILENAME, "-") && strcmp(OUTFILENAME, "stdout") && OUTFILENAME[0] != '-') OUTPUT = fopen(OUTFILENAME, "w");
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) // set the output file for err
        {
            print_help();
            exit(0);
        }
        else if (!strcmp(argv[i], "--buffer")) // set the output file for err
        {
            if ((i + 1) < argc)
            {
                char typeMode;
                bufferSize = 0;
                if (sscanf(argv[i + 1], "%zu%c", &bufferSize, &typeMode) < 1)
                {
                    emiterr("Error reading \"%s\"", argv[i + 1]);
                    return -1;
                }
                if (bufferSize <= 0)
                {
                    emiterr("Ilegal buffer size %zu", bufferSize);
                    return -1;
                }
                switch (typeMode)
                {
                case 'K':
                case 'k':
                    bufferSize *= 1024;
                    break;
                case 'M':
                case 'm':
                    bufferSize *= (1024 * 1024);
                    break;
                case 'G':
                case 'g':
                    bufferSize *= (1024 * 1024 * 1024);
                    break;
                }
                if (DoInternalsDebug)
                    emitdebug("Using bufferSize of %zu\n", bufferSize);
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--batch")) // set the output file for err
        {
            if ((i + 1) < argc)
            {
                char typeMode;
                batchSize = 0;
                if (sscanf(argv[i + 1], "%zu%c", &batchSize, &typeMode) < 1)
                {
                    emiterr("Error reading \"%s\"", argv[i + 1]);
                    return -1;
                }
                switch (typeMode)
                {
                case 'K':
                case 'k':
                    batchSize *= 1024;
                    break;
                case 'M':
                case 'm':
                    batchSize *= (1024 * 1024);
                    break;
                case 'G':
                case 'g':
                    batchSize *= (1024 * 1024 * 1024);
                    break;
                }
                if (batchSize <= 3)
                {
                    emiterr("Ilegal batch size %zu", batchSize);
                    return -1;
                }
                if (DoInternalsDebug)
                    emitdebug("Using batchSize of %zu\n", batchSize);
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--err")) // set the output file for err
        {
            if ((i + 1) < argc)
            {
                INTERNALERR = fopen(argv[i + 1], "w");
                if (!INTERNALERR)
                {
                    emiterr("Unable to open file %s", argv[i + 1]);
                    return -1;
                }
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--warn")) // set output file for warn
        {
            if ((i + 1) < argc)
            {
                INTERNALWARN = fopen(argv[i + 1], "w");
                if (!INTERNALWARN)
                {
                    emiterr("Unable to open file %s", argv[i + 1]);
                    return -1;
                }
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--debug-file")) // set output file for debug
        {
            if ((i + 1) < argc)
            {
                INTERNALDEBUG = fopen(argv[i + 1], "w");
                if (!INTERNALDEBUG)
                {
                    emiterr("Unable to open file %s", argv[i + 1]);
                    return -1;
                }
            }
            i += 2;
        }
        else if (!strcmp(argv[i], "--dump") || !strcmp(argv[i], "-dp")) // set debug mode
        {
            DoDump = true;
            i++;
        }
        else if (!strcmp(argv[i], "--debug") || !strcmp(argv[i], "-d")) // set debug mode
        {
            DoDebug = true;
            i++;
        }
        else if (!strcmp(argv[i], "--internal-debug") || !strcmp(argv[i], "-D")) // set internal debug mode
        {
            DoInternalsDebug = true;
            i++;
        }
        else // tokennize file.
        {
            INFILENAME = argv[i];
            INPUT = fopen(INFILENAME, "r");
            if (!INPUT)
            {
                emiterr("Unable to open file %s", INFILENAME);
                return -1;
            }
            if (DoDebug)
                emitdebug("Tokenizing file \"%s\"...\n", INFILENAME);
            tokenize(INPUT, &tokenlist);
            i++;
        }
    }

    if (DoDebug)
        emitdebug("\n\nParsing...\n\n");

    astnode = (ASTNODE){ROOT_NODE, NULL, &astnode, NULL, 0, 0};
    parse(&tokenlist, &astnode);

    if (DoDebug)
        emitdebug("\nGenerating Code...\n");

    if (!OUTFILENAME)
        OUTFILENAME = "a.out";

    if (!OUTPUT)
        if(strcmp(OUTFILENAME, "-") && strcmp(OUTFILENAME, "stdout") && OUTFILENAME[0] != '-') OUTPUT = fopen(OUTFILENAME, "w");

    if (!OUTPUT)
    {
        emiterr("Unable to open default output file %s", OUTFILENAME);
    }

    uint8_t *batch = (uint8_t *)malloc(sizeof(uint8_t) * batchSize);
    codegen(&astnode, batch, batchSize, NULL);

    free(batch);

    return 0;
}