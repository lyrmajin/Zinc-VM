#ifndef ZVM_H
#define ZVM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "zalloc.h"

#define UNHALTED 0b10000000
#define RUNING 0b01000000

#define MEMSIZE 1024

#define MAXREG 10
#define GPR0 0 // General Porpuse Register 0
#define GPR1 1 // General Porpuse Register 1
#define GPR2 2 // General Porpuse Register 2
#define RT 3   // Return Register 3
#define PC 4   // Program Count
#define SPI 5  // Stack Pointer Index
#define FP 6   // Frame Pointer
#define TI 7   // Thread Id
#define IR 8   // Instruction Register High
#define FL 9   // Flags // 0 0 0 0 0 0 0 0 RUNTIME SYSTEM SEGFAULT 0 0 0 MODE CMP

extern uint16_t REGISTER[];

void storeTask();
void loadTask();

void ExecVMOperation();

#endif