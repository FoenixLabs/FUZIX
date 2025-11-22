#ifndef __PS2_DOT_H__
#define __PS2_DOT_H__

short mau_init(void);
void kbdmo_set_led_matrix_row(unsigned char row, unsigned short color);
void kbdmo_set_led_matrix_fill(unsigned short color);
void kbdmo_set_caps_led(short colors);
void kbdmo_set_fdc_led(short colors);
void kbdmo_set_sdc_led(short colors);
void kbdmo_set_hdc_led(short colors);



#endif