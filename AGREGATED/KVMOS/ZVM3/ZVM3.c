#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <stdio.h>
#include <alsa/asoundlib.h>

#include <pthread.h>

snd_pcm_t *h = NULL;

#define MEM_SIZE (32 * 1024)

uint8_t MEMORY[MEM_SIZE];

#define MAX_REG 8
#define GPR0 0 // General Porpuse Register 0
#define GPR1 1 // General Porpuse Register 1
#define GPR2 2 // General Porpuse Register 2
#define RT 3   // Return Register 3
#define PC 4   // Program Count
#define SPI 5  // Stack Pointer Index
#define FP 6   // Frame Pointer
#define TP 7   // Table Pointer

/* Table Pointer
    0x00 : Syscall Table Pointer <- if not set, sys will fuck the cpu into corruption.
    0x02 : Interrupt Table Pointer <- if not set, segfault will fuck the cpu into halt.
    0x04 : MMU Page Table <- mmu will fail
    0x06 : MMU MMIO Table
*/

typedef union
{
    uint8_t REGISTER8;
    uint16_t REGISTER16;
} REGISTERS;

REGISTERS REGISTER[MAX_REG] = {0};
uint8_t FL = 0, IR = 0; // UNHALTED INTERRUPT SEGFAULT 0 0 LONG MODE CMP

#define UNHALTED 0b10000000
#define INTERRUPT 0b01000000
#define SEGFAULT 0b00100000

#define LONG 0b00000100
#define MODE 0b00000010

void VMloop();

bool ei = true;
bool gie = false;
uint16_t gid = 0;

void interrupt();

uint8_t Extended()
{
    return (FL & 0x1C) >> 2;
}

enum
{
    PCM_INTERRUPT_CODE = 0x6ff
};

FILE *flash = NULL;
uint16_t flash_adr = 0;
uint8_t flash_ctr = 0;
uint8_t flash_dt = 0;

uint8_t pcm_ctr = 0;
uint16_t pcm_assp = 0;
uint16_t pcm_asep = 0;

void *audio(void *args)
{
    if (pcm_ctr & 0x40)
    {
        const int PCM_FORMAT[] = {SND_PCM_FORMAT_U8, SND_PCM_FORMAT_U16, SND_PCM_FORMAT_U24, SND_PCM_FORMAT_U32};
        const int PCM_SAMPLERATE[] = {8000, 16000, 32000, 44100};
        snd_pcm_set_params(
            h,
            PCM_FORMAT[pcm_ctr & 0x03],
            SND_PCM_ACCESS_RW_INTERLEAVED,
            ((pcm_ctr & 0x10) >> 4) + 1,
            PCM_SAMPLERATE[(pcm_ctr & 0x0C) >> 2],
            1,
            500000);

        pcm_ctr &= ~(0xC0);
        return NULL;
    }
    if (pcm_ctr & 0x80)
    {
        while (pcm_assp < pcm_asep)
        {
            snd_pcm_sframes_t r = snd_pcm_writei(h, MEMORY + pcm_assp, (pcm_asep - pcm_assp));
            if (r > 0)
                pcm_assp += r;
        }
        pcm_ctr &= ~(0x80);
        if (pcm_ctr & 0x20 && gie)
        {
            gie = true;
            gid = PCM_INTERRUPT_CODE;
        }
    }
    return NULL;
}

uint8_t ioget(uint16_t address)
{
    switch (address)
    {
    case 1:
    {
        int c = getc(stdin);
        return c;
    }
    case 2:
        return flash_ctr;
    case 3:
        return flash_adr;
    case 4:
        return flash_adr << 8;
    case 5:
        if (flash_ctr & 1)
        {
            fseek(flash, flash_adr, SEEK_SET);
            fread(&flash_dt, 1, 1, flash);
        }
        return flash_dt;
    case 6:
        return pcm_ctr;
    case 7:
        return pcm_assp;
    case 8:
        return pcm_assp << 8;
    case 9:
        return pcm_asep;
    case 10:
        return pcm_asep << 8;
    }

    return 0;
}

