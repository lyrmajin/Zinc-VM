# Zinc Virtual Machine 4

## What is it?

The ZVM4 is the Lastest edition of the ZVM Family, with several improvements compared to ZVM3, aswell as a new frameworking tools.

Is it complete? Absolutly no. I still must make the io model, as currently i am basically using Atmega2560 IO. Architecturally it is tho, indeed completed, and the CORE CPU wont get more updates.

The ISA:

ZVM4 has the following registers:

HOT:

`GPR0`
`GPR1`
`GPR2`
`RRT` Return Register (Old Name) | `GPR3`

COLD:

`IP` Instruction Pointer
`SP` Stack Pointer
`FP` Frame Pointer
`TP` Table Pointer | `RP` Runtime Pointer

`GPR8`
`GPR9`
`GPR10`
`GPR11`
`GPR12`
`GPR13`
`GPR14`
`GPR15`

Base Operations:

0x00: MVI: Move Inmediate: EncodeC

0x01: MOV: Move Registers: EncodeB

0x02: ADD: Add Registers and Inmediate: EncodeC

0x03: SUB: Sub Registers and Inmediate: EncodeA

0x04: AND: Logical AND on Registers and Inmediate: EncodeC

0x05: IOR: Logical OR on Registers and Inmediate: EncodeC

0x06: NOT: Logical Not on Register: EncodeA

0x07: LDM: Load Memory to Register at Register Plus Inmediate: EncodeC

0x08: STM: Store Memory from Register at Register Plus Inmediate: EncodeC

0x09: SHL: Logical Shift Left on Registers and Inmediate: EncodeC

0x0A: SHR: MLogical Shift Right on Registers and Inmediate: EncodeC

0x0B: CMP: Comparison Between Registers: EncodeA

0x0C: JMP: Jump from Register Plus Inmediate: EncodeC

0x0D: BRN: Branch from Register Plus Inmdeiate: EncodeC

0x0E: TGL: Toggle Bit on Register: EncodeA

0x0F: SYS: Syscall: EncodeA


Encodings:

A:
Opcode [15:12] | rX [11:10] | rY [9:8]

B:
Opcode [15:12] | 0 [11:8] | rX [7:4] | rY [3:0]

C:
Opcode [15:12] | rX [11:10] | rY [9:8] | Inm [7:0]

Flags Access:

The ZVM4 Flags access is done throught syscall 0xFE and 0xFF, which Read and Write respectibly to flags.

There are 8 flags, the 4 high are unwrittable when protected flag is on, alongside the `TP`|`RP` Register.
