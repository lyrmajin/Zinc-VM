#include "zvm.h"
#include "zalloc.h"
#include "zfs2.h"
#include "abi.h"

static inline void OP_MVI()
{ // 0000
    switch ((REGISTER[IR] & 0x0C00) >> 0x0A)
    {
    case 0:
        REGISTER[(REGISTER[IR] & 0x0300) >> 0x08] = (((REGISTER[IR] & 0xFF) << 0x08) | vmemget(REGISTER[PC] + 2));
        printf("MVI R%u, #0x%x\n", (REGISTER[IR] & 0x0300) >> 0x08, REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
        REGISTER[PC] += 3;
        return;
    case 1:
        REGISTER[(REGISTER[IR] & 0x0300) >> 0x08] = (REGISTER[IR] & 0xFF);
        break;
    case 2:
        REGISTER[(REGISTER[IR] & 0x0300) >> 0x08] = (REGISTER[IR] & 0xFF) << 0x08;
        break;
    default:
        break;
    }
    REGISTER[PC] += 2;
}
static inline void OP_MOV()
{ // 0001
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] = REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_ADD()
{ // 0010
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] += REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_SUB()
{ // 0011
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] -= REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_AND()
{ // 0100
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] &= REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_OR()
{ // 0101
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] |= REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_XOR()
{ // 0110
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] ^= REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
    REGISTER[PC]++;
}
static inline void OP_CMP()
{ // 0111
    if (REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] == REGISTER[(REGISTER[IR] & 0x0300) >> 0x08])
        REGISTER[FL] |= 0x01;
    else
        REGISTER[FL] &= 0xFE;
    REGISTER[PC]++;
}
static inline void OP_LD()
{ // 1000
    switch ((REGISTER[IR] & 0x0C00) >> 0x0A)
    {
    case 0:
        vmemset(((REGISTER[IR] & 0x00F0) >> 0x04), REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
        REGISTER[PC] += 2;
        return;
    case 1:
        vmemset(((REGISTER[IR] & 0x00F0) >> 0x04), REGISTER[SPI]--);
        REGISTER[PC] += 2;
        break;
    default:
        REGISTER[PC]++;
        break;
    }
}
static inline void OP_ST()
{ // 1001
    switch ((REGISTER[IR] & 0x0C00) >> 0x0A)
    {
    case 0:
        REGISTER[((REGISTER[IR] & 0x00F0) >> 0x04)] = vmemget(REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
        REGISTER[PC] += 2;
        return;
    case 1:
        REGISTER[((REGISTER[IR] & 0x00F0) >> 0x04)] = vmemget(REGISTER[SPI]--);
        REGISTER[PC] += 2;
        break;
    default:
        REGISTER[PC]++;
        break;
    }
}
static inline void OP_JMP()
{ // 1010
    switch ((REGISTER[IR] & 0x0C00) >> 0x0A)
    {
    case 0:
        REGISTER[PC] = REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        break;
    case 1:
        REGISTER[PC] += REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        break;
    case 2:
        vmemset((REGISTER[FP] >> 0x08), REGISTER[SPI]--);
        vmemset(REGISTER[FP], REGISTER[SPI]--);
        REGISTER[FP] = (REGISTER[SPI] + 2);
        vmemset((REGISTER[PC] >> 0x08), REGISTER[SPI]--);
        vmemset(REGISTER[PC], REGISTER[SPI]--);
        REGISTER[PC] += REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        break;
    case 3:
        if (REGISTER[FP] != REGISTER[SPI])
        {
            REGISTER[FP] = ((vmemget(REGISTER[FP]) << 0x08) | vmemget(REGISTER[FP] - 1));
            REGISTER[PC] = ((vmemget(REGISTER[FP] - 2) << 0x08) | vmemget(REGISTER[FP] - 3));
            REGISTER[SPI] += 4;
            return;
        }
        REGISTER[PC]++;
        break;
    }
}
static inline void OP_BRN()
{ // 1011
    switch ((REGISTER[IR] & 0x0C00) >> 0x0A)
    {
    case 0:
        if (REGISTER[FL] & 0x01)
            REGISTER[PC] = REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        else
            REGISTER[PC]++;
        break;
    case 1:
        if (REGISTER[FL] & 0x01)
            REGISTER[PC] += REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        else
            REGISTER[PC]++;
        break;
    case 2:
        if (!(REGISTER[FL] & 0x01))
            REGISTER[PC] = REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        else
            REGISTER[PC]++;
        break;
    case 3:
        if (!(REGISTER[FL] & 0x01))
            REGISTER[PC] += REGISTER[(REGISTER[IR] & 0x0300) >> 0x08];
        else
            REGISTER[PC]++;
        break;
    }
}
static inline void OP_SHL()
{ // 1100
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] = (REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] << REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
    REGISTER[PC]++;
}
static inline void OP_SHR()
{ // 1101
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] = (REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] >> REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
    REGISTER[PC]++;
}
static inline void OP_SCB()
{ // 1110
    REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] = (REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A] ^ REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]);
    REGISTER[PC]++;
}
static inline void OP_SYS()
{ // 1111
    syscall(REGISTER[(REGISTER[IR] & 0x0C00) >> 0x0A], REGISTER[(REGISTER[IR] & 0x0300) >> 0x08] + 1, vmemget(REGISTER[(REGISTER[IR] & 0x0300) >> 0x08]), &vmemset, &vmemget);
    REGISTER[PC]++;
}

