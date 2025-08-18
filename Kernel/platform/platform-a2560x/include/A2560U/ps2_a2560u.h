#ifndef __PS2_A2560U_H
#define __PS2_A2560U_H

/*
 * Ports for the PS/2 keyboard and mouse on the A2560U and A2560U+
 *
 * waring: volatile is required because of gcc optimizations!
 */

#define PS2_STATUS      ((volatile unsigned char *)0x00B02804)
#define PS2_CMD_BUF     ((volatile unsigned char *)0x00B02804)
#define PS2_OUT_BUF     ((volatile unsigned char *)0x00B02800)
#define PS2_INPT_BUF    ((volatile unsigned char *)0x00B02800)
#define PS2_DATA_BUF    ((volatile unsigned char *)0x00B02800)

#endif
