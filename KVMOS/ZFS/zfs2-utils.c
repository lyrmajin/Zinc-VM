#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint8_t *memory;

/*
0x00 --> empty
0x01 --> rw directory
0x02 --> ro directory
0x03 --> rw file
0x04 --> ro file
0x05 --> cluster
*/

/*
Data:
[namesize(Null included) (1)] [namestr (namesize)] [filesize (2)] [filedata (filesize)] [clusterpointer (2)]
*/

uint16_t makeFile(char *name, uint16_t size, uint8_t *buffer, uint16_t clusterptr, uint16_t offset, uint8_t *memory)
{
    memory[offset] = (strlen(name) + 1); // <-- name lenght
    uint16_t afstrpos = (offset - memory[offset]);
    if (name)
        strcpy((char *)&memory[afstrpos], name); // <-- name string
    memory[afstrpos - 1] = (size & 0x00FF);      // <-- size low byte
    memory[afstrpos - 2] = (size & 0xFF00);      // <-- size high byte
    uint16_t afdtpos = (afstrpos - size - 2);
    if (buffer)
        memcpy(memory + afdtpos, buffer, size);  // <-- file data
    memory[afdtpos - 1] = (clusterptr & 0x00FF); // <-- cluster pointer low byte
    memory[afdtpos - 2] = (clusterptr & 0xFF00) >> 0x08; // <-- cluster pointer high byte
    return afdtpos;
}

uint8_t makeEntry(uint8_t type, uint16_t address, uint16_t offset, uint8_t *memory)
{
    // 24 is used here as it is the max number of bytes a directory can have. each entry is 3 bytes, so in totaal 8 entries.
    for (uint16_t i = offset; i < (offset + 24); i += 3)
    {
        if (memory[i] == 0)
        {
            memory[i] = type;                             // <-- type
            memory[i + 1] = ((address & 0xFF00) >> 0x08); // <-- address high byte
            memory[i + 2] = (address & 0x00FF);           // <-- address low byte
            return ((i - offset) / 3);                    // <-- return entry number (0 - 7)
        }
    }
    return 0xFF; // <-- directory has no space left
}

void makeDir(char *name, uint16_t rootptr, uint16_t offset, uint8_t *memory)
{
    uint16_t diraddress = makeFile(name, 24, NULL, offset, offset, memory); // <-- create a dir file
    makeEntry(5, rootptr, diraddress, memory);                              // <-- create a cluster entry pointing to the rootdir, at diraddress
}

uint16_t getEntryAddress(uint8_t entryindex, uint16_t offset, uint8_t *memory)
{
    uint16_t address = offset + (entryindex * 3);                     // <-- converting the entry number in a absolute address based on offset
    if (memory[address])                                              // <-- if the entry isnt empty
        return ((memory[address + 1] << 0x08) | memory[address + 2]); // <-- return the address
    return offset;                                                    // <-- return offset to signal entry is empty
}

uint8_t getEntryType(uint8_t entryindex, uint16_t offset, uint8_t *memory)
{
    uint16_t address = offset + (entryindex * 3); // <-- converting the entry number in a absolute address based on offset
    return memory[address];                       // <-- return entry type
}

uint8_t getFileNameSize(uint16_t offset, uint8_t *memory)
{
    return memory[offset]; // <-- string size
}

char *getFileName(uint16_t offset, uint8_t *memory)
{
    uint16_t size = memory[offset]; // <-- string size
    return (char *)&memory[offset - size];
}

uint16_t getFileDataSize(uint16_t offset, uint8_t *memory)
{
    uint16_t strsize = memory[offset]; // <-- string size
    return ((memory[offset - strsize - 2] << 0x08) | memory[offset - strsize - 1]);
}

uint16_t getFileSize(uint16_t offset, uint8_t *memory)
{
    uint16_t strsize = memory[offset]; // <-- string size
    uint16_t dtsize = ((memory[offset - strsize - 2] << 0x08) | memory[offset - strsize - 1]);

    return (1 + strsize + 2 + dtsize + 2); // <-- (namelen (1)) + (strsize) + (size (2)) + (dtsize) + (clusterptr (2))
}

