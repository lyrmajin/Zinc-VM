#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "zalloc.h"
#include "zfs2.h"
#include "abi.h"
#include "zvm.h"

uint8_t *memory; // emulated, normally offered by zvm

uint16_t REGISTER[MAXREG] = {0};

uint16_t DEVDIR = 0;
uint16_t PROCDIR = 0;
uint16_t RUNDIR = 0;
uint16_t LIBDIR = 0;

uint16_t DATADIR = 0;
uint16_t RAMFILE = 0;
uint16_t MMUA = 0;

uint16_t SYSCALLTABLEFILE = 0;
uint16_t TASKTABLEFILE = 0;
uint16_t MODULETABLEFILE = 0;

uint16_t HOF = 0;

// HELPERS

void scheduleTask(uint8_t taskId, uint16_t code)
{
    storeTask();
    REGISTER[TI] = taskId;

    loadTask();

    uint16_t index = lookPtrOnAlloc(0, code, memory);
    if (REGISTER[PC] >= getAllocEnd(index, memory))
        REGISTER[PC] = getAllocStart(index, memory) + (REGISTER[PC] - getAllocEnd(index, memory));
    if (REGISTER[PC] < getAllocStart(index, memory))
        REGISTER[PC] = getAllocEnd(index, memory) - (getAllocStart(index, memory) - REGISTER[PC]);

    // here the check to maintain the code block clean must exist.
    printf("\n\n");
    REGISTER[FL] |= UNHALTED;
    ExecVMOperation();
    printf("\n\n");
    storeTask();
}

// START

