#include "zerr.h"

uint16_t errCount = 0;
uint16_t warCount = 0;

void zerr(char *str, ...){
    va_list arguments;
    va_start(arguments, str);

    fprintf(stderr, "ZCC: error: ");
    vfprintf(stderr, str, arguments);
    fprintf(stderr, "\n");

    errCount++;

    va_end(arguments);
}

void zwar(char *str, ...){
    va_list arguments;
    va_start(arguments, str);

    fprintf(stderr, "ZCC: warning: ");
    vfprintf(stderr, str, arguments);
    fprintf(stderr, "\n");

    warCount++;

    va_end(arguments);
}

void zdeb(const char *fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);

    vprintf(fmt, arguments);

    warCount++;

    va_end(arguments);
}