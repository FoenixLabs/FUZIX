#include <kernel.h>
#include <kdata.h>
#include <input.h>
#include <devinput.h>
#include <printf.h>
#include <ps2kbd.h>
#include "ps2_reg.h"
#include "interrupt.h"
#include "timers.h"
#include "ps2.h"
#include "../include/A2560K/gabe_a2560k.h"
#include "ring_buffer.h"
/* these bits are ripped-off from dev/ps2.c file from FoenixMCP 
 * with all rights reserved to original authors (see README.md)
 *
 * In the future FoenixMCP routines shall be a new standard for
 * FUZIX port for Foenix - at this moment I just want to Poc my
 * assumpions without introducing changes to core FUZI code
 */

#define PS2_TIMEOUT_JF        20          /* Timeout in jiffies: 1/60 second units */
#define INT_KBD_PS2         0x10    /* SuperIO - PS/2 Keyboard */
#define INT_KBD_A2560K      0x11    /* SuperIO - A2560K Built in keyboard (Mo) */
#define INT_MSE_PS2         0x12    /* SuperIO - PS/2 Mouse */



#if MODEL == MODEL_FOENIX_A2560K

#define KBD_MO_LEDMATRIX    ((volatile unsigned short *)0xFEC01000) /* 6x16 array of 16-bit words: ARGB */
#define KBD_MO_LED_ROWS     6
#define KBD_MO_LED_COLUMNS  16
#define KBD_MO_DATA         ((volatile unsigned int *)0xFEC00040)   /* Data register for the keyboard (scan codes will be here) */
#define KBD_MO_EMPTY        0x8000                                  /* Status flag that will be set if the keyboard buffer is empty */
#define KBD_MO_FULL         0x4000                                  /* Status flag that will be set if the keyboard buffer is full */
/*
 * Modifier bit flags
 */
#define KBD_LOCK_SCROLL     0x01
#define KBD_LOCK_NUM        0x02
#define KBD_LOCK_CAPS       0x04
#define KBD_MOD_SHIFT       0x08
#define KBD_MOD_ALT         0x10
#define KBD_MOD_CTRL        0x20
#define KBD_MOD_OS          0x40
#define KBD_MOD_MENU        0x80
#define KBD_STAT_BREAK      0x80        /* BREAK has been pressed recently */

#define isdigit(c)		((c) >= '0' && (c) <= '9')

unsigned char ps2kbdState = 0;
unsigned char Cmd;
unsigned char Parameter;

/*
 * Driver global variables
 */

static short kbdmo_leds = 0;
static unsigned char g_kbdmo_break_sc = 0x2E;   // Scancode for the BREAK key (must be pressed with CTRL)