void storeTask()
{
    printf("Storing Task %u State\n", REGISTER[TI]);
    uint8_t LTI = REGISTER[TI];
    REGISTER[TI] = 0; // Calling makes the mem access kernel mode, which means is unrestricted
    uint16_t address = getFileDataAddress(getTaskMetadata(LTI, &vmemget), &vmemget);
    vmemset(REGISTER[GPR0] >> 0x08, address);
    vmemset(REGISTER[GPR0], address + 1);
    vmemset(REGISTER[GPR1] >> 0x08, address + 2);
    vmemset(REGISTER[GPR1], address + 3);
    vmemset(REGISTER[GPR2] >> 0x08, address + 4);
    vmemset(REGISTER[GPR2], address + 5);
    vmemset(REGISTER[RT] >> 0x08, address + 6);
    vmemset(REGISTER[RT], address + 7);
    vmemset(REGISTER[PC] >> 0x08, address + 8);
    vmemset(REGISTER[PC], address + 9);
    vmemset(REGISTER[SPI] >> 0x08, address + 10);
    vmemset(REGISTER[SPI], address + 11);
    vmemset(REGISTER[FP] >> 0x08, address + 12);
    vmemset(REGISTER[FP], address + 13);
    vmemset(REGISTER[FL] >> 0x08, address + 14);
    vmemset(REGISTER[FL], address + 15);
    REGISTER[TI] = LTI;
}
void loadTask()
{
    printf("Loading Task %u State\n", REGISTER[TI]);
    uint8_t LTI = REGISTER[TI];
    REGISTER[TI] = 0; // Calling makes the mem access kernel mode, which means is unrestricted
    uint16_t address = getFileDataAddress(getTaskMetadata(LTI, &vmemget), &vmemget);
    REGISTER[GPR0] = ((vmemget(address) << 0x08) | vmemget(address + 1));
    REGISTER[GPR1] = ((vmemget(address + 2) << 0x08) | vmemget(address + 3));
    REGISTER[GPR2] = ((vmemget(address + 4) << 0x08) | vmemget(address + 5));
    REGISTER[RT] = ((vmemget(address + 6) << 0x08) | vmemget(address + 7));
    REGISTER[PC] = ((vmemget(address + 8) << 0x08) | vmemget(address + 9));
    REGISTER[SPI] = ((vmemget(address + 10) << 0x08) | vmemget(address + 11));
    REGISTER[FP] = ((vmemget(address + 12) << 0x08) | vmemget(address + 13));
    REGISTER[FL] = ((vmemget(address + 14) << 0x08) | vmemget(address + 15));
    REGISTER[TI] = LTI;
}

void ExecVMOperation()
{
    const void *label[] = {&&MVI, &&MOV, &&ADD, &&SUB, &&AND, &&OR, &&XOR, &&CMP, &&LD, &&ST, &&JMP, &&BRN, &&SHL, &&SHR, &&SCB, &&SYS};
    if ((REGISTER[FL] & UNHALTED))
    {
        REGISTER[IR] = vmemget(REGISTER[PC]) << 0x08;
        REGISTER[IR] |= vmemget(REGISTER[PC] + 1);
        goto *label[(REGISTER[IR] & 0xF000) >> 6];
    }
    else
        goto END;
MVI:
    OP_MVI();
    goto END;
MOV:
    OP_MOV();
    goto END;
ADD:
    OP_ADD();
    goto END;
SUB:
    OP_SUB();
    goto END;
AND:
    OP_AND();
    goto END;
OR:
    OP_OR();
    goto END;
XOR:
    OP_XOR();
    goto END;
CMP:
    OP_CMP();
    goto END;
LD:
    OP_LD();
    goto END;
ST:
    OP_ST();
    goto END;
JMP:
    OP_JMP();
    goto END;
BRN:
    OP_BRN();
    goto END;
SHL:
    OP_SHL();
    goto END;
SHR:
    OP_SHR();
    goto END;
SCB:
    OP_SCB();
    goto END;
SYS:
    OP_SYS();
    goto END;
END:
    return;
}