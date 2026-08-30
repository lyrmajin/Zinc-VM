#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned short encodeA(char Opcode, char rX, char rY, char inm)
{
	return ((((Opcode & 0x0f) << 4) | ((rX & 0x03) << 2) | (rY & 0x03)) << 8) | inm;
}
unsigned short encodeB(char Opcode, char rX, char rY)
{
	return ((((Opcode & 0x0f) << 4) | ((rX & 0x03) << 2) | (rY & 0x03)) << 8) | (((rX & 0x04) >> 1) | ((rX & 0x04) >> 2));
}

void emitCode(FILE *output, unsigned short instruction)
{
	fwrite(&instruction, sizeof(unsigned short), 1, output);
}

#define OPCODE_LIST 16
const char *opcodes[] = {
	"mvi",
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
	"brn",
	"shl",
	"shr",
	"tgl",
	"sys",
};
const char opcodeID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
int getOpcodeId(char *name)
{
	for (int i = 0; i < OPCODE_LIST; i++)
		if (!strcmp(name, opcodes[i]))
			return opcodeID[i];
	return -1;
}

#define OPCODE_VAR_LIST 12
const char *opcodes_var[] = {
	"mvil",
	"mvih",
	"dmvi",
	"zero",
	"jmp",
	"rjmp",
	"call",
	"ret",
	"ebrn",
	"qbrn",
	"erbrn",
	"qrbrn"};
const char opcodeVarID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
int getOpcodeVariantId(char *name)
{
	for (int i = 0; i < OPCODE_VAR_LIST; i++)
		if (!strcmp(name, opcodes_var[i]))
			return opcodeVarID[i];
	return -1;
}

int getRegister(char *rg)
{
	for (int i = 0; rg[i]; i++)
	{
		if (rg[i] == 'r' && rg[i + 1] - '0' < 10)
			return atoi(rg + i + 1);
	}
	return -1;
}

int getInmediate(char *tk)
{
	if (tk[0] >= '0' && tk[0] <= '9')
		return atoi(tk);
	return -1;
}

