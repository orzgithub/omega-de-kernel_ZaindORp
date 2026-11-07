@;--------------------------------------------------------------------
	.section   .rodata,"a",%progbits
		
	.global  IC_ReplaceIRQ_start
	.global  IC_ReplaceIRQ_end
	.global  IC_Return_address_L
	.global  IC_Sleep_key
	.global  IC_Reset_key
	.global  IC_switch
	.global  IC_payload_ptr
	.global  IC_spend_ptr
	@;.global  ASCII
	
REG_BASE		= 0x4000000
REG_IME			= 0x208
REG_DISPCNT		= 0x00
REG_DISPSTAT	= 0x04
REG_VCOUNT		= 0x06
REG_BG0CNT		= 0x08
REG_BG1CNT		= 0x0A
REG_BG2CNT		= 0x0C
REG_BG3CNT		= 0x0E
REG_BG0HOFS		= 0x10
REG_BG0VOFS		= 0x12
REG_BG1HOFS		= 0x14
REG_BG1VOFS		= 0x16
REG_BG2HOFS		= 0x18
REG_BG2VOFS		= 0x1A
REG_BG3HOFS		= 0x1C
REG_BG3VOFS		= 0x1E
REG_WIN0H		= 0x40
REG_WIN1H		= 0x42
REG_WIN0V		= 0x44
REG_WIN1V		= 0x46
REG_WININ		= 0x48
REG_WINOUT		= 0x4A
REG_BLDCNT		= 0x50
REG_BLDALPHA	= 0x52
REG_BLDY		= 0x54
REG_SOUND1CNT_L	= 0x60
REG_SOUND1CNT_H	= 0x62
REG_SOUND1CNT_X	= 0x64
REG_SOUND2CNT_L	= 0x68
REG_SOUND2CNT_H	= 0x6C
REG_SOUND3CNT_L	= 0x70
REG_SOUND3CNT_H	= 0x72
REG_SOUND3CNT_X	= 0x74
REG_SOUND4CNT_L	= 0x78
REG_SOUND4CNT_H	= 0x7c
REG_SOUNDCNT_L		= 0x80
REG_SOUND2CNT_H		= 0x82
REG_SOUNDCNT_X		= 0x84
REG_SOUNDBIAS		= 0x88
REG_WAVE_RAM0_L		= 0x90
REG_FIFO_A_L	= 0xA0
REG_FIFO_A_H	= 0xA2
REG_FIFO_B_L	= 0xA4
REG_FIFO_B_H	= 0xA6
REG_DM0SAD		= 0xB0
REG_DM0DAD		= 0xB4
REG_DM0CNT_L	= 0xB8
REG_DM0CNT_H	= 0xBA
REG_DM1SAD		= 0xBC
REG_DM1DAD		= 0xC0
REG_DM1CNT_L	= 0xC4
REG_DM1CNT_H	= 0xC6
REG_DM2SAD		= 0xC8
REG_DM2DAD		= 0xCC
REG_DM2CNT_L	= 0xD0
REG_DM2CNT_H	= 0xD2
REG_DM3SAD		= 0xD4
REG_DM3DAD		= 0xD8
REG_DM3CNT_L	= 0xDC
REG_DM3CNT_H	= 0xDE
REG_TM0D		= 0x100
REG_TM0CNT		= 0x102
REG_IE			= 0x200
REG_IF			= 0x202
REG_P1			= 0x130
REG_P1CNT		= 0x132
REG_WAITCNT		= 0x204

@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
	.arm
