.include: start.s
.org: 0x0000

.text:

rsv:
    zero r0

    .ifdef: CLEAR_SECTION
    mvi r1, #SECTION
    mvi r2, #SECTION_COUNT

    rsv_section_clear_loop:
        cmp r2, r1

        stm r1, r0

        add r1, r0+1
        pbrn 2$rsv_section_clear_loop
    .endif:

    .ifdef: CLEAR_REGISTERS
    mov r15, r0
    mov r14, r0
    mov r13, r0
    mov r12, r0
    mov r11, r0
    mov r10, r0
    mov r9, r0
    mov r8, r0
    mov tp, r0
    mov fp, r0
    mov sp, r0
    mov r3, r0
    mov r2, r0
    mov r1, r0
    .endif:

    .ifdef: RESET_STACK
    mvi sp@0, #STACK_DEFAULT
    .endif:

    .ifdef: SET_FRAME_TO_STACK
    mov fp, sp
    .endif

    .ifdef: SET_TP_PREDEF
    mvi tp@0, #TP_PREDEFINED
    .endif:

    jmp 0$_start