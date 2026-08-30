#define DEBUG_BUILD
#define ATMEGA2560

#if defined(ATMEGA2560)
#define MAX_MEM 6 * 1024
#elif defined(ATMEGA328P)
#define MAX_MEM 1024
#endif

uint8_t MEMORY[MAX_MEM];

#define MAX_REG 6
#define GPR0 0
#define GPR1 1
#define GPR2 2
#define GPR3 3
#define PC 4
#define SP 5
uint16_t REGISTER[MAX_REG] = {0};
uint8_t FL = 0;     // RUNTIME SYSTEM SEGFAULT 0 0 0 0 CMP

uint32_t LSM = millis();
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
      address = ((0x100 + (((currentTask * 3) <<  0x08) | (currentTask * 4))) + (REGISTER[(MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06]));
  }
  else
  {
    address = (0x2FF - ((REGISTER[SP] & 0xFF) + (MEMORY[REGISTER[PC] + 1] & 0b00111111)));

    if ((REGISTER[SP] & 0xFF) < ((REGISTER[SP] & 0xFF00) >> 0x08))
      REGISTER[SP] ++;
  }

  if (address <= MEMORY[(0x101 + (((currentTask * 3) <<  0x08) | (currentTask * 4)))])
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
      address = (((0x100 + (((currentTask * 3) <<  0x08) | (currentTask * 4))) + (REGISTER[((MEMORY[REGISTER[PC] + 1] & 0b11000000) >> 0x06)] + (MEMORY[REGISTER[PC] + 1] & 0b00111111))));
  }
  else
  {
    address = (0x2FF - ((REGISTER[SP] & 0xFF) + (MEMORY[REGISTER[PC] + 1] & 0b00111111)));

    if (0 < ((REGISTER[SP] & 0xFF00) >> 0x08))
      REGISTER[SP] --;
  }
  if (address <= MEMORY[(0x101 + (((currentTask * 3) <<  0x08) | (currentTask * 4)))])
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

    if (millis() - LSM >= TIME_SLICE) {
      storeTask();
      currentTask = (currentTask < 0x0F) ? currentTask + 1 : 0;
      loadTask();
      LSM = millis();
    }
  }
}

#define VMsetup() (FL |= 0b11000000)

inline void SysWriteIO(uint16_t argptr) {
  *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8))) = MEMORY[argptr + 2];
}

inline void SysReadIO(uint16_t argptr) {
  REGISTER[3] = *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8)));
}

