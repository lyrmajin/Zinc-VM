#include "abi.h"
#include "zfs2.h"
#include "zalloc.h"
#include "zvm.h"

// ZEFI -----

uint8_t getZEFI(uint16_t offset, memoryget_t memget)
{
    return PRINTFRETURN("Requested ZEFI number (%u)\n", memget(offset));
}
uint8_t getFunctionsCount(uint16_t offset, memoryget_t memget)
{
    return PRINTFRETURN("Requested Function Count (%u)\n", memget(offset + 1));
}
uint16_t getFunctionAddress(uint8_t index, uint16_t offset, memoryget_t memget)
{
    return PRINTFRETURN("Requested Function Address (0x%x)\n", ((memget(offset + (index * 2)) << 0x08) | memget(offset + (index * 2) + 1)));
}
uint8_t getHeapPreallocSize(uint16_t offset, memoryget_t memget)
{
    printf("Requested Heap Preallocation Size{\n");
    uint16_t count = getFunctionsCount(offset, memget);
    return PRINTFRETURN("} Success! (0x%x)\n", memget(offset + (count * 2) + 2));
}
uint8_t getStackPreallocSize(uint16_t offset, memoryget_t memget)
{
    printf("Requested Stack Preallocation Size{\n");
    uint16_t count = getFunctionsCount(offset, memget);
    return PRINTFRETURN("} Success! (0x%x)\n", memget(offset + (count * 2) + 3));
}
uint16_t getCodePreallocSize(uint16_t offset, memoryget_t memget)
{
    printf("Requested Code Preallocation Size{\n");
    uint16_t count = getFunctionsCount(offset, memget);
    return PRINTFRETURN("} Success! (0x%x)\n", ((memget(offset + (count * 2) + 4) << 0x08) | memget(offset + (count * 2) + 5)));
}
uint16_t getCodeAddress(uint16_t offset, memoryget_t memget)
{
    uint16_t count = getFunctionsCount(offset, memget);
    return (offset + (count * 2) + 6);
}

// TASK ----------

uint8_t getTaskId(uint8_t index, memoryget_t memget)
{
    return getEntryType(index, TASKTABLEFILE, memget);
}
uint16_t getTaskMetadata(uint8_t index, memoryget_t memget)
{
    for (uint16_t i = 0; i < 8; i++)
        if (getEntryType(i, TASKTABLEFILE, memget) == index)
            return getEntryAddress(i, TASKTABLEFILE, memget);
    return 0xFFFF;
}
uint16_t getModuleMetadata(uint8_t index, memoryget_t memget)
{
    return getEntryAddress(index, MODULETABLEFILE, memget);
}

uint16_t getRegister(uint8_t index, uint16_t offset, memoryget_t memget)
{
    return ((memget(offset + (index * 2)) << 0x08) | memget(offset + (index * 2) + 1));
}
void setRegister(uint8_t index, uint16_t value, uint16_t offset, memoryset_t memset)
{
    memset(value >> 0x08, (offset + (index * 2)));
    memset(value, (offset + (index * 2) + 1));
}

uint8_t makeTask(uint8_t Taskid, uint16_t name, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    uint8_t entry = makeEntry(Taskid, HOF, offset, memset, memget);
    char *tmp = (char *)calloc(strlenv(name, memget), 1);
    strcpyv(name, tmp, memget);
    makeFile(tmp, 18, NULL, HOF, HOF, memset, memget); // for 8 16b registers + code address

    HOF -= getFileSize(HOF, memget);
    return entry;
}
uint16_t getTaskCodeAlloc(uint8_t taskid, memoryget_t memget)
{
    uint16_t metadata = getTaskMetadata(taskid, memget);
    return getRegister(9, getFileDataAddress(metadata, memget), memget); // return the data from file
}
uint16_t getModuleCodeAlloc(uint8_t taskid, memoryget_t memget)
{
    uint16_t metadata = getModuleMetadata(taskid, memget);
    return getRegister(9, getFileDataAddress(metadata, memget), memget); // return the data from file
}

