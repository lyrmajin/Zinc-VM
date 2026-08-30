#include <cstdio>
#include <iostream>
#include <bitset>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"
#define RESET "\033[0m"

uint16_t PC = 0;
uint16_t ORG = 0;
uint16_t HC = 0;

#define ZHEX 0b0001
#define ZBIN 0b0010
#define ZDEC 0b0100
#define ZCHR 0b1000

bool debug = false;
bool warnall = true;

std::string globiflname;
std::string globoflname;

uint16_t globiyl = 0;
uint16_t globiyl2 = 0;

uint16_t errc = 0;
uint16_t warnc = 0;

std::string sstr;
uint8_t HEX[3] = {0};
uint8_t mode = 0;
std::vector<std::string> Name, Size;
std::vector<std::vector<std::string>> Value;
std::string name, size, tmpval;

void trim(std::string &str, char c)
{
    str.erase(std::remove(str.begin(), str.end(), c), str.end());
}

std::string trim_ends(const std::string &str, char c)
{
    auto is_not_c = [c](char ch)
    { return ch != c; };

    auto start = std::find_if(str.begin(), str.end(), is_not_c);
    auto end = std::find_if(str.rbegin(), str.rend(), is_not_c).base();

    if (start >= end)
        return "";
    else
        return std::string(start, end);
}

void emiterr(std::string format, ...)
{
    va_list args;
    va_start(args, format);
    printf("%s%s:%d: %sERROR:%s ", WHITE, globiflname.c_str(), globiyl, YELLOW, RESET);
    vprintf(format.c_str(), args);
    printf("                %s'%s'%s\n", GREEN, trim_ends(sstr, ' ').c_str(), RESET);
    errc++;

    va_end(args);
}
void emitferr(std::string format, ...)
{
    va_list args;
    va_start(args, format);

    printf("%s%s:%d: %sFATAL ERROR:%s ", WHITE, globiflname.c_str(), globiyl, RED, RESET);
    vprintf(format.c_str(), args);

    va_end(args);
    std::remove(globoflname.c_str());
    exit(1);
}
void emitwarn(std::string format, ...)
{
    va_list args;
    va_start(args, format);

    if (warnall)
    {
        printf("%s%s:%d: %sWARNING:%s ", WHITE, globiflname.c_str(), globiyl, MAGENTA, RESET);
        vprintf(format.c_str(), args);
        printf("                %s'%s'%s\n\n", GREEN, trim_ends(sstr, ' ').c_str(), RESET);

        warnc++;
    }

    va_end(args);
}

void emitdebug(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (debug)
        vprintf(format, args);

    va_end(args);
}

void emit(std::ofstream &fl, const uint8_t *byte, size_t size)
{
    if (!fl)
        emitferr("Couldn't open file \'%s\'.\n", globoflname.c_str());
    for (uint8_t i = 0; i < size; i++)
    {
        fl << byte[i];
        emitdebug("WRITTED BYTE[%d] = 0x%X TO FILE \'%s\'\n", PC + i, (uint16_t)byte[i], globoflname.c_str());
    }
    PC += size;
    return;
}

void emitL(std::ofstream &fl, const uint8_t *byte, size_t size, uint16_t address)
{
    if (!fl)
        emitferr("Couldn't open file \'%s\'.\n", globoflname.c_str());
    fl.seekp(address);
    for (uint8_t i = 0; i < size; i++)
    {
        fl.put(byte[i]);
        emitdebug("REWRITTED BYTE[%d] = 0x%X TO FILE \'%s\'\n", address + i, (uint16_t)byte[i], globoflname.c_str());
    }
    return;
}

bool is_reg(std::string &str)
{
    return ((str.find('R') != std::string::npos) && isdigit(str[1]));
}

bool is_inm(std::string &str, uint8_t f = 0b1111)
{
    size_t hash_pos = str.find('#');
    if (hash_pos == std::string::npos || hash_pos + 1 >= str.size())
        return false;
    if (str[hash_pos + 1] == '0' && (f & 0b0111))
    {
        if (hash_pos + 2 >= str.size())
            return false;

        char next = str[hash_pos + 2];
        return ((next == 'x' && (f & 0b0100)) || (next == 'b' && (f & 0b0010)) || (isdigit(next) && (f & 0b0001)));
    }
    return ((isalnum(str[hash_pos + 1]) && str[hash_pos + 1] != '0') && (f & 0b1000));
}

