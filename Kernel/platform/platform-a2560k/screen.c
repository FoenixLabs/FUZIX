
/* original file txt_a2560k_b.c
 * Copyright (c) 2021, Peter J. Weingartner
 *
 * ripped off elements: 2025 Piotr Meyer
 */

#include <kernel.h>
#include "A2560K/vky_chan_b.h"
#include "mcp_types.h"
#include "txt_screen.h"

#define TXT_SCREEN_A2560K_B 0
#define CHAR_ESC    '\x1B'  /* Escape character */
#define CHAR_TAB    '\t'    /* Vertical tab */
#define CHAR_CR     '\x0D'  /* Carriage return */
#define CHAR_NL     '\x0A'  /* Linefeed */
#define CHAR_BS     '\b'    /* Backspace */


/* default, FUZIX-wide font */
extern unsigned char fontdata_8x8[];


/*
 * Driver level variables for the screen
 */

unsigned char a2560k_b_enable_set_sizes;    /* Flag to enable set_sizes to actually do its computation */
t_txt_capabilities a2560k_b_caps;           /* The capabilities of Channel B */
t_extent a2560k_b_resolutions[6];           /* The list of display resolutions */
t_extent a2560k_b_fonts[1];                 /* The list of font resolutions */
t_rect a2560k_b_region;                     /* The current region */
t_point a2560k_b_cursor;                    /* The current cursor position */
t_extent a2560k_b_resolution;               /* The current display resolution */
t_extent a2560k_b_font_size;                /* The current font size */
t_extent a2560k_b_max_size;                 /* The size of the screen in characters (without border removed) */
t_extent a2560k_b_visible_size;             /* The size of the visible screen in characters (with border removed) */
unsigned short a2560k_b_border_width;                /* Width of the border on one side */
unsigned short a2560k_b_border_height;               /* Height of the border on one side */
unsigned char a2560k_b_color;               /* The current color */
unsigned long msr_shadow_b;                   /* A shadow register for the Master Control Register */

signed char a2560k_b_text_stride() {
    return (a2560k_b_max_size.width);
}

signed char a2560k_b_text_width() {
    return (a2560k_b_visible_size.width - 1);
}

signed char a2560k_b_text_height() {
    return a2560k_b_visible_size.height - 1;
}


/* vt52 text color lookup table values (AARRGGBB) */
const unsigned long pal_vt52[VKY3_B_LUT_SIZE] = {
    0xFF000000, // Black
    0xFF0000AA, // Blue
    0xFFAA0000, // Red
    0xFFAA00AA, // Magenta
    0xFF00AA00, // Green
    0xFF00AAAA, // Cyan
    0xFFAAAA00, // Yellow
    0xFFAAAAAA, // White
    0xFF555555, // Bright Black
    0xFF0000FF, // Bright Blue
    0xFFFF0000, // Bright Red
    0xFFFF00FF, // Bright Magenta
    0xFF00FF00, // Bright Green
    0xFF00FFFF, // Bright Cyan
    0xFFFFFF00, // Bright Yellow
    0xFFFFFFFF  // Bright White
};

