#define DEBUG_BUILD
#define ATTINY85

#ifdef DEBUG_BUILD

#include <TinyDebug.h>

#if defined(ATMEGA2560)
#define MEM_SIZE (7 * 1024)
#elif defined(ATMEGA328P)
#define MEM_SIZE 1024
#elif defined(ATTINY85)
#define MEM_SIZE 256
#endif

#else

#if defined(ATMEGA2560)
#define MEM_SIZE (7 * 1024) + 256
#elif defined(ATMEGA328P)
#define MEM_SIZE 1024 + 512
#elif defined(ATTINY85)
#define MEM_SIZE 256 + 128
#endif

#endif

uint8_t MEMORY[MEM_SIZE];

#define MAX_REG 6
#define GPR0 0
#define GPR1 1
#define GPR2 2
#define GPR3 3
#define PC 4
#define SP 5
uint16_t REGISTER[MAX_REG] = {0};
uint8_t FL = 0;     // RUNTIME SYSTEM SEGFAULT 0 0 0 0 CMP

uint32_t LastSwitchMillis;
uint8_t currentTask = 0;

//----------------------------------------------
void SysWriteIO(uint16_t argptr = 0);
void SysReadIO(uint16_t argptr = 0);
// 2
void SysScheduleTask(uint16_t argptr = 0);
void SysKillTask(uint16_t argptr = 0);
// 4
void SysMalloc(uint16_t argptr = 0);
void SysFree(uint16_t argptr = 0);
// 6
void SysGetPC(uint16_t argptr = 0);
void SysHalt(uint16_t argptr = 0);
void SysKill(uint16_t argptr = 0);
// 9
void SysOpen(uint16_t argptr = 0);
void SysRead(uint16_t argptr = 0);
// B
void SysExec(uint16_t argptr = 0);

typedef void (*SYSCALL)(uint16_t);
const SYSCALL SYSCALLS[13] PROGMEM = {SysWriteIO, SysReadIO, SysScheduleTask, SysKillTask, SysMalloc, SysFree, SysGetPC, SysHalt, SysKill, SysOpen, SysRead, SysExec};

