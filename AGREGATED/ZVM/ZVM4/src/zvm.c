#include <stdio.h>
#include "zvm4_io.c"

unsigned short registers[16];
unsigned char FL, HIR;
signed char LIR;

#define IP 4
#define SPI 5
#define TP 7

#define memsize ((1024 * 2) - 32)
#define iosize 256

const unsigned char debug = 1;
unsigned char interrupt = 0;
unsigned short interruptVector = 0;

#define RUNTIME 0x80
#define INTERUPTED 0x40
#define MMUACTVE 0x20
#define PROTECTED 0x10

#define CARRY 0x08
#define FLOAT 0x04
#define POSITIVE 0x02
#define EQUALS 0x01

void injectInterrupt(unsigned short intId)
{
	if (FL & INTERUPTED || !iointreg)
		return;

	interrupt = 1;
	interruptVector = intId;
}

static inline void vmExecute();

unsigned char memory[memsize] = {0};

#define NMA_ROOB 0x0010
#define NMA_WOOB 0x1010

unsigned char vmmemget(unsigned short address)
{
	unsigned int faddress = (MW << 16) | address;
	if (!(FL & MMUACTVE))
	{
		if (faddress < memsize)
			return memory[faddress];
		else if (faddress >= memsize && faddress < iosize)
			return ioget(faddress - memsize);
		injectInterrupt(NMA_ROOB);
		return 0;
	}
	else
	{
		unsigned short mmupp = (memory[registers[TP] + 4] << 8) | memory[registers[TP] + 5];
		unsigned char mmups = memory[mmupp];

		for (int i = mmupp + 2; i < (mmupp * mmups * 6); i += 6)
		{
			if (faddress > ((memory[i] << 8) | memory[i + 1]) && faddress < ((memory[i + 2] << 8) | memory[i + 3]))
			{
				unsigned short itp = ((memory[i + 4] << 8) | memory[i + 5]);
				return memory[faddress + itp];
			}
		}

		unsigned short mmump = (memory[registers[TP] + 6] << 8) | memory[registers[TP] + 7];
		unsigned char mmums = memory[mmump];

		for (int i = mmump + 2; i < (mmump * mmums * 6); i += 6)
		{
			if (faddress > ((memory[i] << 8) | memory[i + 1]) && faddress < ((memory[i + 2] << 8) | memory[i + 3]))
			{
				unsigned short itp = ((memory[i + 4] << 8) | memory[i + 5]);
				injectInterrupt(0);
				do
					vmExecute();
				while (FL & INTERUPTED);
				return registers[0];
			}
		}

		injectInterrupt(0);

		return 0;
	}
}
void vmmemset(unsigned short address, unsigned char value)
{
	unsigned int faddress = (MW << 16) | address;
	if (!(FL & MMUACTVE))
	{
		if (faddress < memsize)
		{
			memory[faddress] = value;
			return;
		}
		else if (faddress >= memsize && faddress < (memsize + iosize))
		{
			ioset(faddress - memsize, value);
			return;
		}
		injectInterrupt(NMA_WOOB);
		return;
	}
	else
	{
		unsigned short mmupp = (memory[registers[TP] + 4] << 8) | memory[registers[TP] + 5];
		unsigned char mmups = memory[mmupp];

		for (int i = mmupp + 2; i < (mmupp * mmups * 6); i += 6)
		{
			if (faddress > ((memory[i] << 8) | memory[i + 1]) && faddress < ((memory[i + 2] << 8) | memory[i + 3]))
			{
				unsigned short itp = ((memory[i + 4] << 8) | memory[i + 5]);
				memory[faddress + itp] = value;
				return;
			}
		}

		unsigned short mmump = (memory[registers[TP] + 6] << 8) | memory[registers[TP] + 7];
		unsigned char mmums = memory[mmump];

		for (int i = mmump + 2; i < (mmump * mmums * 6); i += 6)
		{
			if (faddress > ((memory[i] << 8) | memory[i + 1]) && faddress < ((memory[i + 2] << 8) | memory[i + 3]))
			{
				unsigned short itp = ((memory[i + 4] << 8) | memory[i + 5]);
				injectInterrupt(0);
				vmmemset(registers[SPI]--, value);
				do
					vmExecute();
				while (FL & INTERUPTED);
				return;
			}
		}

		injectInterrupt(0);
	}
}

