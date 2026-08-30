.org:0x21
.data:
; size byte[]
y 0x02 0x00 0x79
NEWLINE 0x02 0x00 0x0A
.code:
    
    MVI R1, #0x0067     ;load 0x0067 to inizialize uart
    MVI R2, #0x0000

    CALL stdio_init     ;inizialize serial port and stdio

    loop:
        MVI R1, y       ;load y into r1
        CALL putchar    ;put y into serial port

        MVI R1, NEWLINE ;load newline into r1
        CALL putchar    ;put newline into serial port

        NJMP loop       ;loop it