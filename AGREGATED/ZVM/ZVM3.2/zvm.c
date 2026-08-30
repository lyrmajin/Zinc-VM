#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

typedef uint8_t byte;
typedef uint16_t dbyte;
typedef uint32_t qbyte;
typedef uint64_t obyte;

// cache

#define CACHE_L1_SIZE 4096
#define CACHE_L1I_SIZE (3 / 4 * CACHE_L1_SIZE) // around 1024 Instructions.
#define CACHE_L1D_SIZE (1 / 4 * CACHE_L1_SIZE) // 256 QBytes of Data space.

__thread byte *cache;

// memory

#define MEMORY_SIZE (16 * 1024)

_Atomic byte *memory;

// registers

#define REGISTERS_COUNT 32
#define MMUREGISTERS_COUNT 8
#define IOREGISTERS_COUNT 16

__thread dbyte registers[REGISTERS_COUNT];
__thread dbyte mmuregisters[MMUREGISTERS_COUNT];
_Atomic byte ioregisters[IOREGISTERS_COUNT];

#include "zvmio.h"

// level User

#define GPR0 0 // General Porpuse Register 0
#define GPR1 1 // General Porpuse Register 1
#define GPR2 2 // General Porpuse Register 2
#define GPR3 3 // General Porpuse Register 3
#define GPR4 4 // General Porpuse Register 4
#define GPR5 5 // General Porpuse Register 5
#define GPR6 6 // General Porpuse Register 6
#define GPR7 7 // General Porpuse Register 7

#define RET 8
#define ARG0 9
#define ARG1 10
#define ARG2 11
#define ARG3 12
#define SPR5 13 // yet to be decided
#define MSH 14  // Memory Select
#define MSL 15

// level Kernel

#define IRH 16 // Instruction Register
#define IRL 17
#define PC 18  // Program Count
#define FPH 19 // Frame Pointer
#define FPL 20
#define SPH 21 // Stack Pointer
#define SPL 22
#define MMUVTP 23 // (Memory Management Unit) Virtualization Table Pointer
#define MMUATP 24 // (Memory Management Unit) Access Table Pointer
#define MMUSTP 25 // (Memory Management Unit) Segmentation Table Pointer
#define SYSATP 26 // Syscall Access Tabe Pointer
#define ISSRTP 27 // Interrupt System Sub-Rutine Table Pointer
#define SPR20 28
#define SPR21 29
#define TI 30 // Thread ID
#define FL 31 // KP SG MA 0 0 0 0 0 0 0 0 0 0 IE CU EX // Flags
/*
KP: Kernel Permition
SG: Segmentation Fault
MA: (Memory Management Unit) Activated

IE: Interrupt Enable
CU: Core Unit (Dual Core)
EX: Execute
*/

#define KERNEL_PERMITION 0x8000
#define SEGFAULT 0x4000
#define MMU_ACTIVE 0x2000

#define CORE_UNIT 0x0002
#define EXECUTE 0x0001

// Helpers

#define OPCODE_NMSK 0b00000111 // Opcode Negative Mask
#define OPCODE_PMSK 0b11111000 // Opcode Positive Mask
#define OPCODE_B 3             // Opcode Bit

void Exec_OP();
void mset(byte value, qbyte address);
byte mget(qbyte address);

