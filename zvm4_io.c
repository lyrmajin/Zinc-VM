#include <stdio.h>
#include <string.h>

typedef struct
{
    unsigned char ucsra;
    unsigned char ucsrb;
    unsigned char ucsrc;
    unsigned short ubrr;
    unsigned char udr;
} USART;

static USART usart[4];

unsigned char iointreg = 0;

void putcemu(char c)
{
    printf("USART:[%c]\n", c);
}

unsigned char ioget(unsigned short address)
{
    switch (address)
    {
    case 0x00:
        return iointreg;

    case 0xC0:
        return usart[0].ucsra;
    case 0xC1:
        return usart[0].ucsrb;
    case 0xC2:
        return usart[0].ucsrc;
    case 0xC4:
        return usart[0].ubrr & 0xFF;
    case 0xC5:
        return usart[0].ubrr >> 8;
    case 0xC6:
        return usart[0].udr;

    case 0xC8:
        return usart[1].ucsra;
    case 0xC9:
        return usart[1].ucsrb;
    case 0xCA:
        return usart[1].ucsrc;
    case 0xCC:
        return usart[1].ubrr & 0xFF;
    case 0xCD:
        return usart[1].ubrr >> 8;
    case 0xCE:
        return usart[1].udr;

    case 0xD0:
        return usart[2].ucsra;
    case 0xD1:
        return usart[2].ucsrb;
    case 0xD2:
        return usart[2].ucsrc;
    case 0xD4:
        return usart[2].ubrr & 0xFF;
    case 0xD5:
        return usart[2].ubrr >> 8;
    case 0xD6:
        return usart[2].udr;

    case 0xD8:
        return usart[3].ucsra;
    case 0xD9:
        return usart[3].ucsrb;
    case 0xDA:
        return usart[3].ucsrc;
    case 0xDC:
        return usart[3].ubrr & 0xFF;
    case 0xDD:
        return usart[3].ubrr >> 8;
    case 0xDE:
        return usart[3].udr;
    }

    return 0;
}

void ioset(unsigned short address, unsigned char value)
{
    USART *u = 0;

    switch (address)
    {
    case 0x00:
        iointreg = value;
        return;

    case 0xC0:
        usart[0].ucsra = value;
        return;
    case 0xC1:
        usart[0].ucsrb = value;
        return;
    case 0xC2:
        usart[0].ucsrc = value;
        return;
    case 0xC4:
        usart[0].ubrr =
            (usart[0].ubrr & 0xFF00) | value;
        return;
    case 0xC5:
        usart[0].ubrr =
            (usart[0].ubrr & 0x00FF) | ((unsigned short)value << 8);
        return;
    case 0xC6:
        u = &usart[0];
        break;

    case 0xC8:
        usart[1].ucsra = value;
        return;
    case 0xC9:
        usart[1].ucsrb = value;
        return;
    case 0xCA:
        usart[1].ucsrc = value;
        return;
    case 0xCC:
        usart[1].ubrr =
            (usart[1].ubrr & 0xFF00) | value;
        return;
    case 0xCD:
        usart[1].ubrr =
            (usart[1].ubrr & 0x00FF) | ((unsigned short)value << 8);
        return;
    case 0xCE:
        u = &usart[1];
        break;

    case 0xD0:
        usart[2].ucsra = value;
        return;
    case 0xD1:
        usart[2].ucsrb = value;
        return;
    case 0xD2:
        usart[2].ucsrc = value;
        return;
    case 0xD4:
        usart[2].ubrr =
            (usart[2].ubrr & 0xFF00) | value;
        return;
    case 0xD5:
        usart[2].ubrr =
            (usart[2].ubrr & 0x00FF) | ((unsigned short)value << 8);
        return;
    case 0xD6:
        u = &usart[2];
        break;

    case 0xD8:
        usart[3].ucsra = value;
        return;
    case 0xD9:
        usart[3].ucsrb = value;
        return;
    case 0xDA:
        usart[3].ucsrc = value;
        return;
    case 0xDC:
        usart[3].ubrr =
            (usart[3].ubrr & 0xFF00) | value;
        return;
    case 0xDD:
        usart[3].ubrr =
            (usart[3].ubrr & 0x00FF) | ((unsigned short)value << 8);
        return;
    case 0xDE:
        u = &usart[3];
        break;
    }

    if (u)
    {
        u->udr = value;
        putcemu(value);
        u->ucsra |= (1 << 5);
        u->ucsra |= (1 << 6);
    }
}