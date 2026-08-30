#ifndef ZVMIO_H
#define ZVMIO_H

#define PORTAH 0 // Port A
#define PORTAL 1
#define PORTBH 2 // Port B
#define PORTBL 3
#define TICKH 4 // Timer Interrupt Count K
#define TICKL 5
#define TIMERAH 6 // Timer A
#define TIMERAL 7
#define TIMERBH 8 // Timer B
#define TIMERBL 9
#define TMAH 10 // Timer Multipliyer
#define TMAL 11
#define TMBH 12 // Timer Multipliyer
#define TMBL 13
#define DEVA 14 // Device Address
#define DEVD 15 // Device Data

#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t dbyte;
typedef uint32_t qbyte;
typedef uint64_t obyte;

void iowrite(byte value, byte address);
byte ioread(byte address);

void porta();
void portb();

#endif