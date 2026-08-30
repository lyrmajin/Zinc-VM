#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int debug = 1;
unsigned char LIR = 0, HIR = 0;
unsigned short ip = 0;

#define memsize ((1024 * 2) - 32)
unsigned char memory[memsize] = {0};

int verbose = 0;

typedef struct
{
    char magic[4];
    unsigned short lkC;
    unsigned short lkP;
    unsigned short fnC;
    unsigned short fnP;
} metobj;

typedef struct
{
    unsigned short address;
    unsigned short string;
} fn_t;

int mode = 1;

int fncg = 0;

fn_t fnsg[128];
char *fnsgs[128] = {0};
fn_t lksg[128];
char *lksgs[128] = {0};

int fp = 0, lp = 0;

unsigned short getFnAddress(unsigned int d)
{
    for (int i = 0; i < fncg; i++)
    {
        if (!strcmp(lksgs[d], fnsgs[i]))
            return fnsg[i].address;
    }
    return 0;
}

static inline void dispatch(unsigned char opcode, unsigned char rX, unsigned char rY)
{
    if (verbose)
        printf("0x%04X: ", ip);
    const void *label[] = {&&mvi, &&mov, &&add, &&sub, &&and, &&ior, &&not, &&ldm, &&stm, &&shl, &&shr, &&cmp, &&jmp, &&brn, &&adc, &&sys};
    goto *label[opcode];
mvi:
    switch (rX)
    {
    case 0:
        for (int i = 0; i < lp; i++)
            if ((ip) == lksg[i].address)
            {
                LIR = getFnAddress(i);
                if (verbose)
                    printf("L: ");
            }

        if (verbose)
            printf("mvil r%u, 0x%02X\n", rY, LIR);
        break;
    case 1:
        for (int i = 0; i < lp; i++)
            if ((ip - 2) == lksg[i].address)
            {
                LIR = (getFnAddress(i) >> 8);
                if (verbose)
                    printf("   ");
            }
        if (verbose)
            printf("mvih r%u, 0x%02X\n", rY, LIR);
        break;
    case 2:
        if (verbose)
            printf("dmvi r%u, 0x%02X\n", rY, LIR);
        break;
    case 3:
        if (verbose)
            printf("zero r%u\n", rY);
        break;
    }
    ip += 2;
    return;
mov:
    if (verbose)
        printf("mov r%u, r%u\n", LIR >> 4, LIR & 0x0F);
    ip += 2;
    return;
add:
    if (LIR)
    {
        if (verbose)
            printf("add r%u, r%u+%d\n", rX, rY, LIR);
    }
    else if (verbose)
        printf("add r%u, r%u\n", rX, rY);
    ip += 2;
    return;
sub:
    if (LIR)
    {
        if (verbose)
            printf("sub r%u, r%u+%d\n", rX, rY, LIR);
    }
    else if (verbose)
        printf("sub r%u, r%u\n", rX, rY);
    ip += 2;
    return;
and:
    if (LIR)
    {
        if (verbose)
            printf("and r%u, r%u+%d\n", rX, rY, LIR);
    }
    else if (verbose)
        printf("and r%u, r%u\n", rX, rY);
    ip += 2;
    return;
ior:
    if (LIR)
    {
        if (verbose)
            printf("ior r%u, r%u+%d\n", rX, rY, LIR);
    }
    else if (verbose)
        printf("ior r%u, r%u\n", rX, rY);
    ip += 2;
    return;
not:
    if (LIR)
    {
        if (verbose)
            printf("not r%u+%d\n", rX, LIR);
    }
    else if (verbose)
        printf("not r%u\n", rX);
    ip += 2;
    return;
ldm:
    if (verbose)
        printf("ldm [r%u+%d], r%u\n", rX, LIR, rY);
    ip += 2;
    return;
stm:
    if (verbose)
        printf("stm [r%u+%d], r%u\n", rX, LIR, rY);
    ip += 2;
    return;
shl:
    if (verbose)
        printf("shl r%u, r%u+%d\n", rX, rY, LIR);
    ip += 2;
    return;
shr:
    if (verbose)
        printf("shr r%u, r%u+%d\n", rX, rY, LIR);
    ip += 2;
    return;
cmp:
    if (verbose)
        printf("cmp r%u, r%u\n", rX, rY);
    ip += 2;
    return;
jmp:
    switch (rX)
    {
    case 0:

        if (LIR)
        {
            if (verbose)
                printf("jmp r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("jmp r%u\n", rY);
        break;
    case 1:
        if (LIR)
        {
            if (verbose)
                printf("rjmp r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("rjmp r%u\n", rY);
        break;
    case 2:
        if (LIR)
        {
            if (verbose)
                printf("call r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("call r%u\n", rY);
        break;
    case 3:
        if (LIR)
        {
            if (verbose)
                printf("ret +%d\n", LIR);
        }
        else if (verbose)
            printf("ret\n");
        break;
    }
    ip += 2;
    return;
brn:
    switch (rX)
    {
    case 0:
        if (LIR)
        {
            if (verbose)
                printf("ebrn r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("ebrn r%u\n", rY);
        break;
    case 1:
        if (LIR)
        {
            if (verbose)
                printf("qbrn r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("qbrn r%u\n", rY);
        break;
    case 2:
        if (LIR)
        {
            if (verbose)
                printf("pbrn r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("pbrn r%u\n", rY);
        break;
    case 3:
        if (LIR)
        {
            if (verbose)
                printf("nbrn r%u+%d\n", rY, LIR);
        }
        else if (verbose)
            printf("nbrn r%u\n", rY);
        break;
    }
    ip += 2;
    return;
adc:
    if (LIR)
    {
        if (verbose)
            printf("adc r%u, r%u+%d\n", rX, rY, LIR);
    }
    else if (verbose)
        printf("adc r%u, r%u\n", rX, rY);
    ip += 2;
    return;
sys:
    if (verbose)
        printf("sys r%u, r%u\n", rX, rY);
    ip += 2;
    return;
}

static inline void fetch()
{
    HIR = memory[ip];
    LIR = memory[ip + 1];
}

static inline void decode(unsigned char *opcode, unsigned char *rX, unsigned char *rY)
{
    *opcode = HIR >> 4;
    *rX = (HIR >> 2) & 0x03;
    *rY = HIR & 0x03;
}

int main(int argc, char **argv)
{
    int eoc = 0;
    FILE *outfile = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o"))
        {
            mode = 1;
            continue;
        }
        else if (!strcmp(argv[i], "-v"))
        {
            verbose = 1;
            mode = 0;
            continue;
        }
        switch (mode)
        {
        case 1:
            outfile = fopen(argv[i], "w");
            mode = 0;
            break;
        case 0:
        {
            metobj mb;
            if (verbose)
                printf("------------------\nImporting '%s' at 0x%04x\n", argv[i], eoc);
            FILE *file = fopen(argv[i], "rb");

            if (!file)
                continue;

            fread(&mb, sizeof(metobj), 1, file);
            if (verbose)
                printf("Magic:%c%c%c%c\nlkC:0x%04X\nlkP:0x%04X\nfnC:0x%04X\nfnP:0x%04X\n\n", mb.magic[0], mb.magic[1], mb.magic[2], mb.magic[3], mb.lkC, mb.lkP, mb.fnC, mb.fnP);

            if (strncmp(mb.magic, "ZOBJ", 4))
            {
                printf("%s: file format not recognized\n", argv[i]);
                return -1;
            }

            fncg += mb.fnC;

            fread(memory + eoc, 1, mb.lkP - sizeof(metobj), file);

            if (mb.lkC)
            {
                char buffer[1024];
                long long pos = ftell(file);
                fseek(file, mb.lkP + (mb.lkC * sizeof(fn_t)), SEEK_SET);
                fread(buffer, 1, 1024, file);
                fseek(file, pos, SEEK_SET);

                for (int i = 0; i < mb.lkC; i++)
                {
                    fn_t lks;

                    fread(&lks, sizeof(fn_t), 1, file);

                    lks.address -= sizeof(metobj);

                    // if(verbose)printf("lks.a:0x%04X\nlks.s:%u\n%s\n", lks.address, lks.string, buffer);

                    int strindex = 0;
                    int strout = 0;
                    for (int o = 0; o < 1024; o++)
                    {
                        if (strindex == lks.string)
                        {
                            strout = o;
                            break;
                        }

                        if (!buffer[o])
                            strindex++;
                    }

                    if (verbose)
                        printf("L:0x%04X: %s\n", lks.address + eoc, buffer + strout);

                    lksg[lp].address = lks.address + eoc;
                    lksg[lp].string = lks.string;
                    lksgs[lp] = malloc(128);
                    strcpy(lksgs[lp++], buffer + strout);
                }
            }

            clearerr(file);

            if (mb.fnC)
            {
                char buffer[1024];
                fseek(file, mb.fnP + (mb.fnC * sizeof(fn_t)), SEEK_SET);
                fread(buffer, 1, 1024, file);

                fseek(file, mb.fnP, SEEK_SET);

                for (int i = 0; i < mb.fnC; i++)
                {
                    fn_t fns;

                    fread(&fns, sizeof(fn_t), 1, file);

                    fns.address -= sizeof(metobj);

                    // if(verbose)printf("fns.a:0x%04X\nfns.s:%u\n%s\n", fns.address, fns.string, buffer);

                    int strindex = 0;
                    int strout = 0;
                    for (int o = 0; o < 1024; o++)
                    {
                        if (strindex == fns.string)
                        {
                            strout = o;
                            break;
                        }

                        if (!buffer[o])
                            strindex++;
                    }

                    if (verbose)
                        printf("F:0x%04X: %s\n", fns.address + eoc, buffer + strout);
                    fnsg[fp].address = fns.address + eoc;
                    fnsg[fp].string = fns.string;
                    fnsgs[fp] = malloc(128);
                    strcpy(fnsgs[fp++], buffer + strout);
                }
            }

            eoc += (mb.lkP - sizeof(metobj));

            fclose(file);
        }
        break;
        }
    }

    if (verbose)
        printf("\n------------------\n%u Bytes, %u Words\n", eoc, eoc / 2);

    while (ip < eoc)
    {
        for (int i = 0; i < fp; i++)
            if (ip == fnsg[i].address)
                if (verbose)
                    printf("%s:\n", fnsgs[i]);
        unsigned char opcode = 0, rX = 0, rY = 0;
        fetch();
        decode(&opcode, &rX, &rY);
        dispatch(opcode, rX, rY);
        if (outfile)
        {
            fseek(outfile, ip - 2, SEEK_SET);
            unsigned short ins = ((LIR << 8) | HIR);
            fwrite(&ins, 2, 1, outfile);
        }
    }
    return 0;
}