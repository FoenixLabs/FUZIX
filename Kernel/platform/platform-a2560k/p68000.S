
#include "../../cpu-68000/kernel-68000.def"

        .globl plt_reboot
        .globl init_early
        .globl init_hardware
        .globl program_vectors
        .globl interrupt
        .globl plt_idle_internal
        .globl outchar
        .globl plt_monitor
        .globl int_kbd
		.globl int_kbd_mau
        .globl int_timer0
        .globl clear_cpu_ipl

        .globl devide_read_data
        .globl devide_write_data

        .globl uartspi_rx
        .globl uartspi_tx
        .globl uartspi_rx_sector
        .globl uartspi_tx_sector

.mri 1
plt_monitor:				; called on A_SHUTDOWN or A_DUMP
        or #$0700,sr
		jmp plt_monitor

plt_reboot:					; called on A_REBOOT
        or #$0700,sr
        jmp start

init_early:
        lea.l udata_block,a5    ; udata ptr
        move.l a5,udata_shadow  ; shadow copy for entry/exit
        rts

plt_idle_internal:
        stop #($2000+EI_MASK)
        rts
init_hardware:
        move.w #4096,d0         ; XXX in KB, that parameter may be dynamically calculated from model id
        move.w d0,ramsize
        move.l #_end+512,d1
        sub.l #_data,d1
        lsr.l #8,d1
        lsr.l #2,d1
        sub.w  d1,d0        ; Guess for kernel
        move.w d0,procmem       ; guesses for now
        jsr txt_a2560k_b_init
        jsr vtinit
        jsr LPC_Reset       ; Go through a Reset Cycle for the A2560K
        jsr LPC_Config      ; Inititalize the SuperIO Different Memory Zone
        jsr superio_init    ; Intialize a bunch of registers and start the fan and Leds
        rts

clear_cpu_ipl:
        ;andi.w #$F8FF,sr
        clr.l d0 
        move.w sr,d0
        lea $FEC00106,a0
        move d0,(a0)
        rts         

int_timer0:
    movem.l a0/a1/a5/d0/d1,-(sp)
    jsr local_timer_interrupt
    movem.l (sp)+,a0-a1/a5/d0/d1
	rte

int_kbd:
    movem.l a0/a1/a5/d0/d1,-(sp)
    jsr ps2_int
    movem.l (sp)+,a0-a1/a5/d0/d1
    rte

; Serve the Local Keyboard Interrupt (Maurice)
int_kbd_mau:
    movem.l a0/a1/a5/d0/d1,-(sp)
    jsr mau_int
    movem.l (sp)+,a0-a1/a5/d0/d1
    rte

interrupt:
        ; C will save and restore a2+/d2+
        movem.l a0-a1/a5/d0-d1,-(sp)
        move.l udata_shadow,a5  ; set up the register global
        move.b #1,U_DATA__U_ININTERRUPT(a5)
        jsr plt_interrupt
        clr.b U_DATA__U_ININTERRUPT(a5)

        tst.b U_DATA__U_INSYS(a5)
        bne no_preempt
        tst.b need_resched
        beq no_preempt
        ;
        ;   Vanish into the scheduler. Some other task will pop back out
        ;   and eventually we'll re-appear here and continue.
        ;
        ;   FIXME: check IRQ masking
        ;
        move.l U_DATA__U_PTAB(a5),a0
        ; We might be in STOPPED state in which case do not go back
        ; to ready !
        cmp.b #P_RUNNING,P_TAB__P_STATUS_OFFSET(a0)
        bne no_ready
        move.b #P_READY,P_TAB__P_STATUS_OFFSET(a0)
        ori.b #PFL_BATCH,P_TAB__P_FLAGS_OFFSET(a0)
no_ready:
        bsr switchout
no_preempt:
        tst.b U_DATA__U_CURSIG(a5)
        beq no_signal
        ;   FIXME: this is ugly
        movem.l (sp)+,a0-a1/a5/d0-d1
        move.w sr,-(sp)
        move.w #0,-(sp)
        movem.l a0-a6/d0-d7,-(sp)
        move.l (sp),-(sp)
        bsr exception
        addq #4,sp
        movem.l (sp)+,a0-a6/d0-d7
        addq #4,sp
        rte
no_signal:
        movem.l (sp)+,a0-a1/a5/d0-d1
        rte

;
;   Nothing to do in 68000 - all set up once at link time
;
program_vectors:
    rts
;
;   We do no banking so we need to do nothing here.
;
map_process_always:
map_process:
map_kernel:
map_restore:
map_save:
    rts