const uint32_t pal_rgb332[256] = {
	0xFF000000, 0xFF000055, 0xFF0000AA, 0xFF0000FF, 0xFF002400, 0xFF002455, 0xFF0024AA, 0xFF0024FF, 
	0xFF004900, 0xFF004955, 0xFF0049AA, 0xFF0049FF, 0xFF006D00, 0xFF006D55, 0xFF006DAA, 0xFF006DFF, 
	0xFF009200, 0xFF009255, 0xFF0092AA, 0xFF0092FF, 0xFF00B600, 0xFF00B655, 0xFF00B6AA, 0xFF00B6FF, 
	0xFF00DB00, 0xFF00DB55, 0xFF00DBAA, 0xFF00DBFF, 0xFF00FF00, 0xFF00FF55, 0xFF00FFAA, 0xFF00FFFF, 
	0xFF240000, 0xFF240055, 0xFF2400AA, 0xFF2400FF, 0xFF242400, 0xFF242455, 0xFF2424AA, 0xFF2424FF, 
	0xFF244900, 0xFF244955, 0xFF2449AA, 0xFF2449FF, 0xFF246D00, 0xFF246D55, 0xFF246DAA, 0xFF246DFF, 
	0xFF249200, 0xFF249255, 0xFF2492AA, 0xFF2492FF, 0xFF24B600, 0xFF24B655, 0xFF24B6AA, 0xFF24B6FF, 
	0xFF24DB00, 0xFF24DB55, 0xFF24DBAA, 0xFF24DBFF, 0xFF24FF00, 0xFF24FF55, 0xFF24FFAA, 0xFF24FFFF, 
	0xFF490000, 0xFF490055, 0xFF4900AA, 0xFF4900FF, 0xFF492400, 0xFF492455, 0xFF4924AA, 0xFF4924FF, 
	0xFF494900, 0xFF494955, 0xFF4949AA, 0xFF4949FF, 0xFF496D00, 0xFF496D55, 0xFF496DAA, 0xFF496DFF, 
	0xFF499200, 0xFF499255, 0xFF4992AA, 0xFF4992FF, 0xFF49B600, 0xFF49B655, 0xFF49B6AA, 0xFF49B6FF, 
	0xFF49DB00, 0xFF49DB55, 0xFF49DBAA, 0xFF49DBFF, 0xFF49FF00, 0xFF49FF55, 0xFF49FFAA, 0xFF49FFFF, 
	0xFF6D0000, 0xFF6D0055, 0xFF6D00AA, 0xFF6D00FF, 0xFF6D2400, 0xFF6D2455, 0xFF6D24AA, 0xFF6D24FF, 
	0xFF6D4900, 0xFF6D4955, 0xFF6D49AA, 0xFF6D49FF, 0xFF6D6D00, 0xFF6D6D55, 0xFF6D6DAA, 0xFF6D6DFF, 
	0xFF6D9200, 0xFF6D9255, 0xFF6D92AA, 0xFF6D92FF, 0xFF6DB600, 0xFF6DB655, 0xFF6DB6AA, 0xFF6DB6FF, 
	0xFF6DDB00, 0xFF6DDB55, 0xFF6DDBAA, 0xFF6DDBFF, 0xFF6DFF00, 0xFF6DFF55, 0xFF6DFFAA, 0xFF6DFFFF, 
	0xFF920000, 0xFF920055, 0xFF9200AA, 0xFF9200FF, 0xFF922400, 0xFF922455, 0xFF9224AA, 0xFF9224FF, 
	0xFF924900, 0xFF924955, 0xFF9249AA, 0xFF9249FF, 0xFF926D00, 0xFF926D55, 0xFF926DAA, 0xFF926DFF, 
	0xFF929200, 0xFF929255, 0xFF9292AA, 0xFF9292FF, 0xFF92B600, 0xFF92B655, 0xFF92B6AA, 0xFF92B6FF, 
	0xFF92DB00, 0xFF92DB55, 0xFF92DBAA, 0xFF92DBFF, 0xFF92FF00, 0xFF92FF55, 0xFF92FFAA, 0xFF92FFFF, 
	0xFFB60000, 0xFFB60055, 0xFFB600AA, 0xFFB600FF, 0xFFB62400, 0xFFB62455, 0xFFB624AA, 0xFFB624FF, 
	0xFFB64900, 0xFFB64955, 0xFFB649AA, 0xFFB649FF, 0xFFB66D00, 0xFFB66D55, 0xFFB66DAA, 0xFFB66DFF, 
	0xFFB69200, 0xFFB69255, 0xFFB692AA, 0xFFB692FF, 0xFFB6B600, 0xFFB6B655, 0xFFB6B6AA, 0xFFB6B6FF, 
	0xFFB6DB00, 0xFFB6DB55, 0xFFB6DBAA, 0xFFB6DBFF, 0xFFB6FF00, 0xFFB6FF55, 0xFFB6FFAA, 0xFFB6FFFF, 
	0xFFDB0000, 0xFFDB0055, 0xFFDB00AA, 0xFFDB00FF, 0xFFDB2400, 0xFFDB2455, 0xFFDB24AA, 0xFFDB24FF, 
	0xFFDB4900, 0xFFDB4955, 0xFFDB49AA, 0xFFDB49FF, 0xFFDB6D00, 0xFFDB6D55, 0xFFDB6DAA, 0xFFDB6DFF, 
	0xFFDB9200, 0xFFDB9255, 0xFFDB92AA, 0xFFDB92FF, 0xFFDBB600, 0xFFDBB655, 0xFFDBB6AA, 0xFFDBB6FF, 
	0xFFDBDB00, 0xFFDBDB55, 0xFFDBDBAA, 0xFFDBDBFF, 0xFFDBFF00, 0xFFDBFF55, 0xFFDBFFAA, 0xFFDBFFFF, 
	0xFFFF0000, 0xFFFF0055, 0xFFFF00AA, 0xFFFF00FF, 0xFFFF2400, 0xFFFF2455, 0xFFFF24AA, 0xFFFF24FF, 
	0xFFFF4900, 0xFFFF4955, 0xFFFF49AA, 0xFFFF49FF, 0xFFFF6D00, 0xFFFF6D55, 0xFFFF6DAA, 0xFFFF6DFF, 
	0xFFFF9200, 0xFFFF9255, 0xFFFF92AA, 0xFFFF92FF, 0xFFFFB600, 0xFFFFB655, 0xFFFFB6AA, 0xFFFFB6FF, 
	0xFFFFDB00, 0xFFFFDB55, 0xFFFFDBAA, 0xFFFFDBFF, 0xFFFFFF00, 0xFFFFFF55, 0xFFFFFFAA, 0xFFFFFFFF	
};

