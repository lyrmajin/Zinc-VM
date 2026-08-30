// GLobal dependences
#include "../dependences.h"

// Internal Dependences
#include <stdarg.h>

// ERRORS AND WARNINGS

void emiterr(const char *fmt, ...)
{
    va_list arguments;
    va_start(arguments, fmt);

    fprintf(INTERNALERR, "%s:%u:%u: ZASM: ERROR: ", INFILENAME, LINE, COLUMN);
    vfprintf(INTERNALERR, fmt, arguments);
    fprintf(INTERNALERR, "\n");

    fflush(INTERNALERR);

    ERR_COUNT++;

    va_end(arguments);
}

void emitwarn(const char *fmt, ...)
{
    va_list arguments;
    va_start(arguments, fmt);

    fprintf(INTERNALWARN, "%s:%u:%u: ZASM: WARNING: ", INFILENAME, LINE, COLUMN);
    vfprintf(INTERNALWARN, fmt, arguments);
    fprintf(INTERNALWARN, "\n");

    fflush(INTERNALWARN);

    WARN_COUNT++;

    va_end(arguments);
}

// DEBUGING

void emitdebug(const char *fmt, ...)
{
    va_list arguments;
    va_start(arguments, fmt);

    vfprintf(INTERNALDEBUG, fmt, arguments);
    // Not adding new line so can be more dynamically used.

    va_end(arguments);
}

// EMITION

void emit(uint8_t *BYTES, size_t SIZE)
{
    if (!OUTPUT)
    {
        emiterr("Unable to open file \"%s\" at write time", OUTFILENAME);
        if (!strcmp(OUTFILENAME, "-") || !strcmp(OUTFILENAME, "stdout") || (OUTFILENAME[0] == '-'))
        {
            emiterr("falling back to stdout");
            OUTPUT = stdout;
            if (!OUTPUT)
            {
                emiterr("falling back to stderr");
                OUTPUT = stderr;
                if (!OUTPUT){
                    emiterr("What the fuck is you enviorement; falling back to fallback");
                    OUTPUT = fopen("fallback", "w");
                    if (!OUTPUT){
                        emiterr("Are you sure you have libc installed? We couldnt do anything.");
                        exit(-1);
                    }
                }
            }
            goto justdoit;
        }
        exit(-1);
    }
justdoit:
    fwrite(BYTES, 1, SIZE, OUTPUT);
    if (DoInternalsDebug)
        emitdebug("Writted %u bytes into output file %s\n", SIZE, OUTFILENAME);

    if (DoDebug || DoInternalsDebug || DoDump)
    {
        for (uint64_t i = 0; i < SIZE; i++)
        {
            if (i % 16 == 0)
            {
                emitdebug("\n0x%04llX: ", (unsigned long long)i);
            }

            emitdebug("%02X ", BYTES[i]);
        }
        emitdebug("\n\n");
    }
}