.org 0x0124

_start:
		;boot
		;r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29 r30 r31
		;r25 = FL
		;setup

_vmloop:
		_vmloop_fetch:
				;Y = RT
				clr r28
				clr r29

				;R18&R19 = [RT+(4*2)]
				ldd r26, Y+8
				ldd r27, Y+9     ;IP
				
				; load information from byte[rY + HIR]
				; r17:16 will contain result, address is Y
				sbrc r25, 3
				call mmu_mget
				
				sbrs r25, 3
				call rmm_mget

				movw r18, r16

		_vmloop_decode:
				;R18 = LIR
				;R19 = HIR

				mov r20, r18
				mov r21, r18
				mov r22, r18

				;R20 = IR >> 4
				lsr r20
				lsr r20
				lsr r20
				lsr r20

				;R21 = (IR >> 2) & 3
				lsr r21
				lsr r21
				andi r21, 3

				;R22 = IR & 3
				andi r22, 3

		_vmloop_execute:
				ldi r31, 0
				mov r30, r20
				ori r30, 0x10
				ijmp

		_vmloop_end:
				;Y = [RT+(4*2)]
				ldd r26, Y+8
				ldd r27, Y+9     ;IP

				;Move IP fordward.
				adiw r26, 2 ;IP=+2
				std Y+8, r26
				std Y+9, r27

				rjmp _vmloop

.org 0x1000
_vmop_mvi:
		lsl r22
		add r28, r22
		adc r29, r1

		lsl r21
		lsl r21
		or r31, r21
		adiw r30, 32
		ijmp

		.org 0x1020
		_vmop_mvi_lmvi:
				;rY.l = HIR
				std Y+1, r19
				rjmp _vmloop_end

		.org 0x1060
		_vmop_mvi_hmvi:
				;rY.h = HIR
				std Y+0, r19
				rjmp _vmloop_end

		.org 0x10A0
		_vmop_mvi_dmvi:
				;rY.h = HIR && rY.l = HIR
				std Y+0, r19
				std Y+1, r19
				rjmp _vmloop_end

		.org 0x10E0
		_vmop_mvi_zero:
				;rY = 0
				std Y+0, r1
				std Y+1, r1
				rjmp _vmloop_end

.org 0x1100
_vmop_mov:
		;r((HIR & 0xf0) >> 4) = r(HIR & 15)
		mov r21, r19
		mov r22, r19

		lsr r21
		lsr r21
		lsr r21
		andi r21, 30

		andi r22, 15
		lsl r22

		; load rY into R17:16
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; rX address
		add r28, r21
		adc r29, r1

		; store rY into rX
		std Y+0, r17
		std Y+1, r16

		rjmp _vmloop_end

.org 0x1200
_vmop_add:
		;rX = rX + rY

		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; load rX into R19:18
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r19, Y+0
		ldd r18, Y+1

		; operate rX + rY as R17:16 + R19:18
		add r18, r16
		adc r19, r17

		; store result on rX
		std Y+0, r19
		std Y+1, r18

		rjmp _vmloop_end

.org 0x1300
_vmop_sub:
		;rX = rX - rY

		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; load rX into R19:18
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r19, Y+0
		ldd r18, Y+1

		; operate rX - rY as R17:16 - R19:18
		sub r18, r16
		sbc r19, r17

		; store result on rX
		std Y+0, r19
		std Y+1, r18

		rjmp _vmloop_end

.org 0x1400
_vmop_and:
		;rX = rX & rY

		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; load rX into R19:18
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r19, Y+0
		ldd r18, Y+1

		; operate rX & rY as R17:16 & R19:18
		and r18, r16
		and r19, r17

		; store result on rX
		std Y+0, r19
		std Y+1, r18

		rjmp _vmloop_end

.org 0x1500
_vmop_ior:
		;rX = rX | rY
		
		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; load rX into R19:18
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r19, Y+0
		ldd r18, Y+1

		; operate rX | rY as R17:16 | R19:18
		or r18, r16
		or r19, r17

		; store result on rX
		std Y+0, r19
		std Y+1, r18

		rjmp _vmloop_end

.org 0x1600
_vmop_not:
		;rX = ~rX
		
		; load rX into R17:16
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; operate ~rX as ~R17:16
		com r16
		com r17

		; store result on rX
		std Y+0, r17
		std Y+1, R16
		
		;NOTE: Revise the posibility to use LAT here

		rjmp _vmloop_end

.org 0x1700
_vmop_ldm:
		;rX = [rY + HIR]

		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; add HIR to rY
		add r16, r19
		adc r17, r1

		; load rY to Y
		movw r28, r16

		; load information from byte[rY + HIR]
		; r17:16 will contain result, address is Y
		sbrc r25, 3
		call mmu_mget
		
		sbrs r25, 3
		call rmm_mget

		; restart Y at RT
		clr r28
		clr r29

		; get rX address
		lsl r21
		add r28, r21
		adc r29, r1

		std Y+0, r17
		std Y+1, r16
		
		rjmp _vmloop_end

