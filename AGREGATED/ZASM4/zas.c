#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	unsigned char opcode;

	unsigned char rXA;
	unsigned char xOP;
	unsigned char rXB;

	unsigned char rYA;
	unsigned char yOP;
	unsigned char rYB;

	unsigned char inm;
} decoded;

struct
{
	int fnp;

	struct
	{
		unsigned short address;
		char *name;
	} fne[128];
} fndt;

struct
{
	int lkp;

	struct
	{
		unsigned short address;
		char *name;
	} lke[128];
} lkdt;

struct
{
	unsigned int magic;
	unsigned short lkC;
	unsigned short lkP;
	unsigned short fnC;
	unsigned short fnP;
} ZOBJ_H;

int debug = 1;

#define vmmvi 0
#define vmmov 1
#define vmadd 2
#define vmsub 3
#define vmand 4
#define vmior 5
#define vmnot 6
#define vmldm 7
#define vmstm 8
#define vmshl 9
#define vmshr 10
#define vmcmp 11
#define vmjmp 12
#define vmbrn 13
#define vmtgl 14
#define vmsys 15

#define vmr0 0
#define vmr1 1
#define vmr2 2
#define vmr3 3
#define vmip 4
#define vmsp 5
#define vmfp 6
#define vmtp 7
#define vmr8 8
#define vmr9 9
#define vmr10 10
#define vmr11 11
#define vmr12 12
#define vmr13 13
#define vmr14 14
#define vmr15 15

#define vmmvil 0
#define vmmvih 1
#define vmdmvi 2
#define vmzero 3

#define vmajmp 0
#define vmrjmp 1
#define vmcall 2
#define vmret 3

#define vmebrn 0
#define vmqbrn 1
#define vmpbrn 2
#define vmnbrn 3

unsigned short encodeA(unsigned char Opcode, unsigned char rX, unsigned char rY, unsigned char inm)
{
	return ((((Opcode & 0x0f) << 4) | ((rX & 0x03) << 2) | (rY & 0x03)) << 8) | inm;
}
unsigned short encodeB(unsigned char Opcode, unsigned char rX, unsigned char rY)
{
	return ((((Opcode & 0x0f) << 4) | ((rX & 0x03) << 2) | (rY & 0x03)) << 8) | (((rX & 0x04) >> 1) | ((rX & 0x04) >> 2));
}

unsigned short encode(unsigned char Opcode, unsigned char rX, unsigned char rY, unsigned char inm)
{
	switch (Opcode)
	{
	case 1:
		return encodeB(Opcode, rX, rY);
	default:
		return encodeA(Opcode, rX, rY, inm);
	}
}

void emitCode(FILE *output, unsigned short instruction)
{
	unsigned short ins = __builtin_bswap16(instruction);
	fwrite(&ins, sizeof(unsigned short), 1, output);
}

#define operations_count 61
const char *operations[] = {
	// OPCODES
	"mvil",
	"mvih",
	"dmvi",
	"zero",

	"mov",
	"add",
	"sub",
	"and",
	"ior",
	"not",
	"cmp",

	"stm",

	"ldm",

	"jmp",
	"rjmp",
	"call",
	"ret",

	"ebrn",
	"qbrn",
	"erbrn",
	"qrbrn",

	"shl",
	"shr",
	"tgl",
	"sys",

	// MACROS OPS

	"mvi",

	"inc",
	"dec",

	"push",
	"pop",

	"ldiv",

	"iret",

	"ldfl",
	"stfl",

	// ASM OPS

	".org:",
	".global:",
	".local:",

	".text:",
	".data:",
	".bss:",

	".byte:",
	".word:",
	".dword:",
	".qword:",

	".string:",
	".ascii:",
	".asciz:",

	".align:",
	".space:",

	".equ:",
	".include:",

	".macro:",
	".endmacro:",

	".repeat:",
	".endrepeat:",

	".if:",
	".ifdef:",

	".else:",
	".endif:",

	".define:",
	".undef:",
};

#define registers_count 20
const char *registers[] = {
	"r0",
	"r1",
	"r2",
	"r3",

	"ip",
	"r4",

	"sp",
	"r5",

	"fp",
	"r6",

	"tp",
	"r7",

	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",
};

#define register_macros_count 5
const char *register_macros[] = {
	"$",
	":",
	"@",

	"+",
	"-",
};

int getOperationId(const char *op)
{
	for (int i = 0; i < operations_count; i++)
	{
		if (!strcmp(op, operations[i]))
			return i;
	}
	return -1;
}

int getRegisterId(const char *rg)
{
	for (int i = 0; i < registers_count; i++)
	{
		if (!strcmp(rg, registers[i]))
			return i;
	}
	return -1;
}

