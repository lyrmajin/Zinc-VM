#ifndef ZCODEGEN_H
#define ZCODEGEN_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "zerr.h"
#include "utils.h"
#include "zparser.h"

void codegen(ASTNODE *astnode, FILE *filestream);

#endif