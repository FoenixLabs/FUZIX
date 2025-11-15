#include <kernel.h>
#include <version.h>
#include <kdata.h>
#include <devide.h>
#include <blkdev.h>
#include <devsys.h>
#include <tty.h>
#include <vt.h>
#include <devrd.h>
#include "ps2_reg.h"
/*
struct devsw dev_tab[] : This table holds the functions to call for device
driver operations. Each device major number has open, close, read, write and
ioctl (control) methods. Not all of the methods are mandatory and the kernel
provides helper methods - no_open, no_close, no_rdwr, no_ioctl for them.
Also provided is nxio_open which is the open method for a device that is not
present in the system.

See the device section on how to fill this in and write device drivers.
*/

struct devsw dev_tab[] =  /* The device driver switch table */
{
// major    open         close        read      write       ioctl
// -----------------------------------------------------------------
  /* 0: /dev/hd     Disc block devices  */
  {  blkdev_open,  no_close,    blkdev_read,   blkdev_write, blkdev_ioctl },
  /* 1: /dev/fd     Floppy disc block devices (absent) */
  {  nxio_open,    no_close,    no_rdwr,       no_rdwr,      no_ioctl     },
  /* 2: /dev/tty    TTY devices */
  {  tty_open,     tty_close,   tty_read,      tty_write,    vt_ioctl     },
  /* 3: /dev/lpr    Printer devices */
  {  no_open,      no_close,    no_rdwr,       no_rdwr,      no_ioctl     },
  /* 4: /dev/mem etc    System devices (one offs) */
  {  no_open,      no_close,    sys_read,      sys_write,    sys_ioctl    },
  /* Pack to 7 with nxio if adding private devices and start at 8 */
  /*
  {  nxio_open,    no_close,    no_rdwr,       no_rdwr       no_ioctl     },
  {  nxio_open,    no_close,    no_rdwr,       no_rdwr,      no_ioctl     },
  {  nxio_open,    no_close,    no_rdwr,       no_rdwr,      no_ioctl     },
  */
  /* 8: /dev/rd - ramdisk */
  /*
  {  rd_open,      no_close,    rd_read,       rd_write,     no_ioctl     },
  */
};

bool validdev(uint16_t dev)
{
    if(dev > ((sizeof(dev_tab)/sizeof(struct devsw)) << 8) - 1) {
        return false;
    } else {
        return true;
    }
}

// it is called after IRQ enable, thus timers needs it
void device_init(void)
{
  devide_init();
  //devsdc_init();
  devsd_init();

  timers_init();
}

