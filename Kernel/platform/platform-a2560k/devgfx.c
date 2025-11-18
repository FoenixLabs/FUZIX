/*
 *	Graphics logic for the A2560x
 */

#include <kernel.h>
#include <kdata.h>
#include <vt.h>
#include <graphics.h>
#include <devgfx.h>

#include "A2560K/vky_chan_b.h"
#include "mcp_types.h"
#include "txt_screen.h"

short txt_a2560k_b_set_resolution(short width, short height);

uint8_t vidmode = 0;	

static struct display v_modes[2] = {
    {
        0,
        640, 480,
        640, 480,
        0xFF, 0xFF,		/* For now */
        FMT_A2560X_8PAL,
        HW_UNACCEL,
        GFX_MAPPABLE|GFX_TEXT|GFX_PALETTE_SET|GFX_MULTIMODE,
        0,
        0, /* TODO: draw/write/etc */
        80, 60
    },
    {
        1,
        320, 240,
        320, 240,
        0xFF, 0xFF,		/* For now */
        FMT_A2560X_8PAL,
        HW_UNACCEL,
        GFX_MAPPABLE|GFX_TEXT|GFX_PALETTE_SET|GFX_MULTIMODE,
        0,
        0, /* TODO: draw/write/etc */
        40, 30
    },
};

static struct videomap v_map[2] = {
  {
    0,
    0,
    0xA000,
    640 * 480,
    0,
    0,
    0,
    MAP_FBMEM|MAP_FBMEM_SIMPLE
  },
  {
    0,
    0,
    0xA000,
    320 * 240,
    0,
    0,
    0,
    MAP_FBMEM|MAP_FBMEM_SIMPLE
  }
};

int gfx_ioctl(uint_fast8_t minor, uarg_t arg, char *ptr)
{
  if (minor != 1)
    return vt_ioctl(minor, arg, ptr);

  switch(arg) {
  case GFXIOC_GETINFO: { 
    return uput(&v_modes[vidmode], ptr, sizeof(struct display));
  }
  case GFXIOC_MAP: {
    return uput(&v_map[vidmode], ptr, sizeof(struct videomap));
  }
  case GFXIOC_UNMAP: {
    return 0;
  }
  case GFXIOC_GETMODE:
  case GFXIOC_SETMODE: {
    
      uint8_t m = ugetc(ptr);
      if (m > 1) {
          udata.u_error = EINVAL;
          return -1;
      }
      if (arg == GFXIOC_GETMODE) {
        return uput(&v_modes[m], ptr, sizeof(struct display));
      }
      vidmode = m;
      gfx_update();
      return 0;
    }
  }
  return vt_ioctl(minor, arg, ptr);
}

int gfx_update() {
  txt_a2560k_b_set_resolution(v_modes[vidmode].width, v_modes[vidmode].height);
  clear_lines(0, VT_BOTTOM + 1);
  cursor_on(0,0);
  return 0;
}

uint16_t gfx_text_width() {
  return v_modes[vidmode].twidth;
}

uint16_t gfx_text_height() {
  return v_modes[vidmode].theight;
}

uint16_t gfx_text_enabled() {
  return v_modes[vidmode].features && GFX_TEXT;
}

uint16_t gfx_pixel_width() {
  return v_modes[vidmode].width;
}

uint16_t gfx_pixel_height() {
  return v_modes[vidmode].height;
}