// ABI -----

typedef void SYSCALL;

SYSCALL SYS_0xFFF0_EXIT(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // exit
SYSCALL SYS_0xFFF1_SPAWN(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // spawn
SYSCALL SYS_0xFFF2_WAIT(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // wait
SYSCALL SYS_0xFFF3_KILL(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // kill
SYSCALL SYS_0xFFF4_READ(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{ // size[1] file[2] buffer[2]
    if (size < 5)
        return;
    uint8_t *buffer = (uint8_t *)malloc(memget(argptr));
    readFileData(buffer, memget(argptr), ((memget(argptr + 1) << 0x08) | memget(argptr + 2)), memget);
    memcpyv(buffer, ((memget(argptr + 3) << 0x08) | memget(argptr + 4)), memget(argptr), memget);
    free(buffer);
} // read
SYSCALL SYS_0xFFF5_WRITE(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{ // size[1] file[2] buffer[2]
    if (size < 5)
        return;
    uint8_t *buffer = (uint8_t *)malloc(memget(argptr));
    vmemcpy(((memget(argptr + 3) << 0x08) | memget(argptr + 4)), buffer, memget(argptr), memset);
    writeFileData(buffer, memget(argptr), ((memget(argptr + 1) << 0x08) | memget(argptr + 2)), memset, memget);
    free(buffer);
} // write
SYSCALL SYS_0xFFF6_OPEN(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // open
SYSCALL SYS_0xFFF7_CLOSE(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // close
SYSCALL SYS_0xFFF8_IO_READ(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // io read
SYSCALL SYS_0xFFF9_IO_WRITE(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // io write
SYSCALL SYS_0xFFFA_CALL(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // call
SYSCALL SYS_0xFFFB_LOAD(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // load
SYSCALL SYS_0xFFFC_ALLOC(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // alloc
SYSCALL SYS_0xFFFD_FREE(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // free
SYSCALL SYS_0xFFFE_SLEEP(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{

} // sleep
SYSCALL SYS_0xFFFF_REBOOT(uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{
    exit(memget(argptr));
    // the hell am i supposed to know how to do this for x86 version. (No way i am making this call the real sys reboot)
} // reboot

void call(uint8_t type, uint16_t offset, uint8_t size, uint16_t argptr, memoryset_t memset, memoryget_t memget)
{
    switch (type)
    {
    case 0:
        switch (offset)
        {
        case 0xFFF0: // exit
            SYS_0xFFF0_EXIT(size, argptr, memset, memget);
            return;
        case 0xFFF1: // spawn
            SYS_0xFFF1_SPAWN(size, argptr, memset, memget);
            return;
        case 0xFFF2: // wait
            SYS_0xFFF2_WAIT(size, argptr, memset, memget);
            return;
        case 0xFFF3: // kill
            SYS_0xFFF3_KILL(size, argptr, memset, memget);
            return;
        case 0xFFF4: // read
            SYS_0xFFF4_READ(size, argptr, memset, memget);
            return;
        case 0xFFF5: // write
            SYS_0xFFF5_WRITE(size, argptr, memset, memget);
            return;
        case 0xFFF6: // open
            SYS_0xFFF6_OPEN(size, argptr, memset, memget);
            return;
        case 0xFFF7: // close
            SYS_0xFFF7_CLOSE(size, argptr, memset, memget);
            return;
        case 0xFFF8: // io read
            SYS_0xFFF8_IO_READ(size, argptr, memset, memget);
            return;
        case 0xFFF9: // io write
            SYS_0xFFF9_IO_WRITE(size, argptr, memset, memget);
            return;
        case 0xFFFA: // call
            SYS_0xFFFA_CALL(size, argptr, memset, memget);
            return;
        case 0xFFFB: // load
            SYS_0xFFFB_LOAD(size, argptr, memset, memget);
            return;
        case 0xFFFC: // alloc
            SYS_0xFFFC_ALLOC(size, argptr, memset, memget);
            return;
        case 0xFFFD: // free
            SYS_0xFFFD_FREE(size, argptr, memset, memget);
            return;
        case 0xFFFE: // sleep
            SYS_0xFFFE_SLEEP(size, argptr, memset, memget);
            return;
        case 0xFFFF: // reboot
            SYS_0xFFFF_REBOOT(size, argptr, memset, memget);
            return;
        default:
            return;
        }
    case 1:
    {
        uint8_t LTI = REGISTER[TI];
        REGISTER[TI] = 0; // Calling makes the mem access kernel mode, which means is unrestricted
        uint16_t dataAddress = getFileDataAddress(offset, memget);
        if (!getZEFI(dataAddress, memget))
            return;
        uint8_t CTI = (LTI != 0xFF) ? ++LTI : 2; // <-- this should be supplemented with a check for existent task with that id
        uint16_t heap = memAlloc(CTI, getHeapPreallocSize(dataAddress, memget), memory);
        uint16_t stack = memAlloc(CTI, getStackPreallocSize(dataAddress, memget), memory);
        uint16_t code = memAlloc(CTI, 2 * getCodePreallocSize(dataAddress, memget), memory);

        vmemcpyv((argptr - size), ((stack + getStackPreallocSize(dataAddress, memget)) - size), size, memset, memget);

        vmemcpyv(getCodeAddress(dataAddress, memget), code, getCodePreallocSize(dataAddress, memget), memset, memget);

        uint8_t entry = makeTask(CTI, getFileName(offset, memget), TASKTABLEFILE, memset, memget);

        setRegister(8, code, getFileDataAddress(getEntryAddress(entry, TASKTABLEFILE, memget),memget), memset);
        REGISTER[PC] = code;
        REGISTER[TI] = LTI;
        scheduleTask(CTI, code);
        return;
    }
    case 2:
    {
        uint8_t LTI = REGISTER[TI];
        REGISTER[TI] = 0; // Calling makes the mem access kernel mode, which means is unrestricted
        uint16_t dataAddress = getFileDataAddress(offset, memget);
        if (!getZEFI(dataAddress, memget))
            return;
        uint16_t code = memAlloc(LTI, 2 * getCodePreallocSize(dataAddress, memget), memory);

        vmemcpyv(getCodeAddress(dataAddress, memget), code, getCodePreallocSize(dataAddress, memget) * 2, memset, memget);

        makeTask(LTI, getFileName(offset, memget), getFileDataAddress(LIBDIR, memget), memset, memget);

        REGISTER[TI] = LTI;
        return;
    }
    default:
    {
        uint8_t LTI = REGISTER[TI];
        REGISTER[TI] = 0; // Calling makes the mem access kernel mode, which means is unrestricted
        uint16_t dataAddress = getFileDataAddress(offset, memget);
        if (!getZEFI(dataAddress, memget))
            return;

        uint8_t count = getFunctionsCount(dataAddress, memget);
        uint8_t index = (size) ? memget(argptr) : 0xFF; // you cannot load more than 0xFF functions at the time per file.
        if (index >= count || index == 0xFF)
            return;

        // get the code pointer
        uint16_t code = getModuleCodeAlloc(LTI, memget);

        REGISTER[PC] = code + getFunctionAddress(index, dataAddress, memget);

        REGISTER[TI] = LTI;
        return;
    }
    }
}

void syscall(uint16_t number, uint16_t argptr, uint8_t size, memoryset_t memset, memoryget_t memget)
{
    uint16_t dataAdress = getFileDataAddress(SYSCALLTABLEFILE, memget);
    // from 0 - 5 we have the syscalls that use internal BURNED IN CODE calls. (syswriteio sysreadio syswrite sysread sysalloc sysfree, etc). the number might get increased with time.
    call((number > 6) ? 0 : 3, ((memget(dataAdress + (number * 2)) << 0x08) | memget(dataAdress + (number * 2) + 1)), size, argptr, memset, memget);
}

// 13