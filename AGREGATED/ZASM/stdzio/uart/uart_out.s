.code:
    uart_set_UCSR0A:

        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C0
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UCSR0A
        RET

    uart_set_UCSR0B:

        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C1
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UCSR0B
        RET

    uart_set_UCSR0C:

        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C2
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UCSR0C
        RET

    uart_set_UBRR0H:

        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C5
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UBRR0H
        RET

    uart_set_UBRR0L:
    
        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C4
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UBRR0L
        RET

    uart_set_UDR0:
        PUSH R1
        MVI R2, #0x0000
        PUSH R2
        MVI R0, #0x00C6
        PUSH R0
        MVI R0, #0x0000
        PUSH R0

        MVI R0, #0x0007
        SYS R0, R0      ; SysGetSP

        MVI R0, #0x0001
        ADD R3, R0

        MVI R0, #0x0000
        SYS R0, R3      ;write to UDR0
        RET