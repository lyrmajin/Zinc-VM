#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const char keys[4][4] = {
 {'1', '2', '3', 'A'},
 {'4', '5', '6', 'B'},
 {'7', '8', '9', 'C'},
 {'*', '0', '#', 'D'}
};

const uint8_t rowPins[4] = {32, 33, 34, 35};
const uint8_t colPins[4] = {36, 37, 38, 39};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

#define MAX_REG 5
#define MAX_MEM 6144
#define GPR0 0
#define GPR1 1
#define GPR2 2
#define GPR3 3
#define PC 4
uint8_t REGISTER[MAX_REG] = {0};
uint8_t MEMORY[MAX_MEM];
uint8_t FL = 0;

//----------------------------------------------
void SysScreenSetUp(uint8_t argptr = 0);
void SysScreenDraw(uint8_t argptr = 0, uint8_t *buffer = nullptr, uint8_t x = 0, uint8_t y = 0);
void SysScreenWrite(uint8_t argptr = 0, char *text = "", uint8_t x = 0, uint8_t y = 0);
uint8_t SysKeyboardRead(uint8_t argptr = 0);
void SysHalt(uint8_t argptr = 0);
void SysScheduleTask(uint8_t argptr = 0, uint8_t adrs = 0);
uint8_t SysMalloc(uint8_t argptr = 0);
void SysFree(uint8_t argptr = 0, uint8_t adrs = 0);
void SysGetPC(uint8_t argptr = 0);
uint8_t SysOpen(uint8_t argptr = 0, char *file = "", uint16_t baseadrs = 0x1000);
uint8_t SysRead(uint8_t argptr = 0, uint16_t baseadrs = 0x1000, uint8_t offset = 0);
void SysStore(uint8_t argptr = 0, uint8_t block = 0, uint8_t address = 0, uint8_t value = 0);
uint8_t SysLoad(uint8_t argptr = 0, uint8_t block = 0, uint8_t address = 0);
void SysKillTask(uint8_t argptr = 0, uint8_t taskid = 0);

typedef void (*SYSCALL)(uint8_t);
const SYSCALL SYSCALLS[14] = {SysScreenSetUp, SysScreenDraw, SysScreenWrite, SysKeyboardRead, SysHalt, SysScheduleTask, SysMalloc, SysFree, SysGetPC, SysOpen, SysRead, SysStore, SysLoad, SysKillTask};

void OP_MVI() { // 0000
 REGISTER[GPR0] = (MEMORY[REGISTER[PC]] & 0b00001111);
}
void OP_MOV() { // 0001
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_ADD() { //0010
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] += REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_SUB() { //0011
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] -= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_AND() { //0100
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] &= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_OR() { //0101
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] |= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_XOR() { //0110
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_CMP() { //0111
 FL = (REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] - REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]) & 0b00000001;
}
void OP_LD() { //1000
 MEMORY[REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02]] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_ST() { //1001
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = MEMORY[REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]];
}
void OP_JMP() { //1010
 REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_BRZ() { //1011
 if (FL == 0)
   REGISTER[PC] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_SHL() { //1100
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_SHR() { //1101
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] = REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] >> REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)];
}
void OP_SCB() { //1110
 REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02] ^= (1 << REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
}
void OP_SYS() { //1111
 SYSCALLS[REGISTER[(MEMORY[REGISTER[PC]] & 0b00001100) >> 0x02]](REGISTER[(MEMORY[REGISTER[PC]] & 0b00000011)]);
}
typedef void (*OPCODE)();
const OPCODE OPCODES[16] = {OP_MVI, OP_MOV, OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_CMP, OP_LD, OP_ST, OP_JMP, OP_BRZ, OP_SHL, OP_SHR, OP_SCB, OP_SYS};

void VMsetup() {
 FL |= 0b10000000;
}

uint64_t lastSwitch = millis();
uint8_t currentTask = 0;

