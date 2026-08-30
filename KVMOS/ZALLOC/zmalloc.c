#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t *memory;

/*
memory layout

0x00 -> allocations count high byte
0x01 -> allocations count low byte

0x02 -> allocations ptr high byte
0x03 -> allocations ptr low byte

[task id (1)] [start adrs (2)] [end adrs (2)] <-- per allocation (includes stack)

*/

uint16_t getAllocCount(uint8_t *memory)
{
    return ((memory[0x00] << 0x08) | memory[0x01]);
}
void setAllocCount(uint16_t value, uint8_t *memory)
{
    memory[0x00] = value >> 0x08;
    memory[0x01] = value;
}

uint16_t getAllocPtr(uint8_t *memory)
{
    return ((memory[0x02] << 0x08) | memory[0x03]);
}
void setAllocPtr(uint16_t value, uint8_t *memory)
{
    memory[0x02] = value >> 0x08;
    memory[0x03] = value;
}

uint8_t getAllocTaskId(uint16_t index, uint8_t *memory)
{
    if (index < getAllocCount(memory))
        return memory[(index * 5) + 4];
    return (uint8_t)-1;
}
void setAllocTaskId(uint16_t index, uint8_t value, uint8_t *memory)
{
    if (index < getAllocCount(memory))
        memory[(index * 5) + 4] = value;
}
uint16_t getAllocStart(uint16_t index, uint8_t *memory)
{
    if (index < getAllocCount(memory))
        return ((memory[((index * 5) + 4) + 1] << 0x08) | memory[((index * 5) + 4) + 2]);
    return (uint16_t)-1;
}
uint16_t getAllocEnd(uint16_t index, uint8_t *memory)
{
    if (index < getAllocCount(memory))
        return ((memory[((index * 5) + 6) + 1] << 0x08) | memory[((index * 5) + 6) + 2]);
    return (uint16_t)-1;
}

void setAllocData(uint8_t taskid, uint16_t start, uint16_t end, uint8_t *memory)
{
    memory[(getAllocCount(memory) * 5) + 4] = taskid;
    memory[(getAllocCount(memory) * 5) + 4 + 1] = start >> 0x08;
    memory[(getAllocCount(memory) * 5) + 4 + 2] = start;
    memory[(getAllocCount(memory) * 5) + 4 + 3] = end >> 0x08;
    memory[(getAllocCount(memory) * 5) + 4 + 4] = end;
    setAllocCount(getAllocCount(memory) + 1, memory);
}

void reSetAllocData(uint16_t index, uint8_t taskid, uint16_t start, uint16_t end, uint8_t *memory)
{
    memory[(index * 5) + 4] = taskid;
    memory[(index * 5) + 4 + 1] = start >> 0x08;
    memory[(index * 5) + 4 + 2] = start;
    memory[(index * 5) + 4 + 3] = end >> 0x08;
    memory[(index * 5) + 4 + 4] = end;
}

void KillAllocData(uint16_t index, uint8_t *memory)
{
    memory[(index * 5) + 4] = 0;
    memory[(index * 5) + 4 + 1] = 0;
    memory[(index * 5) + 4 + 2] = 0;
    memory[(index * 5) + 4 + 3] = 0;
    memory[(index * 5) + 4 + 4] = 0;
    setAllocCount(getAllocCount(memory) - 1, memory);
}

uint16_t averageFromTo(uint16_t a, uint16_t b, uint8_t *memory)
{
    uint64_t sum = 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < getAllocCount(memory); i++)
    {
        uint16_t size = (getAllocEnd(i, memory) - getAllocStart(i, memory));
        if ((size >= a && size < b) && !getAllocTaskId(i, memory))
        {
            sum += size;
            count++;
        }
    }
    return sum / ((count) ? count : 1);
}

uint16_t firstIndexAverageFromTo(uint16_t a, uint16_t b, uint8_t *memory)
{
    for (uint16_t i = 0; i < getAllocCount(memory); i++)
    {
        uint16_t size = (getAllocEnd(i, memory) - getAllocStart(i, memory));
        if ((size >= a && size <= b) && !getAllocTaskId(i, memory))
        {
            return i;
        }
    }
    return 0xFFFF;
}