/*
A.I. Created Conversion table from Scan Code Set 1 to Scan Code Set 2
*/
const uint8_t scancode_set1_to_set2_table[128] = {
    /* 0x00 - No key */                      0x00,
    /* 0x01 - Esc */                         0x76,
    /* 0x02 - 1 */                           0x16,
    /* 0x03 - 2 */                           0x1e,
    /* 0x04 - 3 */                           0x26,
    /* 0x05 - 4 */                           0x25,
    /* 0x06 - 5 */                           0x2E,
    /* 0x07 - 6 */                           0x36,
    /* 0x08 - 7 */                           0x3D,
    /* 0x09 - 8 */                           0x3E,
    /* 0x0A - 9 */                           0x46,
    /* 0x0B - 0 */                           0x45,
    /* 0x0C - Minus */                       0x4E,
    /* 0x0D - Equals */                      0x55,
    /* 0x0E - Backspace */                   0x66,
    /* 0x0F - Tab */                         0x0d,
    /* 0x10 - Q */                           0x15,
    /* 0x11 - W */                           0x1d,
    /* 0x12 - E */                           0x24,
    /* 0x13 - R */                           0x2d,
    /* 0x14 - T */                           0x2c,
    /* 0x15 - Y */                           0x35,
    /* 0x16 - U */                           0x3c,
    /* 0x17 - I */                           0x43,
    /* 0x18 - O */                           0x44,
    /* 0x19 - P */                           0x4d,
    /* 0x1A - Left Bracket */                0x54,
    /* 0x1B - Right Bracket */               0x5b,
    /* 0x1C - Enter */                       0x5a,
    /* 0x1D - Left Ctrl */                   0x14,
    /* 0x1E - A */                           0x1c,
    /* 0x1F - S */                           0x1b,
    /* 0x20 - D */                           0x23,
    /* 0x21 - F */                           0x2b,
    /* 0x22 - G */                           0x34,
    /* 0x23 - H */                           0x33,
    /* 0x24 - J */                           0x3b,
    /* 0x25 - K */                           0x42,
    /* 0x26 - L */                           0x4b,
    /* 0x27 - Semicolon */                   0x4c,
    /* 0x28 - Apostrophe */                  0x52,
    /* 0x29 - Grave/Tilde */                 0x0e,
    /* 0x2A - Left Shift */                  0x12,
    /* 0x2B - Backslash */                   0x5d,
    /* 0x2C - Z */                           0x1a,
    /* 0x2D - X */                           0x22,
    /* 0x2E - C */                           0x21,
    /* 0x2F - V */                           0x2A,
    /* 0x30 - B */                           0x32,
    /* 0x31 - N */                           0x31,
    /* 0x32 - M */                           0x3a,
    /* 0x33 - Comma */                       0x41,
    /* 0x34 - Period */                      0x49,
    /* 0x35 - Slash */                       0x4A,
    /* 0x36 - Right Shift */                 0x59, // Note: Set 2 Right Shift is 0x59
    /* 0x37 - Keypad * (Print Screen on E0)*/0x7C, // Note: Handled differently for Print Screen
    /* 0x38 - Left Alt */                    0x11,
    /* 0x39 - Space */                       0x29,
    /* 0x3A - Caps Lock */                   0x58,
    /* 0x3B - F1 */                          0x05,  // 0x05
    /* 0x3C - F2 */                          0x06,  // 0x06
    /* 0x3D - F3 */                          0x04,  // 0x07
    /* 0x3E - F4 */                          0x0c,  // 0x0C
    /* 0x3F - F5 */                          0x03,  // 0x03
    /* 0x40 - F6 */                          0x0b,  // 0x0B
    /* 0x41 - F7 */                          0x83,  // Note: 0x83
    /* 0x42 - F8 */                          0x0a,
    /* 0x43 - F9 */                          0x01,
    /* 0x44 - F10 */                         0x09,
    /* 0x45 - Num Lock */                    0x77,
    /* 0x46 - Scroll Lock */                 0x7e,
    /* 0x47 - Keypad 7 (Home) */             0x6c,
    /* 0x48 - Keypad 8 (Up Arrow) */         0x75,
    /* 0x49 - Keypad 9 (Page Up) */          0x7d,
    /* 0x4A - Keypad - */                    0x7b,
    /* 0x4B - Keypad 4 (Left Arrow) */       0x6b,
    /* 0x4C - Keypad 5 */                    0x73,
    /* 0x4D - Keypad 6 (Right Arrow) */      0x74,
    /* 0x4E - Keypad + */                    0x79,
    /* 0x4F - Keypad 1 (End) */              0x69,
    /* 0x50 - Keypad 2 (Down Arrow) */       0x72,
    /* 0x51 - Keypad 3 (Page Down) */        0x7a,
    /* 0x52 - Keypad 0 (Insert) */           0x70,
    /* 0x53 - Keypad . (Delete) */           0x71,
    /* 0x54  */                              0x00, 
    /* 0x55 */                               0x00,
    /* 0x56 */                               0x00,
    /* 0x57 - F11 */                         0x78,
    /* 0x58 - F12 */                         0x07,
    /* 0x59 -  */                            0x00,
    /* 0x5A - */                             0x00,
    /* 0x5B - */                             0x00,
    /* 0x5C - */                             0x00,
    /* 0x5D - */                             0x00,
    /* 0x5E - */                             0x00,
    /* 0x5F - */                             0x00,
    /* 0x60 - */                             0x00,
    /* 0x61 - */                             0x00,
    /* 0x62 - */                             0x00,
    /* 0x62 - */                             0x00,
};

