
# Atari ST

	int_mfp5:   ; 200Hz timer
		movem.l a0/a1/a5/d0/d1,-(sp)
		move.l udata_shadow,a5  ; set up the register global                                                                            
		jsr timer_interrupt
		bclr.b #5,$fffa11   ; and clear service bit
		movem.l (sp)+,a0/a1/a5/d0/d1

		rte

# RCBUS-68008

*autovector 2 is the only one we use*

autovec_irq:
            ; C will save and restore a2+/d2+
            movem.l a0-a1/a5/d0-d1,-(sp)
            move.l udata_shadow,a5      ; set up the register global
            move.b #1,U_DATA__U_ININTERRUPT(a5)
            jsr plt_interrupt
            clr.b U_DATA__U_ININTERRUPT(a5)

            tst.b U_DATA__U_INSYS(a5)
            bne no_preempt
            tst.b need_resched
            beq no_preempt