/**
 * Calculate the size of the text screen in rows and columns so that
 * the kernel printing routines can work correctly.
 *
 * NOTE: this should be called whenever the VKY3 Channel B registers are changed
 */
void txt_a2560k_b_set_sizes() {
    if (a2560k_b_enable_set_sizes) {
        /* Only recalculate after initialization is mostly completed */

        /*
         * Calculate the maximum number of characters visible on the screen
         * This controls text layout in memory
         */
        a2560k_b_max_size.width = a2560k_b_resolution.width / a2560k_b_font_size.width;
        a2560k_b_max_size.height = a2560k_b_resolution.height / a2560k_b_font_size.height;

        /*
         * Calculate the characters that are visible in whole or in part
         */
        unsigned short border_width  = (a2560k_b_border_width * 2U) / a2560k_b_font_size.width;
        unsigned short border_height = (a2560k_b_border_height * 2U) / a2560k_b_font_size.height;
        a2560k_b_visible_size.width = a2560k_b_max_size.width - border_width;
        a2560k_b_visible_size.height = a2560k_b_max_size.height - border_height;
    }
}

/**
 * Get the display resolutions
 *
 * @param text_size the size of the screen in visible characters (may be null)
 * @param pixel_size the size of the screen in pixels (may be null)
 */
void txt_a2560k_b_get_sizes(p_extent text_size, p_extent pixel_size) {
    if (text_size) {
        text_size->width = a2560k_b_visible_size.width;
        text_size->height = a2560k_b_visible_size.height;
    }

    if (pixel_size) {
        pixel_size->width = a2560k_b_resolution.width;
        pixel_size->height = a2560k_b_resolution.height;
    }
}

/**
 * Set the display mode for the screen
 *
 * @param mode a bitfield of desired display mode options
 *
 * @return 0 on success, any other number means the mode is invalid for the screen
 */
