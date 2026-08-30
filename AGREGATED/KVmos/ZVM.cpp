#include <iostream>
#include <fstream>
#include <bitset>
#include <iomanip>
#include <vector>
#include <string.h>

#define MAX_TASK 16

#define DEBUG_BUILD
#define X86_64

#ifdef DEBUG_BUILD
uint16_t HP = 0;

#if defined(ATMEGA2560)
#define SRAM_SIZE (8 * 1024)
#define MEM_SIZE (6 * 1024)
#define FLASH_SIZE (256 * 1024)
#elif defined(ATMEGA328P)
#define SRAM_SIZE (2 * 1024)
#define MEM_SIZE 1024
#define FLASH_SIZE (32 * 1024)
#elif defined(X86_64)
#define SRAM_SIZE (0 * 1024)
#define MEM_SIZE (32 * 1024)
#define FLASH_SIZE (0 * 1024)
#endif

#else

#if defined(ATMEGA2560)
#define SRAM_SIZE (8 * 1024)
#define MEM_SIZE (7 * 1024) + 512 + 256
#define FLASH_SIZE (256 * 1024)
#elif defined(ATMEGA328P)
#define SRAM_SIZE (2 * 1024)
#define MEM_SIZE 1024 + 512 + 256
#define FLASH_SIZE (32 * 1024)
#elif defined(X86_64)
#define SRAM_SIZE (0 * 1024)
#define MEM_SIZE (64 * 1024)
#define FLASH_SIZE (0 * 1024)
#endif

#endif

uint8_t MEMORY[MEM_SIZE];

#define MAX_REG 8
#define GPR0 0 // General Porpuse Register 0
#define GPR1 1 // General Porpuse Register 1
#define GPR2 2 // General Porpuse Register 2
#define GPR3 3 // General Porpuse Register 3
#define PC 4   // Program Count
#define SPI 5  // Stack Pointer Index
#define FP 6   // Frame Pointer
#define TI 7   // Thread Id

bool debug_glob = true;

uint16_t REGISTER[MAX_REG] = {0};
uint8_t FL = 0; // RUNTIME SYSTEM SEGFAULT 0 0 0 0 CMP

//----------------------------------------------
void SysWriteIO(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "MEMORY:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ((uint16_t(MEMORY[argptr]) << 8) | MEMORY[argptr + 1]) << "]:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ((uint16_t(MEMORY[argptr + 2]) << 8) | MEMORY[argptr + 3]) << "]W\n";
    switch ((MEMORY[argptr] << 0x08) | MEMORY[argptr + 1])
    {
    case 0x00C6:
        if (!debug_glob)
            std::cout << static_cast<char>((MEMORY[argptr + 2] << 0x08) | MEMORY[argptr + 3]);
        else
            std::cout << "CHAR:[" << static_cast<char>((MEMORY[argptr + 2] << 0x08) | MEMORY[argptr + 3]) << "]\n";
        break;
    }
}
void SysReadIO(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "MEMORY:[0x" << std::hex << ((MEMORY[argptr] << 0x08) | MEMORY[argptr + 1]) << "]:[0x" << std::hex << ((MEMORY[argptr + 2] << 0x08) | MEMORY[argptr + 3]) << "]R\n";
    switch ((MEMORY[argptr] << 0x08) | MEMORY[argptr + 1])
    {
    case 0x00c0:
        REGISTER[3] = 0x0020;
        break;
    case 0x00C6:
        scanf("%s", reinterpret_cast<char *>(&MEMORY[((MEMORY[argptr + 2] << 0x08) | MEMORY[argptr + 3])]));
        break;
    }
}
void SysScheduleTask(uint16_t argptr) {}
void SysKillTask(uint16_t argptr) {}
void SysMemAllocate(uint16_t argptr) {}
void SysMemFree(uint16_t argptr) {}
// 6
void SysGetPC(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "PC:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[PC] << "]\n";
    REGISTER[3] = REGISTER[PC];
}
// 7
void SysGetSP(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "SP:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[SPI] << "]\n";
    REGISTER[3] = REGISTER[SPI];
}
// 8
void SysGetFP(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "FP:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[FP] << "]\n";
    REGISTER[3] = REGISTER[FP];
}
// 9
void SysGetTI(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "TI:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[TI] << "]\n";
    REGISTER[3] = REGISTER[TI];
}
void SysHalt(uint16_t argptr) {}
// B
void SysKill(uint16_t argptr)
{
    if (debug_glob)
        std::cout << "KILLED" << '\n';
    FL = 0;
}
void SysOpen(uint16_t argptr) {}
void SysExec(uint16_t argptr) {}
void SysNOP(uint16_t argptr) {}

