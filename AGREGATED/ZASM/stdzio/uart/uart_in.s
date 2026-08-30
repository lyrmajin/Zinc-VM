.code:
    uart_get_UCSR0A:

        MVI R0, #0x00C0
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ; read from UCSR0B
        RET

    uart_get_UCSR0B:
    
        MVI R0, #0x00C1
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ;read from UCSR0B
        RET


    uart_get_UCSR0C:

        MVI R0, #0x00C2
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ;read from UCSR0C
        RET

    uart_get_UBRR0H:
    
        MVI R0, #0x00C5
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ;read from UBRR0H
        RET

    uart_get_UBRR0L:

        MVI R0, #0x00C4
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ;read from UBRR0L
        RET

    uart_get_UDR0:

        MVI R0, #0x00C4
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0001
        SYS R0, R3      ;read from UDR0
        RET