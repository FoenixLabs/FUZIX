#include <kernel.h>
#include <timer.h>
#include <kdata.h>
#include <printf.h>
#include <devtty.h>
#include <blkdev.h>
#include <bq4845.h>
#include <ps2kbd.h>
#include "timers.h"
#include "interrupt.h"

uint16_t swap_dev = 0xFFFF;

void do_beep(void)
{
}

/*
 *	MMU initialize
 */

void map_init(void)
{
}

uint8_t ps2kbd_present;
uaddr_t ramtop;
uint8_t need_resched;

uint8_t plt_param(char *p)
{
	return 0;
}

void plt_discard(void)
{
}

void memzero(void *p, usize_t len)
{
	memset(p, 0, len);
}

void pagemap_init(void)
{
	uint8_t r;
	/* Linker provided end of kernel */
	/* TODO: create a discard area at the end of the image and start
	   there */
	extern uint8_t _end;
	uint32_t e = (uint32_t)&_end;
	/* Allocate the rest of memory to the userspace */
	kmemaddblk((void *)e, 0x400000 - e);              // XXX: parametrize that!

	kprintf("Motorola 680%s%d processor detected.\n",
		sysinfo.cpu[1]?"":"0",sysinfo.cpu[1]);
	enable_icache();

    bq4845_init();

    kprintf("ps2: preinit\n");
    ps2kbd_present = ps2_init();        // platform-specific routine
    if (ps2kbd_present) {
    	kprintf("ps2: keyboard found\n");
    }
}

/* Udata and kernel stacks */
/* We need an initial kernel stack and udata so the slot for init is
   set up at compile time */
u_block udata_block[PTABSIZE];

/* This will belong in the core 68K code once finalized */

void install_vdso(void)
{
	extern uint8_t vdso[];
	/* Should be uput etc */
	memcpy((void *)udata.u_codebase, &vdso, 0x20);
}

uint8_t plt_udata_set(ptptr p)
{
	p->p_udata = &udata_block[p - ptab].u_d;
	return 0;
}

void plt_interrupt(void)
{
    tty_interrupt();
}

void local_timer_interrupt(void)
{
    //kprintf("timer0 fired\n");
    int_clear(INT_TIMER0);
    *TIMER_TCR0      = TCR_CNTUP_0 | TCR_CLEAR_0 | TCR_INE_0;
    *TIMER_TCR0      = TCR_ENABLE_0 | TCR_CNTUP_0 | TCR_INE_0;
    timer_interrupt();
}

void plt_idle(void)
{
    irqflags_t flags = di();
    tty_poll();
    irqrestore(flags);
}

void plt_copyright(void)
{
        kprintf("\nFoenix A2560X port version 0.1\n2025 Piotr Meyer <aniou@smutek.pl>\n\n");
}

// eof
