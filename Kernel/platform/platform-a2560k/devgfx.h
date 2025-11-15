#ifndef _DEV_GFX_H
#define _DEV_GFX_H

#include <graphics.h>

extern int gfx_ioctl(uint8_t minor, uarg_t arg, char *ptr);
extern uint16_t gfx_text_width();
extern uint16_t gfx_text_height();
extern uint16_t gfx_text_enabled();
extern uint16_t gfx_pixel_width();
extern uint16_t gfx_pixel_height();

#endif
