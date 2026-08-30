.code:
    stdio_init:

        PUSH R2

        CALL #uart_set_UBRR0H

        POP R2
        MOV R1, R2

        CALL #uart_set_UBRR0L

        MVI R1, #0x0006
        CALL #uart_set_UCSR0C

        RET

    putchar:
        PUSH R1

        MVI R1, #0x0008
        CALL #uart_set_UCSR0B

        pcloop:
            CALL #uart_get_UCSR0A
            MVI R1, #0x0020

            AND R3, R1
            CMP R3, R1
            BRQ #pcloop

        POP R1
        CALL #uart_set_UDR0
        RET

    printf:
        PUSH R1

        pfloop:
            POP R1
            PUSH R1
            LDR R1, R1, #0x00

            CALL #putchar

            MVI R0, #0x0001
            POP R1
            ADD R1, R0
            PUSH R1
            LDR R1, R1, #0x00

            MVI R0, #0x0000
            CMP R1, R0

            RBRQ #pfloop

        RET