void start()
{
    memory = (uint8_t *)calloc(MEMSIZE, 1);

    REGISTER[TI] = 1; // <-- don't forget to set kernel task id!

    memory[0x00] = 0x00;
    memory[0x01] = 0x01; // <-- set atleast 1 allocation

    memory[0x02] = 0x00;
    memory[0x03] = 0xFF; // <-- set Heap Pointer. which will act as the start of allocations

    memory[0x04] = 0x00;
    memory[0x05] = memory[0x02];
    memory[0x06] = memory[0x03];
    memory[0x07] = (uint8_t)(MEMSIZE >> 0x08);
    memory[0x08] = (uint8_t)(MEMSIZE); // <-- set first allocation metadata (free, start=hp, end=size_of(memory))

    printf("Setting InitRamAlloc\n");
    uint16_t address = memAlloc(1, 0x200, memory); // i think we allocating too litle space for the INITRAMFS // <-- i decided to changue it from 0xFF to 0x1FF. now system uses 768 bytes. or so..
    MMUA = memAlloc(1, 0x06, memory);              // MMU argument passing block
    uint16_t index = lookPtrOnAlloc(0, address, memory);

    // FILE SYSTEM SETUP STAT

    printf("Setting Init Ram FS:(0x%x)\n", address);
    memory[address + 0x00] = (address + 0x1E) >> 0x08; // <-- cluster pointer high
    memory[address + 0x01] = (address + 0x1E);         // <-- cluster pointer low
    memory[address + 0x02] = 0x05;                     // <-- parent address // <-- file start
    memory[address + 0x03] = (address + 0x1E) >> 0x08;
    memory[address + 0x04] = address + 0x1E;
    memory[address + 0x05] = 0x00; // <-- address 1
    memory[address + 0x06] = 0x00;
    memory[address + 0x07] = 0x00;
    memory[address + 0x08] = 0x00; // <-- address 2
    memory[address + 0x09] = 0x00;
    memory[address + 0x0A] = 0x00;
    memory[address + 0x0B] = 0x00; // <-- address 3
    memory[address + 0x0C] = 0x00;
    memory[address + 0x0D] = 0x00;
    memory[address + 0x0E] = 0x00; // <-- address 4
    memory[address + 0x0F] = 0x00;
    memory[address + 0x10] = 0x00;
    memory[address + 0x11] = 0x00; // <-- address 5
    memory[address + 0x12] = 0x00;
    memory[address + 0x13] = 0x00;
    memory[address + 0x14] = 0x00; // <-- address 6
    memory[address + 0x15] = 0x00;
    memory[address + 0x16] = 0x00;
    memory[address + 0x17] = 0x00; // <-- address 7
    memory[address + 0x18] = 0x00;
    memory[address + 0x19] = 0x00; // <-- file end
    memory[address + 0x1A] = 0x00; // <-- file size high
    memory[address + 0x1B] = 0x18; // <-- file size low
    memory[address + 0x1C] = '/';  // <-- name str start
    memory[address + 0x1D] = '\0'; // <-- name str end
    memory[address + 0x1E] = 0x02; // <-- name size

    HOF = getAllocEnd(index, memory) - 1;
    printf("Setting HeadOfFS (0x%x)\n", HOF);

    memory[address + 0x1F] = (address + 0x28) >> 0x08;       // <-- cluster pointer high
    memory[address + 0x20] = (address + 0x28);               // <-- cluster pointer low
    memory[address + 0x21] = (((HOF - 1) & 0xFF00) >> 0x08); // <-- Allocation End High Byte // <-- file start
    memory[address + 0x22] = ((HOF - 1) & 0x00FF);           // <-- Allocation End Low Byte
    memory[address + 0x23] = 0x00;                           // <-- Freed Allocation Count High Byte
    memory[address + 0x24] = 0x00;                           // <-- Freed Allocation Count Low Byte // <-- file end
    memory[address + 0x25] = 0x00;                           // <-- file size high
    memory[address + 0x26] = 0x04;                           // <-- file size low
    memory[address + 0x27] = '\0';                           // <-- name str start // <-- name str end
    memory[address + 0x28] = 0x01;                           // <-- name size

    printf("Creatting VFiles Initramfs\n\n");
    uint16_t cdda = getFileDataAddress(address + 0x1E, &vmemget);
    makeEntry(1, HOF, cdda, &vmemset, &vmemget);
    makeDir("dev", address + 0x1E, HOF, &vmemset, &vmemget);

    DEVDIR = HOF;
    printf("\n/dev Created! (0x%x)\n\n", HOF);
    HOF -= getFileSize(HOF, &vmemget);

    makeEntry(1, HOF, cdda, &vmemset, &vmemget);
    TASKTABLEFILE = makeFile("proc", 0x18, NULL, HOF, HOF, &vmemset, &vmemget);

    PROCDIR = HOF;
    printf("\n/proc Created! (0x%x) (0x%x)\n\n", HOF, TASKTABLEFILE);
    HOF -= getFileSize(HOF, &vmemget);

    makeEntry(1, HOF, cdda, &vmemset, &vmemget);
    makeDir("run", address + 0x1E, HOF, &vmemset, &vmemget);

    RUNDIR = HOF;
    printf("\n/run Created! (0x%x)\n\n", HOF);
    HOF -= getFileSize(HOF, &vmemget);

    cdda = getFileDataAddress(RUNDIR, &vmemget);
    makeEntry(1, HOF, cdda, &vmemset, &vmemget);
    makeFile("lib", 0x18, NULL, HOF, HOF, &vmemset, &vmemget);

    LIBDIR = HOF;
    printf("\n/lib Created! (0x%x)\n\n", HOF);
    HOF -= getFileSize(HOF, &vmemget);

    cdda = getFileDataAddress(RUNDIR, &vmemget);
    makeEntry(3, HOF, cdda, &vmemset, &vmemget);
    makeFile("modules", 0x18, NULL, HOF, HOF, &vmemset, &vmemget);

    MODULETABLEFILE = HOF;
    HOF -= getFileSize(HOF, &vmemget);

    cdda = getFileDataAddress(RUNDIR, &vmemget);
    makeEntry(3, HOF, cdda, &vmemset, &vmemget);
    makeFile("syscalls", 32, NULL, HOF, HOF, &vmemset, &vmemget);

    SYSCALLTABLEFILE = HOF;
    printf("\n/run/syscalls Created! (0x%x)\n\n", HOF);
    HOF -= getFileSize(HOF, &vmemget);

    cdda = getFileDataAddress(DEVDIR, &vmemget);
    makeEntry(1, HOF, cdda, &vmemset, &vmemget);
    makeDir("data", DEVDIR, HOF, &vmemset, &vmemget);

    uint16_t datadir = cdda;
    printf("\n/data Created! (0x%x)\n\n", HOF);

    cdda = getFileDataAddress(HOF, &vmemget);
    HOF -= getFileSize(HOF, &vmemget);

    makeEntry(4, HOF, cdda, &vmemset, &vmemget);
    uint8_t *tmp = (uint8_t *)malloc(2);
    tmp[0] = (uint8_t)(MEMSIZE >> 0x08);
    tmp[1] = (uint8_t)(MEMSIZE);
    RAMFILE = makeFile("ram", 2, tmp, HOF, HOF, &vmemset, &vmemget);
    free(tmp);
    printf("\n/data/ram Created! (0x%x)\n\n", HOF);
    // this specific direction should containt the ram size

    HOF -= getFileSize(HOF, &vmemget);
    DATADIR = datadir;

    // FILE SYSTEM SETUP END

    makeEntry(4, HOF, address + 0x1E, &vmemset, &vmemget);
    datadir = makeFile("inittab", 9, NULL, HOF, HOF, &vmemset, &vmemget);
    uint16_t INITTABFILE = HOF;
    HOF -= getFileSize(HOF, &vmemget);
    printf("\ninittab created succesfully!\n\n");

    memory[datadir + 0] = 0x01; // ZEFI
    memory[datadir + 1] = 0x00; // FunCount
    memory[datadir + 2] = 0x00; // Heap
    memory[datadir + 3] = 0x40; // Stack
    memory[datadir + 4] = 0x00; // Code High
    memory[datadir + 5] = 0x03; // Code Low
    // Binary goes on from here
    memory[datadir + 6] = 0x00; // MVI 0x00
    memory[datadir + 7] = 0xFF; // #0xFF
    memory[datadir + 8] = 0xFF; // #0x--FF

    // make a zefi file with the startup --^

    call(1, INITTABFILE, 0, 0, &vmemset, &vmemget);

    storeTask();
    
    while(1)
    {
        storeTask();
        uint16_t index = lookPtrOnAlloc(0, getRegister(8, TASKTABLEFILE, &vmemget), memory);
        if (REGISTER[PC] >= getAllocEnd(index, memory))
            REGISTER[PC] = getAllocStart(index, memory) + (REGISTER[PC] - getAllocEnd(index, memory));
        if (REGISTER[PC] < getAllocStart(index, memory))
            REGISTER[PC] = getAllocEnd(index, memory) - (getAllocStart(index, memory) - REGISTER[PC]);
        ExecVMOperation();
        loadTask();
    }

    // this is debug -->

    printf("\nDumping memory:\n\n");
    FILE *file = fopen("dumphex", "wb");
    if (!file)
        exit(1);
    fwrite(memory, 1, MEMSIZE, file);
    fclose(file);

    //for (uint16_t i = 0; i < ((memory[address + 0x21] << 0x08) | memory[address + 0x22]); i++)
    //    printf("MEM[0x%x]:0x%x\n", i, memory[i]);
}

int main()
{
    start();
}