void mset(byte value, qbyte address)
{
    if (!(registers[FL] & MMU_ACTIVE))
    {
        if (address < CACHE_L1_SIZE)
            cache[address] = value;
        else if (address >= CACHE_L1_SIZE && address < MEMORY_SIZE)
            memory[address - CACHE_L1_SIZE] = value;
        else
        {
            if (!(registers[FL] & SEGFAULT))
                registers[FL] |= SEGFAULT;
            else
                registers[FL] &= ~EXECUTE;
        }
        return;
    }

    for (byte i = 1; i < (memory[registers[MMUATP]] * 8) + 1; i += 8)
    {
#define AccessAddressStart ((((memory[registers[MMUATP] + i] << 8) | memory[registers[MMUATP] + i + 1]) << 16) | ((memory[registers[MMUATP] + i + 2] << 8) | memory[registers[MMUATP] + i + 3]))
#define AccessAddressEnd ((((memory[registers[MMUATP] + i + 4] << 8) | memory[registers[MMUATP] + i + 5]) << 16) | ((memory[registers[MMUATP] + i + 6] << 8) | memory[registers[MMUATP] + i + 7]))
        if (address >= AccessAddressStart && address < AccessAddressEnd)
        {
            for (byte j = 1; j < (memory[registers[MMUVTP]] * 10) + 1; j += 10)
            {
#define VirtualAddressStart ((((memory[registers[MMUVTP] + j] << 8) | memory[registers[MMUVTP] + j + 1]) << 16) | ((memory[registers[MMUVTP] + j + 2] << 8) | memory[registers[MMUVTP] + j + 3]))
#define VirtualAddressEnd ((((memory[registers[MMUVTP] + j + 4] << 8) | memory[registers[MMUVTP] + j + 5]) << 16) | ((memory[registers[MMUVTP] + j + 6] << 8) | memory[registers[MMUVTP] + j + 7]))
                if (address >= VirtualAddressStart && address < VirtualAddressEnd)
                {
                    mmuregisters[0] = registers[ARG0];
                    mmuregisters[1] = registers[ARG1];
                    mmuregisters[2] = registers[ARG2];
                    mmuregisters[3] = registers[ARG3];
                    mmuregisters[4] = registers[RET];
                    mmuregisters[5] = registers[FL];
                    mmuregisters[6] = registers[IRH];
                    mmuregisters[7] = registers[IRL];

                    registers[ARG0] = address >> 16;
                    registers[ARG1] = address;
                    registers[ARG2] = value & 0x00FF;
                    registers[ARG3] = registers[PC];

                    registers[FL] |= (KERNEL_PERMITION | ~MMU_ACTIVE);

                    registers[PC] = ((memory[registers[MMUVTP] + j + 8] << 8) | memory[registers[MMUVTP] + j + 9]);

                    while (registers[FL] & EXECUTE)
                    {
                        // MMU(V/S)TP Should point somewhere That will fit both virtualized and hardware wise, in the exact place
#define InstructionRegisterHigh ((mget(registers[PC]) << 8) | mget(registers[PC] + 1))
#define InstructionRegisterLow ((mget(registers[PC] + 2) << 8) | mget(registers[PC] + 3))

                        registers[IRH] = (InstructionRegisterHigh);
                        registers[IRL] = (InstructionRegisterLow);

#undef InstructionRegisterLow
#undef InstructionRegisterHigh

                        Exec_OP();
                    }

                    registers[PC] = registers[ARG3];

                    registers[FL] = mmuregisters[5];

                    registers[ARG0] = mmuregisters[0];
                    registers[ARG1] = mmuregisters[1];
                    registers[ARG2] = mmuregisters[2];
                    registers[ARG3] = mmuregisters[3];
                    registers[RET] = mmuregisters[4];
                    registers[IRH] = mmuregisters[6];
                    registers[IRL] = mmuregisters[7];

                    return;
                }
#undef VirtualAddressEnd
#undef VirtualAddressStart
            }
        }
#undef AccessAddressEnd
#undef AccessAddressStart
    }

    registers[FL] |= (KERNEL_PERMITION | ~MMU_ACTIVE | SEGFAULT);

    registers[PC] = registers[MMUSTP];

    while (registers[FL] & EXECUTE)
    {
        // MMU(V/S)TP Should point somewhere That will fit both virtualized and hardware wise, in the exact place
#define InstructionRegisterHigh ((mget(registers[PC]) << 8) | mget(registers[PC] + 1))
#define InstructionRegisterLow ((mget(registers[PC] + 2) << 8) | mget(registers[PC] + 3))

        registers[IRH] = (InstructionRegisterHigh);
        registers[IRL] = (InstructionRegisterLow);

#undef InstructionRegisterLow
#undef InstructionRegisterHigh

        Exec_OP();
    }
}
byte mget(qbyte address)
{
    if (!(registers[FL] & MMU_ACTIVE))
    {
        if (address < CACHE_L1_SIZE)
            return cache[address];
        else if (address >= CACHE_L1_SIZE && address < MEMORY_SIZE)
            return memory[address - CACHE_L1_SIZE];
        else
        {
            if (!(registers[FL] & SEGFAULT))
                registers[FL] |= SEGFAULT;
            else
                registers[FL] &= ~EXECUTE;
        }

        return 0;
    }
    for (byte i = 1; i < (memory[registers[MMUATP]] * 8) + 1; i += 8)
    {
#define AccessAddressStart ((((memory[registers[MMUATP] + i] << 8) | memory[registers[MMUATP] + i + 1]) << 16) | ((memory[registers[MMUATP] + i + 2] << 8) | memory[registers[MMUATP] + i + 3]))
#define AccessAddressEnd ((((memory[registers[MMUATP] + i + 4] << 8) | memory[registers[MMUATP] + i + 5]) << 16) | ((memory[registers[MMUATP] + i + 6] << 8) | memory[registers[MMUATP] + i + 7]))
        if (address >= AccessAddressStart && address < AccessAddressEnd)
        {
            for (byte j = 1; j < (memory[registers[MMUVTP]] * 10) + 1; j += 10)
            {
#define VirtualAddressStart ((((memory[registers[MMUVTP] + j] << 8) | memory[registers[MMUVTP] + j + 1]) << 16) | ((memory[registers[MMUVTP] + j + 2] << 8) | memory[registers[MMUVTP] + j + 3]))
#define VirtualAddressEnd ((((memory[registers[MMUVTP] + j + 4] << 8) | memory[registers[MMUVTP] + j + 5]) << 16) | ((memory[registers[MMUVTP] + j + 6] << 8) | memory[registers[MMUVTP] + j + 7]))
                if (address >= VirtualAddressStart && address < VirtualAddressEnd)
                {
                    mmuregisters[0] = registers[ARG0];
                    mmuregisters[1] = registers[ARG1];
                    mmuregisters[2] = registers[ARG2];
                    mmuregisters[3] = registers[ARG3];
                    mmuregisters[4] = registers[RET];
                    mmuregisters[5] = registers[FL];
                    mmuregisters[6] = registers[IRH];
                    mmuregisters[7] = registers[IRL];

                    registers[ARG0] = address >> 16;
                    registers[ARG1] = address;
                    registers[ARG2] = 0x0100;
                    registers[ARG3] = registers[PC];

                    registers[FL] |= (KERNEL_PERMITION | ~MMU_ACTIVE);

                    registers[PC] = ((memory[registers[MMUVTP] + j + 8] << 8) | memory[registers[MMUVTP] + j + 9]);

                    while (registers[FL] & EXECUTE)
                    {
                        // MMU(V/S)TP Should point somewhere That will fit both virtualized and hardware wise, in the exact place
#define InstructionRegisterHigh ((mget(registers[PC]) << 8) | mget(registers[PC] + 1))
#define InstructionRegisterLow ((mget(registers[PC] + 2) << 8) | mget(registers[PC] + 3))

                        registers[IRH] = (InstructionRegisterHigh);
                        registers[IRL] = (InstructionRegisterLow);

#undef InstructionRegisterLow
#undef InstructionRegisterHigh

                        Exec_OP();
                    }

                    registers[PC] = registers[ARG3];

                    registers[FL] = mmuregisters[5];

                    registers[ARG0] = mmuregisters[0];
                    registers[ARG1] = mmuregisters[1];
                    registers[ARG2] = mmuregisters[2];
                    registers[ARG3] = mmuregisters[3];
                    mmuregisters[1] = registers[RET];
                    registers[RET] = mmuregisters[4];
                    registers[IRH] = mmuregisters[6];
                    registers[IRL] = mmuregisters[7];

                    return (byte)mmuregisters[1];
                }
#undef VirtualAddressEnd
#undef VirtualAddressStart
            }
        }
#undef AccessAddressEnd
#undef AccessAddressStart
    }

    registers[FL] |= (KERNEL_PERMITION | ~MMU_ACTIVE | SEGFAULT);

    registers[PC] = registers[MMUSTP];

    while (registers[FL] & EXECUTE)
    {
        // MMU(V/S)TP Should point somewhere That will fit both virtualized and hardware wise, in the exact place
#define InstructionRegisterHigh ((mget(registers[PC]) << 8) | mget(registers[PC] + 1))
#define InstructionRegisterLow ((mget(registers[PC] + 2) << 8) | mget(registers[PC] + 3))

        registers[IRH] = (InstructionRegisterHigh);
        registers[IRL] = (InstructionRegisterLow);

#undef InstructionRegisterLow
#undef InstructionRegisterHigh

        Exec_OP();
    }

    return 0xFF; // Added for C language reasons, should be not reached
}

