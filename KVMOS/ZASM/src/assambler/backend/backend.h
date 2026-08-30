#ifndef BACKEND_H
#define BACKEND_H

extern bool DoDebug;
extern bool DoInternalsDebug;

int codegen(ASTNODE *astnode, uint8_t *batch, size_t batchSize, uint64_t *i);

#endif