uint16_t getFileDataAddress(uint16_t offset, uint8_t *memory)
{
    uint16_t strsize = memory[offset]; // <-- string size
    uint16_t dtsize = ((memory[offset - strsize - 2] << 0x08) | memory[offset - strsize - 1]);

    return (offset - (strsize + 2 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getFileClusterPtr(uint16_t offset, uint8_t *memory)
{
    // --> (offset) - (namelen (1)) - (strsize) - (size (2)) - (dtsize)
    return ((memory[getFileDataAddress(offset, memory) - 2] << 0x08) | memory[getFileDataAddress(offset, memory) - 1]);
}

void readFileData(uint8_t *ptr, uint16_t size, uint16_t offset, uint8_t *memory)
{
    uint16_t datasize = getFileDataSize(offset, memory);
    if (size > datasize)
        size = datasize; // clamp

    memcpy(ptr, memory + getFileDataAddress(offset, memory), size);
}

void writeFileData(uint8_t *data, uint16_t size, uint16_t offset, uint8_t *memory)
{
    uint16_t datasize = getFileDataSize(offset, memory);
    if (size > datasize)
        size = datasize; // clamp

    memcpy(memory + getFileDataAddress(offset, memory), data, size);
}

uint16_t getClusterDataSize(uint16_t offset, uint8_t *memory)
{
    return ((memory[offset - 1] << 0x08) | memory[offset]);
}

uint16_t getClusterSize(uint16_t offset, uint8_t *memory)
{
    uint16_t dtsize = getClusterDataSize(offset, memory);

    return (offset - (4 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getClusterDataAddress(uint16_t offset, uint8_t *memory)
{
    uint16_t dtsize = getClusterDataSize(offset, memory);

    return (offset - (2 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getClusterClusterPtr(uint16_t offset, uint8_t *memory)
{
    // --> (offset) - (namelen (1)) - (strsize) - (size (2)) - (dtsize)
    return ((memory[getClusterDataAddress(offset, memory) - 2] << 0x08) | memory[getClusterDataAddress(offset, memory) - 1]);
}

void readClusterData(uint8_t *ptr, uint16_t size, uint16_t offset, uint8_t *memory)
{
    uint16_t datasize = getClusterDataSize(offset, memory);
    if (size > datasize)
        size = datasize; // clamp

    memcpy(ptr, memory + getClusterDataAddress(offset, memory), size);
}

void writeClusterData(uint8_t *data, uint16_t size, uint16_t offset, uint8_t *memory)
{
    uint16_t datasize = getClusterDataSize(offset, memory);
    if (size > datasize)
        size = datasize; // clamp

    memcpy(memory + getClusterDataAddress(offset, memory), data, size);
}

// teeests

void mkdir(char *dirName, uint16_t currentDirAddress, uint16_t *hof, uint8_t *memory)
{
    uint16_t currentDirDataAddress = getFileDataAddress(currentDirAddress, memory);
    makeEntry(1, *hof, currentDirDataAddress, memory);
    makeDir(dirName, currentDirAddress, *hof, memory);

    *hof -= getFileSize(*hof, memory);
}

void touch(char *Name, uint16_t currentDirAddress, uint16_t *hof, uint8_t *memory)
{
    uint16_t currentDirDataAddress = getFileDataAddress(currentDirAddress, memory);
    makeEntry(3, *hof, currentDirDataAddress, memory);
    makeFile(Name, 0, NULL, *hof, *hof, memory);

    *hof -= getFileSize(*hof, memory);
}

int main(int argc, char **argv)
{
    int str = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "f="))
            str = ++i;
    }
    FILE *file = fopen(argv[str], "r");
    fseek(file, 0, SEEK_END);
    long memsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    memory = (uint8_t *)calloc(memsize, 1);
    if (!memory)
        exit(1);
    fread(memory, 1, memsize, file);

    uint8_t *buffer = (uint8_t *)malloc(4);
    if (!buffer)
        return 1;
    readFileData(buffer, 4, 0x27, memory);
    uint16_t hof = ((buffer[0] << 0x8) | buffer[1]);
    uint16_t fsc = ((buffer[2] << 0x8) | buffer[3]);
    // printf("hdf:0x%x\n", hof);
    // printf("fsc:0x%x\n", fsc);

    uint16_t cda = 0x1D;

    // mkdir("myfolder", cda, &hof, memory);
    // mkdir("myfolder", cda, &hof, memory);

    buffer[0] = ((hof >> 0x08) & 0xFF);
    buffer[1] = (hof & 0xFF);
    buffer[2] = ((fsc >> 0x08) & 0xFF);
    buffer[3] = (fsc & 0xFF);

    writeFileData(buffer, 4, 0x27, memory);

    for (uint8_t i = 0; i < 8; i++)
    {
        // fixed to 0x02 because that is the start of the data in the root dir
        uint16_t address = getEntryAddress(i, 0x02, memory);

        if (address != 0x02)
        {
            printf("===Address:0x%hx===\n", address);
            printf("FileSize:%u\n", getFileSize(address, memory));
            printf("NameSize:%u\n", getFileNameSize(address, memory));
            printf("Name:%s\n", getFileName(address, memory));
            printf("DataSize:%u\n", getFileDataSize(address, memory));
            printf("DataStartAddress:0x%hx\n", getFileDataAddress(address, memory));
            printf("ClusterPtr:0x%hx\n", getFileClusterPtr(address, memory));
        }
    }
    file = fopen(argv[argc - 1], "w");
    fwrite(memory, 1, memsize, file);
    fclose(file);
    return 0;
}