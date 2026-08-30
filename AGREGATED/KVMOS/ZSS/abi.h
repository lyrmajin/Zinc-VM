#ifndef ABI_H
#define ABI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "zalloc.h"

extern uint16_t PROCDIR;
extern uint16_t LIBDIR;
extern uint16_t SYSCALLTABLEFILE;
extern uint16_t TASKTABLEFILE;
extern uint16_t MODULETABLEFILE;

extern uint16_t HOF;

uint8_t getZEFI(uint16_t offset, memoryget_t memget);
uint8_t getFunctionsCount(uint16_t offset, memoryget_t memget);
uint16_t getFunctionAddress(uint8_t index, uint16_t offset, memoryget_t memget);
uint8_t getHeapPreallocSize(uint16_t offset, memoryget_t memget);
uint8_t getStackPreallocSize(uint16_t offset, memoryget_t memget);
uint16_t getCodePreallocSize(uint16_t offset, memoryget_t memget);

uint8_t getTaskId(uint8_t index, memoryget_t memget);
uint16_t getTaskMetadata(uint8_t index, memoryget_t memget);
uint16_t getModuleMetadata(uint8_t index, memoryget_t memget);
uint16_t getRegister(uint8_t index, uint16_t offset, memoryget_t memget);

uint8_t makeTask(uint8_t Taskid, uint16_t name, uint16_t offset, memoryset_t memset, memoryget_t memget);

uint16_t getTaskCodeAlloc(uint8_t taskid, memoryget_t memget);
uint16_t getModuleCodeAlloc(uint8_t taskid, memoryget_t memget);

void call(uint8_t type, uint16_t offset, uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget);
void syscall(uint16_t number, uint16_t argptr, uint8_t size, memoryset_t memset, memoryget_t memget);

void scheduleTask(uint8_t taskId, uint16_t code);

#endif