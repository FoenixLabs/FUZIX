
#include <kernel.h>
#include <tty.h>
#include <vt.h>
#include "A2560K/vky_chan_b.h"

#if defined(CONFIG_TSM)

#define VT_MAP_CHAR(x) (unsigned char)(x)

static uint8_t *char_addr(unsigned int y1, unsigned char x1)
{
	return VKY3_B_TEXT_MATRIX + VT_WIDTH * y1 + (uint16_t)x1;
}

static uint8_t *color_addr(unsigned int y1, unsigned char x1)
{
	return VKY3_B_COLOR_MATRIX + VT_WIDTH * y1 + (uint16_t)x1;
}


void cursor_move(int8_t y, int8_t x)
{
	*VKY3_B_CPR = ((y & 0xffff) << 16) | (x & 0xffff);
}


int vt_screen_draw_cb (
    struct tsm_screen *con,
    uint32_t id,
    const uint32_t *ch,
    size_t len,
    unsigned int width,
    unsigned int posx,
    unsigned int posy,
    const struct tsm_screen_attr *attr,
    tsm_age_t age,
    void *data)
{
    static unsigned char color;

    if (attr->inverse) {
        color = ((attr->bccode & 0x0f) << 4) | (attr->fccode & 0x0f);
    } else {
        color = ((attr->fccode & 0x0f) << 4) | (attr->bccode  & 0x0f);
    }

    for(int i=0; i < len; i++) {
        *char_addr(posy + i, posx) = VT_MAP_CHAR(ch[i]);
        *color_addr(posy + i, posx) = color;
    }

    return 0;


// struct tsm_screen_attr {
// 	int8_t fccode;			/* foreground color code or <0 for rgb */
// 	int8_t bccode;			/* background color code or <0 for rgb */
// 	uint8_t fr;			/* foreground red */
// 	uint8_t fg;			/* foreground green */
// 	uint8_t fb;			/* foreground blue */
// 	uint8_t br;			/* background red */
// 	uint8_t bg;			/* background green */
// 	uint8_t bb;			/* background blue */
// 	unsigned int bold : 1;		/* bold character */
// 	unsigned int italic : 1;	/* italics character */
// 	unsigned int underline : 1;	/* underlined character */
// 	unsigned int inverse : 1;	/* inverse colors */
// 	unsigned int protect : 1;	/* cannot be erased */
// 	unsigned int blink : 1;		/* blinking character */
//};

}

#else

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
#endif
// eof