short txt_a2560k_b_set_mode(short mode) {
    /* Turn off anything not set */
    msr_shadow_b &= ~(VKY3_B_MCR_TEXT | VKY3_B_MCR_TXT_OVR | VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_BITMAP | VKY3_B_MCR_TILE | VKY3_B_MCR_SPRITE);

    if (mode & TXT_MODE_SLEEP) {
        /* Put the monitor to sleep: overrides all other option bits */
        msr_shadow_b |= VKY3_B_MCR_SLEEP;
        *VKY3_B_MCR = msr_shadow_b;
        return 0;

    } else {
        if (mode & ~(TXT_MODE_TEXT | TXT_MODE_BITMAP | TXT_MODE_SPRITE | TXT_MODE_TILE)) {
            /* A mode bit was set beside one of the supported ones... */
            return -1;

        } else {
            if (mode & TXT_MODE_TEXT) {
                msr_shadow_b |= VKY3_B_MCR_TEXT;
            }

            if (mode & TXT_MODE_BITMAP) {
                msr_shadow_b |= VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_BITMAP;
            }

            if (mode & TXT_MODE_SPRITE) {
                msr_shadow_b |= VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_SPRITE;
            }

            if (mode & TXT_MODE_TILE) {
                msr_shadow_b |= VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_TILE;
            }

            if ((msr_shadow_b & (VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_TEXT)) == (VKY3_B_MCR_GRAPHICS | VKY3_B_MCR_TEXT)) {
                msr_shadow_b |= VKY3_B_MCR_TXT_OVR;
            }

            *VKY3_B_MCR = msr_shadow_b;
            return 0;
        }
    }
}

/**
 * Set the display resolution of the screen
 *
 * @param width the desired horizontal resolution in pixels
 * @param height the desired veritical resolution in pixels
 *
 * @return 0 on success, any other number means the resolution is unsupported
 */
short txt_a2560k_b_set_resolution(short width, short height) {
    int i;

    // If no size specified, set it based on the DIP switch
    if ((width == 0) || (height == 0)) {
        if ((*VKY3_B_MCR & VKY3_B_HIRES) == 0) {
            width = 800;
            height = 600;
        } else {
            width = 640;
            height = 480;
        }
    }

    // Kick the PLL
    // If VICKY is generating a 40MHz signal, we need to switch the bit to go to 40MHz before
    // clearing it to go back to 25MHz.
    if (*VKY3_B_MCR & VKY3_B_CLK40) {
        *VKY3_B_MCR |= VKY3_B_PLL | VKY3_B_MODE1;
        *VKY3_B_MCR &= ~(VKY3_B_PLL | VKY3_B_MODE1);
    }

    for (i = 0; i < a2560k_b_caps.resolution_count; i++) {
        if ((a2560k_b_caps.resolutions[i].width == width) && (a2560k_b_caps.resolutions[i].height == height)) {
            msr_shadow_b &= ~(VKY3_B_DOUBLE | VKY3_B_MODE0 | VKY3_B_MODE1);
            if (height < 400) {
                /* We're in pixel doubling range */
                msr_shadow_b |= VKY3_B_DOUBLE;

                /* Figure out what the base resolution is */
                height *= 2;
            }

            // Use the height to determine the resolution we should set
            switch (height) {
                case 400:   // 640x400 or 320x200 (mode = 11)
                    msr_shadow_b |= VKY3_B_MODE0;
                    break;

                case 480:   // 640x480 or 320x240 (mode = 00)
                    break;

                case 600:   // 800x600 or 400x300 (mode = 01)
                    msr_shadow_b |= VKY3_B_PLL | VKY3_B_MODE1;
                    break;

                default:
                    // Shouldn't get here... bad resolution
                    return -1;
            }

            // Update the kernel variables
            a2560k_b_resolution.width = width;
            a2560k_b_resolution.height = height;

            // Recalculate the size of the screen
            txt_a2560k_b_set_sizes();

            // Kick the PLL
            if (*VKY3_B_MCR & VKY3_B_PLL) {
                *VKY3_B_MCR &= ~(VKY3_B_PLL | VKY3_B_MODE0 | VKY3_B_MODE1);
                *VKY3_B_MCR |= (VKY3_B_PLL | VKY3_B_MODE1);
            }
            *VKY3_B_MCR &= ~(VKY3_B_PLL | VKY3_B_MODE0 | VKY3_B_MODE1);

            // Update the register
            *VKY3_B_MCR = msr_shadow_b;
            return 0;
        }
    }
    return -1;
}

