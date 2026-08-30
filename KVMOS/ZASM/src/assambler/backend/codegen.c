// GLobal dependences
#include "../dependences.h"

// Internal dependences
#include "backend.h"
#include "../frontend/frontend.h"
#include "utils.h"

// opcodes

const char *opcodes_0[16] = {"mvih", "mov", "add", "sub", "and", "or", "xor", "cmp", "ld", "st", "jmp", "brn", "shl", "shr", "scb", "sys"};
const char *opcodes_1[16] = {"mvil", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "pop", "push", "rjmp", "bre", NULL, NULL, NULL, NULL};
const char *opcodes_2[16] = {"mvi", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "njmp", "brq", NULL, NULL, NULL, NULL};
const char *opcodes_3[16] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "call", "rbre", NULL, NULL, NULL, NULL};
const char *opcodes_4[16] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "ret", "rbrq", NULL, NULL, NULL, NULL};

const char **opcodes[5] = {opcodes_0, opcodes_1, opcodes_2, opcodes_3, opcodes_4};

// operand mask

const uint8_t opcode_rsh[16] = {0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000000, 0b00000000, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000011};
const uint8_t opcode_rsl[16] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};

const uint8_t opcode_rdh[16] = {0b00000011, 0b00001100, 0b00001100, 0b00001100, 0b00001100, 0b00001100, 0b00001100, 0b00001100, 0b00000011, 0b00000011, 0b00000000, 0b00000000, 0b00001100, 0b00001100, 0b00001100, 0b00001100};
const uint8_t opcode_rdl[16] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};

// shift left operand