;   IDE:
;
devide_read_data:
    move.l  d2,-(sp)
    move.l  blk_op,a0
    move.l  #$FEC00400,a1
    move.w  #255,d0
    ;moveq  #8,d2
devide_read_l:
    move.w  (a1),d1
    ;rol.w  d2,d1
    move.w  d1,(a0)+
    dbra    d0,devide_read_l
    move.l  (sp)+,d2
    rts

devide_write_data:
    move.l  d2,-(sp)
    move.l  blk_op,a0
    move.l  #$FEC00400,a1
    move.w  #255,d0
    ;moveq  #8,d2
devide_write_l:
    move.w  (a0)+,d1
    ;rol.w  d2,d1
    move.w  d1,(a1)
    dbra    d0,devide_write_l
    move.l  (sp)+,d2
    rts

;
;   Bitbangers for the SPI bus
;



; outchar: Wait for UART TX idle, then print the char in d0

outchar:
outcharw:
    ; TODO
    rts
;


;
;   Receive a byte not caring about what we send (FF)
;
uartspi_rx_byte:
    movea.l #$A00008,a0     ; MCR
    lea.l   4(a0),a1        ; MSR
    move.b  (a0),d1
    andb    #$FA,d1     ; Clear inverted clock, data
    move.b  d1,d2
    addq.b  #$04,d2     ; with CLK
    ; Entry point if already have the precalc values
uartspi_rx_next:
    ; Unrolled for speed for SD card use
    ; Send 1 bits
    ; Bit 0
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    ; Bus should now hold our bit to sample
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 1
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 2
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 3
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 4
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 5
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 6
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    ; Bit 7
    move.b  d2,(a0)         ; With CLK low
    move.b  d1,(a0)         ; With CLK high
    move.b  (a1),d3
    lsl.b   #1,d3
    roxl.b  #1,d0
    not.b   d0
    rts

uartspi_tx_byte:
    not.b   d0
    move.l  #$A00008,a0     ; MCR
    lea.l   4(a0),a1        ; MSR
uartspi_tx_next:
    move.b  (a0),d1
    and.b   #$FA,d1         ; Clear inverted clock, data
    moveq   #7,d2
spi_tx_bit:
    lsr.b   #1,d1           ; MCR right
    roxl.b  #1,d0           ; Data bit
    roxl.b  #1,d1           ; Into the MCR
    addq    #$04,d1         ; Plus clock
    move.b  d1,(a0)
    subq    #$04,d1         ; Minus clock
    move.b  d1,(a0)
    dbra.w  d2,spi_tx_bit
    rts

;
;   Wrappers for C calling
;
uartspi_rx:
    movem.l d2-d3,-(sp)
    bsr uartspi_rx_byte
    movem.l (sp)+,d2-d3
    rts

uartspi_tx:
    movem.l d2-d3,-(sp)
    move.w  12(sp),d0
    bsr uartspi_tx_byte
    movem.l (sp)+,d2-d3
    rts

;
;   For an SD card this is the most critical path
;
uartspi_rx_sector:
    movem.l a2/d2-d4,-(sp)
    ; Get the argument - address of buffer
    ; As we are a flat system we don't have to worry about
    ; kernel v user etc
    move.l  20(sp),a2
    move.w  #510,d4
    ; This first call primes all the register setup
    bsr uartspi_rx_byte
    move.b  d0,(a2)+
rx_block:
    bsr uartspi_rx_next
    move.b  d0,(a2)+
    dbra.w  d4,rx_block
    movem.l (sp)+,a2/d2-d4
    rts

uartspi_tx_sector:
    movem.l a2/d2-d4,-(sp)
    move.l  20(sp),a2
    move.w  #510,d4
    ; This first call primes all the register setup
    bsr uartspi_tx_byte
    move.b  d0,(a2)+
tx_block:
    bsr uartspi_tx_next
    move.b  d0,(a2)+
    dbra.w  d4,tx_block
    movem.l (sp)+,a2/d2-d4
    rts

LPC_Reset:
    movem.l d2-d3,-(sp)
    ; Changing the Power ON LED Value (Corner of the unit)
    move.l  #$FEC00008,a0
    move.l  #$00D02102,d0   ;
    move.l  d0,(a0)    ; 

    ; Changing the Color of the Keyboard Status LED Value
    move.l  #$FEC0000C,a0
    ; Turn off the Keyboard Reset Value
    move.l  #$00000000,d0
    move.l  d0,(a0)    ; 

    move.l  #$FEC00000,a0
    ; Make sure the LPC is not RESET
    move.l  (a0),d0    ; Read the Content of Gavin Master Control Register
    and     #$00000000,d0 ; Clear the entire register so led can be turned off
    or      #$00000101,d0
    move.l  d0,(a0)    ; LPC = 1 (not reset)
    
    ; FIRST DELAY LOOP
    clr.l   d0         ; We need to create a loop for delay