// ISA

void OP_MVI()
{

    /*
    MVI (MOVE INMEDIATE):

    Takes 1 Destination Register and loads an inmediate value which can be 16bits (Word), or Low/High of a Word.

    Optionally you can make a secundary load.

    MVI R0, #0x0000
    MVI R0, R1, #0x0000

    MVI R0H, #0x00
    MVI R0L, #0x00

    MVI R0H, R1H, #0x00, #0x00
    MVI R0L, R1L, #0x00, #0x00
    MVI R0H, R1L, #0x00, #0x00
    MVI R0L, R1H, #0x00, #0x00

    *NOTE* H or L on the register doesnt mean a new register, But as refering to the low or high of the register. (For Zinc16v3, means 1 byte)

    */

#define Rd1 (registers[IRH] & (OPCODE_NMSK << 11) >> 8) // 0b00000111 + 0x00
#define Rd2 ((registers[IRH] & 0x00E0) >> 5)            // 0x00 + 0b11100000
#define Selectors (registers[IRH] & 0x001F)             // 0x00 + 0b00011111
#define Inmediate16 registers[IRL]                      // 0xFFFF

    switch ((Selectors & 0xC) >> 2)
    {
    case 0:
        registers[PC] += 2; // Fordward Program Count 2 Bytes
        break;
    case 1: // Load Full Word
        registers[Rd1] = Inmediate16;
        registers[PC] += 4; // Fordward Program Count 4 Bytes to Include Inmediate value
        break;
    case 2: // Load High byte of Word
        registers[Rd1] = (Inmediate16 & 0xFF00);
        registers[PC] += 3; // Fordward Program Count 3 Bytes to Include Inmediate value
        break;
    case 3: // Load Low byte of Word
        registers[Rd1] = ((Inmediate16 & 0xFF00) >> 8);
        registers[PC] += 3; // Fordward Program Count 3 Bytes to Include Inmediate value
        break;
    default:
        registers[PC] += 2; // Fordward default size
        break;
    }
    if (Selectors & 0x10)
    {
        switch (Selectors & 0x3)
        {
        case 0:
            break;
        case 1: // Load Word
            registers[Rd2] = Inmediate16;
            break;
        case 2: // Load High byte of Word
            registers[Rd2] = ((Inmediate16 & 0x00FF) << 8);
            if (((Selectors & 0xC) >> 2) >= 2)
                registers[PC] += 1; // Fordward Program Count 1 Bytes to Include Inmediate value
            else if (((Selectors & 0xC) >> 2) == 0)
                registers[PC] += 2; // Fordward Program Count 2 Bytes to Include Inmediate value
            break;
        case 3: // Load Low byte of Word
            registers[Rd2] = (Inmediate16 & 0x00FF);
            if (((Selectors & 0xC) >> 2) >= 2)
                registers[PC] += 1; // Fordward Program Count 1 Bytes to Include Inmediate value
            else if (((Selectors & 0xC) >> 2) == 0)
                registers[PC] += 2; // Fordward Program Count 2 Bytes to Include Inmediate value
            break;
        default:
            break;
        }
    }

#ifdef DEBUG
    printf("Rd[%d] Rd2[%d] Sl[0x%x] Inm[0x%x]\n", Rd1, Rd2, Selectors, Inmediate16);
#endif

#undef Inmediate16
#undef Selectors
#undef Rd2
#undef Rd1
}

