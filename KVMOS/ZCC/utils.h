#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <ctype.h>

char *substr(const char *str, size_t start, size_t len);
void removeChar(char str[], char target);
bool isNumber(const char *s);
void zemit(FILE *filestream, const char *fmt, ...);

#endif