/**
 * Set the size of the border of the screen (if supported)
 *
 * @param width the horizontal size of one side of the border (0 - 32 pixels)
 * @param height the vertical size of one side of the border (0 - 32 pixels)
 */
void txt_a2560k_b_set_border(short width, short height) {
    if ((width > 0) || (height > 0)) {
        a2560k_b_border_width = width;
        a2560k_b_border_height = height;
        *VKY3_B_BCR = (height & 0x3f) << 16 | (width & 0x3f) << 8 | VKY3_B_BCR_ENABLE;

        // Recalculate the size of the screen
        txt_a2560k_b_set_sizes();

    } else {
        a2560k_b_border_width = 0;
        a2560k_b_border_height = 0;
        *VKY3_B_BCR = 0;

        // Recalculate the size of the screen
        txt_a2560k_b_set_sizes();
    }
}

/**
 * Set the size of the border of the screen (if supported)
 *
 * @param red the red component of the color (0 - 255)
 * @param green the green component of the color (0 - 255)
 * @param blue the blue component of the color (0 - 255)
 */
void txt_a2560k_b_set_border_color(unsigned char red, unsigned char green, unsigned char blue) {
    *VKY3_B_BRDCOLOR = (unsigned long)(((red & 0xff) << 16) | ((green & 0xff) << 8) | (blue & 0xff));
}

/**
 * Load a font as the current font for the screen
 *
 * @param width width of a character in pixels
 * @param height of a character in pixels
 * @param data pointer to the raw font data to be loaded
 */
short txt_a2560k_b_set_font(short width, short height, const unsigned char * data) {
    if ((width == 8) && (height == 8)) {
        int i;

        /* The size is valid... set the font */
        a2560k_b_font_size.width = width;
        a2560k_b_font_size.height = height;

        /* Copy the font data... this assumes a width of one byte! */
        for (i = 0; i < 256 * height; i++) {
            VKY3_B_FONT_MEMORY[i] = data[i];
        }

        return 0;

    } else {
        return -1;
    }
}

/**
 * Set the appearance of the cursor
 *
 * @param enable 0 to hide, any other number to make visible
 * @param rate the blink rate for the cursor (0=1s, 1=0.5s, 2=0.25s, 3=1/5s)
 * @param c the character in the current font to use as a cursor
 */
void txt_a2560k_b_set_cursor(short enable, short rate, char c) {
    *VKY3_B_CCR = ((a2560k_b_color & 0xff) << 24) | ((c & 0xff) << 16) | ((rate & 0x03) << 1) | (enable & 0x01);
}

/**
 * Set if the cursor is visible or not
 *
 * @param enable 0 to hide, any other number to make visible
 */
 void txt_a2560k_b_set_cursor_visible(short enable) {
     if (enable) {
         *VKY3_B_CCR |= 0x01;
     } else {
         *VKY3_B_CCR &= ~0x01;
     }
 }

/**
 * Set a region to restrict further character display, scrolling, etc.
 * Note that a null region pointer and a region of zero size will reset the region to the full size of the screen.
 *
 * @param region pointer to a t_rect describing the rectangular region (using character cells for size and size)
 *
 * @return 0 on success, any other number means the region was invalid
 */
 short txt_a2560k_b_set_region(p_rect region) {
     if ((region->size.width == 0) || (region->size.height == 0)) {
         /* Set the region to the default (full screen) */
         a2560k_b_region.origin.x = 0;
         a2560k_b_region.origin.y = 0;
         a2560k_b_region.size.width = a2560k_b_visible_size.width;
         a2560k_b_region.size.height = a2560k_b_visible_size.height;

     } else {
         a2560k_b_region.origin.x = region->origin.x;
         a2560k_b_region.origin.y = region->origin.y;
         a2560k_b_region.size.width = region->size.width;
         a2560k_b_region.size.height = region->size.height;
     }

     return 0;
 }

