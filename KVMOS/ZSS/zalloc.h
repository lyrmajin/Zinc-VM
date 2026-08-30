#ifndef ZALLOC_H
#define ZALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// MMU

typedef void (*memoryset_t)(uint8_t, uint16_t);
typedef uint8_t (*memoryget_t)(uint16_t);

extern uint16_t MMUA;

extern uint16_t DATADIR;
extern uint16_t RAMFILE;

extern uint8_t *memory;

uint8_t vmemget(uint16_t address);
void vmemset(uint8_t value, uint16_t address);

// MEM-STr CPY?

void vmemcpyv(uint16_t ptra, uint16_t ptrb, uint16_t size, void (*memset)(uint8_t, uint16_t), uint8_t (*memget)(uint16_t));
void memcpyv(uint8_t *buffer, uint16_t start, uint16_t size, uint8_t (*memget)(uint16_t));
void vmemcpy(uint16_t start, uint8_t *buffer, uint16_t size, void (*memset)(uint8_t, uint16_t));

void vstrcpyv(uint16_t ptra, uint16_t ptrb, void (*memset)(uint8_t, uint16_t), uint8_t (*memget)(uint16_t));
void strcpyv(uint16_t ptr, char* string, uint8_t (*memget)(uint16_t));
void vstrcpy(uint16_t ptr, const char* string, void (*memset)(uint8_t, uint16_t));
uint16_t strlenv(uint16_t ptr, uint8_t (*memget)(uint16_t));

// Allocator

uint16_t getAllocCount(uint8_t *memory);
void setAllocCount(uint16_t value, uint8_t *memory);

uint16_t getAllocPtr(uint8_t *memory);
void setAllocPtr(uint16_t value, uint8_t *memory);

uint8_t getAllocTaskId(uint16_t index, uint8_t *memory);
void setAllocTaskId(uint16_t index, uint8_t value, uint8_t *memory);
uint8_t getAllocIndexById(uint8_t id, uint16_t index);

uint16_t getAllocStart(uint16_t index, uint8_t *memory);
uint16_t getAllocEnd(uint16_t index, uint8_t *memory);

void setAllocData(uint8_t taskid, uint16_t start, uint16_t end, uint8_t *memory);
void reSetAllocData(uint16_t index, uint8_t taskid, uint16_t start, uint16_t end, uint8_t *memory);

uint16_t lookPtrOnAlloc(uint16_t start, uint16_t ptr, uint8_t *memory);

uint16_t memAlloc(uint8_t taskid, uint16_t size, uint8_t *memory);
void memFree(uint16_t *ptr, uint8_t *memory);

static inline uint16_t PRINTFRETURN(const char* ftm, uint16_t val){
    printf(ftm, val);
    fflush(stdout);
    return val;
}

#endif