void OP_MOV()
{

    /*
    MOV (MOVE) Takes the values on Rs, and loads it into Rd.

    Rs can be from register 0 to register 31.

    Rd can be from register 0 to register 15.

    When the last bit is set to 1, it will load rfom register 16 to register 31, respectibly to your selection, IF the permisiion is set to kernel. (R0 Corresponds to R16, and so on)

    MOV R0, R1
    MOV R0, PC ;R18 = PC

    MOV PC, R0 ;Only works IF FL (R31) bit 0x8000 is set.

    SWAP R0, R1
    SWAP R0, PC ;Only works IF FL (R31) bit 0x8000 is set.
    SWAP PC, R0 ;Only works IF FL (R31) bit 0x8000 is set.

    */

#define Rd (((registers[IRH] & (OPCODE_NMSK << 8)) >> 7) | ((registers[IRH] & 0x0080) >> 7))
#define Rs ((registers[IRH] & 0x007C) >> 2)
#define Selectors (registers[IRH] & 0x0003)
#define Permission (registers[FL] & 0x8000)

#define KERNEL_OFFSET 0x10

    switch (Selectors)
    {
    case 0: // Load User Registers1 Rd from Rs
        registers[Rd] = registers[Rs];
        break;
    case 1: // Load Kernel Registers1 Rd from Rs
        if (Permission)
            registers[Rd + KERNEL_OFFSET] = registers[Rs];
        break;
    case 2: // swap User-All Registers1 Rd from Rs
        if (Permission || Rs < KERNEL_OFFSET)
        {
            dbyte temp = registers[Rd];
            registers[Rd] = registers[Rs];
            registers[Rs] = temp;
        }
        break;
    case 3: // sawp Kernel-All Registers1 Rd from Rs
        if (Permission)
        {
            dbyte temp = registers[Rd + KERNEL_OFFSET];
            registers[Rd + KERNEL_OFFSET] = registers[Rs];
            registers[Rs] = temp;
        }
        break;
    default:
        break;
    }

    registers[PC] += 2;

#undef KERNEL_OFFSET

#undef Permission
#undef Selectors
#undef Rs
#undef Rd
}

