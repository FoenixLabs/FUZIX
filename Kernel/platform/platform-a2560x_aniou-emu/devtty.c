#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <devtty.h>
#include <vt.h>
#include <tty.h>

static unsigned char tbuf1[TTYSIZ];
static unsigned char tbuf2[TTYSIZ];

struct s_queue ttyinq[NUM_DEV_TTY + 1] = {	/* ttyinq[0] is never used */
	{NULL, NULL, NULL, 0, 0, 0},
	{tbuf1, tbuf1, tbuf1, TTYSIZ, 0, TTYSIZ / 2},
	{tbuf2, tbuf2, tbuf2, TTYSIZ, 0, TTYSIZ / 2},
};

tcflag_t termios_mask[NUM_DEV_TTY + 1] = {
	0,
	_CSYS,
	_CSYS,
};

/* Output for the system console (kprintf etc) */
/* IMPORTANT: VT output is sent directly to tty_putc! */
void kputchar(uint_fast8_t c)
{
	if (c == '\n')
		kputchar('\r');

	//while(tty_writeready(1) != TTY_READY_NOW);
	tty_putc(TTYDEV & 0xff, c);
}

ttyready_t tty_writeready(uint_fast8_t minor)
{
	return TTY_READY_NOW;
}

void tty_putc(uint_fast8_t minor, uint_fast8_t c)
{
    vtoutput(&c, 1);
}

void tty_setup(uint_fast8_t minor, uint_fast8_t flags)
{
}

int tty_carrier(uint_fast8_t minor)
{
	return 1;
}

void tty_sleeping(uint_fast8_t minor)
{
}

void tty_data_consumed(uint_fast8_t minor)
{
}

void tty_interrupt(void)
{
    //tty_poll();
}

void tty_poll(void)
{
    //tty_inproc(1, *UART2_BASE);
    //vt_inproc(1, *UART2_BASE);    // to nie moze byc wlaczone, bo na hw zasmieca bootline
}

// eof