static inline void opmvi(unsigned char rX, unsigned char rY)
{
	const void *label[] = {&&mvil, &&mvih, &&dmvi, &&zero};
	goto *label[rX];

mvil:
	((unsigned char *)registers)[rY << 1] = LIR;
	return;
mvih:
	((unsigned char *)registers)[(rY << 1) + 1] = LIR;
	return;
dmvi:
	((unsigned char *)registers)[rY << 1] = LIR;
	((unsigned char *)registers)[(rY << 1) + 1] = LIR;
	return;
zero:
	registers[rY] = 0;
	return;
}

static inline void opmov(unsigned char rX, unsigned char rY)
{
	unsigned char rEX = (LIR >> 4);
	unsigned char rEY = (LIR & 0x0f);

	if (FL & PROTECTED && rEX == TP)
		return;

	registers[rEX] = registers[rEY];
}

static inline void opadd(unsigned char rX, unsigned char rY)
{
	if (FL & FLOAT)
	{
		char PrX;
		char PrY;

		switch (rX)
		{
		case 0:
			PrX = 0;
			break;
		case 1:
			PrX = 2 / 2;
			break;
		case 2:
			PrX = 12 / 2;
			break;
		case 3:
			PrX = 14 / 2;
			break;
		}

		switch (rY)
		{
		case 0:
			PrY = 0;
			break;
		case 1:
			PrY = 2 / 2;
			break;
		case 2:
			PrY = 12 / 2;
			break;
		case 3:
			PrY = 14 / 2;
			break;
		}
		((float *)registers)[PrX] += ((float *)registers)[PrY];
	}
	else
	{
		int a = registers[rX];
		registers[rX] += (registers[rY] + LIR);
		FL |= (registers[rX] < a) << 3;
	}
}

static inline void opsub(unsigned char rX, unsigned char rY)
{
	if (FL & FLOAT)
	{
		char PrX;
		char PrY;

		switch (rX)
		{
		case 0:
			PrX = 0;
			break;
		case 1:
			PrX = 2 / 2;
			break;
		case 2:
			PrX = 12 / 2;
			break;
		case 3:
			PrX = 14 / 2;
			break;
		}

		switch (rY)
		{
		case 0:
			PrY = 0;
			break;
		case 1:
			PrY = 2 / 2;
			break;
		case 2:
			PrY = 12 / 2;
			break;
		case 3:
			PrY = 14 / 2;
			break;
		}
		((float *)registers)[PrX] -= ((float *)registers)[PrY];
	}
	else
		registers[rX] -= (registers[rY] + LIR);
}

static inline void opand(unsigned char rX, unsigned char rY)
{
	registers[rX] &= (registers[rY] + LIR);
}

static inline void opior(unsigned char rX, unsigned char rY)
{
	registers[rX] |= (registers[rY] + LIR);
}

static inline void opnot(unsigned char rX, unsigned char rY)
{
	registers[rX] = ~(registers[rX] + LIR);
}

static inline void opldm(unsigned char rX, unsigned char rY)
{
	((unsigned char *)registers)[rX << 1] = vmmemget(registers[rY] + LIR);
}

static inline void opstm(unsigned char rX, unsigned char rY)
{
	vmmemset(registers[rX] + LIR, registers[rY]);
}

static inline void opshl(unsigned char rX, unsigned char rY)
{
	registers[rX] = registers[rX] << (registers[rY] + LIR);
}

static inline void opshr(unsigned char rX, unsigned char rY)
{
	registers[rX] = registers[rX] >> (registers[rY] + LIR);
}

static inline void opcmp(unsigned char rX, unsigned char rY)
{
	FL = (FL & ~EQUALS) | (registers[rX] == registers[rY]);
	FL = (FL & ~POSITIVE) | (registers[rX] > registers[rY]);
}

static inline void opjmp(unsigned char rX, unsigned char rY)
{
	const void *label[] = {&&jmp, &&rjmp, &&call, &&ret};
	goto *label[rX];
jmp:
	registers[IP] = registers[rY] + LIR;
	return;
rjmp:
	registers[IP] += registers[rY] + LIR;
	return;
call:
	vmmemset(registers[SPI]--, ((unsigned char *)registers)[IP << 1]);
	vmmemset(registers[SPI], ((unsigned char *)registers)[(IP << 1) + 1]);
	registers[IP] = registers[rY] + LIR;
	return;
ret:
	registers[IP] = ((vmmemget(registers[SPI]++) << 8) | vmmemget(registers[SPI])) + 2 + LIR;
	return;
}

