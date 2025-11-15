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

/*
 * Structure to track the keyboard input
 */
struct s_kdbmo_kbd {
    unsigned char control;      /* Bits to control how the keyboard processes things */
    unsigned char status;       /* Status of the keyboard */
    t_word_ring sc_buf;         /* Buffer containing scancodes that have been processed */
    t_word_ring char_buf;       /* Buffer containing characters to be read */
    unsigned char modifiers;    /* State of the modifier keys (CTRL, ALT, SHIFT) and caps lock */

    /* Scan code to character lookup tables */

    char keys_unmodified[128];
    char keys_shift[128];
    char keys_control[128];
    char keys_control_shift[128];
    char keys_caps[128];
    char keys_caps_shift[128];
    char keys_alt[128];
};

/*
 * Driver global variables
 */

struct s_kdbmo_kbd g_kbdmo_control;
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


/*
 * Mapping of "codepoints" 0x80 - 0x98 (function keys, etc)
 * to ANSI escape codes
 */
const char * ansi_keys[] = {
    "1",    /* HOME */
    "2",    /* INS */
    "3",    /* DELETE */
    "4",    /* END */
    "5",    /* PgUp */
    "6",    /* PgDn */
    "A",    /* Up */
    "B",    /* Left */
    "C",    /* Right */
    "D",    /* Down */
    "11",   /* F1 */
    "12",   /* F2 */
    "13",   /* F3 */
    "14",   /* F4 */
    "15",   /* F5 */
    "17",   /* F6 */
    "18",   /* F7 */
    "19",   /* F8 */
    "20",   /* F9 */
    "21",   /* F10 */
    "23",   /* F11 */
    "24",   /* F12 */
    "30",   /* MONITOR */
    "31",   /* CTX SWITCH */
    "32"    /* MENU HELP */
};

/*
 * US keyboard layout scancode translation tables
 */
