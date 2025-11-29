
/* SPI-based w5500 implementation */

#include <kernel.h>
#include <blkdev.h>
#include <printf.h>
#include "sdc_reg.h"
#include "timers.h"

#define SDC_TIMEOUT_JF 30           /* Timeout in jiffies (1/60 second) */


#ifdef CONFIG_NET

#include <kdata.h>
#include <printf.h>
#include <netdev.h>


#ifdef CONFIG_NET_WIZNET
#include "../../dev/net/net_w5x00.h"
/*
 *	Wiznet 5500 glue to the SPI layer
 */

#define _SLOT(x)	((x) << 2)
#define SOCK2BANK_C(x)	((_SLOT(x) | 1) << 3)
#define SOCK2BANK_W(x)	((_SLOT(x) | 2) << 3)
#define SOCK2BANK_R(x)	((_SLOT(x) | 3) << 3)
#define W_WRITE	0x04


// Flags
#define W5x00_CS			0x01		// 1 = Enable, 0 = Disable
#define W5x00_SLOW 			0x02		// 1 = Slow 400Khz, 0 = 25Mhz
#define W5x00_RESET			0x04		// 1 = RESET, 0 = Normal Operation
#define W5x00_BUSY			0x80		// 1 = Busy
#define W5x00_CTRL      ((volatile unsigned char *)0xFEC00A00)
#define W5x00_DATA	    ((volatile unsigned char *)0xFEC00A01)
static void W5x00_Wait_Busy( void );
void A2560xx_WP5x00_Reset( void );
void A2560xx_WP5x00_Activate_CS( void );
void A2560xx_WP5x00_DeActivate_CS( void );
void A2560xx_WP5x00_Write_SPI( unsigned char Data2Write);
unsigned char A2560xx_WP5x00_Read_SPI( void );
unsigned char A2560xx_WP5x00_RdWr_SPI( unsigned char Data2Write );


/* We can optimize this lot later when it works nicely */
static void spi_transaction(uint8_t ctrl, uint16_t off,
	uint8_t *out, uint16_t outlen, uint8_t *in, uint16_t inlen) {
	irqflags_t irq = di();
	//spi_select_port(1);
    A2560xx_WP5x00_Activate_CS(); // Bring CS Down (Active State)
	A2560xx_WP5x00_RdWr_SPI(off >> 8);
	A2560xx_WP5x00_RdWr_SPI(off);
	A2560xx_WP5x00_RdWr_SPI(ctrl);
	while(outlen--) {
		A2560xx_WP5x00_RdWr_SPI(*out++);
	}
	while(inlen--) {
		*in = A2560xx_WP5x00_RdWr_SPI(0xFF);
		in++;
	}
    A2560xx_WP5x00_DeActivate_CS(); // Bring CS back up
	//spi_deselect_port(1);
	irqrestore(irq);
}

uint8_t w5x00_readcb(uint16_t off) {
	uint8_t r;
	spi_transaction(0, off, NULL, 0, &r, 1);
	return r;
}

uint8_t w5x00_readsb(uint8_t s, uint16_t off) {
	uint8_t r;
	spi_transaction(SOCK2BANK_C(s), off, NULL, 0, &r, 1);
	return r;
}

uint16_t w5x00_readcw(uint16_t off) {
	uint16_t r;
	spi_transaction(0, off, NULL, 0, (uint8_t *)&r, 2);
	return ntohs(r);
}

uint16_t w5x00_readsw(uint8_t s, uint16_t off) {
	uint16_t r;
	spi_transaction(SOCK2BANK_C(s), off, NULL, 0, (uint8_t *)&r, 2);
	return ntohs(r);
}

void w5x00_bread(uint16_t bank, uint16_t off, void *pv, uint16_t n) {
	spi_transaction(bank, off, NULL, 0, pv, n);
}

void w5x00_breadu(uint16_t bank, uint16_t off, void *pv, uint16_t n) {
	spi_transaction(bank, off, NULL, 0, pv, n);
}