/**
 * get the current region
 *
 * @param region pointer to a t_rect describing the rectangular region (using character cells for size and size)
 *
 * @return 0 on success, any other number means the region was invalid
 */
short txt_a2560k_b_get_region(p_rect region) {
    region->origin.x = a2560k_b_region.origin.x;
    region->origin.y = a2560k_b_region.origin.y;
    region->size.width = a2560k_b_region.size.width;
    region->size.height = a2560k_b_region.size.height;

    return 0;
}

/**
 * Set the default foreground and background colors for printing
 *
 * @param foreground the Text LUT index of the new current foreground color (0 - 15)
 * @param background the Text LUT index of the new current background color (0 - 15)
 */
short txt_a2560k_b_set_color(unsigned char foreground, unsigned char background) {
    a2560k_b_color = ((foreground & 0x0f) << 4) | (background & 0x0f);
    return 0;
}

/**
 * Get the default foreground and background colors for printing
 *
 * @param pointer to the foreground the Text LUT index of the new current foreground color (0 - 15)
 * @param pointer to the background the Text LUT index of the new current background color (0 - 15)
 */
short txt_a2560k_b_get_color(unsigned char * foreground, unsigned char * background) {
    *foreground = (a2560k_b_color & 0xf0) >> 4;
    *background = a2560k_b_color & 0x0f;
    return 0;
}


/**
 * Scroll the text in the current region
 *
 * @param screen the number of the text device
 * @param horizontal the number of columns to scroll (negative is left, positive is right)
 * @param vertical the number of rows to scroll (negative is down, positive is up)
 */
void txt_a2560k_b_scroll(short horizontal, short vertical) {
    short x, x0, x1, x2, x3, dx;
    short y, y0, y1, y2, y3, dy;

    /*
     * Determine limits of rectangles to move and fill and directions of loops
     * x0 and y0 are the positions of the first cell to be over-written
     * x1 and y1 are the positions of the first cell to be copyed... TEXT[x0,y0] := TEXT[x1,y1]
     * x2 and y2 are the position of the last cell to be over-written
     * x3 and y3 are the position of the last cell to be copied... TEXT[x2,y2] := TEXT[x3,y3]
     *
     * When blanking, the rectangles (x2,y0) - (x3,y3) and (x0,y2) - (x2,y3) are cleared
     */

    // Determine the row limits

    if (vertical >= 0) {
        y0 = a2560k_b_region.origin.y;
        y1 = y0 + vertical;
        y3 = a2560k_b_region.origin.y + a2560k_b_region.size.height;
        y2 = y3 - vertical;
        dy = 1;
    } else {
        y0 = a2560k_b_region.origin.y + a2560k_b_region.size.height - 1;
        y1 = y0 + vertical;
        y3 = a2560k_b_region.origin.y - 1;
        y2 = y3 - vertical;
        dy = -1;
    }

    // Determine the column limits

    if (horizontal >= 0) {
        x0 = a2560k_b_region.origin.x;
        x1 = x0 + horizontal;
        x3 = a2560k_b_region.origin.x + a2560k_b_region.size.width;
        x2 = x3 - horizontal;
        dx = 1;
    } else {
        x0 = a2560k_b_region.origin.x + a2560k_b_region.size.width - 1;
        x1 = x0 + horizontal;
        x3 = a2560k_b_region.origin.x - 1;
        x2 = x3 - horizontal;
        dx = -1;
    }

    /* Copy the rectangle */

    for (y = y0; y != y2; y += dy) {
        int row_dst = y * a2560k_b_max_size.width;
        int row_src = (y + vertical) * a2560k_b_max_size.width;

        for (x = x0; x != x2; x += dx) {
            int offset_dst = row_dst + x;
            int offset_src = row_src + x + horizontal;

            VKY3_B_TEXT_MATRIX[offset_dst] = VKY3_B_TEXT_MATRIX[offset_src];
            VKY3_B_COLOR_MATRIX[offset_dst] = VKY3_B_COLOR_MATRIX[offset_src];
        }
    }

    /* Clear the rectangles */

    if (horizontal != 0) {
        for (y = y0; y != y3; y += dy) {
            int row_dst = y * a2560k_b_max_size.width;
            for (x = x2; x != x3; x += dx) {
                VKY3_B_TEXT_MATRIX[row_dst + x] = ' ';
                VKY3_B_COLOR_MATRIX[row_dst + x] = a2560k_b_color;
            }
        }
    }

    if (vertical != 0) {
        for (y = y2; y != y3; y += dy) {
            int row_dst = y * a2560k_b_max_size.width;
            for (x = x0; x != x3; x += dx) {
                VKY3_B_TEXT_MATRIX[row_dst + x] = ' ';
                VKY3_B_COLOR_MATRIX[row_dst + x] = a2560k_b_color;
            }
        }
    }
}