const uint8_t opcode_slrsh[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t opcode_slrsl[16] = {0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0};

const uint8_t opcode_slrdh[16] = {0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2};
const uint8_t opcode_slrdl[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t generate_modeless(uint8_t i, uint8_t rd, uint8_t rs)
{
    // mask rs with the operand size. (also it must be shifted to aling). Ored with rd aplied equally.
    return (((i & 0xF) << 0x04) | (((opcode_rsh[i] >> opcode_slrsh[i]) & rs) << opcode_slrsh[i]) | (((opcode_rdh[i] >> opcode_slrdh[i]) & rd) << opcode_slrdh[i]));
}

void flush(uint8_t *batch, size_t batchSize, uint64_t i)
{
    if (i <= batchSize && i > 0)
    {
        if (DoInternalsDebug || DoDebug)
            emitdebug("Flushing batch...\n");
        emit(batch, i);
    }
    else if (i > 0)
        emiterr("Unable to flush the batch");
}

int codegen(ASTNODE *astnode, uint8_t *batch, size_t batchSize, uint64_t *batchindex)
{
    if (!astnode)
    {
        emiterr("unable to open output batch/astnodes");
        return -1;
    }
    if (!batch)
    {
        emiterr("unable to allocate batch, defaulting to 1M");
        batchSize = (1024 * 1024);
        batch = (uint8_t *)malloc(sizeof(uint8_t) * batchSize);
        if (!batch)
        {
            emiterr("unable to allocate batch, defaulting to 1K");
            batchSize = 1024;
            batch = (uint8_t *)malloc(sizeof(uint8_t) * batchSize);
            if (!batch)
            {
                emiterr("unable to allocate batch, defaulting to 10");
                batchSize = 10;
                batch = (uint8_t *)malloc(sizeof(uint8_t) * batchSize);
                if (!batch)
                {
                    emiterr("unable to allocate batch, exiting");
                    exit(-1);
                }
            }
        }
    }
    if (!batchindex)
    {
        batchindex = (uint64_t *)malloc(1);
        if (!batchindex)
        {
            emiterr("unable to open output batch index");
            return -1;
        }
        *batchindex = 0;
    }
    uint64_t j = 0, flushindex = 0, programcount = 0;
    uint8_t opcodeindex = 0, q = 0;
    ASTNODE *scopenode = astnode->parentdnode;
    while (j < astnode->childCount)
    {
        if (astnode->childnode[j]->nodetype == TAG_NODE)
        {
        }
        else if (astnode->childnode[j]->nodetype == OPCODE_NODE)
        {
            opcodeindex = 0;
            for (; opcodeindex < 16; opcodeindex++)
            {
                q = 0;
                for (; q < 5; q++)
                {
                    if (!strcmp(strtolower(astnode->childnode[j]->nodestr), ((opcodes[q][opcodeindex] == NULL) ? "" : opcodes[q][opcodeindex])) && (opcodeindex != 8 && opcodeindex != 9))
                    {
                        programcount += (*batchindex - programcount);
                        if ((*batchindex + 1) < batchSize || *batchindex == 0)
                        {
                            flushindex = *batchindex + 1;
                            batch[(*batchindex == 0) ? 0 : ++(*batchindex)] = ((q << 0x02) | generate_modeless(opcodeindex, 0, 0));
                            j++;
                        }
                        else
                        {
                            flush(batch, batchSize, flushindex);
                            for (uint8_t p = 0; ((flushindex - 1) + p) < *batchindex; p++)
                            {
                                batch[p] = batch[(flushindex - 1) + p];
                            }
                            *batchindex = 0;
                            flushindex = 0;
                        }

                        if (DoInternalsDebug)
                            emitdebug("Loading batch[%u] with 0x%x, opcode n:%u s:%s\n", *batchindex, batch[(*batchindex)], opcodeindex, strtolower(astnode->childnode[j]->nodestr));
                        goto breaker;
                    }
                    if (opcodeindex >= 15)
                    {
                        emiterr("Undefined Opcode %s", astnode->childnode[j]->nodestr);
                        j++;
                    }
                }
                continue;
            breaker:
                break;
            }
        }
        else if (astnode->childnode[j]->nodetype == OPERAND_NODE)
        {
            if (!strcmp(astnode->childnode[j]->nodestr, "#"))
            {
                if ((j + 1) < astnode->childCount)
                {
                    if (astnode->childnode[j + 1]->nodetype == INMEDIATE_NODE)
                    {
                        char *tmp = (astnode->childnode[j + 1]->nodestr);
                        if ((j - 1) >= 0)
                        {
                            if (astnode->childnode[j - 1]->nodetype == OPCODE_NODE)
                            {
                                batch[(*batchindex)] |= generate_modeless(0, strtol(tmp, NULL, 10), 0);
                                j++;
                            }
                            else
                            {
                                if ((j - 2) >= 0)
                                {
                                    if (!strcmp(astnode->childnode[j - 2]->nodestr, opcodes[0][0]) || !strcmp(astnode->childnode[j - 2]->nodestr, opcodes[1][0]) || !strcmp(astnode->childnode[j - 2]->nodestr, opcodes[2][0]))
                                    {
                                        if ((*batchindex + 2) < batchSize)
                                        {
                                            batch[(++(*batchindex))] = ((strtol(tmp, NULL, 10) >> 0x08) & 0xFF);
                                            if (DoInternalsDebug)
                                                emitdebug("Loading batch[%u] with 0x%x, operand s:%s\n", *batchindex, batch[(*batchindex)], astnode->childnode[j + 1]->nodestr);
                                            batch[(++(*batchindex))] = (strtol(tmp, NULL, 10) & 0xFF);
                                            if (DoInternalsDebug)
                                                emitdebug("Loading batch[%u] with 0x%x, operand s:%s\n", *batchindex, batch[(*batchindex)], astnode->childnode[j + 1]->nodestr);
                                            j++;
                                        }
                                        else
                                        {
                                            flush(batch, batchSize, flushindex);
                                            for (uint8_t p = 0; ((flushindex) + p) < (*batchindex + 1); p++)
                                            {
                                                batch[p] = batch[(flushindex - 1) + p];
                                            }

                                            *batchindex = 0;
                                            flushindex = 0;
                                        }
                                    }
                                    else
                                    {
                                        emiterr("Only \"%s\" can load inmediates", opcodes_2[0]);
                                        j++;
                                    }
                                }
                                else
                                {
                                    emiterr("Argument without opcode");
                                    j++;
                                }
                            }
                        }
                        else
                        {
                            emiterr("Argument without opcode");
                            j++;
                        }
                    }
                    else
                    {
                        emiterr("After '#' should be an inmediate value");
                        j++;
                    }
                }
                else
                {
                    emiterr("After '#' should have a value");
                    j++;
                }
            }
            else if (tolower(astnode->childnode[j]->nodestr[0]) == 'r')
            {
                char *tmp = (astnode->childnode[j]->nodestr) + 1;
                if ((j - 1) >= 0)
                {
                    if (astnode->childnode[j - 1]->nodetype == OPCODE_NODE)
                        batch[(*batchindex)] |= generate_modeless(0, 0, strtol(tmp, NULL, 10));
                    else
                        batch[(*batchindex)] |= generate_modeless(0, strtol(tmp, NULL, 10), 0);

                    if (DoInternalsDebug)
                        emitdebug("Loading batch[%u] with 0x%x, operand s:%s\n", *batchindex, batch[(*batchindex)], astnode->childnode[j]->nodestr);
                }
                else
                    emiterr("Argument without opcode");
                j++;
            }
            else
                j++;
        }
        else
        {
            j++;
        }
    }
    flush(batch, batchSize, *batchindex + 1);
    return 0;
}