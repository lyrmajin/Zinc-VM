#ifndef ZFS2_H
#define ZFS2_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "zalloc.h"

uint16_t makeFile(char *name, uint16_t size, uint8_t *buffer, uint16_t clusterptr, uint16_t offset, memoryset_t memset, memoryget_t memget);
uint8_t makeEntry(uint8_t type, uint16_t address, uint16_t offset, memoryset_t memset, memoryget_t memget);
void makeDir(char *name, uint16_t rootptr, uint16_t offset, memoryset_t memset, memoryget_t memget);

uint16_t getEntryAddress(uint8_t entryindex, uint16_t offset, memoryget_t memget);
uint8_t getEntryType(uint8_t entryindex, uint16_t offset, memoryget_t memget);
uint8_t getFileNameSize(uint16_t offset, memoryget_t memget);
uint16_t getFileName(uint16_t offset, memoryget_t memget);
uint16_t getFileDataSize(uint16_t offset, memoryget_t memget);
uint16_t getFileSize(uint16_t offset, memoryget_t memget);
uint16_t getFileDataAddress(uint16_t offset, memoryget_t memget);
uint16_t getFileClusterPtr(uint16_t offset, memoryget_t memget);

void readFileData(uint8_t *data, uint16_t size, uint16_t offset, memoryget_t memget);
void writeFileData(uint8_t *data, uint16_t size, uint16_t offset, memoryset_t memset, memoryget_t memget);

uint16_t getClusterDataSize(uint16_t offset, memoryget_t memget);
uint16_t getClusterSize(uint16_t offset, memoryget_t memget);
uint16_t getClusterDataAddress(uint16_t offset, memoryget_t memget);
uint16_t getClusterClusterPtr(uint16_t offset, memoryget_t memget);

void readClusterData(uint8_t *data, uint16_t size, uint16_t offset, memoryget_t memget);
void writeClusterData(uint8_t *data, uint16_t size, uint16_t offset, memoryset_t memset, memoryget_t memget);

#endif