inline void SysScheduleTask(uint16_t argptr) {
  for (uint8_t i = 0; i < 0x0F; i++) {
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
  for (uint16_t i = baseadrs; i < (baseadrs + 0x0F); i += 2) {
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
  SysScheduleTask(0xff);
  REGISTER[PC] = ((MEMORY[argptr] << 8) | (MEMORY[argptr + 1])) + 1;
}

#if defined(RELEASE_BUILD)

void setup() {
  MEMORY[0xF0] = 0x10;
  SysScheduleTask(0xF0);
  MEMORY[0xF0] = 0x00;
  VMsetup();
}

void loop() {
  VMloop();
}

#elif defined(DEBUG_BUILD)
// DEBUG --------------------------------------------------------------------------------

void callloop(boolean DEBUG);

uint8_t allowStep = 0;

void VMloop(boolean DEBUG) {
  while ((FL & 0b01000000)) {
    if ((FL & 0b10000000)) {
      OPCODE OPCODE_PM = (OPCODE)pgm_read_word(&OPCODES[(MEMORY[REGISTER[PC]] & 0xF0) >> 4]);
      OPCODE_PM();
    }

    if (millis() - LSM >= TIME_SLICE) {
      Serial.println("Task Switched");
      storeTask();
      currentTask = (currentTask < 0x0F) ? currentTask + 1 : 0;
      loadTask();
      LSM = millis();
    }

    Serial.print("MEMORY[PC]:");
    Serial.println(MEMORY[REGISTER[PC]], HEX);
    for (int i = 0; i < MAX_REG; i++) {
      Serial.print("REGISTER[");
      Serial.print(i);
      Serial.print("]:");
      Serial.println(REGISTER[i], HEX);
    }
    Serial.print("FLAGS:");
    Serial.println(FL, HEX);

    callloop(true);
    if (allowStep) allowStep--;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("KVmos 1.2");
}

uint16_t memAddr = 0;

void dumpMemory(uint16_t start = 0, uint16_t end = MAX_MEM) {
  Serial.println("Dumping MEMORY:");
  for (uint16_t i = start; i < end; i++) {
    if (i % 16 == 0) {
      Serial.print("\n0x");
      Serial.print(i, HEX);
      Serial.print(": ");
    }
    if (MEMORY[i] < 0x10) Serial.print('0');
    Serial.print(MEMORY[i], HEX);
    Serial.print(' ');
  }
  Serial.println("\nDone.");
}

void executeToken(const String& token) {
  if (token.startsWith("SET:[0x") && token.indexOf("]:[0x") != -1) {
    int sep = token.indexOf("]:[0x");
    int end = token.lastIndexOf(']');

    if (sep > 0 && end > sep) {
      String addrStr = token.substring(7, sep);
      String dataStr = token.substring(sep + 5, end);

      uint16_t addr = strtoul(addrStr.c_str(), nullptr, 16);
      uint8_t data = strtoul(dataStr.c_str(), nullptr, 16);

      if (addr >= MAX_MEM) {
        Serial.println("SET failed: Address out of range.");
      } else {
        MEMORY[addr] = data;
        Serial.print("Wrote 0x");
        Serial.print(data, HEX);
        Serial.print(" to 0x");
        Serial.println(addr, HEX);
      }
    } else {
      Serial.println("Invalid SET syntax.");
    }
  }
  else if (token.equalsIgnoreCase("RUN")) {
    Serial.println("Running VM...");
    VMsetup();
    MEMORY[0xF0] = 0x10;
    SysScheduleTask(0xF0);
    REGISTER[PC] = MEMORY[REGISTER[3]] + 0xD;
    VMloop();
    Serial.println("VM halted.");
  }
  else if (token.equalsIgnoreCase("RUN:[DEBUG]")) {
    Serial.println("Running VM on DEBUG MODE...");
    VMsetup();
    MEMORY[0xF0] = 0x10;
    SysScheduleTask(0xF0);
    MEMORY[0xF0] = 0x00;
    REGISTER[PC] = MEMORY[REGISTER[3]] + 0xD;
    VMloop(true);
    Serial.println("VM halted.");
  }
  else if (token.equalsIgnoreCase("DUMP")) {
    dumpMemory();
  }
  else if (token.startsWith("DUMP:[0x") && token.indexOf("]:[0x") != -1) {
    int sep = token.indexOf("]:[0x");
    int end = token.lastIndexOf(']');

    if (sep > 0 && end > sep) {
      String startStr = token.substring(6, sep);
      String endStr = token.substring(sep + 5, end);

      uint16_t start = strtoul(startStr.c_str(), nullptr, 16);
      uint16_t endVal = strtoul(endStr.c_str(), nullptr, 16);

      if (start >= endVal || endVal > MAX_MEM) {
        Serial.println("Invalid DUMP range.");
      } else {
        dumpMemory(start, endVal);
      }
    } else {
      Serial.println("Invalid DUMP syntax.");
    }
  }
  else if (token.startsWith("INSPECT:[0x") && token.endsWith("]")) {
    String addrStr = token.substring(10, token.length() - 1);
    uint16_t addr = strtoul(addrStr.c_str(), nullptr, 16);
    if (addr >= MAX_MEM) {
      Serial.println("Address out of range.");
    } else {
      Serial.print("MEM[0x"); Serial.print(addr, HEX);
      Serial.print("] = 0x"); Serial.println(MEMORY[addr], HEX);
    }
  }
  else if (token.equalsIgnoreCase("REGS")) {
    Serial.print("PC: 0x"); Serial.println(REGISTER[PC], HEX);
    Serial.print("SP: 0x"); Serial.println(REGISTER[SP], HEX);
    for (int i = 0; i < MAX_REG - 2; i++) {
      Serial.print("R"); Serial.print(i);
      Serial.print(": 0x"); Serial.println(REGISTER[i], HEX);
    }
    Serial.print("FLAGS: 0x"); Serial.println(FL, HEX);
  }
  else if (token.startsWith("STEP")) {
    allowStep = 1;

    if (token.startsWith("STEP:[0x") && token.endsWith("]")) {
      String numStr = token.substring(8, token.length() - 1);
      allowStep = strtoul(numStr.c_str(), nullptr, 16);
      if (allowStep == 0) allowStep = 1;
    }

    Serial.print("[DEBUG] Performing ");
    Serial.print(allowStep);
    Serial.println(" step(s).");
  }
  else {
    Serial.print("Invalid token: ");
    Serial.println(token);
  }
}

void callloop(boolean DEBUG) {
  if (DEBUG) {
    Serial.println("[DEBUG MODE] Awaiting input...");
    while (!allowStep) {
      while (!Serial.available()) {}
      String input = Serial.readStringUntil('\n');
      input.trim();

      int start = 0;
      while (start < input.length()) {
        int end = input.indexOf(' ', start);
        if (end == -1) end = input.length();
        String token = input.substring(start, end);
        token.trim();
        executeToken(token);
        start = end + 1;
      }

      Serial.println("[DEBUG MODE] Awaiting next input...");
    }
  } else {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      int start = 0;
      while (start < input.length()) {
        int end = input.indexOf(' ', start);
        if (end == -1) end = input.length();
        String token = input.substring(start, end);
        token.trim();
        executeToken(token);
        start = end + 1;
      }
    }
  }
}

void loop() {
  callloop(false);
}

#endif
