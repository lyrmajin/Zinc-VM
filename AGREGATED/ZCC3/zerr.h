#ifndef ZERR_H
#define ZERR_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

extern uint16_t errCount;
extern uint16_t warCount;

void zerr(char *str, ...);
void zwar(char *str, ...);

#define NEWLINE 0xFFFFFFFF

void zdeb(const char *fmt, ...);

#endif