inline void OP_MVI() { // 0000
  if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = (MEMORY[REGISTER[PC] + 1] << 0x08);
  else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = MEMORY[REGISTER[PC] + 2];
  else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000)) {
    REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = ((MEMORY[REGISTER[PC] + 1] << 0x08) | MEMORY[REGISTER[PC] + 2]);
    REGISTER[PC] += 3;
    return;
  }
  REGISTER[PC] += 2;
}
inline void OP_MOV() { // 0001
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_ADD() { //0010
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_SUB() { //0011
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_AND() { //0100
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] &= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_OR() { //0101
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] |= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_XOR() { //0110
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_CMP() { //0111
  FL &= 0b11111100;
  if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) == 0)
    FL |= 0b00000001; // Z flag
  if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) < 0)
    FL |= 0b00000010; // N flag
  REGISTER[PC]++;
}
inline void OP_LD() { //1000
  uint16_t address = 0;
  if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000) {
    if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
      address = (REGISTER[(MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06]);
    else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100)
      address = ((0x80 + (currentTask * 2)) + (REGISTER[(MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06]));
  }
  else
  {
    address = ((MEM_SIZE) - ((REGISTER[SP] & 0xFF) + (MEMORY[REGISTER[PC] + 1] & 0b00111111)));

    if ((REGISTER[SP] & 0xFF) < ((REGISTER[SP] & 0xFF00) >> 0x08))
      REGISTER[SP] ++;

  }

  if (address <= MEMORY[(0x81 + (currentTask * 2))])
    REGISTER[MEMORY[REGISTER[PC]] & 0b00000011] = MEMORY[address] + (MEMORY[REGISTER[PC] + 1] & 0b00111111);
  else
    FL |= 0b00100000;
  REGISTER[PC] += 2;
}
inline void OP_ST() { //1001
  uint16_t address = 0;
  if ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000) {
    if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000)
      address = (REGISTER[((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06)] + (MEMORY[REGISTER[PC] + 1] & 0b00111111));
    else
      address = (((0x80 + (currentTask * 2)) + (REGISTER[((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06)] + (MEMORY[REGISTER[PC] + 1] & 0b00111111))));
  }
  else
  {
    address = ((MEM_SIZE - 1) - ((REGISTER[SP] & 0xFF) + (MEMORY[REGISTER[PC] + 1] & 0b00111111)));

    if (0 < ((REGISTER[SP] & 0xFF00) >> 0x08))
      REGISTER[SP] --;
  }
  if (address <= MEMORY[(0x81 + (currentTask * 2))])
    MEMORY[address] = REGISTER[MEMORY[REGISTER[PC]] & 0b00000011];
  else
    FL |= 0b00100000;
  REGISTER[PC] += 2;
}
inline void OP_JMP() { //1010
  if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000000))
    REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000100))
    REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001000))
    REGISTER[PC] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  else
    REGISTER[PC]++;
}
inline void OP_BRN() {
  if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && (FL & 0b00000001)) || ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && (FL & 0b00000010)))
  {
    if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
      REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
      REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  }
  else
    REGISTER[PC]++;
}
inline void OP_SHL() { //1100
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_SHR() { //1101
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] >> REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
inline void OP_SCB() { //1110
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= (1 << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
  REGISTER[PC]++;
}
inline void OP_SYS() { //1111
  SYSCALL SYSCALL_PM = (SYSCALL)pgm_read_word(&SYSCALLS[(MEMORY[REGISTER[PC]] & 0b00001100) >> 2]);
  SYSCALL_PM(REGISTER[ (MEMORY[REGISTER[PC]] & 0b00000011)]);
  REGISTER[PC]++;
}
typedef void (*OPCODE)();
const OPCODE OPCODES[16] PROGMEM = {OP_MVI, OP_MOV, OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_CMP, OP_LD, OP_ST, OP_JMP, OP_BRN, OP_SHL, OP_SHR, OP_SCB, OP_SYS};

inline void storeTask() {
  if (MEMORY[currentTask]) {
    MEMORY[MEMORY[currentTask]] = REGISTER[0];
    MEMORY[MEMORY[currentTask] + 1] = (REGISTER[0] >> 0x08);
    MEMORY[MEMORY[currentTask] + 2] = REGISTER[1];
    MEMORY[MEMORY[currentTask] + 3] = (REGISTER[1] >> 0x08);
    MEMORY[MEMORY[currentTask] + 4] = REGISTER[2];
    MEMORY[MEMORY[currentTask] + 5] = (REGISTER[2] >> 0x08);
    MEMORY[MEMORY[currentTask] + 6] = REGISTER[3];
    MEMORY[MEMORY[currentTask] + 7] = (REGISTER[3] >> 0x08);
    MEMORY[MEMORY[currentTask] + 8] = REGISTER[PC];
    MEMORY[MEMORY[currentTask] + 9] = (REGISTER[PC] >> 0x08);
    MEMORY[MEMORY[currentTask] + 10] = REGISTER[SP];
    MEMORY[MEMORY[currentTask] + 11] = (REGISTER[SP] >> 0x08);
    MEMORY[MEMORY[currentTask] + 12] = FL;
  }
}
inline void loadTask() {
  if (MEMORY[currentTask]) {
    REGISTER[0] = (MEMORY[MEMORY[currentTask]] | (MEMORY[MEMORY[currentTask] + 1] << 0x08));
    REGISTER[1] = (MEMORY[MEMORY[currentTask] + 2] | (MEMORY[MEMORY[currentTask] + 3] << 0x08));
    REGISTER[2] = (MEMORY[MEMORY[currentTask] + 4] | (MEMORY[MEMORY[currentTask] + 5] << 0x08));
    REGISTER[3] = (MEMORY[MEMORY[currentTask] + 6] | (MEMORY[MEMORY[currentTask] + 7] << 0x08));
    REGISTER[PC] = (MEMORY[MEMORY[currentTask] + 8] | (MEMORY[MEMORY[currentTask] + 9] << 0x08));
    REGISTER[SP] = (MEMORY[MEMORY[currentTask] + 10] | (MEMORY[MEMORY[currentTask] + 11] << 0x08));
    FL = MEMORY[MEMORY[currentTask] + 12];
  }
}

#define TIME_SLICE 10

void VMloop() {
  while ((FL & 0b01000000)) {
    if ((FL & 0b10000000)) {
      OPCODE OPCODE_PM = (OPCODE)pgm_read_word(&OPCODES[(MEMORY[REGISTER[PC]] & 0xF0) >> 4]);
      OPCODE_PM();
    }

    if (millis() - LastSwitchMillis >= TIME_SLICE) {
      storeTask();
      currentTask = (currentTask < 0x08) ? currentTask + 1 : 0;
      loadTask();
      LastSwitchMillis = millis();
    }
  }
}

void VMsetup()
{
  FL |= 0b11000000;
  LastSwitchMillis = millis();
}

inline void SysWriteIO(uint16_t argptr) {
  *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8))) = MEMORY[argptr + 2];
}