void ioset(uint16_t address, uint8_t value)
{
#ifdef DEBUG2
    printf("IOSET[0x%X]:0x%X\n", address, value);
#endif
    switch (address)
    {
    case 0:
        putc(value, stdout);
        fflush(stdout);
        return;
    case 2:
        flash_ctr = value;
        return;
    case 3:
        flash_adr = value;
        return;
    case 4:
        flash_adr = value << 8;
        return;
    case 5:
        if (flash_ctr & 2)
        {
            fseek(flash, flash_adr, SEEK_SET);
            fwrite(&flash_dt, 1, 1, flash);
        }
        flash_dt = value;
        return;
    case 6:
        pcm_ctr = value;
        pthread_t tid;
        pthread_create(&tid, NULL, audio, NULL);
        return;
    case 7:
        pcm_assp = value;
        return;
    case 8:
        pcm_assp = value << 8;
        return;
    case 9:
        pcm_asep = value;
        return;
    case 10:
        pcm_asep = value << 8;
        return;
    }
}

uint8_t memget(uint16_t address);
void memoset(uint16_t address, uint16_t value);

void setregister(uint8_t reg, uint16_t val);
uint16_t getregister(uint8_t reg);

void storecontext()
{
    for (uint8_t i = 0; i < MAX_REG * ((FL & LONG) ? 2 : 1); i++)
    {
        memoset(getregister(SPI) + i, getregister(i / ((FL & LONG) ? 2 : 1)));
    }

    memoset(getregister(SPI) + (MAX_REG * ((FL & LONG) ? 2 : 1)), FL);

    setregister(SPI, getregister(SPI) - (MAX_REG * ((FL & LONG) ? 2 : 1) + 1));
}
void loadcontext()
{
    uint16_t base = getregister(SPI);
    for (uint8_t i = 0; i < MAX_REG * ((FL & LONG) ? 2 : 1); i++)
    {
        setregister(i / ((FL & LONG) ? 2 : 1), memget(base + i));
    }

    FL = memget(base + (MAX_REG * ((FL & LONG) ? 2 : 1)));
}

void interrupt()
{
    if (gie && !(FL & INTERRUPT) && ei)
    {
        storecontext();
        FL |= INTERRUPT;
        gie = false;
        ei = false;
        setregister(GPR2, gid);
        while (FL & INTERRUPT)
        {
            VMloop();
        }
        loadcontext();
        ei = true;
    }
    gie = false;
}

void setregister(uint8_t reg, uint16_t val)
{
    switch (Extended())
    {
    case 0:
    case 1:
        switch (reg)
        {
        case RT:
            REGISTER[GPR2].REGISTER8 = val;
            break;
        case PC:
            REGISTER[RT].REGISTER8 = val;
            break;
        default:
            if (!(reg == TP && FL & MODE))
                REGISTER[reg].REGISTER8 = val;
            break;
        }
        break;
    default:
        REGISTER[reg].REGISTER16 = val;
        break;
    }
}
uint16_t getregister(uint8_t reg)
{
    switch (Extended())
    {
    case 0:
    case 1:
        switch (reg)
        {
        case RT:
            return REGISTER[GPR2].REGISTER8;
        case PC:
            return REGISTER[RT].REGISTER8;
        default:
            if (!(reg == TP && FL & MODE))
                return REGISTER[reg].REGISTER8;
        }
        break;
    default:
        return REGISTER[reg].REGISTER16;
    }
    return 0;
}