#endif 


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
    if (ps2_wait_in()) 
        return -1;
    *PS2_CMD_BUF = cmd;

    if (ps2_wait_out()) 
        return -1;

    return (short)*PS2_DATA_BUF;
}

/*
 * Send a command with parameter to the controller and wait for a response.
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_controller_cmd_param(unsigned char cmd, unsigned char parameter) {
    if (ps2_wait_in()) 
        return -1;
    *PS2_CMD_BUF = cmd;

    if (ps2_wait_in()) 
        return -1;
    *PS2_DATA_BUF = parameter;

    return (short)*PS2_DATA_BUF;
}

/*
 * Send a command with parameter to the keyboard and wait for a response.
 *
 * Returns:
 *  The response from the PS/2 controller, -1 if there was a timeout
 */
short ps2_kbd_cmd_p(unsigned char cmd, unsigned char parameter) {
    if (ps2_wait_in()) 
        return -1;
    *PS2_DATA_BUF = cmd;

    // May need a delay here

    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = parameter;

    if (ps2_wait_out()) 
        return -1;
    return (short)*PS2_DATA_BUF;
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
short __attribute__((optimize("O0"))) ps2_kbd_cmd(unsigned char cmd, int delay) {
    volatile int loop;
    loop = delay;
    
    if (ps2_wait_in()) return -1;
    *PS2_DATA_BUF = cmd;

    // A delay may be needed here
    while (loop-- > 0) {
        asm volatile("NOP");
    }

    if (ps2_wait_out()) 
        return -1;
    return (short)*PS2_DATA_BUF;
}

/*
 * Read from the PS/2 data port until there are no more bytes ready.
 */
unsigned char ps2_flush_out() {
    unsigned char x = 0;

    while (*PS2_STATUS & PS2_STAT_OBF) {
        x = *PS2_DATA_BUF;
    }
    return x;
}

/*
 * Send a command to the controller and wait for a response.
 *
 * Returns: 0 on success, -1 on timeout
 */
short ps2_controller_cmd_noreply(unsigned char cmd) {
    if (ps2_wait_in()) {
		kprintf("ps2_controller_cmd_noreply: ps2_wait_in timeout\n");
		return -1;
	}
    *PS2_CMD_BUF = cmd;
    return 0;
}

short ps2_init() {
    unsigned short status;
    // Disable the PS/2 interrupts...

    int_disable(INT_MSE_PS2);     /* Disable mouse interrupts */
    int_disable(INT_KBD_PS2);   /* Disable keyboar interrupts */

    // Prevent the keyboard and mouse from sending events
    ps2_controller_cmd_noreply(PS2_CTRL_DISABLE_1);
    ps2_controller_cmd_noreply(PS2_CTRL_DISABLE_2);

    // Read and clear out the controller's output buffer
    ps2_flush_out();

    // // Controller selftest...
    status = ps2_controller_cmd(PS2_CTRL_SELFTEST);
    kprintf("Test PS/2 Controller CMD(0xAA): %x\n", status);
    if (status != PS2_RESP_OK) { // 0xAA
        kprintf("PS/2: FAILED controller self test.\n"); // return PS2_FAIL_SELFTEST;
    }

    status = ps2_controller_cmd(PS2_CTRL_KBDTEST);
    kprintf("Test PS/2 Keyboard CMD(0xAB): %x\n", status);    
    // Keyboard test
    if (status != 0) {
        kprintf("PS/2: FAILED port #1 test.\n"); // return PS2_FAIL_KBDTEST;
    }

    /*
    bit Definition
    0	First PS/2 port interrupt (1 = enabled, 0 = disabled)
    1	Second PS/2 port interrupt (1 = enabled, 0 = disabled, only if 2 PS/2 ports supported)
    2	System Flag (1 = system passed POST, 0 = your OS shouldn't be running)
    3	Should be zero
    4	First PS/2 port clock (1 = disabled, 0 = enabled)
    5	Second PS/2 port clock (1 = disabled, 0 = enabled, only if 2 PS/2 ports supported)
    6	First PS/2 port translation (1 = enabled, 0 = disabled)
    7	Must be zero    
    */
    // disable forced translation to scancode 1 and enable interrupts for m&k
    // Write Config Byte
    status = ps2_controller_cmd_param(PS2_CTRL_WRITECMD, 0x03);  /* %00000011 */
    kprintf("PS2 Write CTRL CMD(0x60)\n");

    status = ps2_controller_cmd(PS2_CTRL_READCMD);  // Read Config Byte
    kprintf("PS2 Write CTRL CMD(0x20) Response 0: %x\n", status);

    // Enable the keyboard, don't check response
	ps2_controller_cmd_noreply(PS2_CTRL_ENABLE_1);
	kprintf("PS/2: port #1 enabled.\n");

    // Reset the keyboard... waiting a bit before we check for a result
    status = ps2_kbd_cmd(KBD_CMD_RESET, 200000);
	kprintf("PS2 Write KBD CMD(0xFF) Response 0: %x\n", status); //*PS2_DATA_BUF
    if (ps2_wait_out()) 
        kprintf("PS2 Write KBD CMD(0xFF) Response 1: Timeout\n");
    else
        kprintf("PS2 Write KBD CMD(0xFF) Response 1: %x\n", *PS2_DATA_BUF);
    // Ideally, we would attempt a re-enable several times, but this doesn't work on the U/U+ for some keyboards
    // Make sure everything is read
    ps2_flush_out();
    status = 0;
    while ( status != 0x00FA ) {
        status = ps2_kbd_cmd(KBD_CMD_ENABLE, 0);
    }
	kprintf("PS2 Write CMD(0xF4) %x\n", status);    

    // Make sure everything is read
    ps2_flush_out();

    // set scancode page 2
    status = 0x0000;
    while ( status != 0x00FA ) {
        status = ps2_kbd_cmd_p(KBD_CMD_SCANCODESET, 0x02);
    }
    kprintf("PS2 Write CMD(0xF0) Scan Code Set 2: %x\n", status); //*PS2_DATA_BUF

    // Make sure everything is read
    ps2_flush_out();

    // Clear any pending keyboard interrupts
    int_clear(INT_KBD_PS2);
   // Enable the keyboard interrupt
    int_enable(INT_KBD_PS2);
    kprintf("Interrupt Pending Group1: %x\n", PENDING_GRP0[1]);
    kprintf("Interrupt Mask Group1: %x\n", MASK_GRP0[1]);
	kprintf("PS2 Keyboard interrupt enabled.\n\n");
    ps2busy = 0;
    return 1;   // keyboard present
}


void ps2_int(void)
{
    unsigned char x = *PS2_DATA_BUF;
    kprintf("ps2_interrupts: %x\n", x);
    int_clear(INT_KBD_PS2);

    if (x == 0xFA) {        // mystery stray 0xFA on A2560X kbd after irq enable
        return;
    }
    //kprintf("ps2_interrupts: %x\n", x);
    ps2kbd_byte(x);
}

int ps2kbd_put(uint_fast8_t c)
{
	if ( A2560K_KBD ) {

        //kprintf("Value of C & State: %x, %x\n", c, ps2kbdState);
        switch ( ps2kbdState ) {

            // Capture Command
            case 0:
                Cmd = c;
                // Parse the Command
                switch ( c ) {
                    case 0xED: ps2kbdState = 1; break; // Set the LED Next byte is a parameter
                    case 0xF0: ps2kbdState = 1; break; // We need to capture the Scan Set # - 1 Parameter
                    case 0xF4: ps2kbdState = 0; break; // 0 Parameter
                    case 0xF6: ps2kbdState = 0; break; // 0 Parameter
                    case 0xFF: ps2kbdState = 0; break; // 0 Parameter
                    default: ps2kbdState = 0; break;
                }
                //kprintf("StateMachine0: %x, %x, %x\n", Cmd, Parameter, ps2kbdState);
            break;              //Set Led - 1 Parameter

            // Process the Command Parameter
            case 1: 
                Parameter = c;
                //kprintf("StateMachine1: %x, %x, %x\n", Cmd, Parameter, ps2kbdState);
                if ( Cmd == 0xED ) {
                    if (Parameter & 4) 
                        kbdmo_set_caps_led(0x5);
                    else
                        kbdmo_set_caps_led(0x0);
                    Parameter = 0;
                    Cmd = 0;
                }
        
                if ( Cmd == 0xF0 ) {
                    Parameter = 0;
                    Cmd = 0;
                }
            ps2kbdState = 0;
            break;      //Get or Set Scan Code 

            default: 
                ps2kbdState = 0; 
            break;
        }
    }
    else {
        if (ps2_wait_in() == 0) {
            *PS2_STATUS = c;
            return 0;
        } else {
            kprintf("\nps2: put timeout\n");
            return 0xFFFF;
        }
    }
}

uint16_t ps2kbd_get(void)
{
   	if ( A2560K_KBD ) {
        if ( Cmd == 0xED )
            return (0x00FA);
        if ( Cmd == 0xF0 )
            return (0x00FA);
        if ( Cmd == 0xF0 && Parameter == 0x02) 
            return (0x00FA);
        if ( Cmd == 0xF4 )
            return (0x00FA);            
        if ( Cmd == 0xF6 )
            return (0x00FA);
        if ( Cmd == 0xFF )
            return (0x00FA);            
    }
    else {
        if (ps2_wait_out() == 0) {
            return *PS2_DATA_BUF;
        } else {
            return PS2_NOCHAR;
        }
    }
}


///////////////////////////////////////////////////////////////////////
////////////// MAU's Section
///////////////////////////////////////////////////////////////////////
void kbdmo_flush_out() {
    long data;
    /* While there is data in the buffer ... */
    do {
        data = *KBD_MO_DATA;
    } while ((data & 0x00ff0000) != 0);
}


// Capslock Set 2 - (pressed 0x58) - released (0xf0 0x58)

// Interrupt Response
void mau_int(void)
{
    unsigned long data;
    unsigned char Code;
    unsigned char Converted;
    /* We got an interrupt for MO.
     * While there is data in the input queue...
     */

    int_clear(INT_KBD_A2560K);
    /* While there is data in the buffer ... */
    do {
        // data[31] - FIFO Empty (0 when not empty)
        // data[30] - FIFO Full  (0 when not full)
        // data[23:16] - FIFO Size
        // data[15:0] - Scan Code
        data = *KBD_MO_DATA;
        /* Read and throw out the scan codes */
        unsigned short scan_code = data & 0xffff;
        if ((scan_code & 0x7fff) != 0) {
            Code = (unsigned char) scan_code & 0xff;
            Converted = scancode_set1_to_set2_table[ Code & 0x7F ]; // Convert From Scan Code set 1 to Set 2
            if ( Code & 0x80 ) {
            // Release Key
            // Normally the PS2 would return 2 bytes, $F0, $xx 
                ps2kbd_byte(0xF0);
                //kprintf("Scan Code?: %x, %x\n", Code, Converted);
                ps2kbd_byte(Converted);
            }
            else {
            // Pressed Key, in this case it is only 1 byte
            ps2kbd_byte(Converted);
            }
        }
    } while ((data & 0x00ff0000) != 0);

}

// 
int mau_kbd_put(uint_fast8_t c)
{
    if (ps2_wait_in() == 0) {
        *PS2_STATUS = c;
        return 0;
    } else {
        kprintf("\nps2: put timeout\n");
        return 0xFFFF;
    }
}

uint16_t mau_kbd_get(void)
{
    if (ps2_wait_out() == 0) {
        return *PS2_DATA_BUF;
    } else {
        return PS2_NOCHAR;
    }
}



// A2560Kxx Keyboard Initialization
short mau_init(void) {
    kprintf("A2560Kxx Keyboard Init\n");

    int_disable(INT_KBD_A2560K);

    /* Turn off the LEDs */
    kbdmo_set_led_matrix_fill(0xf);

    /* Make sure everything is read */
    kbdmo_flush_out();

    /* Turn off the LEDs */
    kbdmo_leds = 0;
    *GABE_MO_LEDS = kbdmo_leds;

    /* Clear out any pending interrupt */
    int_clear(INT_KBD_A2560K);
    /* Enable the interrupt for the keyboard */
    int_enable(INT_KBD_A2560K);
    kprintf("Interrupt Pending Group1: %x\n", PENDING_GRP0[1]);
    kprintf("Interrupt Mask Group1: %x\n", MASK_GRP0[1]);

    kprintf("A2560Kxx Keyboard Init Done\n\n");
    ps2busy = 0;    
    return 1;
}

/**
 * Set the color of the A2560K keyboard LED matrix
 *
 * @param row the number of the row to set (0 - 5)
 * @param color the color for the LEDs: ARGB
 */
void kbdmo_set_led_matrix_row(unsigned char row, unsigned short color) {
    int column;
    for (column = 0; column < KBD_MO_LED_COLUMNS; column++) {
        KBD_MO_LEDMATRIX[row * KBD_MO_LED_COLUMNS + column] = color;
    }
}

/**
 * Set all the LEDs to the same color
 *
 * @param color the color for the LEDs: ARGB
 */
void kbdmo_set_led_matrix_fill(unsigned short color) {
    unsigned char row;
    for (row = 0; row < KBD_MO_LED_ROWS; row++) {
        kbdmo_set_led_matrix_row(row, color);
    }
}


/*
 * Set the color of the LED for the capslock
 *
 * Inputs:
 * colors = color specification, three bits: 0x_____RGB
 */
void kbdmo_set_caps_led(short colors) {
    kbdmo_leds = (kbdmo_leds & 0xF1FF) | ((colors & 0x07) << 9);
    *GABE_MO_LEDS = kbdmo_leds;
}

/*
 * Set the color of the LED for the floppy drive
 *
 * Inputs:
 * colors = color specification, three bits: 0x_____RGB
 */
void kbdmo_set_fdc_led(short colors) {
    kbdmo_leds = (kbdmo_leds & 0xFFF8) | (colors & 0x07);
    *GABE_MO_LEDS = kbdmo_leds;
}

/*
 * Set the color of the LED for the SD card slot
 *
 * Inputs:
 * colors = color specification, three bits: 0x_____RGB
 */
void kbdmo_set_sdc_led(short colors) {
    kbdmo_leds = (kbdmo_leds & 0xFFC7) | ((colors & 0x07) << 3);
    *GABE_MO_LEDS = kbdmo_leds;
}

/*
 * Set the color of the LED for the IDE hard drive
 *
 * Inputs:
 * colors = color specification, three bits: 0x_____RGB
 */
void kbdmo_set_hdc_led(short colors)  {
    kbdmo_leds = (kbdmo_leds & 0xFE3F) | ((colors & 0x07) << 6);
    *GABE_MO_LEDS = kbdmo_leds;
}


