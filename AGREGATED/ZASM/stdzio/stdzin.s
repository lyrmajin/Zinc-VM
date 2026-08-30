.code:
    stdio_deinit:

        MVI R1, #0x0000
        CALL #uart_set_UBRR0H

        MVI R1, #0x0000
        CALL #uart_set_UBRR0L

        MVI R1, #0x0000
        CALL #uart_set_UCSR0C

        RET

    getchar:

        MVI R1, #0x0010
        CALL #uart_set_UCSR0B

        gc_loop:
            CALL #uart_get_UCSR0A
            MVI R1, #0x0080

            AND R3, R1
            CMP R3, R1
            BRQ #gc_loop

        CALL #uart_get_UDR0
        RET