/**
 * Fill the current region with a character in the current color
 *
 * @param screen the number of the text device
 * @param c the character to fill the region with
 */
void txt_a2560k_b_fill(char c) {
    int x;
    int y;

    for (y = 0; y < a2560k_b_region.size.height; y++) {
        int offset_row = (a2560k_b_region.origin.y + y) * a2560k_b_max_size.width;
        for (x = 0; x < a2560k_b_region.size.width; x++) {
            int offset = offset_row + a2560k_b_region.origin.x + x;
            VKY3_B_TEXT_MATRIX[offset] = c;
            VKY3_B_COLOR_MATRIX[offset] = a2560k_b_color;
        }
    }
}

/**
 * Set the position of the cursor to (x, y) relative to the current region
 * If the (x, y) coordinate is outside the region, it will be clipped to the region.
 * If y is greater than the height of the region, the region will scroll until that relative
 * position would be within view.
 *
 * @param x the column for the cursor
 * @param y the row for the cursor
 */
void txt_a2560k_b_set_xy(short x, short y) {
    /* Make sure X is within range for the current region... "print" a newline if not */
    if (x < 0) {
        x = 0;
    } else if (x >= a2560k_b_region.size.width) {
        x = 0;
        y++;
    }

    /* Make sure Y is within range for the current region... scroll if not */
    if (y < 0) {
        y = 0;
    } else if (y >= a2560k_b_region.size.height) {
        //txt_a2560k_b_scroll(0, y - a2560k_b_region.size.height + 1);
        y = a2560k_b_region.size.height - 1;
    }

    a2560k_b_cursor.x = x;
    a2560k_b_cursor.y = y;

    /* Set register */
    *VKY3_B_CPR = (((a2560k_b_region.origin.y + y) & 0xffff) << 16) | ((a2560k_b_region.origin.x + x) & 0xffff);
}

/**
 * Get the position of the cursor (x, y) relative to the current region
 *
 * @param screen the number of the text device
 * @param position pointer to a t_point record to fill out
 */
void txt_a2560k_b_get_xy(p_point position) {
    position->x = a2560k_b_cursor.x;
    position->y = a2560k_b_cursor.y;
}

/**
 * Print a character to the current cursor position in the current color
 *
 * @param screen the number of the text device
 * @param c the character to print
 */
void txt_a2560k_b_put(char c) {
    short x;
    short y;
    unsigned int offset;

    x = a2560k_b_region.origin.x + a2560k_b_cursor.x;
    y = a2560k_b_region.origin.y + a2560k_b_cursor.y;
    offset = y * a2560k_b_max_size.width + x;
    VKY3_B_TEXT_MATRIX[offset] = c;
    VKY3_B_COLOR_MATRIX[offset] = a2560k_b_color;

    //txt_a2560k_b_set_xy(a2560k_b_cursor.x + 1, a2560k_b_cursor.y);
}