bool is_char(const std::string &str)
{
    size_t hash_pos = str.find('\'');
    if (hash_pos == std::string::npos || hash_pos + 1 >= str.size())
        return false;
    return (isalnum(str[hash_pos + 1]) && (str[hash_pos + 2] == '\''));
}

bool is_hex(const std::string &str)
{
    size_t hash_pos = str.find("0x");
    if (hash_pos == std::string::npos || hash_pos + 1 >= str.size())
        return false;
    else
        return true;
}

bool is_bin(const std::string &str)
{
    size_t hash_pos = str.find("0b");
    if (hash_pos == std::string::npos || hash_pos + 1 >= str.size())
        return false;
    else
        return true;
}

bool is_dec(const std::string &str)
{
    if (str.size() < 1)
        return false;
    for (uint16_t i = 0; i < str.size(); i++)
        if (!isdigit(str[i]))
            return false;
    return true;
}

uint64_t get_inm(std::string str)
{
    if (is_bin(str))
    {
        size_t bin_pos = str.find('b');
        if (!is_dec(str.substr(bin_pos)))
        {
            emiterr("BIN conversion failed: Unknown \'%s\'", str.c_str());
            return 0xFE00000000;
        }
        return (std::stoi(str.substr(bin_pos), NULL, 2) & 0x00FFFFFFFF);
    }
    if (is_hex(str))
    {
        size_t hex_pos = str.find('x');
        for (uint8_t i = hex_pos + 1; i < str.size(); i++)
        {
            if (!isxdigit(str[i]))
            {
                emiterr("HEX conversion failed: Unknown \'%c\'\n", str[i]);
                return 0xFE00000000;
            }
        }
        return (std::stoi(str.substr(hex_pos + 1), NULL, 16) & 0x00FFFFFFFF);
    }
    if (is_dec(str))
    {
        return (std::stoi(str, NULL, 10) & 0x00FFFFFFFF);
    }
    emiterr("Conversion failed: Unknown type for \'%s\'\n", str.c_str());
    return 0xFF00000000;
}

uint64_t get_reg(std::string str)
{
    if (is_reg(str))
    {
        size_t reg_pos = str.find('R');
        if (isdigit(str[reg_pos + 1]))
        {
            if ((str[reg_pos + 1] - '0') <= 3)
                return (str[reg_pos + 1] - '0');
            else
                emiterr("Register out of bounds: \'%s\'\n", str.c_str());
        }
    }
    return 0xFF00000000;
}

struct UNRESOLVED
{
    std::vector<std::string> TAG_NAME;
    std::vector<std::string> OPCODE;
    std::vector<std::string> SEC;
    std::vector<uint16_t> POGRAM_COUNT;
    std::vector<uint16_t> LINE;
};

UNRESOLVED URS;