static inline void opbrn(unsigned char rX, unsigned char rY)
{
	const void *label[] = {&&ebrn, &&qbrn, &&pbrn, &&nbrn};
	goto *label[rX];
ebrn:
	if (FL & EQUALS)
		registers[IP] += registers[rY] + LIR;
	else
		registers[IP] += 2;
	return;
qbrn:
	if (!(FL & EQUALS))
		registers[IP] += registers[rY] + LIR;
	else
		registers[IP] += 2;
	return;
pbrn:
	if (FL & POSITIVE)
		registers[IP] += registers[rY] + LIR;
	else
		registers[IP] += 2;
	return;
nbrn:
	if (!(FL & POSITIVE))
		registers[IP] += registers[rY] + LIR;
	else
		registers[IP] += 2;
	return;
}

static inline void opadc(unsigned char rX, unsigned char rY)
{
	if (FL & FLOAT)
	{
		char PrX;
		char PrY;

		switch (rX)
		{
		case 0:
			PrX = 0;
			break;
		case 1:
			PrX = 8 / 4;
			break;
		case 2:
			PrX = 12 / 4;
			break;
		case 3:
			return;
		}

		switch (rY)
		{
		case 0:
			PrY = 0;
			break;
		case 1:
			PrY = 8 / 4;
			break;
		case 2:
			PrY = 12 / 4;
			break;
		case 3:
			return;
		}
		((double *)registers)[PrX] += ((double *)registers)[PrY];
		return;
	}

	unsigned int a = registers[rX];
	registers[rX] += (registers[rY] + LIR + ((FL & CARRY) >> 3));
	FL &= ~CARRY;
	FL |= (registers[rX] < a) << 3;
}

