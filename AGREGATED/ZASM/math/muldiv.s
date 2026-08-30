.code:
    MUL:
        MVI R3, #0x00
        mlloop:
            MVI R0, #0x00
            CMP R1, R0
            RBRE mlloop_ex
            ADD R3, R2
            MVI R0, #0x01
            SUB R1, R0
            NJMP mlloop
        mlloop_ex:
        RET

    DIV:
        RET