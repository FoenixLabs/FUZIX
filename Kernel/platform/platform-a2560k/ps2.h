#ifndef __PS2_DOT_H__
#define __PS2_DOT_H__

short mau_init(void);
void kbdmo_set_led_matrix_row(unsigned char row, unsigned short color);
void kbdmo_set_led_matrix_fill(unsigned short color);
void kbdmo_enqueue_scan(unsigned char scan_code);
void kbdmo_makebreak_modifier(short flag, short is_break);
void kbdmo_toggle_modifier(short flag);
void kbdmo_set_caps_led(short colors);
void kbdmo_set_fdc_led(short colors);
void kbdmo_set_sdc_led(short colors);
void kbdmo_set_hdc_led(short colors);
unsigned char kbdmo_getc(void );
unsigned short kbdmo_get_scancode(void);
static unsigned char kbd_to_ansi(unsigned char modifiers, unsigned char c);
unsigned char i_to_bcd(unsigned short n);
short kbdmo_layout(const char * tables);


#endif