void COMPILE(char *str)
{

    //TODO: Make all varnames make sense
    globiyl2 += globiyl;
    globiyl = 0;
    std::ifstream file(str);
    std::ofstream fl;
    fl.open(globoflname, std::ios::binary | std::ios::app);
    if (!file)
        emitferr("Couldn't open file \'%s\'.\n", str);
    if (!fl)
        emitferr("Couldn't open file \'%s\'.\n", globoflname.c_str());

    while (std::getline(file, sstr))
    {
        globiyl++;
        if (sstr.find(";") != std::string::npos)
            sstr = sstr.substr(0, sstr.find(";"));
        if (sstr.find(".") != std::string::npos)
        {
            std::istringstream is(sstr);
            std::string IN;
            if (is >> IN)
            {
                if (IN == ".code:")
                    mode = 0;
                else if (IN == ".data:")
                    mode = 1;
                else if (IN.find(".org:") != std::string::npos)
                {
                    ORG = get_inm(IN.substr(IN.find("0")));
                    PC = ORG;
                    for (uint16_t i = 0; i < PC; i++)
                        fl << (uint8_t)0;
                }
                else
                {
                    mode = 0;
                    emitwarn("Undefined function/macro/section: '%s'\n", IN.c_str());
                }
                continue;
            }
        }
        if (sstr.find(":") != std::string::npos)
        {
            trim(sstr, ' ');
            for (uint16_t i = 0; i < Name.size(); i++)
                if (!(sstr.substr(0, sstr.find(":"))).compare(Name[i]) && PC != ((get_inm(Value[i][0]) << 0x08) | get_inm(Value[i][1])))
                    emiterr("Label redefinition of: \'%s\'\n", Name[i].c_str());
                else if (PC == ((get_inm(Value[i][0]) << 0x08) | get_inm(Value[i][1])))
                    emitwarn("Redundant Label: '%s' already defined at same location\n", Name[i].c_str());
            Name.push_back(sstr.substr(0, sstr.find(":")));
            Size.push_back("0x" + (std::to_string(2)));
            Value.push_back(std::vector<std::string>());
            Value[Value.size() - 1].push_back((std::to_string(((PC >> 8) & 0xFF))));
            Value[Value.size() - 1].push_back((std::to_string((PC & 0xFF))));
            Value[Value.size() - 1].push_back((std::to_string(globiyl)));
            emitdebug("DATA: BYTE[0] = %s: BYTE[1] = %s\n", Value[Value.size() - 1][0].c_str(), Value[Value.size() - 1][1].c_str());
            emitdebug("TAG: NAME: \"%s\" SIZE: \"%s\" PC: \"%X\"\n", Name[Name.size() - 1].c_str(), Size[Size.size() - 1].c_str(), PC);
            continue;
        }
        trim(sstr, '\r');
        trim(sstr, '\n');
        if (mode == 1)
        {
            std::istringstream iss(sstr);
            emitdebug("INPUT: \"%s\"\n", sstr.c_str());
            if (iss >> name >> size)
            {
                Name.push_back(name);
                Size.push_back(size);
                Value.push_back(std::vector<std::string>());
                while ((iss >> tmpval) && Value[Value.size() - 1].size() < std::stoi(Size[Size.size() - 1].substr(2, Size[Size.size() - 1].size() - 2), NULL, 16))
                {
                    Value[Value.size() - 1].push_back(tmpval);
                    emitdebug("DATA: \'%s\' ", Value[Value.size() - 1][Value[Value.size() - 1].size() - 1].c_str());
                }
                emitdebug("ZASM: COMPILER: TAG: NAME: \"%s\" SIZE: \"%s\"\n", Name[Name.size() - 1].c_str(), Size[Size.size() - 1].c_str());
            }
            continue;
        }
        trim(sstr, ',');
        std::istringstream iss(sstr);
        emitdebug("INPUT: \"%s\"\n", sstr.c_str());
        std::string OP = "", Rs = "", Rd = "", I = "";
        std::streampos pos = iss.tellg();
        if (!(iss >> OP >> Rs >> Rd >> I))
        {
            iss.clear();
            iss.seekg(pos);
            if (!(iss >> OP >> Rs >> Rd))
            {
                iss.clear();
                iss.seekg(pos);
                if (!(iss >> OP >> Rs))
                {
                    iss.clear();
                    iss.seekg(pos);
                    if (!(iss >> OP))
                        continue;
                }
            }
        }
        trim(Rs, ' ');
        trim(Rd, ' ');
        trim(I, ' ');
        emitdebug("ZASM: COMPILER: DECODED: OP: \"%s\" Rs: \"%s\" Rd: \"%s\" I: \"%s\"\n", OP.c_str(), Rs.c_str(), Rd.c_str(), I.c_str());
        if (!strcmp(OP.c_str(), "MVI"))
        {
#undef OPSIZE
#define OPSIZE 3
            if (is_reg(Rs) && is_inm(Rd, (ZHEX | ZBIN | ZDEC)))
            {
                uint64_t inm = get_inm(Rd);
                HEX[0] = (0b00001000) | (get_reg(Rs) & 0x03);
                if ((inm & 0xFF00000000000000))
                    emiterr("Unable to convert inmediate: '%s'\n", Rd.c_str());
                else if (((inm & 0x00FFFFFFFFFFFFFF) >> 16))
                    emitwarn("Inmediate Overflow: '0x%X'\n", inm);
                HEX[1] = inm >> 0x08;
                HEX[2] = inm;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rd, (ZCHR)))
            {
                URS.TAG_NAME.push_back(Rd);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000) | (get_reg(Rs) & 0x03);
                for (uint16_t i = 0; i < OPSIZE - 1; i++)
                {
                    HEX[i + 1] = 0;
                }
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
        }
        else if (!strcmp(OP.c_str(), "MVIH"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_inm(Rd, (ZHEX | ZBIN | ZDEC)))
            {
                uint64_t inm = get_inm(Rd);
                HEX[0] = (0b00000000) | (get_reg(Rs) & 0x03);
                if ((inm & 0xFF00000000000000))
                    emiterr("Unable to convert inmediate: '%s'\n", Rd.c_str());
                else if (((inm & 0x00FFFFFFFFFFFFFF) >> 16))
                    emitwarn("Inmediate Overflow: '0x%X'\n", inm);
                HEX[1] = inm >> 0x08;
                HEX[2] = inm;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rd, (ZCHR)))
            {
                URS.TAG_NAME.push_back(Rd);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000) | (get_reg(Rs) & 0x03);
                for (uint16_t i = 0; i < OPSIZE - 1; i++)
                {
                    HEX[i + 1] = 0;
                }
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
        }
        else if (!strcmp(OP.c_str(), "MVIL"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_inm(Rd, (ZHEX | ZBIN | ZDEC)))
            {
                uint64_t inm = get_inm(Rd);
                HEX[0] = (0b00000100) | (get_reg(Rs) & 0x03);
                if ((inm & 0xFF00000000000000))
                    emiterr("Unable to convert inmediate: '%s'\n", Rd.c_str());
                else if (((inm & 0x00FFFFFFFFFFFFFF) >> 16))
                    emitwarn("Inmediate Overflow: '0x%X'\n", inm);
                HEX[1] = inm >> 0x08;
                HEX[2] = inm;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rd, (ZCHR)))
            {
                URS.TAG_NAME.push_back(Rd);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000) | (get_reg(Rs) & 0x03);
                for (uint16_t i = 0; i < OPSIZE - 1; i++)
                {
                    HEX[i + 1] = 0;
                }
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
        }
        else if (!strcmp(OP.c_str(), "MOV"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b00010000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "ADD"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b00100000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "SUB"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b00110000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "AND"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b01000000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "OR"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b01010000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "XOR"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b01100000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "CMP"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b01110000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "STA"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b10010000) | (get_reg(Rd) & 0x03);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "STR"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b10010100) | (get_reg(Rd) & 0x03);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                bool resolved;
                for (uint16_t i = 0; i < Name.size(); i++)
                {
                    if (!Rs.compare(('#' + Name[i])))
                    {
                        for (uint8_t j = 0; j < (get_inm(Size[i]) & 0xFF); j++)
                        {
                            HEX[0] = (0b00001000);
                            HEX[1] = 0;
                            HEX[2] = get_inm(Value[i][j]);
                            emit(fl, HEX, 3 * sizeof(uint8_t));
                            HEX[0] = (0b00001001);
                            HEX[1] = (HC >> 0x08);
                            HEX[2] = (HC);
                            emit(fl, HEX, 3 * sizeof(uint8_t));
                            HEX[0] = (0b10010101);
                            HEX[1] = (0b00000000);
                            emit(fl, HEX, 2 * sizeof(uint8_t));
                            HC++;
                        }
                        resolved = true;
                    }
                }
                if (!resolved)
                    emiterr("Undefined Label/Tag: \'%s\'\n", Rs.c_str());
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "LDA"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b10000000) | (get_reg(Rd) & 0x03);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "LDR"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b10000100) | (get_reg(Rd) & 0x03);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "PUSH"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs))
            {
                HEX[0] = (0b10011000);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                bool resolved;
                for (uint16_t i = 0; i < Name.size(); i++)
                {
                    if (!Rs.compare(('#' + Name[i])))
                    {
                        for (uint8_t j = 0; j < (get_inm(Size[i]) & 0xFF); j++)
                        {
                            HEX[0] = (0b00001000);
                            HEX[1] = 0;
                            HEX[2] = get_inm(Value[i][j]);
                            emit(fl, HEX, 3 * sizeof(uint8_t));
                            HEX[0] = (0b10011000);
                            HEX[1] = 0;
                            emit(fl, HEX, 2 * sizeof(uint8_t));
                        }
                        resolved = true;
                    }
                }
                if (!resolved)
                    emiterr("Undefined Label/Tag: \'%s\'\n", Rs.c_str());
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "POP"))
        {
#undef OPSIZE
#define OPSIZE 2
            if (is_reg(Rs))
            {
                HEX[0] = (0b10001000);
                HEX[1] = (get_reg(Rs) & 0x03) << 0x06;
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "JMP"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10100000) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10100000);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "RJMP"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10100100) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10100100);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "NJMP"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10101000) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10101000);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "CALL"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10101100) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10101100);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "RET"))
        {
#undef OPSIZE
#define OPSIZE 1
            HEX[0] = (0b10101111);
            emit(fl, HEX, OPSIZE * sizeof(uint8_t));
        }
        else if (!strcmp(OP.c_str(), "BRE"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10110000) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10110000);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "BRQ"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10110100) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10110100);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "RBRE"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10111000) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10111000);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "RBRQ"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs))
            {
                HEX[0] = (0b10111100) | (get_reg(Rs) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_inm(Rs, ZCHR))
            {
                URS.TAG_NAME.push_back(Rs);
                URS.OPCODE.push_back(OP);
                URS.SEC.push_back(sstr);
                URS.POGRAM_COUNT.push_back(PC);
                URS.LINE.push_back(globiyl);
                HEX[0] = (0b00001000);
                HEX[1] = 0;
                HEX[2] = 0;
                emit(fl, HEX, 3 * sizeof(uint8_t));
                HEX[0] = (0b10111100);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "SHL"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b11000000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "SHR"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b11010000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "SCB"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b11100000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else if (!strcmp(OP.c_str(), "SYS"))
        {
#undef OPSIZE
#define OPSIZE 1
            if (is_reg(Rs) && is_reg(Rd))
            {
                HEX[0] = (0b11110000) | ((get_reg(Rs) & 0x03) << 0x02) | (get_reg(Rd) & 0x03);
                emit(fl, HEX, OPSIZE * sizeof(uint8_t));
            }
            else if (is_reg(Rs))
                emiterr("Missing/Incorrect operand Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else if (is_reg(Rd))
                emiterr("Missing/Incorrect operand Rs on \'%s\'\n", trim_ends(sstr, ' ').c_str());
            else
                emiterr("Missing/Incorrect operands Rs and Rd on \'%s\'\n", trim_ends(sstr, ' ').c_str());
        }
        else
        {
            emiterr("Unknown Opcode '%s'\n", OP.c_str());
        }
    }
}

void RESOLVE()
{
    globiyl2 += globiyl;
    std::ofstream fl(globoflname, std::ios::in | std::ios::out | std::ios::binary);
    for (uint16_t k = 0; k < URS.TAG_NAME.size(); k++)
        emitdebug("TagDefName:[%s]TagDefPC:[%d]\n", Name[k].c_str(), get_inm(Value[k][0].c_str()) | get_inm(Value[k][1].c_str()));

    for (uint16_t k = 0; k < URS.TAG_NAME.size(); k++)
        emitdebug("TagUseName:[%s]TagUsePC:[%d]\n", URS.TAG_NAME[k].c_str(), URS.POGRAM_COUNT[k]);

    if (URS.TAG_NAME.size() != 0)
    {
        uint8_t resolved;
        emitdebug("RESOLVING\n");
        for (uint16_t i = 0; i < URS.TAG_NAME.size(); i++)
        {
            globiyl = URS.LINE[i];
            sstr = URS.SEC[i];
            resolved = false;
            for (uint16_t j = 0; j < Name.size(); j++)
            {
                if (!URS.TAG_NAME[i].compare(('#' + Name[j])))
                {
                    emitdebug("%s %s\n", URS.OPCODE[i].c_str(), URS.TAG_NAME[i].c_str());
                    if (!URS.OPCODE[i].compare("MVI"))
                    {
                        for (uint8_t o = 0; o < 2; o++)
                        {
                            HEX[o] = get_inm(Value[j][o]);
                        }
                        emitL(fl, HEX, 2 * sizeof(uint8_t), (URS.POGRAM_COUNT[i] + 1));
                        resolved = true;
                    }
                    else if (!URS.OPCODE[i].compare("MVIH") || !URS.OPCODE[i].compare("MVIL"))
                    {
                        for (uint8_t o = 0; o < 1; o++)
                        {
                            HEX[o] = get_inm(Value[j][o]);
                        }
                        emitL(fl, HEX, 1 * sizeof(uint8_t), (URS.POGRAM_COUNT[i] + 1));
                        resolved = true;
                    }
                    else if (!URS.OPCODE[i].compare("JMP") || !URS.OPCODE[i].compare("CALL") || !URS.OPCODE[i].compare("RET") || !URS.OPCODE[i].compare("BRE") || !URS.OPCODE[i].compare("BRQ"))
                    {
                        for (uint8_t o = 0; o < 2; o++)
                        {
                            HEX[o] = get_inm(Value[j][o]);
                            emitdebug("VAL: \'%d\': \'%s\'\n", get_inm(Value[j][o]), Value[j][o].c_str());
                        }
                        emitL(fl, HEX, 2 * sizeof(uint8_t), (URS.POGRAM_COUNT[i] + 1));
                        resolved = true;
                    }
                    else if (!URS.OPCODE[i].compare("NJMP"))
                    {
                        int16_t wrapped = (((get_inm(Value[j][0]) << 0x08) | get_inm(Value[j][1])) - (URS.POGRAM_COUNT[i] + 3));
                        HEX[0] = wrapped >> 0x08;
                        HEX[1] = wrapped;
                        emitdebug("VAL: %d\n", wrapped);
                        if (wrapped > 0)
                            emitwarn("%s to forward Label contains undefined behavior \'%s\' defined at line %d\n", URS.OPCODE[i].c_str(), URS.TAG_NAME[i].c_str(), get_inm(Value[j][2]));
                        emitL(fl, HEX, 2 * sizeof(uint8_t), (URS.POGRAM_COUNT[i] + 1));
                        resolved = true;
                    }
                    else if (!URS.OPCODE[i].compare("RJMP") || !URS.OPCODE[i].compare("RBRE") || !URS.OPCODE[i].compare("RBRQ"))
                    {
                        int16_t wrapped = (((get_inm(Value[j][0]) << 0x08) | get_inm(Value[j][1])) - (URS.POGRAM_COUNT[i] + 3));
                        HEX[0] = wrapped >> 0x08;
                        HEX[1] = wrapped;
                        emitdebug("VAL: %d\n", wrapped);
                        if (wrapped <= 0)
                            emitwarn("%s to backward Label contains undefined behavior \'%s\' defined at line %d\n", URS.OPCODE[i].c_str(), URS.TAG_NAME[i].c_str(), get_inm(Value[j][2]));
                        emitL(fl, HEX, 2 * sizeof(uint8_t), (URS.POGRAM_COUNT[i] + 1));
                        resolved = true;
                    }
                }
            }
            if (!resolved)
                emiterr("Undefined Label/Tag: \'%s\'\n", URS.TAG_NAME[i].c_str());
        }
    }
}

int main(int argc, char **argv)
{
    for (uint16_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o"))
        {
            if ((i + 1) < argc)
            {
                globoflname = argv[i + 1];
                std::ofstream fl(globiflname, std::ios::out | std::ios::trunc);
                fl.close();
                i++;
            }
        }
        else if (!strcmp(argv[i], "-v"))
            debug = true;
        else if (!strcmp(argv[i], "-Wall"))
            warnall = true;
        else if (!strcmp(argv[i], "-Wnone"))
            warnall = false;
        else if (!strcmp(argv[i], "--version"))
        {
            printf("ZASM:1.9.0\n");
            return 0;
        }
        else
        {
            globiflname = argv[i];
            COMPILE(argv[i]);
        }
    }
    RESOLVE();
    if (warnc || errc)
    {
        printf("Compilation failed: %d errors, %d warnings\n", errc, warnc);
        printf("Wrote %u bytes (from %d) to '%s'. %u lines\n", (PC - ORG), ORG, globoflname.c_str(), globiyl2);
    }
    if (errc)
    {
        std::remove(globoflname.c_str());
        return 1;
    }
    return 0;
}