inline void SysReadIO(uint16_t argptr) {
  REGISTER[3] = *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8)));
}

inline void SysScheduleTask(uint16_t argptr) {
  for (uint8_t i = 0; i < 0x08; i++) {
    if (MEMORY[i])
      continue;
    MEMORY[i] = MEMORY[argptr];
    REGISTER[3] = i;
    return;
  }
  REGISTER[3] = 0xFF;
}

inline void SysKillTask(uint16_t argptr) {
  MEMORY[MEMORY[argptr]] = 0;
}

inline void SysMalloc(uint16_t argptr) {
}

inline void SysFree(uint16_t argptr) {
}

inline void SysGetPC(uint16_t argptr) {
  REGISTER[3] = REGISTER[PC];
}

inline void SysHalt(uint16_t argptr) {
  FL &= 0b01111111;
}

inline void SysKill(uint16_t argptr) {
  FL &= 0b10111111;
}

inline void SysOpen(uint16_t argptr) {
  char *file = reinterpret_cast<char*>(&MEMORY[argptr]);
  uint16_t baseadrs = (MEMORY[argptr + strlen(file) + 1] << 8) | (MEMORY[argptr + strlen(file) + 2]);
  for (uint16_t i = baseadrs; i < (baseadrs + 0x08); i += 2) {
    if (MEMORY[i]) {
      if (strcmp(reinterpret_cast<char*>(&MEMORY[baseadrs + MEMORY[i + 1]]), file) == 0) {
        REGISTER[3] = (strlen(reinterpret_cast<char*>(&MEMORY[baseadrs + MEMORY[i + 1]])) + 1 - baseadrs + MEMORY[i + 1]);
        return;
      }
    }
    if (MEMORY[i] == 3) {
      baseadrs = MEMORY[i + 1];
      i = baseadrs;
    }
  }
  REGISTER[3] = 0xFF;
}

inline void SysRead(uint16_t argptr) {
  REGISTER[3] = MEMORY[((MEMORY[argptr] << 8) | (MEMORY[argptr + 1])) + MEMORY[argptr + 2]];
  return;
}

inline void SysExec(uint16_t argptr) {
  SysScheduleTask(0x7F);
  REGISTER[PC] = ((MEMORY[argptr] << 8) | (MEMORY[argptr + 1])) + 1;
}

#if defined(RELEASE_BUILD)

void setup() {
  MEMORY[0x70] = 0x10;
  SysScheduleTask(07F0);
  MEMORY[0x70] = 0x00;
  VMsetup();
}

void loop() {
  VMloop();
}

#elif defined(DEBUG_BUILD)
// DEBUG --------------------------------------------------------------------------------

uint8_t allowStep = 0;

#define INPUT_BUFFER_SIZE 48
char inputBuffer[INPUT_BUFFER_SIZE];

