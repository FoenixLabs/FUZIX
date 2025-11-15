
#include <kernel.h>
#include <tty.h>
#include <vt.h>
#include "mcp_types.h"

void txt_a2560k_b_set_xy(short x, short y);
void txt_a2560k_b_get_xy(short x, short y);
short txt_a2560k_b_get_region(p_rect region);
short txt_a2560k_b_set_region(p_rect region);
void txt_a2560k_b_fill(char c);
void txt_a2560k_b_put(char c);
void txt_a2560k_b_scroll(short horizontal, short vertical);

/* for begginging no inverse, no italic etc. */
uint8_t vtattr_cap = 0;

void cursor_off()
{
}

void cursor_disable()
{
}

void cursor_on(int8_t y, int8_t x)
{
    txt_a2560k_b_set_xy(x, y);
}

void clear_across(int8_t y, int8_t x, int16_t l)
{
    t_rect region, old_region;

    txt_a2560k_b_get_region(&old_region);
    region.origin.x = x;
    region.origin.y = y;
    region.size.width = l;
    region.size.height = 1;
    txt_a2560k_b_set_region(&region);
    txt_a2560k_b_fill(' ');
    txt_a2560k_b_set_region(&old_region);
}

void clear_lines(int8_t y, int8_t ct)
{
    t_rect region, old_region;

    txt_a2560k_b_get_region(&old_region);
    region.origin.x = 0;
    region.origin.y = y;
    region.size.width = VT_RIGHT + 1;
    region.size.height = ct;
    txt_a2560k_b_set_region(&region);
    txt_a2560k_b_fill(' ');
    txt_a2560k_b_set_region(&old_region);
}

void scroll_up(void)
{
	txt_a2560k_b_scroll(0, 1);
}

void scroll_down(void)
{
	txt_a2560k_b_scroll(0, -1);
}

void plot_char(int8_t y, int8_t x, uint16_t c)
{
	txt_a2560k_b_set_xy(x, y);
    txt_a2560k_b_put(c);    
}

void vtattr_notify(void)
{
}

// eof