void txt_a2560k_b_init() {
    t_rect region;
    int i;

    // Kick the PLL
    // If VICKY is generating a 40MHz signal, we need to switch the bit to go to 40MHz before
    // clearing it to go back to 25MHz.
    if (*VKY3_B_MCR & VKY3_B_CLK40) {
        *VKY3_B_MCR |= VKY3_B_PLL | VKY3_B_MODE1;
        *VKY3_B_MCR &= ~(VKY3_B_PLL | VKY3_B_MODE1);
    }

    a2560k_b_resolution.width = 0;
    a2560k_b_resolution.height = 0;
    a2560k_b_font_size.width = 0;
    a2560k_b_font_size.height = 0;

    /* Disable the set_sizes call for now */
    a2560k_b_enable_set_sizes = 0;

    /* Start with nothing on */
    msr_shadow_b = 0;

    /* Define the capabilities */

    /* Specify the screen number */
    a2560k_b_caps.number = TXT_SCREEN_A2560K_B;

    /* This screen can be nothing, sleep, or any combination of text, sprite, bitmap, and tile */
    a2560k_b_caps.supported_modes = TXT_MODE_TEXT | TXT_MODE_SPRITE | TXT_MODE_BITMAP | TXT_MODE_TILE | TXT_MODE_SLEEP;

    /* Resolutions supported: 320x200, 320x240, 400x300, 640x400, 640x480, 800x600 */
    a2560k_b_resolutions[0].width = 320;
    a2560k_b_resolutions[0].height = 200;
    a2560k_b_resolutions[1].width = 320;
    a2560k_b_resolutions[1].height = 240;
    a2560k_b_resolutions[2].width = 400;
    a2560k_b_resolutions[2].height = 300;
    a2560k_b_resolutions[3].width = 640;
    a2560k_b_resolutions[3].height = 400;
    a2560k_b_resolutions[4].width = 640;
    a2560k_b_resolutions[4].height = 480;
    a2560k_b_resolutions[5].width = 800;
    a2560k_b_resolutions[5].height = 600;
    a2560k_b_caps.resolution_count = 6;
    a2560k_b_caps.resolutions = a2560k_b_resolutions;

    /* Channel B supports 8x8 fonts ONLY */
    a2560k_b_fonts[0].width = 8;
    a2560k_b_fonts[0].height = 8;
    a2560k_b_caps.font_size_count = 1;
    a2560k_b_caps.font_sizes = a2560k_b_fonts;

    /* Initialze the color lookup tables */
    for (i = 0; i < VKY3_B_LUT_SIZE; i++) {
        VKY3_B_TEXT_LUT_FG[i] = pal_vt52[i];
        VKY3_B_TEXT_LUT_BG[i] = pal_vt52[i];
    }

    /* Set the mode to text */
    txt_a2560k_b_set_mode(TXT_MODE_TEXT);

    /* Set the resolution */
    txt_a2560k_b_set_resolution(640, 480);                  /* Default resolution is 640x480 */
    //txt_a2560k_b_set_resolution(800, 600);                  /* Default resolution is 640x480 */

    /* Set the default color: light grey on dark grey */
    // txt_a2560k_b_set_color(0x0F, 0x08);
    txt_a2560k_b_set_color(0x0C, 0x04);

    /* Set the font */
    txt_a2560k_b_set_font(8, 8, fontdata_8x8);         /* Use 8x8 font */

    /* Set the cursor */
    txt_a2560k_b_set_cursor(1, 0, 0xB1);

    /* Set the border */
    txt_a2560k_b_set_border(8, 8);                            /* Set up the border */
    txt_a2560k_b_set_border_color(0x10, 0x00, 0x10);

    /*
     * Enable set_sizes, now that everything is set up initially
     * And calculate the size of the screen
     */
    a2560k_b_enable_set_sizes = 1;
    txt_a2560k_b_set_sizes();

    /* Set region to default */
    region.origin.x = 0;
    region.origin.y = 0;
    region.size.width = 0;
    region.size.height = 0;
    txt_a2560k_b_set_region(&region);

    /* Home the cursor */
    txt_a2560k_b_set_xy(0, 0);

    /* Clear the screen */
    txt_a2560k_b_fill('x');
}

