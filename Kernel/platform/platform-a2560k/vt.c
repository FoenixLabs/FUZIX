
#include <kernel.h>
#include <tty.h>
#include <vt.h>
#include "A2560K/vky_chan_b.h"

uint8_t vtattr_cap = VTA_INVERSE;

static unsigned char *cpos;
static unsigned char csave;
static unsigned char color;

static uint8_t *char_addr(unsigned int y1, unsigned char x1)
{
	return VKY3_B_TEXT_MATRIX + VT_WIDTH * y1 + (uint16_t)x1;
}

static uint8_t *color_addr(unsigned int y1, unsigned char x1)
{
	return VKY3_B_COLOR_MATRIX + VT_WIDTH * y1 + (uint16_t)x1;
}

void cursor_off(void)
{
}

/* Only needed for hardware cursors */
void cursor_disable(void)
{
}

void cursor_on(int8_t y, int8_t x)
{
	*VKY3_B_CPR = ((y & 0xffff) << 16) | (x & 0xffff);
}

void plot_char(int8_t y, int8_t x, uint16_t c)
{
	*char_addr(y, x) = VT_MAP_CHAR(c);
    *color_addr(y, x) = color;
}

void clear_lines(int8_t y, int8_t ct)
{
	unsigned char *s = char_addr(y, 0);
    unsigned char *c = color_addr(y, 0);
	memset(s, VT_MAP_CHAR(' '), ct * VT_WIDTH);
    memset(c, color, ct * VT_WIDTH);
}

void clear_across(int8_t y, int8_t x, int16_t l)
{
	unsigned char *s = char_addr(y, x);
    unsigned char *c = color_addr(y, x);
	memset(s, VT_MAP_CHAR(' '), l);
    memset(c, color, l);
}

void vtattr_notify(void)
{
    if (vtattr & VTA_INVERSE) {
        color = ((vtpaper & 0x0f) << 4) | (vtink & 0x0f);
    } else {
        color = ((vtink & 0x0f) << 4) | (vtpaper & 0x0f);
    }    
}

void scroll_up(void)
{
	memmove(VKY3_B_TEXT_MATRIX, VKY3_B_TEXT_MATRIX + VT_WIDTH, VT_WIDTH * VT_BOTTOM);
    memmove(VKY3_B_COLOR_MATRIX, VKY3_B_COLOR_MATRIX + VT_WIDTH, VT_WIDTH * VT_BOTTOM);
}

void scroll_down(void)
{
	memmove(VKY3_B_TEXT_MATRIX + VT_WIDTH, VKY3_B_TEXT_MATRIX, VT_WIDTH * VT_BOTTOM);
    memmove(VKY3_B_COLOR_MATRIX + VT_WIDTH, VKY3_B_COLOR_MATRIX, VT_WIDTH * VT_BOTTOM);
}

// eof