.org 0x1800
_vmop_stm:
		;[rX + HIR] = rY
		
		rjmp _vmloop_end

.org 0x1900
_vmop_shl:
		;rX = rX << (rY + HIR)
		rjmp _vmloop_end

.org 0x1A00
_vmop_shr:
		;rX = rX >> (rY + HIR)
		rjmp _vmloop_end

.org 0x1B00
_vmop_cmp:
		;FL ≡ rX - rY

		; load rY into R17:16
		lsl r22
		add r28, r22
		adc r29, r1

		ldd r17, Y+0
		ldd r16, Y+1

		; restart Y at RT
		clr r28
		clr r29

		; load rX into R19:18
		lsl r21
		add r28, r21
		adc r29, r1

		ldd r19, Y+0
		ldd r18, Y+1

		cp r16, r18
		cpc r17, r19
		brne _vmop_cmp_clfl

		ori r25, 1

		rjmp _vmloop_end

		_vmop_cmp_clfl:
			ldi r16, 0xFE
			and r25, r16

			rjmp _vmloop_end

.org 0x1C00
_vmop_jmp:

		lsl r21
		lsl r21
		or r31, r21
		adiw r30, 32
		ijmp

		.org 0x1C20
		_vmop_jmp_jmp:
				;IP = rY + HIR

				; load rY into R17:16
				lsl r22
				add r28, r22
				adc r29, r1

				ldd r17, Y+0
				ldd r16, Y+1

				add r16, r19
				adc r17, r1

				;clear Y to RT
				clr r28
				clr r29

				;write IP back
				std Y+8, r16
				std Y+9, r17

				rjmp _vmloop

		.org 0x1C60
		_vmop_jmp_rjmp:
				;IP = IP + rY + HIR

				;IP
				ldd r20, Y+8
				ldd r21, Y+9     ;IP
			
				; load rY into R17:16
				lsl r22
				add r28, r22
				adc r29, r1

				ldd r17, Y+0
				ldd r16, Y+1

				add r16, r19
				adc r17, r1

				add r16, r20
				adc r17, r21

				;clear Y to RT
				clr r28
				clr r29

				;write IP back
				std Y+8, r16
				std Y+9, r17

				rjmp _vmloop

		.org 0x1CA0
		_vmop_jmp_call:
				;IP = rY + HIR
				;[SP] = IP

				; = [RT+(4*2)]
				ldd r20, Y+8
				ldd r21, Y+9     ;IP

				;Move IP fordward. ;revise on future
				subi r20, -2 ;IP=+2
				sbc r21, r1

				; = [RT+(5*2)]
				ldd r27, Y+10
				ldd r26, Y+11    ;SP

				; load information
				; X is address
				; r21:20 will contain value to write
				sbrc r25, 3
				call mmu_mset
				
				sbrs r25, 3
				call rmm_mset

				;clear Y to RT
				clr r28
				clr r29

				; = [RT+(5*2)]
				ldd r27, Y+10
				ldd r26, Y+11    ;SP

				subi r26, 2 ;IP=+2
				sbc r27, r1
				
				;write SP back
				std Y+10, r26
				std Y+11, r27
				
				; load rY into R17:16
				lsl r22
				add r28, r22
				adc r29, r1

				ldd r17, Y+0
				ldd r16, Y+1

				add r16, r19
				adc r17, r1

				;clear Y to RT
				clr r28
				clr r29

				;write IP back
				std Y+8, r16
				std Y+9, r17

				rjmp _vmloop

		.org 0x1CE0
		_vmop_jmp_ret:
				;IP = [SP]

				; = [RT+(5*2)]
				ldd r27, Y+10
				ldd r26, Y+11    ;SP

				movw r28, r26

				; load information
				; r17:r16 containt result, address is Y
				sbrc r25, 3
				call mmu_mget
				
				sbrs r25, 3
				call rmm_mget

				;clear Y to RT
				clr r28
				clr r29

				; = [RT+(5*2)]
				ldd r27, Y+10
				ldd r26, Y+11    ;SP

				subi r26, -2 ;IP=+2
				sbc r27, r1
				
				;write SP back
				std Y+10, r26
				std Y+11, r27

				;write IP back
				std Y+8, r16
				std Y+9, r17

				rjmp _vmloop

.org 0x1D00
_vmop_brn:
		_vmop_brn_ebrn:
				;IP = rY + HIR ZERO FL
				rjmp _vmloop

		_vmop_brn_qbrn:
				;IP = rY + HIR !ZERO FL
				rjmp _vmloop

		_vmop_brn_erbrn:
				;IP = rY + HIR POS FL
				rjmp _vmloop

		_vmop_brn_nrbrn:
				;IP = rY + HIR !POS FL
				rjmp _vmloop

.org 0x1E00
_vmop_trb:
		;rX = rX ^ (1 << rY)
		rjmp _vmloop_end

.org 0x1F00
_vmop_sys:
		;syscall
		rjmp _vmloop_end

.org 0x2000

mmu_mget:
	ret
mmu_mset:
	ret

rmm_mget:
	ret
rmm_mset:
	ret
