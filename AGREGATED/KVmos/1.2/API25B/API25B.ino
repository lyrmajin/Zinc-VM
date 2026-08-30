#define MAX_REG 5
#define MAX_MEM 7 * 1024
#define GPR0 0
#define GPR1 1
#define GPR2 2
#define GPR3 3
#define PC 4
uint16_t REGISTER[MAX_REG] = {0};
uint8_t MEMORY[MAX_MEM];
uint8_t FL = 0;     // RUNTIME 0 0 0 0 0 0 CMP

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
void SysKill(uint16_t argptr = 0);
// 8
void SysOpen(uint16_t argptr = 0);
void SysRead(uint16_t argptr = 0);
//A
void SysExec(uint16_t argptr = 0);

typedef void (*SYSCALL)(uint16_t);
const SYSCALL SYSCALLS[11] = {SysWriteIO, SysReadIO, SysScheduleTask, SysKillTask, SysMalloc, SysFree, SysGetPC, SysKill, SysOpen, SysRead, SysExec};

void OP_MVI() { // 0000
  if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = (MEMORY[REGISTER[PC] + 1] << 0x08);
  else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = MEMORY[REGISTER[PC] + 2];
  else if ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && ((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000)){
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)] = ((MEMORY[REGISTER[PC] + 1] << 0x08) | MEMORY[REGISTER[PC] + 2]);
  REGISTER[PC] += 3;
  return;
  }
  REGISTER[PC] += 2;
}
void OP_MOV() { // 0001
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_ADD() { //0010
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_SUB() { //0011
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_AND() { //0100
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] &= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_OR() { //0101
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] |= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_XOR() { //0110
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_CMP() { //0111
  FL &= 0b11111100;
  if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) == 0)
    FL |= 0b00000001; // Z flag
  if ((REGISTER[((MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02)] - REGISTER[((MEMORY[REGISTER[PC]] & 0b00000011))]) < 0)
    FL |= 0b00000010; // N flag
  REGISTER[PC]++;
}
void OP_LD() { //1000
  MEMORY[REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02]] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_ST() { //1001
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = MEMORY[REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]];
  REGISTER[PC]++;
}
void OP_JMP() { //1010
  if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000000))
    REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00000100))
    REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  else if (((MEMORY[REGISTER[PC]] & 0b00001100) == 0b00001000))
    REGISTER[PC] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_BRN() {
  if (((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000000 && (FL & 0b00000001)) || ((MEMORY[REGISTER[PC]] & 0b00000100) == 0b00000100 && (FL & 0b00000010)))
  {
    if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00000000))
      REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
    else if (((MEMORY[REGISTER[PC]] & 0b00001000) == 0b00001000))
      REGISTER[PC] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  }
}
void OP_SHL() { //1100
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_SHR() { //1101
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] >> REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
  REGISTER[PC]++;
}
void OP_SCB() { //1110
  REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= (1 << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
  REGISTER[PC]++;
}
void OP_SYS() { //1111
  SYSCALLS[REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02]](REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
  REGISTER[PC]++;
}
typedef void (*OPCODE)();
const OPCODE OPCODES[16] = {OP_MVI, OP_MOV, OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_CMP, OP_LD, OP_ST, OP_JMP, OP_BRN, OP_SHL, OP_SHR, OP_SCB, OP_SYS};

void VMsetup() {
  FL |= 0b10000000;
}

uint64_t lastSwitch = millis();
uint8_t currentTask = 0;

void storeTask() {
  if (MEMORY[currentTask]) {
    MEMORY[MEMORY[currentTask]] = REGISTER[0];
    MEMORY[MEMORY[currentTask] + 1] = REGISTER[1];
    MEMORY[MEMORY[currentTask] + 2] = REGISTER[2];
    MEMORY[MEMORY[currentTask] + 3] = REGISTER[3];
    MEMORY[MEMORY[currentTask] + 4] = REGISTER[PC];
    MEMORY[MEMORY[currentTask] + 5] = FL;
  }
}
void loadTask() {
  if (MEMORY[currentTask]) {
    REGISTER[0] = MEMORY[MEMORY[currentTask]];
    REGISTER[1] = MEMORY[MEMORY[currentTask] + 1];
    REGISTER[2] = MEMORY[MEMORY[currentTask] + 2];
    REGISTER[3] = MEMORY[MEMORY[currentTask] + 3];
    REGISTER[PC] = MEMORY[MEMORY[currentTask] + 4];
    FL = MEMORY[MEMORY[currentTask] + 5];
  }
}

#define TIME_SLICE_MS 20

void VMloop(boolean DEBUG = false) {
  while ((FL & 0b10000000)) {
    OPCODES[((MEMORY[REGISTER[PC]] & 0b11110000) >> 0x04)]();
    if (millis() - lastSwitch >= TIME_SLICE_MS) {
      storeTask();
      currentTask = (currentTask < 0x0F) ? currentTask + 1 : 0;
      loadTask();
      lastSwitch = millis();
    }
    if (DEBUG) {
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
    }
  }
}

void SysWriteIO(uint16_t argptr) {
  *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8))) = MEMORY[argptr + 2];
}

