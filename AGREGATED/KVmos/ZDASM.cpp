#include <iostream>
#include <fstream>
#include <bitset>
#include <iomanip>
#include <vector>
#include <string.h>

#define MEM_SIZE (32 * 1024)

uint8_t MEMORY[MEM_SIZE];

#define MAX_REG 8
#define PC 4 // Program Count

uint16_t REGISTER[MAX_REG] = {0};
uint8_t FL = 0;

inline void OP_MVI()
{ // 0000
        if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[MVIH " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint16_t)MEMORY[REGISTER[PC] + 1] << "]\n";
                REGISTER[PC] += 2;
        }
        else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[MVIL " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << MEMORY[REGISTER[PC] + 1] << "]\n";
                REGISTER[PC] += 2;
        }

        else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 2]) << "]--->[MVI " << "R" << (MEMORY[REGISTER[PC]] & 0b00000011) << ", #0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ((MEMORY[REGISTER[PC] + 1] << 0x08) | MEMORY[REGISTER[PC] + 2]) << "]\n";
                REGISTER[PC] += 3;
        }
}
inline void OP_MOV()
{ // 0001
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[MOV R" << (uint16_t)((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_ADD()
{ // 0010
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[ADD R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_SUB()
{ // 0011
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SUB R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_AND()
{ // 0100
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[AND R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_OR()
{ // 0101
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[OR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_XOR()
{ // 0110
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[XOR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_CMP()
{ // 0111
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[CMP R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";

        REGISTER[PC] += 1;
}
inline void OP_LD()
{ // 1000
        if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000)
        {
                if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[LDA R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
                }
                else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100)
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[LDR R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
                }
        }
        else
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[POP R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << "]\n";
        }
        REGISTER[PC] += 2;
}
inline void OP_ST()
{ // 1001
        if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000)
        {
                if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[STA R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
                }
                else
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[STR R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << ", R" << (uint16_t)(MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
                }
        }
        else
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]:[0b" << std::bitset<8>(MEMORY[REGISTER[PC] + 1]) << "]-->[PUSH R" << (uint16_t)((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06) << "]\n";
        }
        REGISTER[PC] += 2;
}
inline void OP_JMP()
{ // 1010
        if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000000))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[JMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000100))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RJMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001000))
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[NJMP R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001100))
        {
                if ((MEMORY[REGISTER[PC]] & 0b00000011) != 3)
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[CALL R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
                }
                else
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RET]\n";
                }
        }
        else
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[COULD NOT JUMP]\n";
        }
        REGISTER[PC] += 1;
}
inline void OP_BRN()
{ // 1011
        if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && (FL & 0b00000001)))
        {
                if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[BRE R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
                }
                else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RBRE R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
                }
        }
        else if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && !(FL & 0b00000001)))
        {
                if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[BRQ R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
                }
                else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
                {
                        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[RBRQ R" << (MEMORY[REGISTER[PC]] & 0b000011) << "]\n";
                }
        }
        else
        {
                std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[COULD NOT BRANCH]\n";
        }
        REGISTER[PC] += 1;
}
inline void OP_SHL()
{ // 1100
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SHL R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_SHR()
{ // 1101
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SHR R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_SCB()
{ // 1110
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SCB R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        REGISTER[PC] += 1;
}
inline void OP_SYS()
{ // 1111
        std::cout << "MEMORY:[0b" << std::bitset<8>(MEMORY[REGISTER[PC]]) << "]->[SYS R" << ((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) << ',' << " R" << (MEMORY[REGISTER[PC]] & 0b00000011) << "]\n";
        if (((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02) == 0x0A)
                exit(0);
        else
                REGISTER[PC] += 1;
}

typedef void (*OPCODE)();
const OPCODE OPCODES[16] = {OP_MVI, OP_MOV, OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_CMP, OP_LD, OP_ST, OP_JMP, OP_BRN, OP_SHL, OP_SHR, OP_SCB, OP_SYS};

int main(int argc, char **argv)
{
        std::ifstream file(argv[1], std::ios::binary | std::fstream::ate);
        if (!file)
                return 1;
        size_t size = file.tellg();
        file.seekg(0);
        file.read(reinterpret_cast<char *>(MEMORY), size);
        file.close();
        REGISTER[PC] = std::stoi(argv[2], NULL, 16);
        while (REGISTER[PC] < size)
                OPCODES[(MEMORY[REGISTER[PC]] & 0xF0) >> 4]();
        return 0;
}