void w5x00_writecb(uint16_t off, uint8_t n) {
	spi_transaction(W_WRITE, off, &n, 1, NULL, 0);
}

void w5x00_writesb(uint8_t sock, uint16_t off, uint8_t n) {
	spi_transaction(SOCK2BANK_C(sock) | W_WRITE, off, &n, 1, NULL, 0);
}

void w5x00_writecw(uint16_t off, uint16_t n) {
	n = ntohs(n);
	spi_transaction(W_WRITE, off, (uint8_t *)&n, 2, NULL, 0);
}

void w5x00_writesw(uint8_t sock, uint16_t off, uint16_t n) {
	n = ntohs(n);
	spi_transaction(SOCK2BANK_C(sock) | W_WRITE, off, (uint8_t *)&n, 2, NULL, 0);
}

void w5x00_bwrite(uint16_t bank, uint16_t off, void *pv, uint16_t n) {
	spi_transaction(bank|W_WRITE, off, pv, n, NULL, 0);
}

void w5x00_bwriteu(uint16_t bank, uint16_t off, void *pv, uint16_t n) {
	spi_transaction(bank|W_WRITE, off, pv, n, NULL, 0);
}

void w5x00_setup(void) {
	/* These delays are excessive but the reset seems to be very fragile */
	volatile uint32_t n = 0;
	//GPOC = 1 << RESETPIN;
	//while(n++ < 5000000);
	//GPOS = 1 << RESETPIN;
	//spi_set_clock(1, 1);
    A2560xx_WP5x00_Reset(); // Go reset the external Donbgle Module
	n = 0;
	while(n++ < 5000000);
	w5x00_writecb(0, 0x80);
	n = 0;
	while(n++ < 5000000);
	w5x00_readcb(0x19);
	w5x00_readcb(0x19);
	w5x00_readcb(0x19);
	w5x00_readcb(0x1A);
	w5x00_readcb(0x1A);
}

/*
// Flags
#define W5x00_CS			0x01		// 1 = Enable, 0 = Disable
#define W5x00_SLOW 			0x02		// 1 = Slow 400Khz, 0 = 25Mhz
#define W5x00_RESET			0x04		// 1 = RESET, 0 = Normal Operation
#define W5x00_BUSY			0x80		// 1 = Busy
#define W5x00_CTRL      ((volatile unsigned char *)0xFEC00A00)
#define W5x00_DATA	    ((volatile unsigned char *)0xFEC00A01)
*/




static void W5x00_Wait_Busy( void ) {
	unsigned char i;

	  do {
	  	i = (*W5x00_CTRL & W5x00_BUSY);
	  } while (i == W5x00_BUSY);
}


void A2560xx_WP5x00_Reset( void ) {
	volatile uint32_t n = 0;
	*W5x00_CTRL = *W5x00_CTRL | W5x00_RESET;
	while(n++ < 5000000);
	*W5x00_CTRL = *W5x00_CTRL & ~W5x00_RESET;
}

void A2560xx_WP5x00_Activate_CS( void ) {
	*W5x00_CTRL = *W5x00_CTRL | W5x00_CS;
}

void A2560xx_WP5x00_DeActivate_CS( void ) {
	*W5x00_CTRL = *W5x00_CTRL & ~W5x00_CS;
}
// Write Only
void A2560xx_WP5x00_Write_SPI( unsigned char Data2Write ) {
    *W5x00_DATA = Data2Write;
    W5x00_Wait_Busy();
}
// Read Only
unsigned char A2560xx_WP5x00_Read_SPI( void ) {
    *W5x00_DATA = 0xff;
    W5x00_Wait_Busy();
    return ( *W5x00_DATA );
}
// Read Write
unsigned char A2560xx_WP5x00_RdWr_SPI( unsigned char Data2Write ) {
    *W5x00_DATA = Data2Write;
    W5x00_Wait_Busy();
    return ( *W5x00_DATA );
}

#endif
#endif



// eof
