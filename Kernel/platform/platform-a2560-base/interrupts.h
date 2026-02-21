
#ifndef __INTERRUPTS_DOT_H__
#define __INTERRUPTS_DOT_H__

unsigned short int_mask(unsigned short n);
unsigned short int_group(unsigned short n);
void int_clear(unsigned short n);
void int_enable(unsigned short n);
void int_disable(unsigned short n);

#endif