static inline void opsys(unsigned char rX, unsigned char rY)
{
	switch (registers[rX])
	{
	case 0xF0:
		char PrX;

		switch (rX)
		{
		case 0:
			PrX = 0;
			break;
		case 1:
			PrX = 8 / 4;
			break;
		case 2:
			PrX = 12 / 4;
			break;
		case 3:
			return;
		}
		((float *)registers)[PrX] = (float)registers[rY];
		break;
	case 0xF1:
		char PrY;

		switch (rY)
		{
		case 0:
			PrY = 0;
			break;
		case 1:
			PrY = 2 / 2;
			break;
		case 2:
			PrY = 12 / 2;
			break;
		case 3:
			PrY = 14 / 2;
			break;
		}
		registers[rX] = ((float *)registers)[PrY];
		break;
	case 0xF2:
		char PrX;

		switch (rX)
		{
		case 0:
			PrX = 0;
			break;
		case 1:
			PrX = 8 / 4;
			break;
		case 2:
			PrX = 12 / 4;
			break;
		case 3:
			return;
		}
		((double *)registers)[PrX] = (double)registers[rY];
		break;
	case 0xF3:
		char PrY;

		switch (rY)
		{
		case 0:
			PrY = 0;
			break;
		case 1:
			PrY = 8 / 4;
			break;
		case 2:
			PrY = 12 / 4;
			break;
		case 3:
			return;
		}
		registers[rX] = ((double *)registers)[PrY];
		break;
	case 0xF4:
		registers[rX] = ioget(registers[rY]);
		break;
	case 0xF5:
		ioset(registers[rY], (unsigned char)LIR);
		break;
	case 0xF6:
		registers[rY] = MW;
		break;
	case 0xF7:
		MW = registers[rY];
		break;
	case 0xF8:
		FL &= ~FLOAT;
		break;
	case 0xF9:
		FL |= FLOAT;
		break;
	case 0xFA:
		FL &= ~CARRY;
		break;
	case 0xFB:
		FL |= CARRY;
		break;
	case 0xFC:
		registers[rY] = interruptVector;
		break;
	case 0xFD:
		FL = vmmemget(registers[SPI]++);
		registers[1] = ((vmmemget(registers[SPI]++) << 8) | vmmemget(registers[SPI]++));
		registers[0] = ((vmmemget(registers[SPI]++) << 8) | vmmemget(registers[SPI]++));
		registers[IP] = ((vmmemget(registers[SPI]++) << 8) | vmmemget(registers[SPI]));
		break;
	case 0xFE:
		registers[rY] = FL;
		break;
	case 0xFF:
		if (FL & PROTECTED)
		{
			FL &= 0xF0;
			FL |= (registers[rY] & 0x000F);
		}
		else
			FL = registers[rY];
		break;
	default:
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[IP << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(IP << 1) + 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[0 << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(0 << 1) + 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[1 << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(1 << 1) + 1]);
		vmmemset(registers[SPI], FL);

		registers[IP] = ((vmmemget(registers[TP]) << 8) | vmmemget(registers[TP] + 1)) - 2; // substract 2 to avoid jumping 1 instruction

		// I cannot emember how is this supposed to detect corrupted runtimes
		if (!(interruptVector & 0x0010) &&
			((interruptVector & 0x1000) >> 12) <= 1 &&
			!interrupt)
			return; // avoid entering real mode with corrupted runtime.

		FL &= ~PROTECTED; // clear user mode

		break;
	}
}

static inline void dispatch(unsigned char opcode, unsigned char rX, unsigned char rY)
{
	if (debug)
		printf("IR :0x%02X%02X:", HIR, LIR);
	const void *label[] = {&&mvi, &&mov, &&add, &&sub, &&and, &&ior, &&not, &&ldm, &&stm, &&shl, &&shr, &&cmp, &&jmp, &&brn, &&adc, &&sys};
	goto *label[opcode];
mvi:
	if (debug)
		printf("mvi.%u r%u, 0x%02X\n", rX, rY, LIR);
	opmvi(rX, rY);
	registers[IP] += 2;
	return;
mov:
	if (debug)
		printf("mov r%u, r%u\n", LIR >> 4, LIR & 0x0F);
	opmov(rX, rY);
	registers[IP] += 2;
	return;
add:
	if (debug)
	{
		if (FL & FLOAT)
			printf("fadd r%u, r%u+%d\n", rX, rY, LIR);
		else
			printf("add r%u, r%u+%d\n", rX, rY, LIR);
	}
	opadd(rX, rY);
	registers[IP] += 2;
	return;
sub:
	if (debug)
	{
		if (FL & FLOAT)
			printf("fsub r%u, r%u+%d\n", rX, rY, LIR);
		else
			printf("sub r%u, r%u+%d\n", rX, rY, LIR);
	}
	opsub(rX, rY);
	registers[IP] += 2;
	return;
and:
	if (debug)
		printf("and r%u, r%u+%d\n", rX, rY, LIR);
	opand(rX, rY);
	registers[IP] += 2;
	return;
ior:
	if (debug)
		printf("ior r%u, r%u+%d\n", rX, rY, LIR);
	opior(rX, rY);
	registers[IP] += 2;
	return;
not:
	if (debug)
		printf("not r%u\n", rX);
	opnot(rX, rY);
	registers[IP] += 2;
	return;
ldm:
	if (debug)
		printf("ldm [r%u+%d], r%u\n", rX, LIR, rY);
	opldm(rX, rY);
	registers[IP] += 2;
	return;
stm:
	if (debug)
		printf("stm [r%u+%d], r%u\n", rX, LIR, rY);
	opstm(rX, rY);
	registers[IP] += 2;
	return;
shl:
	if (debug)
		printf("shl r%u, r%u+%d\n", rX, rY, LIR);
	opshl(rX, rY);
	registers[IP] += 2;
	return;
shr:
	if (debug)
		printf("shr r%u, r%u+%d\n", rX, rY, LIR);
	opshr(rX, rY);
	registers[IP] += 2;
	return;
cmp:
	if (debug)
		printf("cmp r%u, r%u\n", rX, rY);
	opcmp(rX, rY);
	registers[IP] += 2;
	return;
jmp:
	if (debug)
		printf("jmp.%u r%u+%d\n", rX, rY, LIR);
	opjmp(rX, rY);
	return;
brn:
	if (debug)
		printf("brn.%u r%u+%d\n", rX, rY, LIR);
	opbrn(rX, rY);
	return;
adc:
	if (debug)
	{
		if (FL & FLOAT)
			printf("fadc r%u, r%u+%d\n", rX, rY, LIR);
		else
			printf("adc r%u, r%u+%d\n", rX, rY, LIR);
	}
	opadc(rX, rY);
	registers[IP] += 2;
	return;
sys:
	if (debug)
		printf("sys r%u, r%u\n", rX, rY);
	opsys(rX, rY);
	registers[IP] += 2;
	return;
}

static inline void fetch()
{
	HIR = vmmemget(registers[IP]);
	LIR = vmmemget(registers[IP] + 1);
}

static inline void decode(unsigned char *opcode, unsigned char *rX, unsigned char *rY)
{
	*opcode = HIR >> 4;
	*rX = (HIR >> 2) & 0x03;
	*rY = HIR & 0x03;
}

static inline void vmExecute()
{
	if (interrupt)
	{
		interrupt = 0;

		vmmemset(registers[SPI]--, ((unsigned char *)registers)[IP << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(IP << 1) + 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[0 << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(0 << 1) + 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[1 << 1]);
		vmmemset(registers[SPI]--, ((unsigned char *)registers)[(1 << 1) + 1]);
		vmmemset(registers[SPI], FL);

		if (!(interruptVector & 0x0010) &&
			((interruptVector & 0x1000) >> 12) <= 1 &&
			!interrupt)
			goto protected_int;

		FL &= ~PROTECTED;

	protected_int:

		FL |= INTERUPTED;

		registers[IP] = ((vmmemget(registers[TP] + 2) << 8) | vmmemget(registers[TP] + 3));
	}
	unsigned char op, rx, ry;
	fetch();
	decode(&op, &rx, &ry);
	dispatch(op, rx, ry);
	if (debug)
	{
		for (int i = 0; i < 16; i++)
			printf("R%02d:0x%04X\n", i, registers[i]);
		for (int i = 7; i >= 0; i--)
			printf("%u", ((FL >> i) & 1));
		printf(":FL\n----------\n");
	}
}

int main(int argc, char **argv)
{
	int eoc = 0;
	for (int i = 1; i < argc; i++)
	{
		FILE *file = fopen(argv[i], "rb");
		fread(memory + eoc, 1, memsize, file);
		eoc += ftell(file);
		fclose(file);
	}
	FL = RUNTIME;

	while (FL & RUNTIME)
	{
		if (!debug)
		{
			vmExecute();
			continue;
		}

		printf("> ");

		char input[256];

		if (!fgets(input, sizeof(input), stdin))
			return 0;

		switch (input[0])
		{
		case '\n':
			vmExecute();
			break;

		case 'I':
		case 'i':
		{
			unsigned int interrupt;

			if (sscanf(input + 1, "%x", &interrupt) == 1)
				injectInterrupt(interrupt);
			else
				printf("Usage: i <interrupt>\n");

			break;
		}

		case 'R':
		case 'r':
		{
			char arg[32];
			unsigned int cycles;

			if (sscanf(input + 1, "%31s", arg) == 1)
			{
				if (sscanf(arg, "%x", &cycles) == 1)
				{
					while (cycles-- && (FL & 0x80))
						vmExecute();
				}
				else
				{
					printf("Invalid cycle count.\n");
				}
			}
			else
			{
				printf("Usage: r <cycles>\n");
			}

			break;
		}

		case 'X':
		case 'x':
			return 0;

		case 'D':
		case 'd':
		{
			unsigned int address;
			unsigned int count;

			if (sscanf(input + 1, "%x %x", &address, &count) != 2)
			{
				printf("Usage: d <address> <count>\n");
				break;
			}

			if (address >= memsize)
			{
				printf("Address out of range.\n");
				break;
			}

			if (address + count > memsize)
				count = memsize - address;

			for (unsigned int i = 0; i < count; i += 16)
			{
				unsigned int row = count - i;

				if (row > 16)
					row = 16;

				printf("%04X: ", address + i);

				for (unsigned int j = 0; j < row; j++)
					printf("%02X ", memory[address + i + j]);

				printf("\n");
			}

			printf("---------------\n");

			break;
		}

		case 'W':
		case 'w':
		{
			unsigned int address;
			unsigned int value;

			if (sscanf(input + 1, "%x %x", &address, &value) != 2)
			{
				printf("Usage: w <address> <byte>\n");
				break;
			}

			if (address >= memsize)
			{
				printf("Address out of range.\n");
				break;
			}

			if (value > 0xFF)
			{
				printf("Byte out of range.\n");
				break;
			}

			memory[address] = value;

			printf("[0x%04X]:0x%02X\n",
				   address,
				   memory[address]);

			break;
		}

		case 'B':
		case 'b':
		{
			unsigned int address;

			if (sscanf(input + 1, "%x", &address) != 1)
			{
				printf("Usage: b <address>\n");
				break;
			}

			if (address >= memsize)
			{
				printf("Address out of range.\n");
				break;
			}

			printf("[0x%04X]:0x%02X\n",
				   address,
				   memory[address]);

			break;
		}

		case 'F':
		case 'f':
		{
			unsigned int bit;

			if (sscanf(input + 1, "%x", &bit) != 1)
			{
				printf("Usage: f <bit>\n");
				break;
			}

			if (bit > 7)
			{
				printf("FL bit out of range.\n");
				break;
			}

			FL ^= (1 << bit);

			printf("FL:0x%02X\n", FL);

			break;
		}

		case 'G':
		case 'g':
		{
			unsigned int reg;
			unsigned int value;

			if (sscanf(input + 1, "%x %x", &reg, &value) != 2)
			{
				printf("Usage: g <register> <value>\n");
				break;
			}

			if (reg > 15)
			{
				printf("Register out of range.\n");
				break;
			}

			registers[reg] = value;

			printf("R%02u:0x%04X\n",
				   reg,
				   registers[reg]);

			break;
		}

		default:
			printf("Unknown command: %c\n", input[0]);
			break;
		}
	}
	return 0;
}