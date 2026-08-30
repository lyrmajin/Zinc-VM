#include "utils.h"

char *substr(const char *str, size_t start, size_t len) {
    if (!str) return NULL;

    size_t str_len = strlen(str);
    if (start >= str_len) return NULL;

    if (start + len > str_len) {
        len = str_len - start;
    }

    char *result = malloc(len + 1);
    if (!result) return NULL;

    strncpy(result, str + start, len);
    result[len] = '\0';
    return result;
}

void removeChar(char str[], char target) {
    int i = 0, j = 0;

    while (str[i] != '\0') {
        if (str[i] != target) {
            str[j] = str[i];
            j++;
        }
        i++;
    }
    str[j] = '\0';
}

bool isNumber(const char *s) {
    if (*s == '\0') return 0;
    while (*s) {
        if (!isdigit(*s)) return 0;
        s++;
    }
    return 1;
}

#include <stdarg.h>
#include <stdio.h>

void zemit(FILE *filestream, const char *fmt, ...) {
    va_list args1, args2;

    va_start(args1, fmt);
    va_copy(args2, args1);

    vfprintf(filestream, fmt, args1);
    vprintf(fmt, args2);

    va_end(args1);
    va_end(args2);
}
