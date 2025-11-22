#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <devtty.h>
#include <vt.h>
#include <tty.h>
#include "uart.h"
#include "include/uart_reg.h"

static unsigned char tbuf1[TTYSIZ];
static unsigned char tbuf2[TTYSIZ];
static unsigned char tbuf3[TTYSIZ];

struct s_queue ttyinq[NUM_DEV_TTY + 1] = {	/* ttyinq[0] is never used */
	{NULL, NULL, NULL, 0, 0, 0},
	{tbuf1, tbuf1, tbuf1, TTYSIZ, 0, TTYSIZ / 2},	// Screen
	{tbuf2, tbuf2, tbuf2, TTYSIZ, 0, TTYSIZ / 2},	// COM1
	{tbuf3, tbuf3, tbuf3, TTYSIZ, 0, TTYSIZ / 2}	// COM2
};

tcflag_t termios_mask[NUM_DEV_TTY + 1] = {
	0,
	_CSYS,
	_CSYS,
	_CSYS,	
};

/*
// Group the tty into a single object. That lets 8bit processors keep all
//   the data indexed off a single register 
struct tty {
    // Put flag first: makes it cheaper when short of registers
    uint8_t flag;		// make the whole struct
                        //          32 byte - a nice number for CPUs with no 
                        //         multiplier
    uint8_t users;
#define TTYF_STOP	1
#define TTYF_DISCARD	2
#define TTYF_DEAD	4
 
    uint16_t pgrp;
    struct termios termios;
    struct winsize winsize; // 8 byte so takes us up to 32
};

struct winsize {		//Keep me 8bytes on small boxes
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

*/

uint16_t divisor_table[16] = { 0, 24576, 12288, 6144, 3072, 1536, 768, 384, 96, 48, 24, 12, 6, 3, 2, 1 };

// There are 2 UART (LPC) in the follow products:
// A2560X40 	(LPC 2 Ports)
// A2560X60 	(LPC 2 Ports)
// A2560X60+ 	(LPC 2 Ports)
// A2560KC 		(LPC 2 Ports)
// A2560K40 	(LPC 2 Ports)
// A2560K60 	(LPC 2 Ports)
// A2560M 		(LPC 2 Ports) + 1 Simple UART (USB-C)
// A2560M Pro 	(LPC 2 Ports) + 1 Simple UART (USB-C)
// FA2560K2 	(NO LPC) + 1 Simple UART (USB-C)
volatile unsigned char * uart_get_base(short uart) {
    
	if (uart == 2) // TTY Number 1
        return (volatile unsigned char *)UART1_BASE;

	if (uart == 3) // TTY Number 2
        return (volatile unsigned char *)UART2_BASE;

	return (volatile unsigned char *)UART1_BASE;
}
// Early Init for the COM1 with 115K 8N1
// /dev/tty2
void early_COM1_init( void ) {
    volatile unsigned char * uart_base = uart_get_base(2);
	unsigned short bps_code = 1;	// 1 = 115200
		// Set Speed!
		/* Enable divisor latch */
		uart_base[UART_LCR] = uart_base[UART_LCR] | 0x80;
        /* Set the divisor */
        uart_base[UART_TRHB] = bps_code & 0xff;
        uart_base[UART_TRHB+1] = (bps_code >> 8) & 0xff;
        /* Disable divisor latch */
        uart_base[UART_LCR] = uart_base[UART_LCR] & 0x7F;	
		
		// Set Format
		uart_base[UART_LCR] = LCR_PARITY_NONE | LCR_STOPBIT_1 | LCR_DATABITS_8;
        uart_base[UART_FCR] = 0xC1;
}

// Early Init for the COM2 with 115K 8N1
// /dev/tty3
void early_COM2_init( void ) {
	    volatile unsigned char * uart_base = uart_get_base(3);
		unsigned short bps_code = 1;	// 1 = 115200
		// Set Speed!
		/* Enable divisor latch */
		uart_base[UART_LCR] = uart_base[UART_LCR] | 0x80;
        /* Set the divisor */
        uart_base[UART_TRHB] = bps_code & 0xff;
        uart_base[UART_TRHB+1] = (bps_code >> 8) & 0xff;
        /* Disable divisor latch */
        uart_base[UART_LCR] = uart_base[UART_LCR] & 0x7F;	
		
		// Set Format
		uart_base[UART_LCR] = LCR_PARITY_NONE | LCR_STOPBIT_1 | LCR_DATABITS_8;
        uart_base[UART_FCR] = 0xC1;
}

