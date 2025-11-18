#include <kernel.h>
#include <kdata.h>
#include <blkdev.h>
#include <devsd.h>
#include "config.h"
#include "sys_general.h"

#ifdef CONFIG_SD

// Flags
#define SDx_CS				0x01		// 1 = Enable 
#define SDx_SLOW 			0x02		// 1 = Slow 400Khz, 0 = 25Mhz
#define SDx_BUSY			0x80		// 1 = Busy

#define GAVIN_LED_CONTROL ((volatile unsigned int *)0xFEC00000) // Register 0 - Bit[1]
#define MAU_LED_CONTROL   ((volatile unsigned int *)0xFEC0000C) // Register 3 - [11:0] -  000 000
#define MAU_SDCARD0_COLOR 0x00000018 // CYAN (Second LED)
#define MAU_SDCARD1_COLOR 0x00000030 // YELLOW (Second LED)

// A2560K
#if MODEL == MODEL_FOENIX_A2560K
#define SD0_STAT    ((volatile unsigned int *)0xFEC00518)
#define SD0_STAT_CD      0x01000000      /* Is an SD card present? --- 0:Yes, 1:No */
#define SD0_STAT_WP      0x02000000  /* Is the SD card write protected? --- 0:Yes, 1:No */
/* SPI Controler 0 Registers - External Access (Front of Unit)*/
// ONly 1 SDCard
#define SD0_CTRL    ((volatile unsigned char *)0xFEC00300)
#define SD0_DATA	  ((volatile unsigned char *)0xFEC00301)
#define SD1_CTRL    ((volatile unsigned char *)0xFEC00300)
#define SD1_DATA	  ((volatile unsigned char *)0xFEC00301)
// A2560X (GENX)
#elif MODEL == MODEL_FOENIX_A2560X || MODEL == MODEL_FOENIX_GENX
#define SD0_STAT    ((volatile unsigned short *)0xFEC0051A)
#define SD0_STAT_CD      0x0100      /* Is an SD card present? --- 0:Yes, 1:No */
#define SD0_STAT_WP      0x0200      /* Is the SD card write protected? --- 0:Yes, 1:No */
/* SPI Controler 0 Registers - External Access (Front of Unit)*/
// ONly 1 SDCard
#define SD0_CTRL    ((volatile unsigned char *)0xFEC00300)
#define SD0_DATA	  ((volatile unsigned char *)0xFEC00301)
#define SD1_CTRL    ((volatile unsigned char *)0xFEC00300)
#define SD1_DATA	  ((volatile unsigned char *)0xFEC00301)
// A2560M
#elif MODEL == MODEL_FOENIX_A2560M
#define SD0_STAT     ((volatile unsigned int *)0xFEC00518)
#define SD0_STAT_CD      0x01000000      /* Is an SD card present? --- 0:Yes, 1:No */
#define SD0_STAT_WP      0x02000000  /* Is the SD card write protected? --- 0:Yes, 1:No */
/* SPI Controler 0 Registers - External Access (Front of Unit)*/
#define SD0_CTRL    ((volatile unsigned char *)0xFEC00300)
#define SD0_DATA	  ((volatile unsigned char *)0xFEC00301)
/* SPI Controler 1 Registers - External Access (Front of Unit)*/
#define SD1_CTRL    ((volatile unsigned char *)0xFEC00380)
#define SD1_DATA	  ((volatile unsigned char *)0xFEC00381)
// FA2560K2
#elif MODEL == MODEL_FOENIX_FA2560K2 /* FFB0_8000 */
#define SD0_STAT         ((volatile unsigned int *)0xFFB00000)
#define SD0_STAT_CD      0x00004000     /* Is an SD card present? --- 0:Yes, 1:No */
#define SD0_STAT_WP      0x00008000  /* Is the SD card write protected? --- 0:Yes, 1:No */
// FA2560K2
#define SD0_CTRL    ((volatile unsigned char *)0xFFB08000)
#define SD0_DATA    ((volatile unsigned char *)0xFFB08002)
#define DEV_SD0     0       /* Frontal SDCard - Removable Media 0 - SDCARD */
/* SPI Controler 0 Registers - External Access (Front of Unit)*/
#define SD1_CTRL    ((volatile unsigned char *)0xFFB0A000)
#define SD1_DATA    ((volatile unsigned char *)0xFFB0A002)
#define DEV_SD1     1       /* Internal SDCard - Removable Media 1 - SDCARD */
#endif 

////////////////////////////////////////////////////////////////////////////
/// STEFANY's CODE STARTS HERE
/*-----------------------------------------------------------------------*/
/* Transmit Busy Flag Check - SPI Controler 0                          */
/*-----------------------------------------------------------------------*/
static void SDx_Wait_Busy( void ) {
	unsigned char i;
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;

  if (sd_drive == 0) {  
	  i = (*SD0_CTRL & SDx_BUSY);
	  do {
	  	i = (*SD0_CTRL & SDx_BUSY);
	  } while (i == SDx_BUSY);
  }
  else {
	  i = (*SD1_CTRL & SDx_BUSY);
	  do {
	  	i = (*SD1_CTRL & SDx_BUSY);
	  } while (i == SDx_BUSY);
  }
}