lpc_delay1_loop:
    add.l   #$1,d0 
    cmpi.l  #$00010000,d0       ; Let's do 2048 Cycles
    bne     lpc_delay1_loop
    
    ; Now RESET the LPC
    move.l  (a0),d0    ; Read the Content of Gavin Master Control Register
    and     #$FFFFFEFF,d0
    move.l  d0,(a0)    ; LPC = 1 (not reset)

    ; SECOND DELAY LOOP (800,000 Cycles)
    clr.l   d0         ; We need to create a loop for delay
lpc_delay2_loop:
    add.l   #$1,d0 
    cmpi.l  #$00080000,d0       ; Let's do 800000 Cycles
    bne     lpc_delay2_loop

    ; FINALY, unreset the LPC and wait again for a moment
    move.l  (a0),d0    ; Read the Content of Gavin Master Control Register
    or      #$00000100,d0
    move.l  d0,(a0)    ; LPC = 1 (not reset)

    ; THIRD DELAY LOOP (800,000 Cycles)
    clr.l   d0         ; We need to create a loop for delay
lpc_delay3_loop:
    add.l   #$1,d0 
    cmpi.l  #$00020000,d0       ; Let's do 400000 Cycles
    bne     lpc_delay3_loop

    ; Normally, getting out of that Routine, the LPC should be ready to be configured and ready to go
    movem.l (sp)+,d2-d3
    rts

LPC_Config:
    movem.l d2-d3,-(sp)
    move.l  #$FEC0202E,a0   ; 0xFEC0202E
    move.l  #$FEC0202F,a1   ; 0xFEC0202F
    clr.l   d0
    clr.l   d1
    ; Enter Config Device Address Space
    move.b  #$55,d0 
    move.b  d0,(a0)         ; This gets us in the configuration mode 
    ; Let's setup device 0 - (floppy)
    ; LD (03F0)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$F0,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT6 
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$06,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device 3 - (Parallel Port)
    ; LD (0378)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$78,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT7
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$07,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; Parallel Port Mode 
    move.b  #$F0,d0     ; Offset Regiset
    move.b  #$3A,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)    
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device 4 - (Serial Port COM1)
    ; LD (03F8)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$04,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$F8,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT4
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$04,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device 5 - (Serial Port COM2)
    ; LD (02F8)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$05,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$02,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$F8,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT3
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device 7 - (Keyboard PS2)
    ; LD (0060)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$07,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$60,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT1 (Keyboard)
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT2 (Mouse)
    move.b  #$72,d0     ; Offset Regiset
    move.b  #$02,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device 9 - (GAME Port)
    ; LD (0200)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$09,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$02,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT0 (no Interrupt)
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device A - (PME)
    ; LD (0100)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$0A,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT0 (no Interrupt)
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$00,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)

    ; Let's setup device A - (MIDI)
    ; LD (0330)
    move.b  #$07,d0     ; Offset Regiset
    move.b  #$0B,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_L (Base Address)
    move.b  #$60,d0     ; Offset Regiset
    move.b  #$03,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; BA_H (Base Address)
    move.b  #$61,d0     ; Offset Regiset
    move.b  #$30,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; INT5 (no Interrupt)
    move.b  #$70,d0     ; Offset Regiset
    move.b  #$05,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)
    ; ENABLE Zone
    move.b  #$30,d0     ; Offset Regiset
    move.b  #$01,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value)        

    ; Power Management - Turn all the devices ON
    move.b  #$22,d0     ; Offset Regiset
    move.b  #$FF,d1     ; Device #
    move.b  d0,(a0)     ; Store in 2E (offset)
    move.b  d1,(a1)     ; Store in 2F (Value) 

    ; Exit Config Device Address Space (we are done)
    ; Now we ought to have full control over all devices:
    ; Floppy Controller
    ; Parallel Port
    ; Keyboard PS2 8042 Controller
    ; Serial 1
    ; Serial 2
    ; PME 
    ; MIDI
    move.b  #$AA,d0 
    move.b  d0,(a0)         ; This gets us in the configuration mode 


    move.l  #$FEC02147,a0   ; 0xFEC02147 - GP60 REG
    move.b  #$84,d0     
    move.b  d0,(a0)         ;

    movem.l (sp)+,d2-d3
    rts