uint8_t memget(uint16_t address)
{
    if (!(FL & MODE))
    {
        if (address < MEM_SIZE)
            return MEMORY[((FL & LONG) ? address : (address & 0x00FF))];
        else
            return ioget(address - MEM_SIZE);
    }

    else
    {
        uint16_t mmupp = (MEMORY[getregister(TP) + 4] << 8) | MEMORY[getregister(TP) + 5];
        uint8_t mmups = MEMORY[mmupp];

        for (int i = mmupp + 2; i < (mmupp * mmups * 6); i += 6)
        {
            if (address > ((MEMORY[i] << 8) | MEMORY[i + 1]) && address < ((MEMORY[i + 2] << 8) | MEMORY[i + 3]))
            {
                uint16_t itp = ((MEMORY[i + 4] << 8) | MEMORY[i + 5]);
                return MEMORY[address + itp];
            }
        }

        uint16_t mmump = (MEMORY[getregister(TP) + 6] << 8) | MEMORY[getregister(TP) + 7];
        uint8_t mmums = MEMORY[mmump];

        for (int i = mmump + 2; i < (mmump * mmums * 6); i += 6)
        {
            if (address > ((MEMORY[i] << 8) | MEMORY[i + 1]) && address < ((MEMORY[i + 2] << 8) | MEMORY[i + 3]))
            {
                uint16_t itp = ((MEMORY[i + 4] << 8) | MEMORY[i + 5]);
                storecontext();
                setregister(PC, itp);
                setregister(GPR1, address);
                FL &= ~MODE;
                while (FL & INTERRUPT)
                {
                    VMloop();
                }
                FL |= MODE;
                itp = getregister(RT);
                loadcontext();
                return itp;
            }
        }

        uint16_t itp = ((MEMORY[getregister(TP) + 2] << 8) | MEMORY[getregister(TP) + 3]);

        FL |= SEGFAULT;
        setregister(PC, itp);
        return 0;
    }
}
void memoset(uint16_t address, uint16_t value)
{
    if (!(FL & MODE))
    {
        if (address < MEM_SIZE)
            MEMORY[((FL & LONG) ? address : (address & 0x00FF))] = value;
        else
            ioset(address - MEM_SIZE, value);
    }
    else
    {
        uint16_t mmupp = (MEMORY[getregister(TP) + 4] << 8) | MEMORY[getregister(TP) + 5];
        uint8_t mmups = MEMORY[mmupp];

        for (int i = mmupp + 2; i < (mmupp * mmups * 6); i += 6)
        {
            if (address > ((MEMORY[i] << 8) | MEMORY[i + 1]) && address < ((MEMORY[i + 2] << 8) | MEMORY[i + 3]))
            {
                uint16_t itp = ((MEMORY[i + 4] << 8) | MEMORY[i + 5]);
                MEMORY[address + itp] = value;
                return;
            }
        }

        uint16_t mmump = (MEMORY[getregister(TP) + 6] << 8) | MEMORY[getregister(TP) + 7];
        uint8_t mmums = MEMORY[mmump];

        for (int i = mmump + 2; i < (mmump * mmums * 6); i += 6)
        {
            if (address > ((MEMORY[i] << 8) | MEMORY[i + 1]) && address < ((MEMORY[i + 2] << 8) | MEMORY[i + 3]))
            {
                uint16_t itp = ((MEMORY[i + 4] << 8) | MEMORY[i + 5]);
                storecontext();
                setregister(PC, itp);
                setregister(GPR1, address);
                setregister(GPR2, value);
                FL &= ~MODE;
                while (FL & INTERRUPT)
                {
                    VMloop();
                }
                FL |= MODE;
                itp = getregister(RT);
                loadcontext();
                return;
            }
        }

        uint16_t itp = ((MEMORY[getregister(TP) + 2] << 8) | MEMORY[getregister(TP) + 3]);

        FL |= SEGFAULT;
        setregister(PC, itp);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////

static inline void OP_MVI()
{ // 0000
    switch (Extended())
    {
    case 1:
    case 2:
    case 3:
        switch ((IR & 0x0C) >> 2)
        {
        case 1:
            setregister((IR & 0x03), ((memget(getregister(PC) + 1) << 8) | memget(getregister(PC) + 2)));
            setregister(PC, getregister(PC) + 3);
            break;
        case 2:
            setregister((IR & 0x03), (IR & 0xFF));
            setregister(PC, getregister(PC) + 2);
            break;
        case 0:
            setregister((IR & 0x03), (IR & 0xFF) << 0x08);
            setregister(PC, getregister(PC) + 2);
            break;
        default:
            setregister(PC, getregister(PC) + 1);
            break;
        }
        break;
    case 4:
    case 5:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            setregister((IR & 0x03), ((memget(getregister(PC) + 1) << 8) | memget(getregister(PC) + 2)));
            setregister(PC, getregister(PC) + 3);
            break;
        case 1:
            setregister((IR & 0x03), (IR & 0xFF));
            setregister(PC, getregister(PC) + 2);
            break;
        case 2:
            setregister((IR & 0x03), (IR & 0xFF) << 0x08);
            setregister(PC, getregister(PC) + 2);
            break;
        default:
            setregister((IR & 0x03), 0);
            setregister(PC, getregister(PC) + 1);
            break;
        }
        break;
    default:
        setregister(GPR0, (IR & 0x0F));
        setregister(PC, getregister(PC) + 1);
        break;
    }
}
static inline void OP_MOV()
{ // 0001
    switch (Extended())
    {
    case 5:
        setregister((IR & 0x0E) >> 1, getregister(((IR & 0x01) << 2) | (memget(getregister(PC) + 1) >> 5)));
        setregister(PC, getregister(PC) + 2);
        break;
    default:
        setregister((IR & 0x0C) >> 2, getregister((IR & 0x03)));
        setregister(PC, getregister(PC) + 1);
    }
}
static inline void OP_ADD()
{ // 0010
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) + getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_SUB()
{ // 0011
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) - getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_AND()
{ // 0100
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) & getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_OR()
{ // 0101
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) | getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_XOR()
{ // 0110
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) ^ getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_CMP()
{ // 0111
    if (getregister((IR & 0x0C) >> 2) == getregister((IR & 0x03)))
        FL |= 0x01;
    else
        FL &= 0xFE;
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_LD()
{ // 1000
    switch (Extended())
    {
    case 3:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            memoset(getregister(IR & 0x03), getregister((memget(getregister(PC) + 1) & 0xC0) >> 6));
            break;
        case 1:
            // Dont ask why the +0x100, the original code had it, and it would break asumtions..
            memoset(0x100 + getregister(IR & 0x03), getregister((memget(getregister(PC) + 1) & 0xC0) >> 6));
            break;
        case 2:
            memoset(getregister(SPI), getregister((memget(getregister(PC) + 1) & 0xC0) >> 6));
            setregister(SPI, getregister(SPI) - 1);
            break;
        }
        setregister(PC, getregister(PC) + 2);
        break;
    default:
        setregister((IR & 0x0C) >> 2, memget(getregister(IR & 0x03)));
        setregister(PC, getregister(PC) + 1);
    }
}
static inline void OP_ST()
{ // 1001
    switch (Extended())
    {
    case 3:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            setregister((memget(getregister(PC) + 1) & 0xC0) >> 6, memget(getregister(IR & 0x3)));
            break;
        case 1:
            // Dont ask why the +0x100, the original code had it, and it would break asumtions..
            setregister((memget(getregister(PC) + 1) & 0xC0) >> 6, 0x100 + memget(getregister(IR & 0x3)));
            break;
        case 2:
            setregister(SPI, getregister(SPI) + 1);
            setregister((memget(getregister(PC) + 1) & 0xC0) >> 6, memget(getregister(SPI)));
            break;
        }
        setregister(PC, getregister(PC) + 2);
        break;
    default:
        memoset(getregister((IR & 0x0C) >> 2), getregister((IR & 0x03)));
        setregister(PC, getregister(PC) + 1);
    }
}
static inline void OP_JMP()
{ // 1010
    switch (Extended())
    {
    case 0:
        setregister(PC, getregister(PC) + getregister((IR & 0x03)));
        break;
    case 3:
        switch ((IR & 0xC) >> 2)
        {
        case 0:
            setregister(PC, getregister((IR & 0x03)));
            break;
        case 1:
            setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            break;
        case 2:
            setregister(PC, getregister(PC) - getregister((IR & 0x03)));
            break;
        default:
            switch (IR & 0x03)
            {
            case 3:
                // I am too much in pain to transcript this.
                // REGISTER[PC] = ((MEMORY[(REGISTER[FP] - 3)] << 0x08) | MEMORY[(REGISTER[FP] - 2)]);
                // REGISTER[SPI] = REGISTER[FP];
                // REGISTER[FP] = ((MEMORY[(REGISTER[FP] - 1)] << 0x08) | MEMORY[(REGISTER[FP])]);
                break;
            default:
                /*
                // same but in comments block
                MEMORY[REGISTER[SPI]] = REGISTER[FP];
                MEMORY[REGISTER[SPI] - 1] = (REGISTER[FP] >> 0x08);
                MEMORY[REGISTER[SPI] - 2] = REGISTER[PC] + 1;
                MEMORY[REGISTER[SPI] - 3] = ((REGISTER[PC] + 1) >> 0x08);
                REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
                REGISTER[FP] = REGISTER[SPI];
                REGISTER[SPI] -= 4;
                */
                break;
            }
            break;
        }
        break;
    default:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            setregister(PC, getregister((IR & 0x03)));
            break;
        case 1:
            setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            break;
        case 2:
            setregister(PC, getregister(PC) - getregister((IR & 0x03)));
            break;
        case 3:
            setregister(PC, getregister(PC) + 1);
            break;
        }
    }
}
static inline void OP_BRN()
{ // 1011
    switch (Extended())
    {
    case 0:
        if (!(FL & 0x01))
            setregister(PC, getregister(PC) + getregister((IR & 0x03)));
        else
            setregister(PC, getregister(PC) + 1);
        break;
    case 3:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            if (FL & 0x01)
                setregister(PC, getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        case 2:
            if (FL & 0x01)
                setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        case 1:
            if (!(FL & 0x01))
                setregister(PC, getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        default:
            if (!(FL & 0x01))
                setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        }
        break;
    default:
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            if (FL & 0x01)
                setregister(PC, getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        case 1:
            if (FL & 0x01)
                setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        case 2:
            if (!(FL & 0x01))
                setregister(PC, getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        default:
            if (!(FL & 0x01))
                setregister(PC, getregister(PC) + getregister((IR & 0x03)));
            else
                setregister(PC, getregister(PC) + 1);
            break;
        }
    }
}
static inline void OP_SHL()
{ // 1100
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) << getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_SHR()
{ // 1101
    setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) >> getregister((IR & 0x03)));
    setregister(PC, getregister(PC) + 1);
}
static inline void OP_SCB()
{ // 1110
    switch (Extended())
    {
    case 4:
        setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) ^ (getregister((IR & 0x03))));
        setregister(PC, getregister(PC) + 1);
        break;
    default:
        setregister((IR & 0x0C) >> 2, getregister((IR & 0x0C) >> 2) ^ (1 << getregister((IR & 0x03))));
        setregister(PC, getregister(PC) + 1);
    }
}
static inline void OP_SYS()
{ // 1111
    switch (Extension())
    {
    case 0:
        if (((IR & 0x0C) >> 2) == 3 && (IR & 0x03) == 3)
        {
            FL |= LONG;
            setregister(PC, getregister(PC) + 1);
            return;
        }
        else
        {
            FL &= ~UNHALTED;
            setregister(PC, getregister(PC) + 1);
            return;
        }
        break;
    default:

        switch (getregister((IR & 0x0C) >> 2))
        {
        case 0xFE:
            setregister(RT, FL);
            break;
        case 0xFF:
            if (!(FL & MODE))
                FL = getregister((IR & 0x03));
            else
                FL = ((getregister(IR & 0x03) & 0x82) | FL); // protect bits 7 and 2.
            break;
        default:
            if (FL & INTERRUPT)
                break;
            FL |= INTERRUPT;
            uint16_t stp = ((memget(getregister(TP)) << 8) | memget(getregister(TP) + 1));
            storecontext();
            setregister(PC, (memget(stp + ((IR & 0x0C) >> 1)) << 8) | memget(stp + ((IR & 0x0C) >> 1) + 1));
            setregister(GPR2, getregister((IR & 0x03)));
            while (FL & INTERRUPT)
            {
                VMloop();
            }
            stp = getregister(RT);
            loadcontext();
            setregister(RT, stp);
            break;
        }
        setregister(PC, getregister(PC) + 1);
    }
}

