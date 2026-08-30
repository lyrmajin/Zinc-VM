#include "zvmio.h"

#define REGISTERS_COUNT 32
#define MMUREGISTERS_COUNT 8
#define IOREGISTERS_COUNT 16

extern __thread dbyte registers[REGISTERS_COUNT];
extern __thread dbyte mmuregisters[MMUREGISTERS_COUNT];
extern _Atomic byte ioregisters[IOREGISTERS_COUNT];

void iowrite(byte value, byte address)
{
    if (address < REGISTERS_COUNT)
        registers[address / 2] = (address % 2) ? (value << 8) : value;
    else if (address >= (REGISTERS_COUNT * 2) && address < MMUREGISTERS_COUNT)
        mmuregisters[(address - REGISTERS_COUNT) / 2] = (address % 2) ? (value << 8) : value;
    else if (address >= (MMUREGISTERS_COUNT * 2) && address < IOREGISTERS_COUNT)
        ioregisters[address - ((MMUREGISTERS_COUNT * 2) - (REGISTERS_COUNT * 2))] = value;
    else
        ;
}
byte ioread(byte address)
{
    if (address < (REGISTERS_COUNT * 2))
        return (address % 2) ? (registers[address / 2] << 8) : registers[address / 2];
    else if (address >= (REGISTERS_COUNT * 2) && (address - (REGISTERS_COUNT * 2)) < (MMUREGISTERS_COUNT * 2))
        return (address % 2) ? (registers[(address - REGISTERS_COUNT) / 2] << 8) : registers[(address - REGISTERS_COUNT) / 2];
    else if (address >= (MMUREGISTERS_COUNT * 2) && (address - (REGISTERS_COUNT * 2) - (MMUREGISTERS_COUNT * 2)) < IOREGISTERS_COUNT)
        return ioregisters[address - (MMUREGISTERS_COUNT * 2) - (REGISTERS_COUNT * 2)];
    else
        return -1;
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

void writeSerial(byte Value)
{
    int serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (serial_fd < 0)
    {
        perror("Failed to open serial port");
        return;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);

    // Set baud rate
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag &= ~PARENB; // no parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8; // 8 data bits

    options.c_cflag |= (CLOCAL | CREAD); // enable receiver

    tcsetattr(serial_fd, TCSANOW, &options);

    write(serial_fd, &Value, sizeof(byte));

    close(serial_fd);
}

byte readSerial()
{
    int serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (serial_fd < 0)
    {
        perror("Failed to open serial port");
        return 1;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);

    // Set baud rate
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag &= ~PARENB; // no parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8; // 8 data bits

    options.c_cflag |= (CLOCAL | CREAD); // enable receiver

    tcsetattr(serial_fd, TCSANOW, &options);

    byte Value = 0;
    write(serial_fd, &Value, sizeof(byte));
    read(serial_fd, &Value, sizeof(byte));

    close(serial_fd);

    return Value;
}

void porta()
{
    while (1)
    {
        if (ioregisters[PORTAH] & 0x80)
        {
            writeSerial(PORTAH);
        }
        else
            ioregisters[PORTAH] = readSerial();
    }
}
void portb()
{
    while (1)
    {
        if (ioregisters[PORTBH] & 0x80)
        {
            // write to real IO
        }
        else
            ; // write from real IO
    }
}