void OP_ST()
{
#define Rd (((registers[IRH] & (OPCODE_NMSK << 8)) >> 7) | ((registers[IRH] & 0x00C0) >> 6))
#define Rs ((registers[IRH] & 0x003E) >> 1)
#define StackFlag (registers[IRH] & 0x0001)
#define ReturnFlag (registers[IRL] & 0x8000)
#define AddressSelector ((registers[IRL] & 0x6000) >> 13)
#define ReturnSelector ((registers[IRL] & 0x1000) >> 12)
#define Rd2 ((registers[IRL] & 0x0700) >> 8)

#define BaseAddressMS ((registers[MSH] << 16) | registers[MSL])
#define BaseAddressSP ((registers[SPH] << 16) | registers[SPL])

    qbyte AddressMS, AddressSP;

    switch (AddressSelector)
    {
    case 0: // Only Base

        AddressMS = BaseAddressMS;
        AddressSP = BaseAddressSP;

        if (!StackFlag)
            mset((registers[Rs] & 0x00FF), AddressMS);
        else
        {
            mset((registers[Rs] & 0x00FF), AddressSP);
            dbyte temp = BaseAddressSP - 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 1: // Base + offset

        AddressMS = BaseAddressMS + registers[Rd];
        AddressSP = BaseAddressSP + registers[Rd];

        if (!StackFlag)
            mset((registers[Rs] & 0x00FF), AddressMS);
        else
        {
            mset((registers[Rs] & 0x00FF), AddressSP);
            dbyte temp = BaseAddressSP - 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 2: // Base - offset

        AddressMS = BaseAddressMS - registers[Rd];
        AddressSP = BaseAddressSP - registers[Rd];

        if (!StackFlag)
            mset((registers[Rs] & 0x00FF), AddressMS);
        else
        {
            mset((registers[Rs] & 0x00FF), AddressSP);
            dbyte temp = BaseAddressSP - 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 3: // Base + offset

        AddressMS = ((registers[MSH] << 16) | registers[Rd]);
        AddressSP = (((registers[SPH] << 16) | registers[Rd]) - 1);

        if (!StackFlag)
            mset((registers[Rs] & 0x00FF), AddressMS);
        else
        {
            mset((registers[Rs] & 0x00FF), AddressSP);
            dbyte temp = BaseAddressSP - 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    default:
        break;
    }

    if (ReturnFlag)
        switch (ReturnSelector)
        {
        case 0:
            if (StackFlag)
                registers[Rd2] = AddressSP >> 16;
            else
                registers[Rd2] = AddressMS >> 16;
            break;
        case 1:
            if (StackFlag)
                registers[Rd2] = AddressSP;
            else
                registers[Rd2] = AddressMS;
            break;
        default:
            break;
        }

    registers[PC] += 3;

#undef BaseAddressSP
#undef BaseAddressMS

#undef Rd2
#undef ReturnSelector
#undef AddressSelector
#undef ReturnFlag
#undef StackFlag
#undef Rs
#undef Rd
}

void OP_LD()
{
#define Rs (((registers[IRH] & (OPCODE_NMSK << 8)) >> 7) | ((registers[IRH] & 0x00C0) >> 6))
#define Rd ((registers[IRH] & 0x003E) >> 1)
#define StackFlag (registers[IRH] & 0x0001)
#define ReturnFlag (registers[IRL] & 0x8000)
#define AddressSelector ((registers[IRL] & 0x6000) >> 13)
#define ReturnSelector ((registers[IRL] & 0x1000) >> 12)
#define Rd2 ((registers[IRL] & 0x0700) >> 8)

#define BaseAddressMS ((registers[MSH] << 16) | registers[MSL])
#define BaseAddressSP ((registers[SPH] << 16) | registers[SPL])

    qbyte AddressMS, AddressSP;

    switch (AddressSelector)
    {
    case 0: // Only Base

        AddressMS = BaseAddressMS;
        AddressSP = BaseAddressSP + 1;

        if (!StackFlag)
            registers[Rd] = mget(AddressMS);
        else
        {
            registers[Rd] = mget(AddressSP);
            dbyte temp = BaseAddressSP + 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 1: // Base + offset

        AddressMS = BaseAddressMS + registers[Rs];
        AddressSP = BaseAddressSP + registers[Rs] + 1;

        if (!StackFlag)
            registers[Rd] = mget(AddressMS);
        else
        {
            registers[Rd] = mget(AddressSP);
            dbyte temp = BaseAddressSP + 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 2: // Base - offset

        AddressMS = BaseAddressMS - registers[Rs];
        AddressSP = BaseAddressSP - registers[Rs] + 1;

        if (!StackFlag)
            registers[Rd] = mget(AddressMS);
        else
        {
            registers[Rd] = mget(AddressSP);
            dbyte temp = BaseAddressSP + 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    case 3: // Base + offset

        AddressMS = ((registers[MSH] << 16) | registers[Rs]);
        AddressSP = (((registers[SPH] << 16) | registers[Rs]) + 1);

        if (!StackFlag)
            registers[Rd] = mget(AddressMS);
        else
        {
            registers[Rd] = mget(AddressSP);
            dbyte temp = BaseAddressSP + 1;
            registers[SPH] = (temp >> 16);
            registers[SPL] = temp;
        }
        break;
    default:
        break;
    }

    if (ReturnFlag)
        switch (ReturnSelector)
        {
        case 0:
            if (StackFlag)
                registers[Rd2] = AddressSP >> 16;
            else
                registers[Rd2] = AddressMS >> 16;
            break;
        case 1:
            if (StackFlag)
                registers[Rd2] = AddressSP;
            else
                registers[Rd2] = AddressMS;
            break;
        default:
            break;
        }

    registers[PC] += 3;

#undef BaseAddressSP
#undef BaseAddressMS

#undef Rd2
#undef ReturnSelector
#undef AddressSelector
#undef ReturnFlag
#undef StackFlag
#undef Rs
#undef Rd
}

void OP_ADD()
{
}

void Exec_OP()
{
}

int main()
{
    memory = (_Atomic byte *)malloc(MEMORY_SIZE * sizeof(byte));
    cache = (byte *)malloc(CACHE_L1_SIZE * sizeof(byte));

    for (byte i = 0; i < ((REGISTERS_COUNT*2) + (MMUREGISTERS_COUNT*2) + IOREGISTERS_COUNT); i++)
        printf("R[%2d]: 0x%x\n", i, ioread(i));

    free(cache);
    free(memory);

    return 0;
}