mvil r0, 0xDF <--- RVP
mvih r0, 0x07
mov sp, r0
mvil r0, 0x14
mvih r0, 0x00
jmp r0
0x0000:TOLINK
0x0000:TOLINK
0x0000:TOLINK
0x0000:TOLINK
mvil r0, 0xFC <--- IVP <--- start
mvih r0, 0x00
sys r0, r1
mvil r0, 0x22
mvih r0, 0x20
cmp r0, r1
mvil r0, 0x00:TOLINK
mvih r0, 0x00:TOLINK
ebrn r0
mvil r0, 0xFD
mvih r0, 0x00
sys r0, r0
mvil r0, 0x00:TOLINK
mvih r0, 0x00:TOLINK
call r0
mov r0, tp <--- STORE_CONTEXT
zero r1
stm [r0+8], r2
shr r2, r1+4
stm [r0+9], r2
stm [r0+10], r3
shr r3, r1+4
stm [r0+11], r3
mov r2, .r4
mov r3, .r5
stm [r0+12], r2
shr r2, r1+4
stm [r0+13], r2
stm [r0+14], r3
shr r3, r1+4
stm [r0+15], r3
mov r2, .r6
mov r3, .r7
stm [r0+16], r2
shr r2, r1+4
stm [r0+17], r2
stm [r0+18], r3
shr r3, r1+4
stm [r0+19], r3
mov r2, .r8
mov r3, .r9
stm [r0+20], r2
shr r2, r1+4
stm [r0+21], r2
stm [r0+22], r3
shr r3, r1+4
stm [r0+23], r3
mov r2, .r10
mov r3, .r11
stm [r0+24], r2
shr r2, r1+4
stm [r0+25], r2
stm [r0+26], r3
shr r3, r1+4
stm [r0+27], r3
mov r2, .r12
mov r3, .r13
stm [r0+28], r2
shr r2, r1+4
stm [r0+29], r2
stm [r0+30], r3
shr r3, r1+4
stm [r0+31], r3
mov r2, .r14
mov r3, .r15
stm [r0+32], r2
shr r2, r1+4
stm [r0+33], r2
stm [r0+34], r3
shr r3, r1+4
stm [r0+35], r3
mov r1, sp
ldm r2, [r1]
stm [r0+36], r2
ldm r2, [r1+1]
stm [r0+37], r2
ldm r2, [r1+2]
stm [r0+38], r2
ldm r2, [r1+3]
stm [r0+39], r2
ldm r2, [r1+4]
stm [r0+40], r2
ldm r2, [r1+5]
stm [r0+41], r2
ldm r2, [r1+6]
stm [r0+42], r2