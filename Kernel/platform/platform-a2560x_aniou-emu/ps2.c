#include <kernel.h>
#include <kdata.h>
#include <input.h>
#include <devinput.h>
#include <printf.h>
#include <ps2kbd.h>
#include "ps2_reg.h"
#include "interrupts.h"
#include "timers.h"

/* these bits are ripped-off from dev/ps2.c file from FoenixMCP 
 * with all rights reserved to original authors (see README.md)
 *
 * In the future FoenixMCP routines shall be a new standard for
 * FUZIX port for Foenix - at this moment I just want to Poc my
 * assumpions without introducing changes to core FUZI code
 */

#define PS2_TIMEOUT_JF          20          /* Timeout in jiffies: 1/60 second units */
#define INT_KBD_PS2           0x10          /* SuperIO - PS/2 Keyboard */

/*
 * Wait for the output buffer of the PS/2 controller to have data.
 *
 * Returns:
 *  0 if successful, -1 if there was no response after PS2_RETRY_MAX tries
 */
short ps2_wait_out() {
    long target_ticks;

    target_ticks = rtc_get_jiffies() + PS2_TIMEOUT_JF;
    while ((*PS2_STATUS & PS2_STAT_OBF) == 0) {
        if (rtc_get_jiffies() > target_ticks) {
            kprintf("ps2: out status timeout: %2x %2x\n", *PS2_STATUS, *PS2_STATUS & PS2_STAT_OBF);
            return -1;
        }
    }
    return 0;
}

/*
 * Wait for the input buffer of the PS/2 controller to be clear.
 *
 * Returns:
 *  0 if successful, -1 if there was no response after PS2_RETRY_MAX tries
 */
short ps2_wait_in() {
    long target_ticks;

    target_ticks = rtc_get_jiffies() + PS2_TIMEOUT_JF;
    while ((*PS2_STATUS & PS2_STAT_IBF) != 0) {
        if (rtc_get_jiffies() > target_ticks) {
            kprintf("ps2 wait in timeout\n");
            return -1;
        }
    }
    //kprintf("ps2 in status: %2x\n", *PS2_STATUS);

    return 0;
}

/*
 * Send a command to the controller and wait for a response.
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_controller_cmd(unsigned char cmd) {
    if (ps2_wait_in()) return -1;
    *PS2_CMD_BUF = cmd;

    if (ps2_wait_out()) return -1;
    return (short)*PS2_DATA_BUF;
}

/*
 * Send a command with parameter to the controller and wait for a response.
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_controller_cmd_param(unsigned char cmd, unsigned char parameter) {
    if (ps2_wait_in()) return -1;
    *PS2_CMD_BUF = cmd;

    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = parameter;

    return 0;
}

/*
 * Send a command with parameter to the keyboard and wait for a response.
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_kbd_cmd_p(unsigned char cmd, unsigned char parameter) {
    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = cmd;

    // May need a delay here

    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = parameter;

    // Return 0 by default... maybe read DATA?
    return 0;
}

/*
 * Send a command to the keyboard and wait for a response.
 *                                                                                                                                  
 * Inputs:
 *  cmd = the command byte to send to the keyboard
 *  delay = an indication of how much to wait before checking for output
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_kbd_cmd(unsigned char cmd, short delay) {
    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = cmd;

    // A delay may be needed here
    while (delay-- > 0) {
        ;
    }

    if (ps2_wait_out()) return -1;
    return (short)*PS2_DATA_BUF;
}

/*
 * Read from the PS/2 data port until there are no more bytes ready.
 */
unsigned char ps2_flush_out() {
    unsigned char x;
    while (*PS2_STATUS & PS2_STAT_OBF) {
        x = *PS2_DATA_BUF;
    }
    return x;
}

short ps2_init() {

    // Prevent the keyboard and mouse from sending events
    ps2_controller_cmd(PS2_CTRL_DISABLE_1);
    ps2_controller_cmd(PS2_CTRL_DISABLE_2);

    // Read and clear out the controller's output buffer
    ps2_flush_out();

    // // Controller selftest...
    if (ps2_controller_cmd(PS2_CTRL_SELFTEST) != PS2_RESP_OK) {
        ; // return PS2_FAIL_SELFTEST;
    }

    // Keyboard test
    if (ps2_controller_cmd(PS2_CTRL_KBDTEST) != 0) {
        ; // return PS2_FAIL_KBDTEST;
    }

    // disable forced translation to scancode 1 and enable interrupts for m&k
    ps2_controller_cmd_param(PS2_CTRL_WRITECMD, 0x03);  /* %00000011 */

    // Enable the keyboard, don't check response
    ps2_wait_in();
    *PS2_CMD_BUF = PS2_CTRL_ENABLE_1;

    // Reset the keyboard... waiting a bit before we check for a result
    ps2_kbd_cmd(KBD_CMD_RESET, 1000);

    // Ideally, we would attempt a re-enable several times, but this doesn't work on the U/U+ for some keyboards
    ps2_kbd_cmd(KBD_CMD_ENABLE, 0);

    ps2_wait_in();
    *PS2_CMD_BUF = PS2_CTRL_ENABLE_2;

    // set scancode page 2
    ps2_kbd_cmd_p(0xF0, 0x02);

    // Make sure everything is read
    ps2_flush_out();

    // Clear any pending keyboard interrupts
    int_clear(INT_KBD_PS2);

    ps2busy = 0;
    return 1;   // keyboard present
}


void ps2_int(void)
{
    unsigned char x = *PS2_DATA_BUF;

    int_clear(INT_KBD_PS2);

    if (x == 0xFA) {        // mystery stray 0xFA on A2560X kbd after irq enable
        return;
    }
    ps2kbd_byte(x);
}

int ps2kbd_put(uint_fast8_t c)
{
    if (ps2_wait_in() == 0) {
        *PS2_STATUS = c;
        return 0;
    } else {
        kprintf("\nps2: put timeout\n");
        return 0xFFFF;
    }
}

uint16_t ps2kbd_get(void)
{
    if (ps2_wait_out() == 0) {
        return *PS2_DATA_BUF;
    } else {
        return PS2_NOCHAR;
    }
}

// eof