// Send a Single Byte on Channel 0 or Channel 1 (When it is a A2560M/PRO/FA2560K2)
void SDx_Tx_Byte( unsigned char byte) {
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;

  if (sd_drive == 0) {  
    *SD0_DATA = byte;
    SDx_Wait_Busy();
  }
  else {
    *SD1_DATA = byte;
    SDx_Wait_Busy();
  }
}
// Receive a Single Byte on Channel 0 or Channel 1 (When it is a A2560M/PRO/FA2560K2)
unsigned char SDx_Rx_Byte( void ) {
    uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;

  if (sd_drive == 0) {  
    *SD0_DATA = 0xFF;
    SDx_Wait_Busy();
    return ( *SD0_DATA );
  }
  else {
    *SD1_DATA = 0xFF;
    SDx_Wait_Busy();
    return ( *SD1_DATA );    
  }
}

/*
#define GAVIN_LED_CONTROL ((volatile unsigned int *)0xFEC00000) // Register 0 - Bit[1]
#define MAU_LED_CONTROL   ((volatile unsigned int *)0xFEC0000C) // Register 3 - [11:0] -  000 000
#define MAU_SDCARD0_COLOR 0b011 // CYAN
#define MAU_SDCARD1_COLOR 0x110 // YELLOW
*/
//////////////////////////////////////////////////////
// FUZIX OFFICIAL DRIVER CODE
//////////////////////////////////////////////////////
void sd_spi_raise_cs(void) {
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;
    *GAVIN_LED_CONTROL = *GAVIN_LED_CONTROL & ~0x2;   // Turn off the Media LED on the main board
    *MAU_LED_CONTROL = 0;

  if (sd_drive == 0) {  
    *SD0_CTRL = *SD0_CTRL & ~ SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
  }
  else {
    *SD1_CTRL = *SD1_CTRL & ~ SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
  }
}

void sd_spi_lower_cs(void) {
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;
  *GAVIN_LED_CONTROL = *GAVIN_LED_CONTROL | 0x2;  // Turn on the Media LED on the main board

  if (sd_drive == 0) {
    *SD0_CTRL = *SD0_CTRL | SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
    *MAU_LED_CONTROL = MAU_SDCARD0_COLOR;
  }
  else {
    *SD1_CTRL = *SD1_CTRL | SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
    *MAU_LED_CONTROL = MAU_SDCARD1_COLOR;    
  }
}

// if 1 go fast 
//#define SDx_SLOW 			0x02		// 1 = Slow 400Khz, 0 = 25Mhz
void sd_spi_clock(bool go_fast) {
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;

  if (sd_drive == 0) {
    if ( go_fast ) {
      *SD0_CTRL = *SD0_CTRL & ~SDx_CS; // 1 = Slow 400Khz, 0 = 25Mhz
    }
    else {
    *SD0_CTRL = *SD0_CTRL | SDx_CS; // 1 = Slow 400Khz, 0 = 25Mhz
    }
  }
  else {
    if ( go_fast ) {
      *SD1_CTRL = *SD1_CTRL & ~SDx_CS; // 1 = Slow 400Khz, 0 = 25Mhz
    }
    else {
    *SD1_CTRL = *SD1_CTRL | SDx_CS; // 1 = Slow 400Khz, 0 = 25Mhz
    }    
  }
}

// Poll before sending if not in Fast mode
// Otherwise 
void sd_spi_transmit_byte(uint8_t byte) {
  SDx_Tx_Byte(byte);
}

// if in slow mode - Poll Sd status
// in Fast mode (it doesn't seem to check )
uint8_t sd_spi_receive_byte(void) {
  return (SDx_Rx_Byte());
}

// Read the Sector and dump in a point from the Block Structure
// blk_op.addr
// Constant: BLKSIZE = 512
/*
    addr = ((uint16_t) blk_op.addr) % 0x4000 + 0x8000;
    page_offset = (((uint16_t)blk_op.addr) / 0x4000);
    page = &udata.u_page;

    len = 0xC000 - addr;
*/
// Go fetch a block of 512 Bytes and store in the blk_op.addr for Fuzix to deal with later! 
bool sd_spi_receive_sector(void) {
	unsigned int bc = 512;
  unsigned char *buff = (uint8_t *) blk_op.addr;  // Setup the Pointer for the buffer
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;

  if (sd_drive == 0) {
    // Drive 0    
	  do {
      	*SD0_DATA = 0xff; // Set the Data in the Transmit Register
      	SDx_Wait_Busy(); // Wait for the transmit to be over with 
	  		*buff++ = *SD0_DATA;			/* Store a received byte */
	  } while (--bc);  
  }
  else {
    // Drive 1
	  do {
      	*SD1_DATA = 0xff; // Set the Data in the Transmit Register
      	SDx_Wait_Busy(); // Wait for the transmit to be over with 
	  		*buff++ = *SD1_DATA;			/* Store a received byte */
	  } while (--bc); 
  }
  
  return 0;

}

bool sd_spi_transmit_sector(void) {
  unsigned int bc = 512;
  unsigned char *buff = (uint8_t *) blk_op.addr;  // Setup the Pointer for the buffer	
  unsigned char d;
  uint_fast8_t sd_drive = blk_op.blkdev->driver_data & 0x0F;
  
  if (sd_drive == 0) {
    // Drive 0
    do {
	  	d = *buff++;	/* Get a byte to be sent */
      *SD0_DATA = d; // Set the Data in the Transmit Register
      SDx_Wait_Busy(); // Wait for the transmit to be over with 
	  } while (--bc);  
  }
  else {
    // Drive 1
    do {
	  	d = *buff++;	/* Get a byte to be sent */
      *SD1_DATA = d; // Set the Data in the Transmit Register
      SDx_Wait_Busy(); // Wait for the transmit to be over with 
	  } while (--bc);  
  }
  return 0;
}

#endif