// Set the Baudrate, The Size, The Stop Bit & the Parity
void tty_setup(uint_fast8_t minor, uint_fast8_t flags) {
	uint16_t b;
	uint8_t lcr = 0;
	//kprintf("tty_setup - Minor: %x, Flags: %x\n", minor, ttydata[minor].termios.c_cflag);
	if ((minor == 2) || (minor == 3)) {
    	volatile unsigned char * uart_base = uart_get_base(minor);
		b = ttydata[minor].termios.c_cflag & CBAUD;
		// Set the Baudrate
        uart_base[UART_LCR] = uart_base[UART_LCR] | 0x80;
        // Set the divisor
        uart_base[UART_TRHB] = divisor_table[b] & 0xff;
        uart_base[UART_TRHB+1] = (divisor_table[b] >> 8) & 0xff;
        // Disable divisor latch
        uart_base[UART_LCR] = uart_base[UART_LCR] & 0x7F;
		// word length 5(00), 6(01), 7(10), or 8(11) 
		lcr = (ttydata[minor].termios.c_cflag & CSIZE) >> 4;
		// stop bits 1(0), or 2(1)
		lcr |= (ttydata[minor].termios.c_cflag & CSTOPB) >> 4;
		// parity disable(0), or enable(1)
		lcr |= (ttydata[minor].termios.c_cflag & PARENB) >> 5;
		// parity odd(0), or even(1)
		lcr |= ((ttydata[minor].termios.c_cflag & PARODD) ^ PARODD) >> 5;
		uart_base[UART_LCR] = lcr;
		// UART0_MCR = 0x03;  DTR = ON, RTS = ON 
        // Enable FIFO, set for 56 byte trigger level
       	uart_base[UART_FCR] = 0xC1;		
	}
}

// minor 1 or 2
int tty_carrier(uint_fast8_t minor) {

	uint8_t c;	
	if ((minor == 2) || (minor == 3)) {
		volatile unsigned char * uart_base = uart_get_base(minor);		
		c = uart_base[UART_MCR];
		return (c & 0x80) ? 1 : 0; /* test DCD */
	}
	else 
		return 1;
}


void tty_interrupt(void)
{
    //tty_poll();
}


void tty_putc(uint_fast8_t minor, uint_fast8_t c) {
    unsigned char status = 0;   
    unsigned char Timeout = 64;

	if ((minor == 2) || (minor == 3))  {
    volatile unsigned char * uart_base = uart_get_base(minor);		
        do {
            status = uart_base[UART_LSR];
            Timeout = Timeout - 1;
        } while (((status & LSR_XMIT_EMPTY) == 0) && Timeout);
		uart_base[UART_TRHB] = c;
	}
	else {
	   vtoutput(&c, 1);
	}
}

void tty_sleeping(uint_fast8_t minor) {
	
	if ((minor == 2) || (minor == 3))  {
		volatile unsigned char * uart_base = uart_get_base(minor);
		uart_base[UART_IER] = 0x0B; /* enable all but LSR interrupt */
	}
}

// Not used
void tty_data_consumed(uint_fast8_t minor) {

}


ttyready_t tty_writeready(uint_fast8_t minor) {
	uint8_t c;
	if ((minor == 2) || (minor == 3))  {
		volatile unsigned char * uart_base = uart_get_base(minor);
		c = uart_base[UART_MSR];
		if ((ttydata[minor].termios.c_cflag & CRTSCTS) && (c & 0x10) == 0) /* CTS not asserted? */
			return TTY_READY_LATER;
		c = uart_base[UART_LSR];
		if (c & 0x20) /* THRE? */
			return TTY_READY_NOW;
		return TTY_READY_SOON;
	}

	return TTY_READY_NOW;
}

/* Output for the system console (kprintf etc) */
/* IMPORTANT: VT output is sent directly to tty_putc! */
void kputchar(uint_fast8_t c)
{
	if (c == '\n')
		kputchar('\r');
	tty_putc(TTYDEV & 0xff, c);
}



void tty_poll(uint_fast8_t minor)
{
	volatile unsigned char * uart_base = uart_get_base(minor);	// Minor 1
	uint8_t iir, msr, lsr;

	while (true) {
		// Let's UART0 (Minor 1)
		iir = uart_base[UART_IIR];
		lsr = uart_base[UART_LSR];
		/* IRR bits
		 * 3 2 1 0
		 * -------
		 * x x x 1     no interrupt pending
		 * 0 1 1 0  6  LSR changed -- read the LSR
		 * 0 1 0 0  4  receive FIFO >= threshold
		 * 1 1 0 0  C  received data sat in FIFO for a while
		 * 0 0 1 0  2  transmit holding register empty
		 * 0 0 0 0  0  MSR changed -- read the MSR
		 */
		switch (iir & 0x0F) {
		case 0x0: /* MSR changed */
		case 0x2: /* transmit register empty */
			msr = uart_base[UART_MSR];
			if ((msr & 0x10) && (lsr & 0x20)){
				/* CTS high, transmit reg empty */
				tty_outproc(minor);
			}
			/* fall through */
		case 0x6: /* LSR changed */
			/* we already read the LSR register so int has cleared */
			uart_base[UART_IER] = 0x01; /* enable only receive interrupts */
			break;
		case 0x4: /* receive (FIFO >= threshold) */
		case 0xC: /* receive (timeout waiting for FIFO to fill) */
			while (lsr & 0x01) { /* Data ready */
				tty_inproc(minor, uart_base[UART_TRHB]);
				lsr = uart_base[UART_LSR];
			}
			break;

		default:
			return;
		}
	}
}




// eof