IC_ReplaceIRQ_start:
	MOV             R0, #0x4000000
	ADR             R1, IC_irq
	STR             R1, [R0,#-4] @; 0x3FFFFFC = IC_irq;
	LDR             R0, =0x12345678
	LDR             R1, =0x03007FE0
	MOV             R2, #0
	STR             R2, [R1]
	STR             R2, [R1,#4]
	BX              R0
	.align
IC_Return_address_L:
	.ltorg 			@;return address need modify
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IC_spend_ptr:
spend_0x80:
	.word 0x0203FE00   @; default
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IC_irq:
	LDR			R1, [R0,#0x200]
	TST			R1, #0x10000
	TSTEQ		R1, #0x10000000
	LDREQ		PC, [R0,#-0xC]			@;old_interrupt_handler

	ldr 		r2,[r0,#REG_P1]
	bic 		r2,r2,#0xFF000000
	bic 		r2,r2,#0x00FF0000
	
	@;check ingamemenu
	adr 		r3,IC_Reset_key 
	ldr 		r3,[r3]
	cmp 		r2,r3		
	bne			check_sleep	
	@;----------------------
	adrl		r12, spend_0x80
	ldr			r12,[r12]
	stmia		r12!,{r4-r11,sp,lr} @;0x0
	mrs	 		r2,SPSR
	stmia		r12!,{r2}       @;0x4
	
	stmfd   SP!, {LR}  			@;0x4000000,	
	bl 			ingamemenu_now	
	ldmfd   SP!, {LR}
	@;----------------------
	@;check sleep
check_sleep:
	adr 		r3,IC_Sleep_key 
	ldr 		r3,[r3]
	cmp 		r2,r3 			
	beq 		sleep_now
	
	@;instant cheat: apply every enabled item
	@;LR must survive: the game IRQ handler returns via subs pc,lr,#4
	stmfd	sp!,{lr}
	bl		IC_apply
	ldmfd	sp!,{lr}
	mov		r0,#0x4000000
nocheat:
	ldr 		pc,[r0,#-(0x04000000-0x03FFFFF4)] @;to normal IRQ routine									
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
reset_now:
	adr r1,reset_code
	adr r3,reset_end
	mov r2,#0x02000000
copy_loop:
	ldr r0,[r1],#4
	str r0,[r2],#4
	cmp r1,r3
	blt copy_loop
	mov r0,#0x02000000
	add r0,r0,#1
	bx r0
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IC_Sleep_key:
	.word 0xFB @;L+R+select
IC_Reset_key:
	.word 0xF7 @;L+R+Start
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.align	
	.thumb
reset_code:
	mov r0,#0x20
	lsl r3,r0,#22 @;#0x8000000 r3
	lsl r0,r0,#12 @;#0x0020000
	add r4,r3,r0  @;#0x8020000 r4
	add r5,r4,r0  @;#0x8040000 r5
	lsl r1,r0,#8  @;#0x2000000
	add r2,r3,r1  @;#0xa000000
	lsr r1,r3,#4  @;#0x0800000
	sub r6,r2,r1  @;#0x9800000
	lsr r1,r1,#4  @;#0x0080000
	add r6,r6,r1  @;#0x9880000 r6
	sub r2,r2,r0  @;#0x9fe0000 r2
	sub r7,r2,r0  @;#0x9fc0000 r7

	mov r0,#210
	lsl r0,r0,#8  @;0xd200 r0
	mov r1,#21
	lsl r1,r1,#8  @;0x1500 r1

	strh r0,[r2]
	strh r1,[r3]
	strh r0,[r4]
	strh r1,[r5]

	lsr r0,r3,#12 @;#0x0008000 r0
	add r0,#2 		@;#0x0008002 r0

	strh r0,[r6]
	strh r1,[r7]

	lsl r1,r0,#11 @;#0x4000000
	sub r1,r1,#8  @;#0x3FFFFFA
	mov r0,#0xfc  @;#252 r0
	str r0,[r1]   @;#0x3FFFFFA (mirror of #0x3007FFA
	swi 0x01
	swi 0x00
	.align	
reset_end:
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.arm
sleep_now:
	stmfd sp!,{r4-r11,lr}
	add r1,r0,#REG_SOUND1CNT_L
	@;copy and push 32 bytes
	ldmia r1!,{r2-r9}
	stmfd sp!,{r2-r9}
	@;copy and push 32 bytes
	ldmia r1!,{r2-r9}
	stmfd sp!,{r2-r9}
	@;r3 = contents of REG_SOUND3CNT_X

	@;save old io values
	add r1,r0,#REG_IE
	ldrh r4,[r1]
	ldr r5,[r0,#REG_P1]
	ldrh r6,[r0,#REG_DISPCNT]

	@;enable ints on Keypad, Game Pak
	ldr r1,=0xFFFF1000
	str r1,[r0,#REG_IE]
	mov r1,#0xC0000000		@;interrupt on start+sel
	@;orr r1,r1,#0x000C0000
	adr r2,IC_Wakeup_key
	ldr r2,[r2]
	MVN R2,R2
	lsl	r2,r2,#0x10
	orr r1,r1,r2
	str r1,[r0,#REG_P1]
	strh r0,[r0,#REG_SOUNDCNT_X]	@;sound off
	orr r1,r6,#0x80
	strh r1,[r0,#REG_DISPCNT]			@;LCD off

	swi 0x030000

	@;Loop to wait for letting go of Sel+start
loop:
	mov r0,#REG_BASE
	ldr r1,[r0,#REG_P1]
	adr r7,IC_Wakeup_key
	ldr r7,[r7]
	and r1,r1,r7
	@;cmp r1,#0x000C
	cmp r1,r7
	bne loop

	@;spin until VCOUNT==159
spin2:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#159
	bne spin2
	@;spin until VCOUNT==160
spin4:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#160
	bne spin4
	@;spin until VCOUNT==159
spin5:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#159
	bne spin5
	@;spin until VCOUNT==160
spin6:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#160
	bne spin6
	@;spin until VCOUNT==159
spin7:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#159
	bne spin7

	@;restore interrupts
	add r1,r0,#REG_IE
	strh r4,[r1]
	@;restore joystick interrupt
	str r5,[r0,#REG_P1]
	mov r4,#0x1000 @;clear the damn joystick interrupt
	strh r4,[r1,#2]

	@;restore screen
	strh r6,[r0,#REG_DISPCNT]
	ldmfd sp!,{r2-r9}
	@;restore sound state
	str r3,[r0,#REG_SOUNDCNT_X]
	add r1,r0,#0x80
	stmia r1!,{r2-r9}
	add r1,r0,#0x60
	ldmfd sp!,{r2-r9}
	stmia r1!,{r2-r9}
	ldmfd sp!,{r4-r11,lr}
	@;spin until VCOUNT==160, triggers next vblank
spin3:
	ldrh r1,[r0,#REG_VCOUNT]
	cmp r1,#160
	bne spin3  @<insert ytmnd cliche here>
	@;all done!
	ldr pc,[r0,#-(0x04000000-0x03FFFFF4)] @to IRQ routine
	@b b_pressed_quit
	.align
IC_Wakeup_key:
	.word 0x3F3 @;start and select
	.ltorg
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.arm
@;------------------------------------------------------
@;------------------------------------------------------
clean_screen:
	stmfd	sp!,{r0-r3}
	mov		r2,#0x6000000
	add		r3,r2,#0x12C00
	mov		r0,#0
clearLCD:
	str		r0,[r2],#+4
	cmp		r2,r3
	blt		clearLCD
	
	ldmfd	sp!,{r0-r3}
	bx		lr		
@;------------------------------------------------------
SetRampage:
	ldr 	r1,=0xD200
	ldr 	r2,=0x1500
	ldr 	r3,=0x9fe0000
	strh 	r1,[r3]
	mov 	r3,#0x8000000
	strh  r2,[r3]
	ldr 	r3,=0x8020000
	strh 	r1,[r3]
	ldr 	r3,=0x8040000
	strh  r2,[r3]
	ldr 	r3,=0x9C00000
	strh  r0,[r3]
	ldr 	r3,=0x9FC0000
	strh  r2,[r3]
	bx		lr
@;------------------------------------------------------
@;------------------------------------------------------
@;WriteSram_val:        @; sram address, val
@;	STRB    R1, [R0],#1
@;	LSR     R1, R1, #0x8
@;	STRB    R1, [R0],#1
@;	LSR     R1, R1, #0x8
@;	STRB    R1, [R0],#1
@;	LSR     R1, R1, #0x8
@;	STRB    R1, [R0]
@;	BX      LR
@;------------------------------------------------------
WriteSram: @;(u32 address, u8 *data, u32 size)
	ADD 		R2,R2,R0
	SUB 		R1,R1,R0
wSram_loop1:
	CMP     R0, R2
	BNE     wSram_loop
	BX      LR
wSram_loop:
	LDR     R3, [R1,R0]
	STRB    R3, [R0],#1
	LSR     R3, R3, #0x8
	STRB    R3, [R0],#1
	LSR     R3, R3, #0x8
	STRB    R3, [R0],#1
	LSR     R3, R3, #0x8
	STRB    R3, [R0],#1	
	B       wSram_loop1
@;------------------------------------------------------
ReadSram: @;(u32 address, u8 *data, u32 size)
	ADDS    R2, R2, R0
	SUBS    R1, R1, R0
rSram_loop1:
	CMP     R0, R2
	BNE     rSram_loop
	BX      LR
rSram_loop:
	LDRB    R3, [R0]
	LSL 		R4,R3,#0
	LDRB    R3, [R0,#1]
	LSL 		R5,R3,#8
	ORR			R4,R5
	LDRB    R3, [R0,#2]
	LSL 		R5,R3,#16
	ORR			R4,R5
	LDRB    R3, [R0,#3]
	LSL 		R5,R3,#24
	ORR			R4,R5
	STR     R4, [R1,R0]
	ADDS    R0, #4
	B       rSram_loop1
@;------------------------------------------------------
backup_LCD:
	stmfd	sp!,{r0-r7,lr}
	mov 	r0,#0x20
	bl 		SetRampage	
	mov 	r0,#0x0E000000
	mov 	r1,#0x6000000
	mov 	r2,#0x10000		@;0x12C00
	bl 		WriteSram   	@;(u32 address, u8 *data, u32 size)
	mov 	r0,#0x30
	bl 		SetRampage	
	mov 	r0,#0x0E000000
	ldr 	r1,=0x6010000
	ldr 	r2,=0x2C00
	bl 		WriteSram   	@;(u32 address, u8 *data, u32 size)
	
	ldmfd	sp!,{r0-r7,PC}

@;------------------------------------------------------
restore_LCD:
	stmfd	sp!,{r0-r7,lr}
	mov 	r0,#0x20
	bl 		SetRampage
	mov r0,#0x0E000000
	mov r1,#0x6000000
	mov r2,#0x10000		@;0x12C00
	bl  ReadSram    	@;(u32 address, u8 *data, u32 size)  
	mov 	r0,#0x30
	bl 		SetRampage	
	mov r0,#0x0E000000
	ldr r1,=0x6010000
	ldr r2,=0x2C00
	bl 	ReadSram   	 @;(u32 address, u8 *data, u32 size)


	ldmfd	sp!,{r0-r7,PC}
@;------------------------------------------------------
restore2_IO:        @; IOaddress, offset
	LDRB    R3, [R1]
	LSL 		R4,R3,#0
	LDRB    R3, [R1,#1]
	LSL 		R5,R3,#8
	ORR			R4,R5
	STRH    R4, [R0]
	bx lr
@;restore4_IO:        @; IOaddress, offset
@;	LDRB    R3, [R1]
@;	LSL 		R4,R3,#0
@;	LDRB    R3, [R1,#1]
@;	LSL 		R5,R3,#8
@;	ORR			R4,R5	
@;	LDRB    R3, [R1,#2]
@;	LSL 		R5,R3,#16
@;	ORR			R4,R5
@;	LDRB    R3, [R1,#3]
@;	LSL 		R5,R3,#24
@;	ORR			R4,R5
@;	STR     R4, [R0]
@;	bx lr
@;------------------------------------------------------
@;------------------------------------------------------
ingamemenu_now:
	stmfd	sp!,{r0-r12,lr}
	@;--- snapshot GAME io (0x000-0x3FF) before any menu clobber ---
	ldr	r0,=0x04000000
	ldr	r1,=0x06012C00
	mov	r2,#0x200
icl_snap:
	ldrh	r3,[r0],#2
	strh	r3,[r1],#2
	subs	r2,r2,#1
	bne	icl_snap
	
	mov 	r7,#0x4000000
	add 	r1,r7,#REG_SOUND1CNT_L
	adrl	r11, spend_0x80
	ldr		r11,[r11]  		
	add		r11,#0x40     @;0x3007EC0
  
	add   r3,r11,#0x32 	@; IO 0x60-0x90 offset 0x100-130
loopbak:
	ldrh  r2,[r1],#2
	strh  r2,[r11],#2
	cmp   r11,r3
	bne   loopbak	
	
	ldrh r2,[r7,#0xBA]@;DMA0CNT_H	
	strh  r2,[r11],#2 @;offset 0x72
	ldrh r2,[r7,#0xC6]@;DMA1CNT_H	
	strh  r2,[r11],#2 @;offset 0x74	
	ldrh r2,[r7,#0xD2]@;DMA2CNT_H	
	strh  r2,[r11],#2 @;offset 0x76
	ldrh r2,[r7,#0xDE]@;DMA3CNT_H	
	strh  r2,[r11],#2 @;offset 0x78	
		
	mov		r7,#0x4000000
	ldrh 	r3,[r7,#REG_SOUNDCNT_L]  @;san guo
	stmfd	sp!,{r3}		
	ldrh 	r3,[r7,#REG_SOUNDCNT_X]  @;bak
	stmfd	sp!,{r3}		
	
	ldrh  r6,[r7]     @;Displaly Control
	stmfd	sp!,{r6}
	
	mov 	r3,#0x0100
	strh 	r3,[r7,#0x20]	@;Rotation/Scaling BG2P
	strh 	r3,[r7,#0x26]			
	mov 	r3,#0x0	
	strh 	r3,[r7,#0x22]	
	strh 	r3,[r7,#0x24]		
	str 	r3,[r7,#0x28]	@;BG2X/Y
	str 	r3,[r7,#0x2c]	
	
	strh 	r3,[r7,#0x54]	@;Bldy Brightness

	strh 	r3,[r7,#0xBA]	@;DMA0CNT_H	0086game
	strh 	r3,[r7,#0xC6]	@;DMA1CNT_H		
	strh 	r3,[r7,#0xD2]	@;DMA2CNT_H	
	strh 	r3,[r7,#0xDE]	@;DMA3CNT_H	
	ldr  	r6,=0x403 		@;MODE_3 | BG2_ENABLE
	strh 	r6,[r7]	
	@;mov 	r2,#0	
	add		r7,r7,#0x200		@;IME lives at REG_BASE+0x208
	strh 	r3,[r7,#8]		@;IME =0 Disable
	sub		r7,r7,#0x200

	strh 	r3,[r7,#REG_SOUNDCNT_X] @;sound off
			
	bl 		backup_LCD
	bl 		clean_screen

@; start
mainmenu_begin:
	mov		r11,#0
mainmenu_redraw:
	mov		r12,#0
	adrl	r8,ingameMENU
	mov		r9,#40
	mov		r10,#86
mainmenu_show:
	ldrb	r0,[r8],#+1
	cmp		r0,#0xA5
	beq		mainmenu_wait
	cmp		r0,#0
	addeq	r9,#16
	moveq	r10,#86
	addeq	r12,#1
	beq		mainmenu_show
	mov		r1,r10
	add		r2,r9,#0
	cmp		r12,r11
	ldreq	r3,=0x6A80
	ldrne	r3,=0x7FFF
	bl		printchar
	add		r10,#8
	b		mainmenu_show

mainmenu_wait:
	mov		r12,#0
	ldr		r0,=0x3FF
	ldr		r8,=0x4000130
mainmenu_waitloop:
	ldrh	r9,[r8]
	cmp		r9,r0
	beq		mainmenu_waitloop
mainmenu_press:
	mov		r12,r9
	ldrh	r9,[r8]
	cmp		r9,r0
	bne		mainmenu_press

	ldr		r0,=0x3BF
	cmp		r12,r0
	bne		mainmenu_chkdown
	subs	r11,r11,#1
	movmi	r11,#3
	b		mainmenu_redraw
mainmenu_chkdown:
	ldr		r0,=0x37F
	cmp		r12,r0
	bne		mainmenu_chka
	add		r11,r11,#1
	cmp		r11,#4
	moveq	r11,#0
	b		mainmenu_redraw
mainmenu_chka:
	ldr		r0,=0x3FE
	cmp		r12,r0
	bne		mainmenu_chkb
	cmp		r11,#0
	beq		reset_now
	cmp		r11,#1
	beq		call_Save
	cmp		r11,#2
	beq		call_Load
	bl		cheat_menu
	b		mainmenu_redraw
mainmenu_chkb:
	ldr		r0,=0x3FD
	cmp		r12,r0
	beq		b_pressed_quit
	b		mainmenu_wait
	@;-------------------------------------
@;ingamemenu_exit:
b_pressed_quit:	
	ldmfd	sp!,{r6}
	mov		r7,#0x4000000
	strh 	r6,[r7] @;re Displaly Control
	bl 		restore_LCD	
save_exit:
		
	mov		r7,#0x4000000
	mov 	r2,#1
	add		r7,r7,#0x200
	strh 	r2,[r7,#8]		@;IME =1
	sub		r7,r7,#0x200
	
	ldmfd	sp!,{r3}
	strh 	r3,[r7,#REG_SOUNDCNT_X]
	
	ldmfd	sp!,{r3}	
	strh 	r3,[r7,#REG_SOUNDCNT_L]  @;san guo
	
	adrl	r11, spend_0x80
	ldr		r11,[r11]  
	add		r0,r11,#0x40 @;0x3007EC0
	
	ldrh  r3,[r0],#2
	strh 	r3,[r7,#0x60]	@;SOUND1CNT_L
	ldrh  r3,[r0],#6
	strh 	r3,[r7,#0x62]	@;SOUND1CNT_H
	ldrh  r3,[r0],#8
	strh 	r3,[r7,#0x68]	@;SOUND2CNT_L
	ldrh  r3,[r0],#8
	strh 	r3,[r7,#0x70]	@;SOUND3CNT_L
	ldrh  r3,[r0],#8
	strh 	r3,[r7,#0x78]	@;SOUND3CNT_L
	
	add		r11,#0x70   @;0x3007EC0+0x30
	add 	r11,#2
	ldrh  r3,[r11],#2
	strh 	r3,[r7,#0xBA]	@;DMA0CNT_H
	ldrh  r3,[r11],#2
	strh 	r3,[r7,#0xC6]	@;DMA1CNT_H	
	ldrh  r3,[r11],#2	
	strh 	r3,[r7,#0xD2]	@;DMA2CNT_H	
	ldrh  r3,[r11],#2
	strh 	r3,[r7,#0xDE]	@;DMA3CNT_H	
		
	mov 	r0,#0x00
	bl 		SetRampage		
	
	ldmfd	sp!,{r0-r12,PC}
	.ltorg		
	@;===================================================	
call_Save:	
	adrl	r7,IC_switch
	ldr		r7,[r7]
	cmp 	r7,#1
	bne 	errorRTS
	
	@;02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes)
	mov		r8,#0x40   @; 0x40 0x50 0x60 0x70
	mov   r9,#0x2000000
wram_2000000:
	mov 	r0,r8
	bl 		SetRampage	
	mov 	r0,#0x0E000000
	mov 	r1,r9
	mov		r2,#0x10000
	bl 		WriteSram
	add 	r8,#0x10
	add 	r9,#0x10000
	cmp 	r8,#0x80	
	bne 	wram_2000000
	
	@;03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
	mov 	r0,#0x80
	bl 		SetRampage	
	mov r0,#0x0E000000
	mov r1,#0x3000000
	mov r2,#0x8000
	bl 	WriteSram
	
	@;05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
	ldr r0,=0x0E008000
	mov r1,#0x5000000
	mov r2,#0x400
	bl 	WriteSram
	
	@;06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
	ldmfd	sp!,{r6}
	ldr		r7,=0x4000000
	strh 	r6,[r7] @;re
	bl 		restore_LCD		
		
	mov 	r0,#0x90
	bl 		SetRampage	
	mov r0,#0x0E000000
	mov r1,#0x6000000
	mov r2,#0x10000
	bl 	WriteSram
	
	mov 	r0,#0xA0
	bl 		SetRampage	
	mov r0,#0x0E000000
	ldr r1,=0x6010000
	mov r2,#0x8000
	bl 	WriteSram
	
	@;07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
	ldr r0,=0x0E008000
	mov r1,#0x7000000
	mov r2,#0x400
	bl 	WriteSram

	@;R4-R11
	mrs	  r0,CPSR    @;Back up
	adrl	r7, spend_0x80
	ldr		r7,[r7]
	add		r7,#0x30  @;{r4-r11,sp,lr} SPSR  0x28+4
		
	mov		r1, #0xDF		@; Switch to systme Mode
	msr		cpsr_cf, r1
	NOP
	mov		r6,sp
	stmia r7!,{r6,lr}	
	
	msr 	cpsr_cf,r0	;@return IRQ mode	
	NOP
	
	ldr 	r0,=0x0E008400
	adrl 	r1, spend_0x80
	ldr		r1,[r1]
	mov 	r2,#0x80
	bl 		WriteSram	
	
	@;04000000-040003FE   I/O Registers
	ldr r0,=0x0E009000
	ldr r1,=0x06012C00			@;game io snapshot
	mov r2,#0x60					@;0x0-0x60
	bl 	WriteSram
	
	ldr 	r0,=0x0E009060
	adrl 	r1, spend_0x80
	ldr		r1,[r1] 
	add 	r1,#0x40	
	@;ldr 	r1,=0x2010000
	mov 	r2,#0x30   				@;0x60-0x90
	bl 		WriteSram		
	
	ldr r0,=0x0E009090
	ldr r1,=0x06012C90			@;game io snapshot+0x90
	mov r2,#0x370
	bl 	WriteSram
	
	@;FLAG	
	ldr r0,=0x0E00FFF0
	adrl r1,S_RTS_FLAG
	mov r2,#0x10
	bl 	WriteSram	
		
	mov r7,#0x50000
delay_loop:
	cmp			r7,#0
	beq			save_exit	
	nop
	sub 		r7,#0x01
	b				delay_loop
	
	b save_exit	
	@;===================================================		
	@;===================================================	
call_Load:
	@;check 	;FLAG	
	mov 	r0,#0xA0
	bl 		SetRampage
	ldr 	r0,=0x0E00FFF0
	adrl 	r1, spend_0x80  @;temp buff
	ldr 	r1,[r1]
	mov 	r2,#0x10
	bl 		ReadSram	
	
	adrl 	r1,S_RTS_FLAG	
	adrl 	r2, spend_0x80 @;temp buff
	ldr 	r2,[r2]
	mov 	r3,#0
loop_check:
	ldr 	r4,[r1],#4
	ldr 	r5,[r2],#4
	cmp 	r4,r5
	@;bne errorRTS
	add 	r3,#1
	cmp 	r3,#4
	bne 	loop_check
	
	@;02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes)
	mov		r8,#0x40   @; 0x40 0x50 0x60 0x70
	mov   r9,#0x2000000
wram_2000000_Load:
	mov 	r0,r8
	bl 		SetRampage	
	mov 	r0,#0x0E000000
	mov 	r1,r9
	mov		r2,#0x10000
	bl 		ReadSram
	add 	r8,#0x10
	add 	r9,#0x10000
	cmp 	r8,#0x80	
	bne 	wram_2000000_Load

	@;03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
	mov 	r0,#0x80
	bl 		SetRampage	
	mov r0,#0x0E000000
	mov r1,#0x3000000
	mov r2,#0x8000
	bl 	ReadSram
	
	@;05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
	ldr r0,=0x0E008000
	mov r1,#0x5000000
	mov r2,#0x400
	bl 	ReadSram

	@;06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
	mov 	r0,#0x90
	bl 		SetRampage	
	mov r0,#0x0E000000
	mov r1,#0x6000000
	mov r2,#0x10000
	bl 	ReadSram
	mov 	r0,#0xA0
	bl 		SetRampage	
	mov r0,#0x0E000000
	ldr r1,=0x6010000
	mov r2,#0x8000
	bl 	ReadSram
	
	@;07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
	ldr r0,=0x0E008000
	mov r1,#0x7000000
	mov r2,#0x400
	bl 	ReadSram
	
	@;-------------------------------------
	mov r10,#0x4000000
	LDR r11,=0x0E009000
	
	adr r9,register_list
register_list_loop:
	ldrh r2,[r9],#2
	cmp r2 ,#0xFF00
	beq register_list_end

	add r0,r10,r2   @;0x4000000  0x4000002 0x4000004
	add r1,r11,r2
	bl  restore2_IO
	b 	register_list_loop
register_list_end:

	@;mov r10,#0x4000000
	LDR r11,=0x0E008500		
@;	add r0,r10,#0xBA  @;0x40000BA  DMA
@;	add r1,r11,#0x32
@;	bl  restore2_IO	
	add r0,r10,#0xC6  @;0x40000C6
	add r1,r11,#0x34
	bl  restore2_IO	
@;	add r0,r10,#0xD2  @;0x40000D2
@;	add r1,r11,#0x36
@;	bl  restore2_IO	
@;	add r0,r10,#0xDE  @;0x40000DE
@;	add r1,r11,#0x38
@;	bl  restore2_IO	
	
	@;mov   r4,#0x8F
	@;mov   r7,#0x4000000
	@;strh 	r4,[r7,#REG_SOUNDCNT_X]
	
	ldr r0,=0x4000202   @;0x4000202
	mov r1,#0
	strh r1,[r0]
	

	ldr 	r0,=0x0E008400
	adrl 	r1, spend_0x80 @;temp buff
	ldr		r1,[r1] 
	mov 	r2,#0x80
	bl 		ReadSram	
	
	mrs	  r0,CPSR
	adrl	r7, spend_0x80
	ldr		r7,[r7] 
	add		r7,#0x28      @;SPSR offset
	
	ldmia	r7!,{r2}			@;r7=0x2C
	msr		SPSR_cxsf,r2	@;restore SPSR_irq  

	mov		r1, #0xDF		  @;Switch to systme Mode
	msr		cpsr_cf, r1
	NOP
	add		r7,#0x4					@;offset 0x30
	ldmia r7!,{r13-r14}

	msr 	cpsr_cf,r0	  @;restore IRQ	
	NOP

	mov 	r0,#0x0
	bl 		SetRampage
	
	@;spin until VCOUNT==160, triggers next vblank
	mov 	r0,#0x4000000 
spin3_load:
	ldrh 	r1,[r0,#REG_VCOUNT]
	cmp 	r1,#160
	bne 	spin3_load  
	
	add	r2,r0,#0x200			@;IME=1 fallback (old saves carry menu io)
	mov	r3,#1
	strh	r3,[r2,#8]
	
	adrl	r12, spend_0x80
	ldr		r12,[r12] 
	ldmia r12!,{r4-r11,sp,lr}
	
	ldr 	pc,[r0,#-(0x04000000-0x03FFFFF4)] @;to normal IRQ routine		

	@;===================================================	
errorRTS:
	adrl  r8,s_badRTS
	mov		r9,#145	   @;Y
	mov		r10,#64    @;X
showerror:	
	ldrb	r0,[r8],#+1
	cmp		r0,#0x00
	beq		enderror
	mov		r1,r10
	add		r2,r9,#0
	mov	  r3,#0x1F  @;red
	bl		printchar    @; r0 char£¬ r1, X  £¬ r2, Y , r3 color
	add		r10,#8
	b			showerror
enderror:	
waitB:
	ldr		r0,=0x3FF		@;No key press
	ldr		r8,=0x4000130
	ldrh	r9,[r8]
	cmp		r9,r0
	beq		waitB	
pressB:
	mov		r12,r9
	ldrh	r9,[r8]
	cmp		r9,r0
	bne		pressB
	
	ldr		r0,=0x3FD		;@B 
	cmp		r12,r0
	moveq	r11,#5
	beq		b_pressed_quit
	b		waitB

	
	
	@;===================================================	
draw_plot:
	MOV     R11, #0xF0
	MLA     R3, R1, R11, R0
	MOV     R3, R3,LSL#1
	ADD     R3, R3, #0x6000000
	STRH    R2, [R3]
	BX			LR
		@;===================================================	
printchar:	@; r0 ´òÓ¡µÄ×Ö·û£¬ r1, X  £¬ r2, Y , r3 ÑÕÉ«
	STMFD   SP!, {R8-R11,LR}
	MOV     R8, R1
	MOV     R6, R2
	MOV     R9, R3
	MOV     R10, #0x80
	ADRL     R4, ASCII
	SUB     R0, R0, #0x41
	ADD     R4, R4, R0,LSL#4
	ADD     R7, R4, #0x10

loc_A0DD7C:                             
	MOV     R5, #0
loc_A0DD80:
	LDRB    R3, [R4]
	ANDS    R3, R3, R10,ASR R5
	MOVNE   R2, R9
	MOVNE   R1, R6
	ADDNE   R0, R5, R8
	BLNE    draw_plot
	ADD     R5, R5, #1
	CMP     R5, #8
	BNE     loc_A0DD80
	ADD     R4, R4, #1
	CMP     R4, R7
	ADD     R6, R6, #1
	BNE     loc_A0DD7C

	LDMFD   SP!, {R8-R11,PC}
	.align
register_list:
	.hword 0x0000,0x0002,0x0004,0x0008,0x000A,0x000C,0x000E,0x0048
	.hword 0x004A,0x0050,0x0052
	.hword 0x0084
	.hword 0x0060,0x0062,0x0068,0x0070,0x0072,0x0078
	.hword 0x0080,0x0082,0x0088                 @;0x008C,0x008E,
	.hword 0x0090,0x0092,0x0094,0x0096,0x0098,0x009A,0x009C,0x009E
	
	.hword 0x00B8,0x00C4,0x00D0,0x00DC  @;DMA
	.hword 0x0120,0x0122,0x0124,0x0126,0x0128,0x012A,0x012C,0x0132,0x0134
	.hword 0x0140,0x0150,0x0154,0x0200,0x0204,0x0208,0xFF00
	.align
@;------------------------------------------------	
ASCII:	
	.byte		0x00,0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE  @;// -A-
	.byte		0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x66  @;// -B-
	.byte		0x66,0x66,0x66,0xFC,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xC0  @;// -C-
	.byte		0xC0,0xC2,0x66,0x3C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xF8,0x6C,0x66,0x66,0x66,0x66  @; // -D-
	.byte		0x66,0x66,0x6C,0xF8,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68  @;// -E-
	.byte		0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68  @;// -F-
	.byte		0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xDE  @;// -G-
	.byte		0xC6,0xC6,0x66,0x3A,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6  @;// -H-
	.byte		0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18  @;// -I-
	.byte		0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C  @;// -J-
	.byte		0xCC,0xCC,0xCC,0x78,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xE6,0x66,0x6C,0x6C,0x78,0x78  @;// -K-
	.byte		0x6C,0x66,0x66,0xE6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xF0,0x60,0x60,0x60,0x60,0x60  @;// -L-
	.byte		0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6  @;// -M-
	.byte		0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE  @;// -N-
	.byte		0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x38,0x6C,0xC6,0xC6,0xC6,0xC6  @;// -O-
	.byte		0xC6,0xC6,0x6C,0x38,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x60  @;// -P-
	.byte		0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6  @;// -Q-
	.byte		0xC6,0xD6,0xDE,0x7C,0x0C,0x0E,0x00,0x00

	.byte		0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x6C  @;// -R-
	.byte		0x66,0x66,0x66,0xE6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x7C,0xC6,0xC6,0x60,0x38,0x0C  @;// -S-
	.byte		0x06,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x7E,0x7E,0x5A,0x18,0x18,0x18  @;// -T-
	.byte		0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6  @;// -U-
	.byte		0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6 @;// -V-
	.byte		0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,0x00
	
	.byte		0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xD6 @;// -W-
	.byte		0xD6,0xFE,0x6C,0x6C,0x00,0x00,0x00,0x00
	
	.byte		0x00,0x00,0xC6,0xC6,0x6C,0x6C,0x38,0x38  @;// -X-
	.byte		0x6C,0x6C,0xC6,0xC6,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0x66,0x66,0x66,0x66,0x3C,0x18  @;// -Y-
	.byte		0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00

	.byte		0x00,0x00,0xFE,0xC6,0x86,0x0C,0x18,0x30  @;// -Z- 0x5A
	.byte		0x60,0xC2,0xC6,0xFE,0x00,0x00,0x00,0x00
	
	.align
ingameMENU:
s_reset:
 	.byte  'R','E','S','E','T',0x0
s_save:
 	.byte  'S','A','V','E',0x0
s_load:
 	.byte  'L','O','A','D',0x0
s_codes:
 	.byte  'C','O','D','E','S',0x0
	.byte  0xA5  @;end
ic_txt_cheats:
 	.byte  'C','H','E','A','T','S',0x0
ic_txt_on:
 	.byte  'O','N',0x0
ic_txt_off:
 	.byte  'O','F','F',0x0
	.align	
S_RTS_FLAG:
 	.byte  'E','Z','-','O','m','e','g','a','R','T','C','F','I','L','E','.'
	.align	
s_badRTS:
 	.byte  'R','T','S','F','I','L','E','D','A','M','A','G','E','D',0x00
	.align	
IC_switch: 
	.word 0x00000000
	.align
IC_payload_ptr:
	.word 0x00000000

@;==========================================================
@; instant cheat support
@; payload layout (built by C, appended behind this stub):
@;  +0x0010 item table    (item_count * 8)
@;  +0x0210 entry table   (entry_count * 8)
@;  +0x1210 glyph widths  (glyph_count * 2)
@;  +0x1410 glyph bitmaps (glyph_count * 24)
@;  +0x2C10 name blob     (u16 glyph indices, 0xFFFF terminated)
@;==========================================================
	.align
IC_print_text:	@; r0=char* (A-Z only, 0 terminated), r1=x, r2=y, r3=color
	stmfd	sp!,{r4-r11,lr}
	mov		r8,r0
	mov		r9,r1
	mov		r10,r2
	mov		r11,r3
icpt_loop:
	ldrb	r0,[r8],#1
	cmp		r0,#0
	beq		icpt_end
	mov		r1,r9
	mov		r2,r10
	mov		r3,r11
	bl		printchar
	add		r9,#8
	b		icpt_loop
icpt_end:
	ldmfd	sp!,{r4-r11,pc}

	.align
IC_draw_glyph:	@; r0=bitmap(12 x u16), r1=x, r2=y, r3=color
	stmfd	sp!,{r4-r7,lr}
	mov		r4,r0
	mov		r5,#12
	ldr		r6,=0x6000000
	mov		r7,r2,lsl #9
	sub		r7,r7,r2,lsl #5
	add		r7,r7,r1,lsl #1
	add		r6,r6,r7
icdg_row:
	ldrh	r7,[r4],#2
	mov		r7,r7,lsr #4
	mov		r0,#12
icdg_col:
	tst		r7,#0x800
	beq		icdg_nopix
	strh		r3,[r6]
icdg_nopix:
	add		r6,r6,#2
	mov		r7,r7,lsl #1
	subs	r0,r0,#1
	bne		icdg_col
	add		r6,r6,#456
	subs	r5,r5,#1
	bne		icdg_row
	ldmfd	sp!,{r4-r7,pc}

	.align
IC_draw_name:	@; r0=name ptr(u8 indices, 0xFF end), r1=x, r2=y, r3=color
	stmfd	sp!,{r4-r11,lr}
	adrl	r4,IC_payload_ptr
	ldr		r4,[r4]
	mov		r8,r0
	mov		r9,r1
	mov		r10,r2
	mov		r11,r3
	ldr		r6,[r4,#0x20]	@; glyph bitmap offset
	add		r6,r6,r4
	ldr		r7,[r4,#0x1C]	@; glyph width bitmap offset
	add		r7,r7,r4
icdn_loop:
	ldrb	r5,[r8],#1
	cmp		r5,#0xFF
	beq		icdn_end
	cmp		r9,#170	@;leave room for the ON/OFF column at x=196
	bhi		icdn_end
	mov		r0,r5,lsl #4
	add		r0,r0,r5,lsl #3
	add		r0,r0,r6
	mov		r1,r9
	mov		r2,r10
	mov		r3,r11
	bl		IC_draw_glyph
	lsr		r0,r5,#3
	ldrb	r1,[r7,r0]
	and		r0,r5,#7
	mov		r1,r1,lsr r0
	mov		r2,#6
	tst		r1,#1
	addne	r2,r2,#6
	add		r9,r9,r2
	b		icdn_loop
icdn_end:
	ldmfd	sp!,{r4-r11,pc}

@;--------------------------------------------------------
cheat_menu:
	stmfd	sp!,{r0-r12,lr}
	adrl	r0,IC_payload_ptr
	ldr		r0,[r0]
	mov		r8,r0
	ldr		r9,[r8,#4]
	cmp		r9,#0
	beq		icm_exit
	mov		r11,#0
icm_redraw:
	mov		r0,r11
	bic		r0,r0,#7
	mov		r12,r0
	bl		clean_screen
	adrl	r0,ic_txt_cheats
	mov		r1,#16
	mov		r2,#8
	ldr		r3,=0x7FFF
	bl		IC_print_text
	mov		r4,#0
icm_row:
	add		r5,r12,r4
	cmp		r5,r9
	bhs		icm_rows_end
	mov		r6,r4,lsl #4
	sub		r6,r6,r4
	add		r6,r6,#32
	ldr		r7,[r8,#0x10]	@; item table offset
	add		r7,r7,r8
	add		r7,r7,r5,lsl #3
	ldrh	r0,[r7]			@; name offset inside the name blob
	ldr		r1,[r8,#0x24]	@; name blob offset
	add		r0,r0,r1
	add		r0,r0,r8
	mov		r1,#16
	mov		r2,r6
	cmp		r5,r11
	ldreq	r3,=0x6A80
	ldrne	r3,=0x7FFF
	bl		IC_draw_name
	ldr		r0,=0x03007FE0
	lsr		r1,r5,#3
	ldrb	r2,[r0,r1]
	and		r3,r5,#7
	mov		r2,r2,lsr r3
	tst		r2,#1
	beq		icm_state_off
	adrl	r0,ic_txt_on
	b		icm_state_draw
icm_state_off:
	adrl	r0,ic_txt_off
icm_state_draw:
	mov		r1,#196
	mov		r2,r6
	ldr		r3,=0x7FFF
	bl		IC_print_text
	add		r4,r4,#1
	cmp		r4,#8
	blo		icm_row
icm_rows_end:
	ldr		r0,=0x3FF
	ldr		r1,=0x4000130
icm_wait:
	ldrh	r2,[r1]
	cmp		r2,r0
	beq		icm_wait
icm_press:
	mov		r3,r2
	ldrh	r2,[r1]
	cmp		r2,r0
	bne		icm_press
	ldr		r0,=0x3BF
	cmp		r3,r0
	bne		icm_kdown
	cmp		r11,#0
	beq		icm_redraw
	sub		r11,r11,#1
	b		icm_redraw
icm_kdown:
	ldr		r0,=0x37F
	cmp		r3,r0
	bne		icm_ka
	add		r0,r11,#1
	cmp		r0,r9
	beq		icm_redraw
	mov		r11,r0
	b		icm_redraw
icm_ka:
	ldr		r0,=0x3FE
	cmp		r3,r0
	bne		icm_kb
	ldr		r0,=0x03007FE0
	lsr		r1,r11,#3
	ldrb	r2,[r0,r1]
	and		r3,r11,#7
	mov		r4,#1
	mov		r4,r4,lsl r3
	eor		r2,r2,r4
	strb	r2,[r0,r1]
	b		icm_redraw
icm_kb:
	ldr		r0,=0x3FD
	cmp		r3,r0
	bne		icm_wait
icm_exit:
	ldmfd	sp!,{r0-r12,pc}

@;--------------------------------------------------------
IC_apply:	@; called from the IRQ handler, writes every enabled item
	stmfd	sp!,{r4-r12,lr}
	adrl	r9,IC_payload_ptr
	ldr		r9,[r9]
	cmp		r9,#0
	beq		ica_done
	ldr		r8,[r9,#4]			@; item count
	cmp		r8,#0
	beq		ica_done
	ldr		r10,[r9,#0x10]		@; item table
	add		r10,r10,r9
	ldr		r11,[r9,#0x14]		@; entry address table
	add		r11,r11,r9
	ldr		r12,[r9,#0x18]		@; entry value table
	add		r12,r12,r9
	mov		r7,#0
ica_item:
	cmp		r7,r8
	bhs		ica_done
	ldr		r6,=0x03007FE0
	lsr		r0,r7,#3
	ldrb	r1,[r6,r0]
	and		r0,r7,#7
	mov		r1,r1,lsr r0
	tst		r1,#1
	beq		ica_next
	add		r6,r10,r7,lsl #3
	ldrh	r4,[r6,#2]			@; first entry
	ldrh	r5,[r6,#4]			@; entry count
	add		r0,r11,r4,lsl #2
	add		r1,r12,r4
ica_ent:
	cmp		r5,#0
	beq		ica_next
	ldr		r2,[r0],#4
	ldrb	r3,[r1],#1
	strb	r3,[r2]
	sub		r5,r5,#1
	b		ica_ent
ica_next:
	add		r7,r7,#1
	b		ica_item
ica_done:
	ldmfd	sp!,{r4-r12,pc}

	.ltorg
	.align
IC_ReplaceIRQ_end:
	.end