int getRegisterMacroId(const char *mc)
{
	for (int i = 0; i < register_macros_count; i++)
	{
		if (!strcmp(mc, register_macros[i]))
			return i;
	}
	return -1;
}

long long getInmediate(const char *im, unsigned char base)
{
	if (base > 16)
		return -1;
	short number = 0;
	int i = 0;
	for (; im[i]; i++)
		switch (im[i])
		{
		case '-':
			if (!i)
				number *= -1;
			else
				return -1;
			break;
		case '9':
			if (base < 10)
				return -1;
		case '8':
			if (base < 9)
				return -1;
		case '7':
			if (base < 8)
				return -1;
		case '6':
			if (base < 7)
				return -1;
		case '5':
			if (base < 6)
				return -1;
		case '4':
			if (base < 5)
				return -1;
		case '3':
			if (base < 4)
				return -1;
		case '2':
			if (base < 3)
				return -1;
		case '1':
			if (base < 2)
				return -1;
		case '0':
			number = (number * base) + (im[i] - '0');
			break;
		case 'f':
			if (base < 16)
				return -1;
		case 'e':
			if (base < 15)
				return -1;
		case 'd':
			if (base < 14)
				return -1;
		case 'c':
			if (base < 13)
				return -1;
		case 'b':
			if (base < 12)
				return -1;
		case 'a':
			if (base < 11)
				return -1;
			number = (number * base) + (im[i] - 'a') + 10;
			break;
		default:
			return -1;
		}
	if (i)
		return number;

	return -1;
}

char *allocCpyStr(const char *str)
{
	char *new_str = malloc(strlen(str) + 1);
	strcpy(new_str, str);
	return new_str;
}

void addFn(char *str, unsigned short address)
{
	fndt.fne[fndt.fnp++].name = str;
	fndt.fne[fndt.fnp - 1].address = address;
}

void addLk(char *str, unsigned short address)
{
	lkdt.lke[lkdt.lkp++].name = str;
	lkdt.lke[lkdt.lkp - 1].address = address;
}

typedef struct
{
	char *str;
	int type;
	int x, y;
} token_t;

token_t tokens[1024];
int tokensCount = 0;

enum
{
	TOKEN_DEFAULT = 0,
	TOKEN_OP,
	TOKEN_NL,
	TOKEN_IDENT
};

void addToken(token_t token)
{
	tokens[tokensCount++] = token;
}

int compile(const char *file)
{
	if (debug)
		printf("Compiling:%s\n", file);

	FILE *inputfile = fopen(file, "rb");
	if (!inputfile)
		return -1;

	char buffer[1024];

	int comment = 0;
	int lineIndex = 0;
	char line[256] = {0};
	int type = 3;
	int old_type = 3;

	int x = 1, y = 0;

	while (1)
	{
		if (fread(&buffer, 1, sizeof(buffer), inputfile) <= 0)
			break;

		for (int bufferIndex = 0; buffer[bufferIndex] && bufferIndex < sizeof(buffer); bufferIndex++)
		{
			old_type = type;

			switch (buffer[bufferIndex])
			{
			case ' ':
				type = TOKEN_DEFAULT;
				x++;
				break;
			case '-':
			case '+':
			case '!':
			case '@':
			case '#':
			case '$':
			case '%':
			case '^':
			case '&':
			case '*':
			case '(':
			case ')':
			case '=':
			case '{':
			case '}':
			case '<':
			case '>':
			case '?':
			case ',':
			case '.':
			case '/':
			case '\\':
			case '[':
			case ']':
			case '~':
			case '\"':
			case '\'':
			case ':':
				type = TOKEN_OP;
				break;
			case ';':
				comment = 1;
				continue;
			case '\n':
				comment = 0;
				type = TOKEN_NL;
				break;
			default:
				type = TOKEN_IDENT;
				break;
			}

			if (comment)
				continue;

			if (old_type != type && (line[0] || old_type == TOKEN_NL))
			{
				printf("%s[%u:%u]: %s\n", file, x, y, line);
				char *nl = malloc(strlen(line) + 1);
				if (!nl)
					return -1;
				strcpy(nl, line);
				token_t tk = {nl, old_type, x, y};
				if(nl[0] || old_type == TOKEN_NL)
				addToken(tk);
				x += lineIndex;
				lineIndex = 0;
				line[lineIndex] = 0;
			}

			if (lineIndex < (sizeof(line) - 1))
			{
				if (buffer[bufferIndex] != '\n')
				{
					if (buffer[bufferIndex] != ' ')
					{
						line[lineIndex++] = buffer[bufferIndex];
						line[lineIndex] = 0;
					}
				}
				else
				{
					y++;
					x = 1;
					printf("[%u:%u]\n", x, y);
				}
			}
			else
			{
				printf("Element too long\n");
				return -1;
			}
		}
	}
	printf("%s[%u:%u]: %s\n", file, x, y, line);
	char *nl = malloc(strlen(line) + 1);
	if (!nl)
		return -1;
	strcpy(nl, line);
	token_t tk = {nl, old_type, x, y};
	if (nl[0])
		addToken(tk);
	x += lineIndex;
	lineIndex = 0;
	line[lineIndex] = 0;

	printf("----\n");

	int cicle = 0;
	int rX = 0;
	int rY = 0;

	for (int i = 0; i < tokensCount; i++)
	{
		// printf("tk[%u]:%u:%s\n", i, tokens[i].type, tokens[i].str);
		if (tokens[i].type == TOKEN_IDENT)
		{
			int operationId = getOperationId(tokens[i].str);
			if (operationId != -1)
			{
				cicle = 0;
				if (operationId > 0 && operationId < 25)
				{
					printf("OP[%s]:%d\n", tokens[i].str, getOperationId(tokens[i].str));
					cicle++;
				}
				continue;
			}
			operationId = getRegisterId(tokens[i].str);
			if (operationId != -1)
			{
				char regc = 0;
				if (cicle == 1)
					regc = 'X';
				else if (cicle == 2)
					regc = 'Y';
				printf("R%C[%s]\n", regc, tokens[i].str);
				cicle++;
				continue;
			}
		}
		if (tokens[i].type == TOKEN_OP)
		{
			int operationId = getRegisterMacroId(tokens[i].str);
			if (operationId != -1)
			{
				if (i < 1 || i > tokensCount - 1)
					continue;

				//printf("P:%d|N:%d\n", tokens[i - 1].type, tokens[i + 1].type);

				if (tokens[i - 1].type == TOKEN_OP || tokens[i - 1].type == TOKEN_NL || tokens[i + 1].type == TOKEN_OP || tokens[i + 1].type == TOKEN_NL)
					continue;

				switch (operationId)
				{
				case 0:
					printf("[R:%s] AS -->", tokens[i - 1].str);
					break;
				case 1:
					printf("^--- THROUGHT [R:%s]\n", tokens[i + 1].str);
					break;
				case 2:
					printf("^--- AT [R:%s]\n", tokens[i + 1].str);
					break;
				case 3:
					break;
				case 4:
					break;
				case 5:
					break;
				}
				continue;
			}
		}
	}

	fclose(inputfile);

	return 0;
}