int main(int argc, char **argv)
{
	FILE *file = NULL;
	FILE *ff = NULL;
	int mode = 0;

	int lbp = 0;
	char *lbls[128] = {NULL};
	unsigned short lbip[128] = {0};

	int lkp = 0;
	char *lkls[128] = {NULL};
	unsigned short lkip[128] = {0};

	int ip = 0;
	struct
	{
		unsigned int magic;
		unsigned short lkC;
		unsigned short lkP;
		unsigned short fnC;
		unsigned short fnP;
	} ZOBJ_H;

	for (int k = 1; k < argc; k++)
	{
		if (!strcmp(argv[k], "-o"))
		{
			mode = 1;
		}
		else if (!strncmp(argv[k], "-cpu=", 5))
		{
			mode = 0;
			continue;
		}
		else
		{
			char *line = NULL;
			long long size = 0;
			size_t tsize = 0;
			char *tk = NULL;

			int opcode = -1;
			int variant = -1;
			int rX = -1;
			int rY = -1;
			int inm = -1;
			int opmode = 0;

			switch (mode)
			{
			case 0:
				file = fopen(argv[k], "rb");
				while (1)
				{
					size = getline(&line, &tsize, file);
					if (size <= 0)
						break;
					tk = strtok(line, " ");

					opcode = -1;
					variant = -1;
					rX = -1;
					rY = -1;
					inm = -1;
					opmode = 0;
					int doMod = 0;
					for (int i = 0; tk; i++)
					{
						printf("%u:%s\n", i, tk);

						switch (opmode)
						{
						case 0:
							opcode = getOpcodeId(tk);
							variant = getOpcodeVariantId(tk);
							printf("%s:[OC:%d]:[VT:%d]\n", tk, opcode, variant);
							if (opcode == -1)
							{
								if (variant != -1)
								{
									doMod = 1;
									if (variant >= 0 && variant < 4)
									{
										opcode = 0;
									}
									else if (variant >= 4 && variant < 8)
									{
										opcode = 10;
										variant -= 4;
									}
									else if (variant >= 8 && variant < 12)
									{
										opcode = 11;
										variant -= 8;
									}
								}
								else
								{
									char *ps = calloc(128, 1);
									int lkmm = 0;
									while (tk[lkmm] && tk[lkmm] != '\n' && tk[lkmm] != ':')
										lkmm++;
									strncpy(ps, tk, lkmm);
									printf("LABEL:[%s]\n", ps);
									lbls[lbp++] = ps;
									lbip[lbp - 1] = ip;
									tk = strtok(NULL, " ");
									continue;
								}
							}
							else
							{
							}
							printf("%s:[OC:%d]:[VT:%d]\n", tk, opcode, variant);
							opmode++;
							break;
						case 1:
							printf("doMod:%u\n", doMod);
							rX = getRegister(tk);
							printf("RX:[D:%d]\n", rX);
							if (rX == -1)
							{
								if (inm == -1)
								{
									inm = getInmediate(tk);
									printf("INM:[D:%d]\n", inm);
									if (inm == -1)
									{
										printf("LABEL:[%s]\n", tk);
										char *spr[] = {"ip", "sp", "fp", "tp"};
										for (int l = 0; l < 4; l++)
											if (!strncmp(tk, spr[l], 2))
											{
												rX = l + 4;
												printf("RX:[D:%d]\n", rX);
												break;
											}
									}
								}
							}
							opmode++;
							if (doMod)
							{
								rY = rX;
								rX = variant;
								printf("RX:[D:%d]\n", rX);
								printf("RY:[D:%d]\n", rY);
							}
							break;
						case 2:
							if (rY == -1)
								rY = getRegister(tk);
							else
								opmode++;

							printf("RY:[D:%d]\n", rY);
							if (rY == -1)
							{
								if (inm == -1)
								{
									inm = getInmediate(tk);
									printf("INM:[D:%d]\n", inm);
									if (inm == -1)
									{
										printf("LABEL:[%s]\n", tk);
										if (rY == -1)
										{
											char *ps = calloc(128, 1);
											int lkmm = 0;
											while (tk[lkmm] && tk[lkmm] != ',' && tk[lkmm] != '\n')
												lkmm++;
											strncpy(ps, tk, lkmm);
											printf("LABEL:[%s]\n", ps);
											lkls[lkp++] = ps;
											lkip[lkp - 1] = ip;
										}
									}
								}
							}
							opmode++;
							break;
						default:
							if (inm == -1)
							{
								inm = getInmediate(tk);
								printf("INM:[D:%d]\n", inm);
								if (inm == -1)
								{
									char *ps = calloc(128, 1);
									int lkmm = 0;
									while (tk[lkmm] && tk[lkmm] != ',' && tk[lkmm] != '\n')
										lkmm++;
									strncpy(ps, tk, lkmm);
									printf("LABEL:[%s]\n", ps);
									lkls[lkp++] = ps;
									lkip[lkp - 1] = ip;
									inm = 0;
								}
							}
							break;
						}

						tk = strtok(NULL, " ");
					}

					if (inm == -1)
						inm = 0;

					unsigned short outbyte = 0;

					switch (opcode)
					{
					case 1:
						outbyte = encodeB(opcode, ((rX > 0) ? rX : 0), ((rY > 0) ? rY : 0));
						printf("I:0x%04X:O[%u]X[%u]Y[%u]\n", outbyte, opcode, ((rX > 0) ? rX : 0), ((rY > 0) ? rY : 0));
						break;
					case -1:
						break;
					default:
						outbyte = encodeA(opcode, ((rX > 0) ? rX : 0), ((rY > 0) ? rY : 0), inm);
						printf("I:0x%04X:O[%u]X[%u]Y[%u][%u]\n", outbyte, opcode, ((rX > 0) ? rX : 0), ((rY > 0) ? rY : 0), inm);
						break;
					}

					if (opcode != -1)
					{
						emitCode(ff, __builtin_bswap16(outbyte));
						ip += 2;
					}
				}
				break;
			case 1:
				ff = fopen(argv[k], "w");
				ZOBJ_H.magic = __builtin_bswap32(((((('Z' << 8) | 'O') << 8) | 'B') << 8) | 'J');
				ZOBJ_H.lkC = 0;
				ZOBJ_H.lkP = 0;
				ZOBJ_H.fnC = 0;
				ZOBJ_H.fnP = 0;

				fwrite(&ZOBJ_H, sizeof(ZOBJ_H), 1, ff);
				mode = 0;
				continue;
				break;
			}
		}
	}

	ZOBJ_H.lkC = lkp;
	ZOBJ_H.lkP = ftell(ff);
	ZOBJ_H.fnC = lbp;

	struct
	{
		unsigned short fnP;
		unsigned short fnI;
	} zobj_lk;

	for (int i = 0; i < lbp; i++)
	{
		zobj_lk.fnP = lkip[i] + sizeof(ZOBJ_H);
		zobj_lk.fnI = i;
		fwrite(&zobj_lk, sizeof(zobj_lk), 1, ff);
	}

	for (int i = 0; i < lbp; i++)
	{
		fwrite(lkls[i], strlen(lkls[i]) + 1, 1, ff);
	}

	ZOBJ_H.fnP = ftell(ff);

	for (int i = 0; i < lbp; i++)
	{
		zobj_lk.fnP = lbip[i] + sizeof(ZOBJ_H);
		zobj_lk.fnI = i;
		fwrite(&zobj_lk, sizeof(zobj_lk), 1, ff);
	}

	for (int i = 0; i < lbp; i++)
	{
		fwrite(lbls[i], strlen(lbls[i]) + 1, 1, ff);
	}

	fseek(ff, 0, SEEK_SET);
	fwrite(&ZOBJ_H, sizeof(ZOBJ_H), 1, ff);

	return 0;
}