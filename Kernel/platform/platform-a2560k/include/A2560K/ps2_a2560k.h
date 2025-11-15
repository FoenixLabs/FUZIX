#ifndef __PS2_A2560K_H
#define __PS2_A2560K_H

/*
 * Ports for the PS/2 keyboard and mouse on the A2560K
 *
 * waring: volatile is required because of gcc optimizations!
 */

#define PS2_STATUS      ((volatile unsigned char *)0xFEC02064)
#define PS2_CMD_BUF     ((volatile unsigned char *)0xFEC02064)
#define PS2_OUT_BUF     ((volatile unsigned char *)0xFEC02060)
#define PS2_INPT_BUF    ((volatile unsigned char *)0xFEC02060)
#define PS2_DATA_BUF    ((volatile unsigned char *)0xFEC02060)

#endif