void SysReadIO(uint16_t argptr) {
  REGISTER[3] = *((volatile uint8_t*)(MEMORY[argptr] | (MEMORY[argptr + 1] << 8)));
}

void SysScheduleTask(uint16_t argptr) {
  for (uint8_t i = 0; i < 0x0F; i++) {
    if (MEMORY[i])
      continue;
    MEMORY[i] = MEMORY[argptr];
    REGISTER[3] = i;
    return;
  }
  REGISTER[3] = 0xFF;
}

void SysKillTask(uint16_t argptr) {
  MEMORY[MEMORY[argptr]] = 0;
}

void SysMalloc(uint16_t argptr) {
}

void SysFree(uint16_t argptr) {
}

void SysGetPC(uint16_t argptr) {
  REGISTER[3] = REGISTER[PC];
}

void SysKill(uint16_t argptr) {
  FL &= 0b01111111;
}

void SysOpen(uint16_t argptr) {
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

void SysRead(uint16_t argptr) {
  REGISTER[3] = MEMORY[((MEMORY[argptr] << 8) | (MEMORY[argptr + 1])) + MEMORY[argptr + 2]];
  return;
}

void SysExec(uint16_t argptr) {
  SysScheduleTask(0xff);
  REGISTER[PC] = ((MEMORY[argptr] << 8) | (MEMORY[argptr + 1])) + 1;
}

//

// ---------------- DEBUG --------------------

//

void loadbootloader() {
  MEMORY[0x300] = 0x02; // folder
  MEMORY[0x301] = 0x10; // flptr -- "/dev"
  /////////////////////
  MEMORY[0x310] = 0x2F; // '/'
  MEMORY[0x311] = 0x64; // 'd'
  MEMORY[0x312] = 0x65; // 'e'
  MEMORY[0x313] = 0x76; // 'v'
  MEMORY[0x314] = 0x00; // '\0'
  MEMORY[0x315] = 0x02; // folder
  MEMORY[0x316] = 0x10; // flptr -- "/portb"
  ////////////////////
  MEMORY[0x325] = 0x2F; // '/'
  MEMORY[0x326] = 0x70; // 'p'
  MEMORY[0x327] = 0x6F; // 'o'
  MEMORY[0x328] = 0x72; // 'r'
  MEMORY[0x329] = 0x74; // 't'
  MEMORY[0x32A] = 0x62; // 'b'
  MEMORY[0x32B] = 0x00; // '\0'
  //----------------//
  MEMORY[0x32C] = 0x01; // file
  MEMORY[0x32D] = 0x10; // flptr -- "/ddrb"
  MEMORY[0x32E] = 0x01; // file
  MEMORY[0x32F] = 0x18; // flptr -- "/portb"
  MEMORY[0x330] = 0x01; // file
  MEMORY[0x331] = 0x21; // flptr -- "/pinb"
  ////////////////////
  MEMORY[0x33C] = 0x2F; // '/'
  MEMORY[0x33D] = 0x64; // 'd'
  MEMORY[0x33E] = 0x64; // 'd'
  MEMORY[0x33F] = 0x72; // 'r'
  MEMORY[0x340] = 0x62; // 'b'
  MEMORY[0x341] = 0x00; // '\0'
  ////////////////////
  MEMORY[0x343] = 0x2F; // '/'
  MEMORY[0x344] = 0x70; // 'p'
  MEMORY[0x345] = 0x6F; // 'o'
  MEMORY[0x346] = 0x72; // 'r'
  MEMORY[0x347] = 0x74; // 't'
  MEMORY[0x348] = 0x62; // 'b'
  MEMORY[0x349] = 0x00; // '\0'
  ////////////////////
  MEMORY[0x34B] = 0x2F; // '/'
  MEMORY[0x34C] = 0x70; // 'p'
  MEMORY[0x34D] = 0x69; // 'i'
  MEMORY[0x34E] = 0x6E; // 'n'
  MEMORY[0x34F] = 0x62; // 'b'
  MEMORY[0x350] = 0x00; // '\0'
  ////////////////////
}

void setup() {
  Serial.begin(9600);
  Serial.println("KVmos 1.2");
  loadbootloader();
}

uint16_t memAddr = 0;

void dumpMemory() {
  Serial.println("Dumping MEMORY:");
  for (uint16_t i = 0; i < MAX_MEM; i++) {
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
void resetAll() {
  for (uint16_t i = 0; i < MAX_MEM; i++) MEMORY[i] = 0;
  for (uint8_t i = 0; i < 8; i++) REGISTER[i] = 0; // adjust size
  memAddr = 0;
  Serial.println("All memory and registers have been reset.");
}

void executeToken(const String& token) {
  if (token.startsWith("A:")) {
    memAddr = strtoul(token.substring(2).c_str(), nullptr, 16);
    Serial.print("Address set to 0x");
    Serial.println(memAddr, HEX);
  }
  else if (token.startsWith("D:")) {
    uint8_t data = strtoul(token.substring(2).c_str(), nullptr, 16);
    MEMORY[memAddr++] = data;
    Serial.print("Wrote 0x");
    Serial.print(data, HEX);
    Serial.print(" to 0x");
    Serial.println(memAddr - 1, HEX);
  }
  else if (token.equalsIgnoreCase("RUN")) {
    Serial.println("Running VM...");
    VMsetup();
    MEMORY[0xF0] = 0x10;
    SysScheduleTask(0x10);
    REGISTER[PC] = MEMORY[0x00] + 5;
    VMloop();
    Serial.println("VM halted.");
  }
  else if (token.equalsIgnoreCase("DEBUG")) {
    Serial.println("Running VM on DEBUG MODE...");
    VMsetup();
    MEMORY[0xF0] = 0x10;
    SysScheduleTask(0xF0);
    REGISTER[PC] = MEMORY[REGISTER[3]] + 5;
    VMloop(true);
    Serial.println("VM halted.");
  }
  else if (token.equalsIgnoreCase("DUMP")) {
    dumpMemory();
  }
  else if (token.equalsIgnoreCase("RESET")) { // NEW
    resetAll();
  }
  else if (token.startsWith("FOR:[") && token.indexOf("]DO:[") != -1) { // NEW
    int forIdx = token.indexOf("FOR:[") + 5;
    int endForIdx = token.indexOf("]", forIdx);
    int doIdx = token.indexOf("DO:[", endForIdx) + 4;
    int endDoIdx = token.indexOf("]", doIdx);

    if (forIdx < 0 || endForIdx < 0 || doIdx < 0 || endDoIdx < 0) {
      Serial.println("Invalid FOR:DO syntax.");
      return;
    }

    int repetitions = token.substring(forIdx, endForIdx).toInt();
    String command = token.substring(doIdx, endDoIdx);

    Serial.print("Repeating command '");
    Serial.print(command);
    Serial.print("' ");
    Serial.print(repetitions);
    Serial.println(" times...");

    for (int i = 0; i < repetitions; i++) {
      executeToken(command);
    }
  }
  else {
    Serial.print("Invalid token: ");
    Serial.println(token);
  }
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // remove leading/trailing whitespace

    int start = 0;
    while (start < input.length()) {
      int end = input.indexOf(' ', start);
      if (end == -1) end = input.length(); // last token
      String token = input.substring(start, end);
      token.trim();

      executeToken(token); // MODIFIED

      start = end + 1;
    }
  }
}