const char g_us_kbdmo_layout[] = {
    // Unmodified
    0x00, 0x1B, '1', '2', '3', '4', '5', '6',           /* 0x00 - 0x07 */
    '7', '8', '9', '0', '-', '=', 0x08, 0x09,           /* 0x08 - 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',             /* 0x10 - 0x17 */
    'o', 'p', '[', ']', 0x0D, 0x00, 'a', 's',           /* 0x18 - 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',             /* 0x20 - 0x27 */
    0x27, '`', 0x00, '\\', 'z', 'x', 'c', 'v',          /* 0x28 - 0x2F */
    'b', 'n', 'm', ',', '.', '/', 0x00, '*',            /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, 0x80,     /* 0x40 - 0x47 */
    0x86, 0x84, '-', 0x89, '5', 0x88, '+', 0x83,        /* 0x48 - 0x4F */
    0x87, 0x85, 0x81, 0x82, 0x96, 0x97, 0x98, 0x94,     /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */

    // Shifted
    0x00, 0x1B, '!', '@', '#', '$', '%', '^',           /* 0x00 - 0x07 */
    '&', '*', '(', ')', '_', '+', 0x08, 0x09,           /* 0x08 - 0x0F */
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',             /* 0x10 - 0x17 */
    'O', 'P', '{', '}', 0x0A, 0x00, 'A', 'S',           /* 0x18 - 0x1F */
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',             /* 0x20 - 0x27 */
    0x22, '~', 0x00, '|', 'Z', 'X', 'C', 'V',           /* 0x28 - 0x2F */
    'B', 'N', 'M', '<', '>', '?', 0x00, 0x00,           /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, 0x80,     /* 0x40 - 0x47 */
    0x86, 0x84, '-', 0x89, '5', 0x88, '+', 0x83,        /* 0x48 - 0x4F */
    0x87, 0x85, 0x81, 0x82, 0x96, 0x97, 0x98, 0x94,     /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */

    // Control
    0x00, 0x1B, '1', '2', '3', '4', '5', 0x1E,          /* 0x00 - 0x07 */
    '7', '8', '9', '0', 0x1F, '=', 0x08, 0x09,          /* 0x08 - 0x0F */
    0x11, 0x17, 0x05, 0x12, 0x14, 0x19, 0x15, 0x09,     /* 0x10 - 0x17 */
    0x0F, 0x10, 0x1B, 0x1D, 0x0A, 0x00, 0x01, 0x13,     /* 0x18 - 0x1F */
    0x04, 0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C, ';',      /* 0x20 - 0x27 */
    0x22, '`', 0x00, '\\', 0x1A, 0x18, 0x03, 0x16,      /* 0x28 - 0x2F */
    0x02, 0x0E, 0x0D, ',', '.', 0x1C, 0x00, 0x00,       /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, 0x80,     /* 0x40 - 0x47 */
    0x86, 0x84, '-', 0x89, '5', 0x88, '+', 0x83,        /* 0x48 - 0x4F */
    0x87, 0x85, 0x81, 0x82, 0x96, 0x97, 0x98, 0x94,     /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */


    // Control-Shift
    0x00, 0x1B, '!', '@', '#', '$', '%', '^',           /* 0x00 - 0x07 */
    '&', '*', '(', ')', '_', '+', 0x08, 0x09,           /* 0x08 - 0x0F */
    0x11, 0x17, 0x05, 0x12, 0x14, 0x19, 0x15, 0x09,     /* 0x10 - 0x17 */
    0x0F, 0x10, 0x1B, 0x1D, 0x0A, 0x00, 0x01, 0x13,     /* 0x18 - 0x1F */
    0x04, 0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C, ';',      /* 0x20 - 0x27 */
    0x22, '`', 0x00, '\\', 0x1A, 0x18, 0x03, 0x16,      /* 0x28 - 0x2F */
    0x02, 0x0E, 0x0D, ',', '.', 0x1C, 0x00, 0x00,       /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, 0x80,     /* 0x40 - 0x47 */
    0x86, 0x84, '-', 0x89, '5', 0x88, '+', 0x83,        /* 0x48 - 0x4F */
    0x87, 0x85, 0x81, 0x82, 0x96, 0x97, 0x98, 0x94,     /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */

    // Capslock
    0x00, 0x1B, '1', '2', '3', '4', '5', '6',           /* 0x00 - 0x07 */
    '7', '8', '9', '0', '-', '=', 0x08, 0x09,           /* 0x08 - 0x0F */
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',             /* 0x10 - 0x17 */
    'O', 'P', '[', ']', 0x0D, 0x00, 'A', 'S',           /* 0x18 - 0x1F */
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',             /* 0x20 - 0x27 */
    0x27, '`', 0x00, '\\', 'Z', 'X', 'C', 'V',          /* 0x28 - 0x2F */
    'B', 'N', 'M', ',', '.', '/', 0x00, 0x00,           /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, '7',      /* 0x40 - 0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',             /* 0x48 - 0x4F */
    '2', '3', '0', '.', 0x96, 0x97, 0x98, 0x94,         /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */

    // Caps-Shift
    0x00, 0x1B, '!', '@', '#', '$', '%', '^',           /* 0x00 - 0x07 */
    '&', '*', '(', ')', '_', '+', 0x08, 0x09,           /* 0x08 - 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',             /* 0x10 - 0x17 */
    'o', 'p', '{', '}', 0x0A, 0x00, 'a', 's',           /* 0x18 - 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ':',             /* 0x20 - 0x27 */
    0x22, '~', 0x00, '|', 'z', 'x', 'c', 'v',           /* 0x28 - 0x2F */
    'b', 'n', 'm', '<', '>', '?', 0x00, 0x00,           /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, '7',      /* 0x40 - 0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',             /* 0x48 - 0x4F */
    '2', '3', '0', '.', 0x96, 0x97, 0x98,  0x94,         /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */

    // ALT
    0x00, 0x1B, '1', '2', '3', '4', '5', '6',           /* 0x00 - 0x07 */
    '7', '8', '9', '0', '-', '=', 0x08, 0x09,           /* 0x08 - 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',             /* 0x10 - 0x17 */
    'o', 'p', '[', ']', 0x0D, 0x00, 'a', 's',           /* 0x18 - 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',             /* 0x20 - 0x27 */
    0x27, '`', 0x00, '\\', 'z', 'x', 'c', 'v',          /* 0x28 - 0x2F */
    'b', 'n', 'm', ',', '.', '/', 0x00, '*',            /* 0x30 - 0x37 */
    0x00, ' ', 0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,      /* 0x38 - 0x3F */
    0x8F, 0x90, 0x91, 0x92, 0x93, 0x00, 0x00, 0x80,     /* 0x40 - 0x47 */
    0x86, 0x84, '-', 0x89, '5', 0x88, '+', 0x83,        /* 0x48 - 0x4F */
    0x87, 0x85, 0x81, 0x82, 0x96, 0x97, 0x98, 0x94,     /* 0x50 - 0x57 */
    0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x58 - 0x5F */
    0x00, 0x00, 0x81, 0x80, 0x84, 0x82, 0x83, 0x85,     /* 0x60 - 0x67 */
    0x86, 0x89, 0x87, 0x88, '/', 0x0D, 0x00, 0x00,      /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0x78 - 0x7F */
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
    kprintf("ps2kbd_get:\n");    
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
    kprintf("ps2kbd_get:\n");
    if (ps2_wait_out() == 0) {
        return *PS2_DATA_BUF;
    } else {
        return PS2_NOCHAR;
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

    rb_word_init(&g_kbdmo_control.sc_buf);        /* Scan-code ring buffer is empty */
    rb_word_init(&g_kbdmo_control.char_buf);      /* Character ring buffer is empty */

    /* Set the default keyboard layout to US */
    kbdmo_layout(g_us_kbdmo_layout);

    g_kbdmo_control.status = 0;
    g_kbdmo_control.modifiers = 0;

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
 * Add the scan code to the queue of scan codes
 */
void kbdmo_enqueue_scan(unsigned char scan_code) {
    // Make sure the scan code isn't 0 or 128, which are invalid make/break codes
    if ((scan_code != 0) && (scan_code != 0x80)) {
        unsigned char is_break = scan_code & 0x80;

        // If CTRL-C pressed, treat it as a break
        if ((scan_code == g_kbdmo_break_sc) & ((g_kbdmo_control.modifiers & KBD_MOD_CTRL) != 0)) {
            g_kbdmo_control.status |= KBD_STAT_BREAK;
        }

        // Check the scan code to see if it's a modifier key or a lock key
        // update the modifier and lock variables accordingly...
        switch (scan_code & 0x7f) {
            case 0x01:
                /* ESC key pressed... check to see if it's a press with the Foenix key */
                if (((g_kbdmo_control.modifiers & KBD_MOD_OS) != 0) && (is_break == 0)) {
                    /* ESC pressed with Foenix key... flag a BREAK. */
                    g_kbdmo_control.status |= KBD_STAT_BREAK;
                }
                break;

            case 0x2A:
            case 0x36:
                kbdmo_makebreak_modifier(KBD_MOD_SHIFT, is_break);
                break;

            case 0x1D:
            case 0x5E:
                kbdmo_makebreak_modifier(KBD_MOD_CTRL, is_break);
                break;

            case 0x38:
            case 0x5C:
                kbdmo_makebreak_modifier(KBD_MOD_ALT, is_break);
                break;

            case 0x5D:
                kbdmo_makebreak_modifier(KBD_MOD_MENU, is_break);
                break;

            case 0x5B:
                kbdmo_makebreak_modifier(KBD_MOD_OS, is_break);
                break;

            case 0x3A:
                if (!is_break) kbdmo_toggle_modifier(KBD_LOCK_CAPS);
                break;

            case 0x45:
                if (!is_break) kbdmo_toggle_modifier(KBD_LOCK_NUM);
                break;

            case 0x46:
                if (!is_break) kbdmo_toggle_modifier(KBD_LOCK_SCROLL);
                break;

            default:
                break;
        }

        rb_word_put(&g_kbdmo_control.sc_buf, g_kbdmo_control.modifiers << 8 | scan_code);
    }
}

/*
 * Try to get a character from the keyboard...
 *
 * Returns:
 *      the next character to be read from the keyboard (0 if none available)
 */
unsigned char kbdmo_getc(void ) {
    if (!rb_word_empty(&g_kbdmo_control.char_buf)) {
        // If there is a character waiting in the character buffer, return it...
        return (char)rb_word_get(&g_kbdmo_control.char_buf);

    } else {
        // Otherwise, we need to check the scan code queue...
        unsigned short raw_code = kbdmo_get_scancode();
        while (raw_code != 0) {
            if ((raw_code & 0x80) == 0) {
                // If it's a make code, let's try to look it up...
                unsigned char modifiers = (raw_code >> 8) & 0xff;    // Get the modifiers
                unsigned char scan_code = raw_code & 0x7f;           // Get the base code for the key

                // Check the modifiers to see what we should lookup...

                if ((modifiers & (KBD_MOD_ALT | KBD_MOD_SHIFT | KBD_MOD_CTRL | KBD_LOCK_CAPS)) == 0) {
                    // No modifiers... just return the base character
                    return kbd_to_ansi(modifiers, g_kbdmo_control.keys_unmodified[scan_code]);

                } else if (modifiers & KBD_MOD_ALT) {
                    return kbd_to_ansi(modifiers, g_kbdmo_control.keys_alt[scan_code]);

                } else if (modifiers & KBD_MOD_CTRL) {
                    // If CTRL is pressed...
                    if (modifiers & KBD_MOD_SHIFT) {
                        // If SHIFT is also pressed, return CTRL-SHIFT form
                        return kbd_to_ansi(modifiers, g_kbdmo_control.keys_control_shift[scan_code]);

                    } else {
                        // Otherwise, return just CTRL form
                        return kbd_to_ansi(modifiers, g_kbdmo_control.keys_control[scan_code]);
                    }

                } else if (modifiers & KBD_LOCK_CAPS) {
                    // If CAPS is locked...
                    if (modifiers & KBD_MOD_SHIFT) {
                        // If SHIFT is also pressed, return CAPS-SHIFT form
                        return kbd_to_ansi(modifiers, g_kbdmo_control.keys_caps_shift[scan_code]);

                    } else {
                        // Otherwise, return just CAPS form
                        return kbd_to_ansi(modifiers, g_kbdmo_control.keys_caps[scan_code]);
                    }

                } else {
                    // SHIFT is pressed, return SHIFT form
                    return kbd_to_ansi(modifiers, g_kbdmo_control.keys_shift[scan_code]);
                }
            }

            // If we reach this point, it wasn't a useful scan-code...
            // So try to fetch another
            raw_code = kbdmo_get_scancode();
        }

        // If we reach this point, there are no useful scan codes
        return 0;
    }
}

/*
 * Try to retrieve the next scancode from the keyboard.
 *
 * Returns:
 *      The next scancode to be processed, 0 if nothing.
 */
unsigned short kbdmo_get_scancode(void) {
    unsigned short scan_code = rb_word_get(&g_kbdmo_control.sc_buf);
    if (scan_code != 0) {
        /* Got a result... return it */
        return scan_code;

    } else {
        return 0;
    }
}

/*
 * Catch special keys and convert them to their ANSI terminal codes
 *
 * Characters 0x80 - 0x98 are reserved for function keys, arrow keys, etc.
 * This function maps them to the ANSI escape codes
 *
 * Inputs:
 * modifiers = the current modifier bit flags (ALT, CTRL, META, etc)
 * c = the character found from the scan code.
 */
static unsigned char kbd_to_ansi(unsigned char modifiers, unsigned char c) {
    if ((c >= 0x80) && (c <= 0x98)) {
        /* The key is a function key or a special control key */
        const char * ansi_key = ansi_keys[c - 0x80];
        const char * sequence;
        short modifiers_after = 0;

        // Figure out if the modifiers come before or after the sequence code
        if (isdigit(ansi_key[0])) {
            // Sequence is numeric, modifiers come after
            modifiers_after = 1;
        }

        // After ESC, all sequences have [
        rb_word_put(&g_kbdmo_control.char_buf, '[');

        if (modifiers_after) {
            // Sequence is numberic, get the expanded sequence and put it in the queue
            for (sequence = ansi_keys[c - 0x80]; *sequence != 0; sequence++) {
                rb_word_put(&g_kbdmo_control.char_buf, *sequence);
            }
        }

        // Check to see if we need to send a modifier sequence
        if (modifiers & (KBD_MOD_SHIFT | KBD_MOD_CTRL | KBD_MOD_ALT | KBD_MOD_OS)) {
            unsigned char code_bcd;
            short modifier_code = 0;
            //short i;

            if (modifiers_after) {
                // Sequence is numeric, so put modifiers after the sequence and a semicolon
                rb_word_put(&g_kbdmo_control.char_buf, ';');
            }

            modifier_code = ((modifiers >> 3) & 0x1F) + 1;
            code_bcd = i_to_bcd(modifier_code);

            if (code_bcd & 0xF0) {
                rb_word_put(&g_kbdmo_control.char_buf, ((code_bcd & 0xF0) >> 4) + '0');
            }
            rb_word_put(&g_kbdmo_control.char_buf, (code_bcd & 0x0F) + '0');
        }

        if (!modifiers_after) {
            // Sequence is a letter code
            rb_word_put(&g_kbdmo_control.char_buf, ansi_key[0]);
        } else {
            // Sequence is numeric, close it with a tilda
            rb_word_put(&g_kbdmo_control.char_buf, '~');
        }

        return 0x1B;    /* Start the sequence with an escape */

    } else if (c == 0x1B) {
        /* ESC should be doubled, to distinguish from the start of an escape sequence */
        rb_word_put(&g_kbdmo_control.char_buf, 0x1B);
        return c;

    } else {
        /* Not a special key: return the character unmodified */

        return c;
    }
}


/*
 * Toggle the lock bit based on the flag.
 */
void kbdmo_toggle_modifier(short flag) {
    g_kbdmo_control.modifiers ^= flag;

    if (flag == KBD_LOCK_CAPS) {
        if (g_kbdmo_control.modifiers & flag) {
            /* CAPS is on... set it to purple */
            kbdmo_set_caps_led(5);

        } else {
            /* CAPS is off... turn off the LED */
            kbdmo_set_caps_led(0);
        }
    }
}

/*
 * Set or clear the modifier flag depending on if the scan code is a make or break code.
 */
void kbdmo_makebreak_modifier(short flag, short is_break) {
    if (is_break) {
        g_kbdmo_control.modifiers &= ~flag;
    } else {
        g_kbdmo_control.modifiers |= flag;
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


/*
 * Convert a number from 0 to 99 to BCD
 *
 * Inputs:
 * n = a binary number from 0 to 99
 *
 * Returns:
 * a byte containing n as a BCD number
 */
unsigned char i_to_bcd(unsigned short n) {
    if (n > 99) {
        /* Input was out of range... just return 0 */
        return 0;

    } else {
        unsigned short tens = n / 10;
        unsigned short ones = n - (tens * 10);

        return tens << 4 | ones;
    }
}

/*
 * Set the keyboard translation tables
 *
 * The translation tables provided to the keyboard consist of eight
 * consecutive tables of 128 characters each. Each table maps from
 * the MAKE scan code of a key to its appropriate 8-bit character code.
 *
 * The tables included must include, in order:
 * - UNMODIFIED: Used when no modifier keys are pressed or active
 * - SHIFT: Used when the SHIFT modifier is pressed
 * - CTRL: Used when the CTRL modifier is pressed
 * - CTRL-SHIFT: Used when both CTRL and SHIFT are pressed
 * - CAPSLOCK: Used when CAPSLOCK is down but SHIFT is not pressed
 * - CAPSLOCK-SHIFT: Used when CAPSLOCK is down and SHIFT is pressed
 * - ALT: Used when only ALT is presse
 * - ALT-SHIFT: Used when ALT is pressed and either CAPSLOCK is down
 *   or SHIFT is pressed (but not both)
 *
 * Inputs:
 * tables = pointer to the keyboard translation tables
 */
short kbdmo_layout(const char * tables) {
    short i;

    for (i = 0; i < 128; i++) {
        g_kbdmo_control.keys_unmodified[i] = tables[i];
        g_kbdmo_control.keys_shift[i] = tables[i + 128];
        g_kbdmo_control.keys_control[i] = tables[i + 256];
        if (g_kbdmo_control.keys_control[i] == 0x03) {
            // We have set the scan code for CTRL-C?
            g_kbdmo_break_sc = i;
        }
        // Check for CTRL-C
        g_kbdmo_control.keys_control_shift[i] = tables[i + 384];
        g_kbdmo_control.keys_caps[i] = tables[i + 512];
        g_kbdmo_control.keys_caps_shift[i] = tables[i + 640];
        g_kbdmo_control.keys_alt[i] = tables[i + 768];
    }

    return 0;
}