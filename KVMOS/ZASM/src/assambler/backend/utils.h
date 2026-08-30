#ifndef UTILS_H
#define UTILS_H

//Errors and Warnings

extern FILE *INTERNALERR;
extern FILE *INTERNALWARN;
extern FILE *INTERNALDEBUG;
extern FILE *OUTPUT;

extern char *INFILENAME;
extern char *OUTFILENAME;

extern uint32_t LINE, COLUMN;
extern uint32_t ERR_COUNT, WARN_COUNT;

extern bool DoDump;
extern bool DoDebug;
extern bool DoInternalsDebug;

void emiterr(const char *fmt, ...);
void emitwarn(const char *fmt, ...);
void emitdebug(const char *fmt, ...);
void emit(uint8_t *BYTES, size_t SIZE);

//Utilities

bool isSeparatorChar(char c);

// Internal dependences
#include "../frontend/frontend.h"


extern bool DoDebug;
extern bool DoInternalsDebug;

extern uint32_t LINE, COLUMN;

int emittoken(char *line, size_t *j, TOKENLIST *tokenlist, TOKENTYPE tokentype, int (*allocateToken)(TOKENLIST*, TOKEN));

char *strtolower(char *str);

void print_help(void);

#endif