void VMloop()
{
    interrupt();
    const void *label[] = {&&MVI, &&MOV, &&ADD, &&SUB, &&AND, &&OR, &&XOR, &&CMP, &&LD, &&ST, &&JMP, &&BRN, &&SHL, &&SHR, &&SCB, &&SYS};
    if ((FL & UNHALTED))
    {
        IR = memget(getregister(PC));

#ifdef DEBUG1

        printf("\n");
        for (int i = 0; i < MAX_REG; i++)
        {
            printf("R%u:0x%X\n", i, getregister(i));
        }
        printf("IR:0x%X\n", IR);
        printf("FL:0x%X\n\n", FL);
#endif

        goto *label[(IR & 0xF0) >> 4];
    }
    else
        return;
MVI:
#ifdef DEBUG
    if (FL & LONG)
    {
        switch ((IR & 0x0C) >> 2)
        {
        case 0:
            printf("MVI R%u, #0x%X\n", IR & 0x3, ((memget(getregister(PC) + 1) << 8) | memget(getregister(PC) + 2)));
            break;
        case 1:
            printf("MVIL R%u, #0x%X\n", IR & 0x3, (IR & 0xFF));
            break;
        case 2:
            printf("MVIH R%u, #0x%X\n", IR & 0x3, (IR & 0xFF));
            break;
        default:
            printf("MVI.ZERO R%u\n", IR & 0x3);
            break;
        }
    }
    else
    {
        printf("MVI #0x%X\n", IR & 0x0F);
    }
#endif
    OP_MVI();
    return;
MOV:
#ifdef DEBUG

    if (FL & LONG)
        printf("MOV R%u, R%u\n", (IR & 0xE) >> 1, (IR & 0x1 << 2) | (memget(getregister(PC) + 1) >> 5));
    else
        printf("MOV R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_MOV();
    return;
ADD:
#ifdef DEBUG
    printf("ADD R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_ADD();
    return;
SUB:
#ifdef DEBUG
    printf("SUB R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_SUB();
    return;
AND:
#ifdef DEBUG
    printf("AND R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_AND();
    return;
OR:
#ifdef DEBUG
    printf("OR R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_OR();
    return;
XOR:
#ifdef DEBUG
    printf("XOR R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_XOR();
    return;
CMP:
#ifdef DEBUG
    printf("CMP R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_CMP();
    return;
LD:
#ifdef DEBUG
    printf("LD R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_LD();
    return;
ST:
#ifdef DEBUG
    printf("ST R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_ST();
    return;
JMP:
#ifdef DEBUG
    printf("JMP R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_JMP();
    return;
BRN:
#ifdef DEBUG
    printf("BRN R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_BRN();
    return;
SHL:
#ifdef DEBUG
    printf("SHL R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_SHL();
    return;
SHR:
#ifdef DEBUG
    printf("SHR R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_SHR();
    return;
SCB:
#ifdef DEBUG
    printf("SCB R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_SCB();
    return;
SYS:
#ifdef DEBUG
    printf("SYS R%u, R%u\n", (IR & 0xC) >> 2, IR & 0x3);
#endif
    OP_SYS();
    return;
}

int main(int argc, char **argv)
{
    flash = fopen(argv[argc - 1], "r+b");
    FILE *src = fopen(argv[argc - 2], "rb");

    int err = snd_pcm_open(&h, "plug:default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        fprintf(stderr, "snd_pcm_open failed: %s\n", snd_strerror(err));
        return 1;
    }

    /*snd_pcm_set_params(
        h,
        SND_PCM_FORMAT_U8,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        1,
        8000,
        1,
        500000);*/

    fseek(src, 0, SEEK_END);
    long long endoffile = ftell(src);
    fseek(src, 0, SEEK_SET);

    fread(MEMORY, 1, MEM_SIZE, src);

    //

    /*fseek(flash, 0, SEEK_END);
    long long flashsize = ftell(flash);
    fseek(flash, 44, SEEK_SET);

    fread(MEMORY + 0x1000, 1, MEM_SIZE - 0x1000, flash);
*/
    FL |= UNHALTED;

#ifdef DEBUG1
    printf("F:%llx\n", endoffile);
#endif

    while (getregister(PC) < endoffile)
    {
        // getchar();
        VMloop();
    }

#ifdef DEBUG1

    printf("\n");
    for (int i = 0; i < MAX_REG; i++)
    {
        printf("R%u:0x%X\n", i, getregister(i));
    }
    printf("IR:0x%X\n", IR);
    printf("FL:0x%X\n\n", FL);

#endif
    getchar();
    return 0;
}