uint16_t getBestFit(uint16_t size, uint8_t *memory)
{
    uint16_t lastsize = 0xFFFF;
    uint16_t lastsize2 = 0;
    while (lastsize)
    {
        lastsize = averageFromTo(size, lastsize, memory);
        if (!lastsize)
        { // if size 0, means it found fit
            lastsize = firstIndexAverageFromTo(size, lastsize2, memory);
            return lastsize;
        }
        lastsize2 = lastsize;
    }
    return lastsize;
}

uint16_t memAlloc(uint8_t taskid, uint16_t size, uint8_t *memory)
{
    uint16_t index = getBestFit(size, memory);
    uint16_t start = getAllocStart(index, memory);
    uint16_t end = getAllocEnd(index, memory);

    reSetAllocData(index, taskid, start, start + size, memory);
    setAllocData(0, start + size, end, memory);

    uint16_t AllocPtr = getAllocPtr(memory);
    if ((start + size) >= AllocPtr)
        setAllocPtr(start + size, memory);

    return start;
}

uint16_t lookPtrOnAlloc(uint16_t ptr, uint8_t *memory)
{
    uint16_t Index = 0;
    for (uint16_t i = 0; i < getAllocCount(memory); i++)
    {
        if (getAllocStart(i, memory) == ptr)
            return Index;
    }
    return getAllocCount(memory);
}

void memFree(uint16_t *ptr, uint8_t *memory)
{
    uint16_t index = lookPtrOnAlloc(*ptr, memory);
    setAllocTaskId(index, 0, memory);

    uint16_t indexB = (index > 0) ? index - 1 : 0;
    if(!getAllocTaskId(indexB, memory)){
        setAllocTaskId(indexB, 0xFF, memory);
        reSetAllocData(index, 0, getAllocStart(indexB, memory), getAllocEnd(index, memory), memory);
    }

    uint16_t indexC = (index < getAllocCount(memory)) ? index + 1 : 0;
    if(!getAllocTaskId(indexC, memory)){
        setAllocTaskId(index, 0xFF, memory);
        reSetAllocData(indexC, 0, getAllocStart(indexB, memory), getAllocEnd(indexC, memory), memory);
        index = indexC;
    }

    if (getAllocEnd(index, memory) == getAllocPtr(memory)){
        indexB = (index > 0) ? index - 1 : 0;
        if(getAllocTaskId(indexB, memory) == 0xFF){
            reSetAllocData(indexB, 0, getAllocStart(index, memory), getAllocEnd(index, memory), memory);
            for (uint8_t i = indexB + 1; i < getAllocCount(memory); i++)
            {
                KillAllocData(i, memory);
            }
        }
        setAllocPtr(getAllocEnd(index, memory), memory);
    }
    *ptr = 0;
}

int main()
{
    memory = (uint8_t *)calloc(1024, 1);
    if (!memory)
        return 1;

    memory[0x00] = 0x00;
    memory[0x01] = 0x01; // <-- set atleast 1 allocation

    memory[0x02] = 0x00;
    memory[0x03] = 0xFF; // <-- set Heap Pointer. which will act as the start of allocations

    memory[0x04] = 0x00;
    memory[0x05] = memory[0x02];
    memory[0x06] = memory[0x03];
    memory[0x07] = 0xFF;
    memory[0x08] = 0xFF; // <-- set first allocation metadata (free, start=hp, end=size_of(memory))

    uint16_t ptrA = memAlloc(1, 128, memory);
    printf("alloc[128]:0x%x:HP:0x%x\n", ptrA, getAllocPtr(memory));

    uint16_t ptrB = memAlloc(1, 64, memory);
    printf("alloc[64]:0x%x:HP:0x%x\n", ptrB, getAllocPtr(memory));

    memFree(&ptrA, memory);
    printf("freeA:0x%x\n", ptrA);

    uint16_t ptrC = memAlloc(1, 32, memory);
    printf("alloc[32]:0x%x:HP:0x%x\n", ptrC, getAllocPtr(memory));

    uint16_t ptrD = memAlloc(1, 16, memory);
    printf("alloc[16]:0x%x:HP:0x%x\n", ptrD, getAllocPtr(memory));

    memFree(&ptrB, memory);
    printf("freeB:0x%x\n", ptrB);

    uint16_t ptrE = memAlloc(1, 64, memory);
    printf("alloc[64]:0x%x:HP:0x%x\n", ptrE, getAllocPtr(memory));

    free(memory);
    return 0;
}