void executeCommand(const char* input) {
  if (strncmp(input, "SET:[0x", 7) == 0) {
    char* mid = strstr(input, "]:[0x");
    char* end = strchr(input + 7, ']');
    if (mid && end && end > mid) {
      char addrStr[8] = {0}, dataStr[4] = {0};
      strncpy(addrStr, input + 7, mid - (input + 7));
      strncpy(dataStr, mid + 5, end - (mid + 5));

      uint16_t addr = strtoul(addrStr, NULL, 16);
      uint8_t data = strtoul(dataStr, NULL, 16);

      if (addr < MEM_SIZE) {
        MEMORY[addr] = data;
        Debug.print(F("Wrote 0x")); Debug.print(data, HEX);
        Debug.print(F(" to 0x")); Debug.println(addr, HEX);
      } else {
        Debug.println(F("SET failed: Address out of range."));
      }
    } else {
      Debug.println(F("Invalid SET syntax."));
    }
  }

  else if (strcasecmp(input, "DUMP") == 0) {
    Debug.println(F("Dumping memory:"));
    for (uint16_t i = 0; i < MEM_SIZE; i++) {
      if (i % 16 == 0) {
        Debug.print(F("\n0x")); Debug.print(i, HEX); Debug.print(F(": "));
      }
      if (MEMORY[i] < 0x10) Debug.print('0');
      Debug.print(MEMORY[i], HEX); Debug.print(' ');
    }
    Debug.println(F("\nDone."));
  }

  else if (strncmp(input, "DUMP:[0x", 8) == 0) {
    char* mid = strstr(input, "]:[0x");
    char* end = strchr(mid ? mid + 5 : NULL, ']');
    if (mid && end) {
      char startStr[8] = {0}, endStr[8] = {0};
      strncpy(startStr, input + 6, mid - (input + 6));
      strncpy(endStr, mid + 5, end - (mid + 5));

      uint16_t start = strtoul(startStr, NULL, 16);
      uint16_t endVal = strtoul(endStr, NULL, 16);

      if (start < endVal && endVal <= MEM_SIZE) {
        Debug.println(F("Dumping memory range:"));
        for (uint16_t i = start; i < endVal; i++) {
          if (i % 16 == 0) {
            Debug.print(F("\n0x")); Debug.print(i, HEX); Debug.print(F(": "));
          }
          if (MEMORY[i] < 0x10) Debug.print('0');
          Debug.print(MEMORY[i], HEX); Debug.print(' ');
        }
        Debug.println(F("\nDone."));
      } else {
        Debug.println(F("Invalid DUMP range."));
      }
    } else {
      Debug.println(F("Invalid DUMP syntax."));
    }
  }

  else if (strcasecmp(input, "STEP") == 0) {
    allowStep = 1;
    Debug.println(F("[DEBUG] Stepping once."));
  }

  else if (strncmp(input, "STEP:[0x", 8) == 0 && input[strlen(input) - 1] == ']') {
    char valStr[5] = {0};
    strncpy(valStr, input + 8, strlen(input) - 9);  // 8 is offset, -1 for ']'
    uint8_t val = strtoul(valStr, NULL, 16);
    allowStep = val ? val : 1;
    Debug.print(F("[DEBUG] Stepping ")); Debug.print(allowStep); Debug.println(F(" times."));
  }

  else {
    if (strlen(input) == 0) {
      return;
    }
    Debug.println(F("Unknown command."));
  }
}

void handleInput() {
  static uint8_t index = 0;

  while (!allowStep) {
    while (!Debug.available());
    char c = Debug.read();
    if (c == '\n' || c == '\r') {
      inputBuffer[index] = '\0';
      if (index > 0) {
        executeCommand(inputBuffer);
        index = 0;
      }
    } else if (index < INPUT_BUFFER_SIZE - 1) {
      inputBuffer[index++] = c;
    } else {
      index = 0; // Buffer overflow fallback
    }
  }
}

void VMloop(boolean DEBUG) {
  while ((FL & 0b01000000)) {
    if ((FL & 0b10000000)) {
      OPCODE OPCODE_PM = (OPCODE)pgm_read_word(&OPCODES[(MEMORY[REGISTER[0]] & 0xF0) >> 4]);
      OPCODE_PM();
    }

    if (millis() - LastSwitchMillis >= TIME_SLICE) {
      Debug.println(F("Task Switched"));
      storeTask();
      currentTask = (currentTask < 0x08) ? currentTask + 1 : 0;
      loadTask();
      LastSwitchMillis = millis();
    }

    Debug.print(F("MEMORY[PC]:"));
    Debug.println(MEMORY[REGISTER[0]], HEX);
    for (int i = 0; i < MAX_REG; i++) {
      Debug.print(F("REGISTER["));
      Debug.print(i);
      Debug.print(F("]:"));
      Debug.println(REGISTER[i], HEX);
    }
    Debug.print(F("FLAGS:"));
    Debug.println(FL, HEX);

    if (DEBUG && allowStep > 0) allowStep--;

    handleInput();
  }
}

void setup() {
  Debug.begin();
  MEMORY[0x70] = 0x10;
  SysScheduleTask(0x70);
  MEMORY[0x70] = 0x00;
  VMsetup();
  Debug.println(F("KVmos 1.2\n"));
}

void loop() {
  VMloop(true);
}

#endif