void storeTask() {
 if(MEMORY[currentTask]){
 MEMORY[MEMORY[currentTask]] = REGISTER[0];
 MEMORY[MEMORY[currentTask] + 1] = REGISTER[1];
 MEMORY[MEMORY[currentTask] + 2] = REGISTER[2];
 MEMORY[MEMORY[currentTask] + 3] = REGISTER[3];
 MEMORY[MEMORY[currentTask] + 4] = REGISTER[PC];
 MEMORY[MEMORY[currentTask] + 5] = FL;
 }
}
void loadTask() {
 if(MEMORY[currentTask]){
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
   REGISTER[PC]++;
   OPCODES[((MEMORY[REGISTER[PC]] & 0b11110000) >> 0x04)]();
   if (millis() - lastSwitch >= TIME_SLICE_MS) {
     storeTask();
     currentTask = (currentTask < 0x0F) ? currentTask + 1 : 0;
     loadTask();
     lastSwitch = millis();
   }
   if(DEBUG){
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

void SysScreenSetUp(uint8_t argptr) {
 lcd.init();
 lcd.backlight();
}
void SysScreenDraw(uint8_t argptr, uint8_t *buffer, uint8_t x, uint8_t y) {
 if (argptr) {
   lcd.createChar(0, &MEMORY[argptr]);
   x = MEMORY[argptr + 8];
   y = MEMORY[argptr + 9];
 }
 else {
   lcd.createChar(0, buffer);
 }

 lcd.setCursor(x, y);
 lcd.write(byte(0));
}
void SysScreenWrite(uint8_t argptr, char *text, uint8_t x, uint8_t y) {
 if (argptr) {
   text = reinterpret_cast<char*>(&MEMORY[argptr]);
   x = MEMORY[argptr + strlen(text) + 1];
   y = MEMORY[argptr + strlen(text) + 2];
 }
 lcd.setCursor(x, y);
 lcd.print(text);
}

uint8_t SysKeyboardRead(uint8_t argptr) {
 REGISTER[3] = keypad.getKey();
 return REGISTER[3];
}

void SysHalt(uint8_t argptr) {
 FL &= 0b01111111;
}

void SysScheduleTask(uint8_t argptr, uint8_t adrs) {
 if (argptr)
   adrs = MEMORY[argptr];
 for(uint8_t i = 0; i < 0x0F; i++){
   if(MEMORY[i])
     continue;
   MEMORY[i] = adrs;
   break;
 }
}

uint8_t SysMalloc(uint8_t argptr) {
 for (uint16_t i = 0x100; i < MAX_MEM; i += 10) {
   if (!MEMORY[i]) {
     MEMORY[i] = true;
     MEMORY[i + 1] = currentTask;
     REGISTER[3] = (i - 0x100) / 10;
     return REGISTER[3];
   }
 }
}

void SysFree(uint8_t argptr, uint8_t block) {
 if (argptr) {
   block = MEMORY[argptr];
 }
 MEMORY[(block * 10) + 0x100] = 0x00; // u use the malloc return block id
}

void SysGetPC(uint8_t argptr) {
 REGISTER[3] = REGISTER[PC];
}

uint8_t SysOpen(uint8_t argptr, char *file, uint16_t baseadrs) {
 if (argptr){
   file = reinterpret_cast<char*>(&MEMORY[argptr]);
   baseadrs = (MEMORY[argptr + strlen(file) + 1] << 8) | (MEMORY[argptr + strlen(file) + 2]);
 }
 for (uint16_t i = baseadrs; i < (baseadrs + 0x0F); i += 2) {
   if (MEMORY[i]) {
     if (!strcmp(reinterpret_cast<char*>(&MEMORY[baseadrs + MEMORY[i + 1]]), file)) {
       REGISTER[3] = (strlen(reinterpret_cast<char*>(&MEMORY[baseadrs + MEMORY[i + 1]])) + 1 - baseadrs + MEMORY[i + 1]);
       return REGISTER[3];
     }
   }
 }
}

uint8_t SysRead(uint8_t argptr, uint16_t baseadrs, uint8_t offset){
 if (argptr){
   baseadrs = (MEMORY[argptr] << 8) | (MEMORY[argptr + 1]);
   offset = MEMORY[argptr + 2];
 }
 REGISTER[3] = MEMORY[baseadrs + offset];
 return REGISTER[3];
}

void SysKillTask(uint8_t argptr, uint8_t taskid){
 if(argptr)
   taskid = MEMORY[argptr];
 MEMORY[taskid] = 0;
}

void SysStore(uint8_t argptr, uint8_t block, uint8_t address, uint8_t value){
 if(argptr){
   block = MEMORY[argptr];
   address = MEMORY[argptr + 1];
   value = MEMORY[argptr + 2];
 }
 if(MEMORY[((block * 10) + 0x100) + 1] == currentTask && address >= 2 && address < 10)
   MEMORY[((block * 10) + 0x100) + address] = value;
 else{
   SysKillTask(0, currentTask);
   FL |= 0b01000000; //signal segfault
 }
}
uint8_t SysLoad(uint8_t argptr, uint8_t block, uint8_t address){
 if(argptr){
   block = MEMORY[argptr];
   address = MEMORY[argptr + 1];
 }
 if(MEMORY[((block * 10) + 0x100) + 1] == currentTask && address >= 2 && address < 10)
   REGISTER[3] = MEMORY[((block * 10) + 0x100) + address];
 else{
   SysKillTask(0, currentTask);
   FL |= 0b01000000; //signal segfault
 }
 return REGISTER[3];
}

//

//

//

void setup() {
 SysScreenSetUp();
 Serial.begin(9600);
 Serial.println("KINO OSS 2");
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

     if (token.startsWith("A:")) {
       memAddr = strtoul(&token[2], nullptr, 16);
       Serial.print("Address set to 0x");
       Serial.println(memAddr, HEX);
     }
     else if (token.startsWith("D:")) {
       uint8_t data = strtoul(&token[2], nullptr, 16);
       MEMORY[memAddr++] = data;
       Serial.print("Wrote 0x");
       Serial.print(data, HEX);
       Serial.print(" to 0x");
       Serial.println(memAddr - 1, HEX);
     }
     else if (token.equalsIgnoreCase("RUN")) {
       Serial.println("Running VM...");
       VMsetup();
       SysScheduleTask(0, 0x10);
       REGISTER[PC] = MEMORY[0x00] + 5;
       VMloop();
       Serial.println("VM halted.");
     }
     else if (token.equalsIgnoreCase("DEBUG")) {
       Serial.println("Running VM on DEBUG MODE...");
       VMsetup();
       SysScheduleTask(0, 0x10);
       REGISTER[PC] = MEMORY[0x00] + 5;
       VMloop(true);
       Serial.println("VM halted.");
     }
     else if (token.equalsIgnoreCase("DUMP")) {
       dumpMemory();
     }
     else {
       Serial.print("Invalid token: ");
       Serial.println(token);
     }

     start = end + 1;
   }
 }
}