typedef void (*SYSCALL)(uint16_t);
const SYSCALL SYSCALLS[15] = {SysWriteIO, SysReadIO, SysNOP, SysNOP, SysNOP, SysNOP, SysGetPC, SysGetSP, SysGetTI, SysNOP, SysKill, SysNOP, SysNOP};

inline void OP_MVI()
{ // 0000
    if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[MVIH " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint16_t)MEMORY[REGISTER[PC] + 1] << "]\n";
        REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = (MEMORY[REGISTER[PC] + 1] << 0x08);
    }
    else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[MVIL " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << MEMORY[REGISTER[PC] + 1] << "]\n";
        REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = MEMORY[REGISTER[PC] + 1];
    }
    else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 2]) << "]--->[MVI " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ((MEMORY[REGISTER[PC] + 1] << 0x08) | MEMORY[REGISTER[PC] + 2]) << "]\n";
        REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = ((MEMORY[REGISTER[PC] + 1] << 0x08) | MEMORY[REGISTER[PC] + 2]);
        REGISTER[PC] += 3;
        return;
    }
    REGISTER[PC] += 2;
}
inline void OP_MOV()
{ // 0001
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[MOV R" << (uint16_t)((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_ADD()
{ // 0010
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[ADD R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_SUB()
{ // 0011
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SUB R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_AND()
{ // 0100
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[AND R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] &= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_OR()
{ // 0101
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[OR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] |= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_XOR()
{ // 0110
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[XOR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_CMP()
{ // 0111
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[CMP R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    FL &= 0b11111100;
    if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) == 0)
        FL |= 0b00000001; // Z flag
    if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) != 0)
        FL |= 0b00000000; // N flag
    REGISTER[PC]++;
}
inline void OP_LD()
{ // 1000
    uint16_t address = 0;
    if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000)
    {
        if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[LDA R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
            address = (((0x200 / MAX_TASK) * REGISTER[TI]) + REGISTER[(MEMORY[REGISTER[PC] + 1] & 0b00000011)]);
        }
        else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100)
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[LDR R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
            address = (((0x100 + (0x200 / MAX_TASK) * REGISTER[TI])) + REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
        }
    }
    else
    {
        if (debug_glob)
        {
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[POP R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << "]\n";
            HP--;
        }
        address = (REGISTER[SPI] + 1);

        REGISTER[SPI]++;
    }
    REGISTER[(MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06] = MEMORY[address];
    if (debug_glob)
    {
        std::cout << "ADDRESS:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << address << "]:DATA:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << MEMORY[address] << "]\n";
        HP++;
    }
    REGISTER[PC] += 2;
}
inline void OP_ST()
{ // 1001
    uint16_t address = 0;
    if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000)
    {
        if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[STA R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
            address = ((0x200 / MAX_TASK) * REGISTER[TI]) + REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
        }
        else
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[STR R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
            address = (((0x100 + (0x200 / MAX_TASK) * REGISTER[TI])) + REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
        }
    }
    else
    {
        if (debug_glob)
        {
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[PUSH R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << "]\n";
            HP--;
        }
        address = ((REGISTER[SPI]));

        REGISTER[SPI]--;
    }
    MEMORY[address] = REGISTER[((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06)];
    if (debug_glob)
    {
        std::cout << "ADDRESS:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << address << "]:DATA:[0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << REGISTER[((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06)] << "]\n";
        HP++;
    }
    REGISTER[PC] += 2;
}
inline void OP_JMP()
{ // 1010
    if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000000))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[JMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    }
    else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000100))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RJMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    }
    else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001000))
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[NJMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        REGISTER[PC] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    }
    else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001100))
    {
        if ((MEMORY[REGISTER[PC]] & 0b00000011) != 3)
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[CALL R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
            MEMORY[REGISTER[SPI]] = REGISTER[FP];
            MEMORY[REGISTER[SPI] - 1] = (REGISTER[FP] >> 0x08);
            MEMORY[REGISTER[SPI] - 2] = REGISTER[PC] + 1;
            MEMORY[REGISTER[SPI] - 3] = ((REGISTER[PC] + 1) >> 0x08);
            REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
            REGISTER[FP] = REGISTER[SPI];
            REGISTER[SPI] -= 4;
        }
        else
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RET]\n";
            REGISTER[PC] = ((MEMORY[(REGISTER[FP] - 3)] << 0x08) | MEMORY[(REGISTER[FP] - 2)]);
            REGISTER[SPI] = REGISTER[FP];
            REGISTER[FP] = ((MEMORY[(REGISTER[FP] - 1)] << 0x08) | MEMORY[(REGISTER[FP])]);
        }
    }
    else
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[COULD NOT JUMP]\n";
        REGISTER[PC]++;
    }
}
inline void OP_BRN()
{ // 1011
    if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && (FL & 0b00000001)))
    {
        if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[BRE R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
            REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RBRE R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
            REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
        }
    }
    else if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && !(FL & 0b00000001)))
    {
        if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[BRQ R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
            REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
        {
            if (debug_glob)
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RBRQ R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
            REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
        }
    }
    else
    {
        if (debug_glob)
            std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[COULD NOT BRANCH]\n";
        REGISTER[PC]++;
    }
}
inline void OP_SHL()
{ // 1100
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SHL R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_SHR()
{ // 1101
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SHR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] >> REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    REGISTER[PC]++;
}
inline void OP_SCB()
{ // 1110
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SCB R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= (1 << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
    REGISTER[PC]++;
}
inline void OP_SYS()
{ // 1111
    if (debug_glob)
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SYS R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
    SYSCALLS[REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 2]](REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
    REGISTER[PC]++;
}
typedef void (*OPCODE)();
const OPCODE OPCODES[16] = {OP_MVI, OP_MOV, OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_CMP, OP_LD, OP_ST, OP_JMP, OP_BRN, OP_SHL, OP_SHR, OP_SCB, OP_SYS};

inline void storeTask()
{
    if (MEMORY[REGISTER[TI]])
    {
        MEMORY[MEMORY[REGISTER[TI]]] = REGISTER[0];
        MEMORY[MEMORY[REGISTER[TI]] + 1] = (REGISTER[0] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 2] = REGISTER[1];
        MEMORY[MEMORY[REGISTER[TI]] + 3] = (REGISTER[1] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 4] = REGISTER[2];
        MEMORY[MEMORY[REGISTER[TI]] + 5] = (REGISTER[2] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 6] = REGISTER[3];
        MEMORY[MEMORY[REGISTER[TI]] + 7] = (REGISTER[3] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 8] = REGISTER[PC];
        MEMORY[MEMORY[REGISTER[TI]] + 9] = (REGISTER[PC] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 10] = REGISTER[SPI];
        MEMORY[MEMORY[REGISTER[TI]] + 11] = (REGISTER[SPI] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 12] = REGISTER[FP];
        MEMORY[MEMORY[REGISTER[TI]] + 13] = (REGISTER[FP] >> 0x08);
        MEMORY[MEMORY[REGISTER[TI]] + 14] = FL;
    }
}
inline void loadTask()
{
    if (MEMORY[REGISTER[TI]])
    {
        REGISTER[0] = (MEMORY[MEMORY[REGISTER[TI]]] | (MEMORY[MEMORY[REGISTER[TI]] + 1] << 0x08));
        REGISTER[1] = (MEMORY[MEMORY[REGISTER[TI]] + 2] | (MEMORY[MEMORY[REGISTER[TI]] + 3] << 0x08));
        REGISTER[2] = (MEMORY[MEMORY[REGISTER[TI]] + 4] | (MEMORY[MEMORY[REGISTER[TI]] + 5] << 0x08));
        REGISTER[3] = (MEMORY[MEMORY[REGISTER[TI]] + 6] | (MEMORY[MEMORY[REGISTER[TI]] + 7] << 0x08));
        REGISTER[PC] = (MEMORY[MEMORY[REGISTER[TI]] + 8] | (MEMORY[MEMORY[REGISTER[TI]] + 9] << 0x08));
        REGISTER[SPI] = (MEMORY[MEMORY[REGISTER[TI]] + 10] | (MEMORY[MEMORY[REGISTER[TI]] + 11] << 0x08));
        REGISTER[FP] = (MEMORY[MEMORY[REGISTER[TI]] + 12] | (MEMORY[MEMORY[REGISTER[TI]] + 13] << 0x08));
        FL = MEMORY[MEMORY[REGISTER[TI]] + 14];
    }
}

void VMloop()
{
    while ((FL & 0b01000000))
    {
        if ((FL & 0b10000000))
            OPCODES[(MEMORY[REGISTER[PC]] & 0xF0) >> 4]();
        storeTask();
        REGISTER[TI] = (REGISTER[TI] & 0x0F) + 1;
        loadTask();
    }
}

#define VMsetup() (FL |= 0b11000000)

uint16_t DREG[MAX_REG] = {0};

int main(int argc, char **argv)
{
    bool mem = false, stack = false, heap = false, steped = false, tags = false;
    for (uint8_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-f"))
            debug_glob = false;
        else if (!strcmp(argv[i], "-m"))
        {
            mem = true;
            if (debug_glob)
                std::cout << "MEM" << '\n';
        }
        else if (!strcmp(argv[i], "-s"))
        {
            stack = true;
            if (debug_glob)
                std::cout << "STACK" << '\n';
        }
        else if (!strcmp(argv[i], "-h"))
        {
            heap = true;
            if (debug_glob)
                std::cout << "HEAP" << '\n';
        }
        else if (!strcmp(argv[i], "-st"))
        {
            steped = true;
            if (debug_glob)
                std::cout << "STEPED" << '\n';
        }
        else if (std::string arg = argv[i]; arg.find("-org:0x") != std::string::npos)
        {
            REGISTER[PC] = std::stoi(arg.substr(arg.find("x") + 1), NULL, 16);
            if (debug_glob)
                std::cout << "PC:" << std::hex << REGISTER[PC] << '\n';
        }
        else if (std::string arg = argv[i]; arg.find(".tgs") != std::string::npos)
        {
            tags = true;
            if (debug_glob)
                std::cout << "TAGS\n";
        }
    }
    std::ifstream file(argv[1], std::ios::binary | std::fstream::ate);
    size_t size = file.tellg();
    file.seekg(0);
    file.read(reinterpret_cast<char *>(MEMORY), size);
    file.close();
    if (mem)
        for (uint16_t i = 0; i < size; i++)
            std::cout << "MEM[" << +i << "]:[0x" << std::hex << std::setfill('0') << std::uppercase << (uint16_t)MEMORY[i] << "]\n";

    std::vector<std::string> Tags;
    std::vector<uint16_t> Tpcs;
    if (tags)
    {
        std::string istr;
        file.open(strcat(argv[1], ".tgs"));
        while (std::getline(file, istr))
        {
            if (istr.find(':') != std::string::npos)
            {
                Tags.push_back(istr.substr(0, istr.find(':')));
                Tpcs.push_back(std::stoi(istr.substr(istr.find(':') + 1)));
                std::cout << Tags[Tags.size() - 1] << ":" << Tpcs[Tpcs.size() - 1] << '\n';
            }
        }
    }
    VMsetup();
    REGISTER[SPI] = MEM_SIZE - 1;
    REGISTER[FP] = MEM_SIZE - 1;
    // VMloop();
    for (uint8_t i = 0; i < MAX_REG; i++)
        if (debug_glob)
            std::cout << "REG[" << +i << "]:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[i] << "]\n";
    if (debug_glob)
        std::cout << "FL:[0b" << std::bitset<8>(FL) << "]\n";
    while ((FL & 0b01000000))
    {
        OPCODES[(MEMORY[REGISTER[PC]] & 0xF0) >> 4]();
        if (tags)
            for (uint16_t i = 0; i < Tags.size(); i++)
                if (REGISTER[4] == Tpcs[i])
                    std::cout << "TAG:[" << Tags[i] << "]\n";
        if (heap)
            for (uint16_t i = (0x100 + (REGISTER[TI] * (0x200 / MAX_TASK))); i < (0x100 + ((REGISTER[TI] * (0x200 / MAX_TASK)) + HP)); i++)
                std::cout << "HEAP[" << +i << "]:[0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (uint16_t)MEMORY[i] << "]\n";
        for (uint8_t i = 0; i < MAX_REG; i++)
            if (debug_glob)
            {
                std::cout << "REG[" << +i << "]:[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << REGISTER[i];
                if (DREG[i] == REGISTER[i])
                    std::cout << "]\n";
                else
                    std::cout << "]<--[0x" << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << DREG[i] << "]\n";
            }
        if (stack)
            for (uint16_t i = (MEM_SIZE - 1); i > REGISTER[SPI]; i--)
                std::cout << "STACK[" << +i << "]:[0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (uint16_t)MEMORY[i] << "]\n";
        if (debug_glob)
            std::cout << "FL:[0b" << std::bitset<8>(FL) << "]\n";
        if (steped)
            std::cin.get();
        for (uint8_t i = 0; i < MAX_REG; i++)
            DREG[i] = REGISTER[i];
    }
    return REGISTER[(MEMORY[REGISTER[PC] - 1] & 0b00000011)];
}
