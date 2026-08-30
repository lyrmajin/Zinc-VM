#include "zalloc.h"
#include "abi.h"
#include "zfs2.h"
#include "zvm.h"

uint8_t unsafememget(uint16_t address)
{
    return memory[address];
}

void unsafememset(uint8_t value, uint16_t address)
{
    memory[address] = value;
}

uint8_t readDevice(uint16_t address)
{
    if (memory[DATADIR] == 0)
    { // <-- No fs is loaded yet
        if (address >= 0 && address < (0x100 + 0x200))
        {
            return memory[address];
        }
        else
        {
            REGISTER[FL] |= 0x20;
            return 0xFF;
        }
    }
    uint16_t dta = RAMFILE;
    if (address >= 0 && address < ((memory[dta] << 0x08) | memory[dta + 1]))
    {
        return memory[address];
    }

    printf("\nUsing virtual device!\n");
    dta = getFileDataAddress(getEntryAddress(2, DATADIR, &unsafememget), &unsafememget);
    for (uint16_t i = dta; i < (dta + getFileDataSize(DATADIR, &unsafememget)); i += 7)
    {
        if (address >= ((memory[i + 1] << 0x08) | memory[i + 2]) && address < ((memory[i + 3] << 0x08) | memory[i + 4]))
        {
            memory[MMUA] = address >> 0x08;
            memory[MMUA + 1] = address;
            memory[MMUA + 2] = MMUA >> 0x08;
            memory[MMUA + 3] = MMUA;
            memory[MMUA + 4] = 0;
            memory[MMUA + 5] = 0; // read
            call(3, ((memory[i + 1] << 0x08) | memory[i + 2]) - ((memory[i + 5] << 0x08) | memory[i + 6]), 2, MMUA, &unsafememset, &unsafememget);
            return memory[MMUA + 3];
            // that was the probitional -->
            // return 0x00; //sys or function call device trough entry in memory[i]
        }
    }
    REGISTER[FL] |= 0x20;
    return 0xFF;
}

void writeDevice(uint8_t value, uint16_t address)
{
    if (memory[DATADIR] == 0)
    { // <-- No fs is loaded yet
        if (address >= 0 && address < (0x100 + 0x200))
        {
            memory[address] = value;
            return;
        }
        else
        {
            REGISTER[FL] |= 0x20;
            return;
        }
    }

    uint16_t dta = RAMFILE;
    if (address >= 0 && address < ((memory[dta] << 0x08) | memory[dta + 1]))
    {
        memory[address] = value;
        return;
    }

    printf("\nUsing virtual device!\n");

    dta = getFileDataAddress(getEntryAddress(2, DATADIR, &unsafememget), &unsafememget);
    for (uint16_t i = dta; i < (dta + getFileDataSize(DATADIR, &unsafememget)); i += 5)
    {
        if (address >= ((memory[i + 1] << 0x08) | memory[i + 2]) && address < ((memory[i + 3] << 0x08) | memory[i + 4]))
        {
            memory[MMUA] = address >> 0x08;
            memory[MMUA + 1] = address;
            memory[MMUA + 2] = MMUA >> 0x08;
            memory[MMUA + 3] = MMUA;
            memory[MMUA + 4] = value;
            memory[MMUA + 5] = 1; // write
            call(3, ((memory[i + 1] << 0x08) | memory[i + 2]) - ((memory[i + 5] << 0x08) | memory[i + 6]), 2, MMUA, &unsafememset, &unsafememget);
            // return ((memory[MMUA + 2] << 0x08) | memory[MMUA + 3]); // no return happens here
            // that was the probitional -->
            // return 0x00; //sys or function call device trough entry in memory[i]
            return;
        }
    }
    REGISTER[FL] |= 0x20;
    return;
}

int isOnRange(uint16_t id, uint16_t address)
{
    for (uint16_t i = 0; i < getAllocCount(memory); i++)
    {
        if (getAllocTaskId(i, memory) == id)
        {
            if (address >= getAllocStart(i, memory) && address < getAllocEnd(i, memory))
                return 1;
        }
    }
    return 0;
}

uint8_t vmemget(uint16_t address)
{
    printf("Task %u Requested Memory Access [Read](0x%x)", REGISTER[TI], address);
    fflush(stdout);
    if (isOnRange(REGISTER[TI], address) || REGISTER[TI] == 0)
    {
        return PRINTFRETURN(": Allowed! (0x%x)\n", readDevice(address));
    }
    printf("\nSEGFAULT\n");
    REGISTER[FL] |= 0x20;
    if (REGISTER[TI] == 1)
        exit(1);
    return 0xFF; // <-- here call sys sigkill
}
void vmemset(uint8_t value, uint16_t address)
{
    printf("Task %u Requested Memory Access [Write](0x%x) --> (0x%x)", REGISTER[TI], address, value);
    fflush(stdout);
    if (isOnRange(REGISTER[TI], address) || REGISTER[TI] == 0)
    {
        writeDevice(value, address);
        printf(": Allowed!\n");
    }
    else
    {
        printf("\nSEGFAULT\n");
        REGISTER[FL] |= 0x20;
        if (REGISTER[TI] == 1)
            exit(1);
    }
    return;
}

/*
uint8_t id: represents the task id owner
uint16_t index: index to start from
*/
uint8_t getAllocIndexById(uint8_t id, uint16_t index)
{
    for (uint16_t i = index; i < getAllocCount(memory); i++)
    {
        if (getAllocTaskId(i, memory) == id)
        {
            return i;
        }
    }
    return 0;
}

// 7