int main(int argc, char **argv)
{
	FILE *inputfile = NULL;
	FILE *outfile = NULL;
	int mode = 0;

	int ip = 0;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-o"))
		{
			mode = 1;
		}
		else if (!strncmp(argv[i], "-cpu=", 5))
		{
			mode = 0;
			continue;
		}
		else
		{
			switch (mode)
			{
			case 0:
				compile(argv[i]);
				break;
			case 1:
				outfile = fopen(argv[i], "w");
				if (!outfile)
					return 0;
				ZOBJ_H.magic = __builtin_bswap32(((((('Z' << 8) | 'O') << 8) | 'B') << 8) | 'J');
				ZOBJ_H.lkC = 0;
				ZOBJ_H.lkP = 0;
				ZOBJ_H.fnC = 0;
				ZOBJ_H.fnP = 0;

				fwrite(&ZOBJ_H, sizeof(ZOBJ_H), 1, outfile);
				mode = 0;
				continue;
				break;
			}
		}
	}

	if (!outfile)
		return -1;

	ZOBJ_H.lkC = lkdt.lkp;
	ZOBJ_H.lkP = ftell(outfile);
	ZOBJ_H.fnC = fndt.fnp;

	struct
	{
		unsigned short P;
		unsigned short I;
	} zobj_el;

	for (int i = 0; i < ZOBJ_H.lkC; i++)
	{
		zobj_el.P = lkdt.lke[i].address + sizeof(ZOBJ_H);
		zobj_el.I = i;
		fwrite(&zobj_el, sizeof(zobj_el), 1, outfile);
	}

	for (int i = 0; i < ZOBJ_H.lkC; i++)
	{
		fwrite(lkdt.lke[i].name, strlen(lkdt.lke[i].name) + 1, 1, outfile);
	}

	ZOBJ_H.fnP = ftell(outfile);

	for (int i = 0; i < ZOBJ_H.lkC; i++)
	{
		zobj_el.P = fndt.fne[i].address + sizeof(ZOBJ_H);
		zobj_el.I = i;
		fwrite(&zobj_el, sizeof(zobj_el), 1, outfile);
	}

	for (int i = 0; i < ZOBJ_H.lkC; i++)
	{
		fwrite(fndt.fne[i].name, strlen(fndt.fne[i].name) + 1, 1, outfile);
	}

	fseek(outfile, 0, SEEK_SET);
	fwrite(&ZOBJ_H, sizeof(ZOBJ_H), 1, outfile);

	return 0;
}