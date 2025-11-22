
#ifdef CONFIG_RAMDISK

#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <devrd.h>


uint32_t rd_src_address;
uint32_t rd_dst_address;
bool     rd_dst_userspace;
uint16_t rd_cpy_count;
uint8_t  rd_reverse;      /* reverse the copy direction: false=read, true=write */

void rd_plt_copy(void) {
    unsigned char *dst;
    unsigned char *src;
    uint16_t size;

    /*
        kprintf("\nrd_transfer: src=0x%lx, dst=0x%x(%s) reverse=%d count=%d\n",
                rd_src_address, rd_dst_address, rd_dst_userspace?"user":"kern",
                rd_reverse, rd_cpy_count);
        kprintf("ublock %lx uptr %lx\n", udata.u_nblock, udata.u_dptr);
    */

    if (rd_dst_userspace) {
        size = rd_cpy_count;
    } else {
        size = 512;
    	rd_cpy_count = size;
    }

    if (rd_reverse) {
        src = rd_dst_address;
        dst = rd_src_address;
    } else {
        src = rd_src_address;
        dst = rd_dst_address;
    }

  	while(size--) {
    	*dst++ = *src++;
        //kprintf("%2x ", *src);
    }
	return;
};

static const uint32_t dev_limit[NUM_DEV_RD] = {
    DEV_RD_ROM_START+DEV_RD_ROM_SIZE, /* /dev/rd0: ROM */
    DEV_RD_RAM_START+DEV_RD_RAM_SIZE, /* /dev/rd1: RAM */
};

static const uint32_t dev_start[NUM_DEV_RD] = {
    DEV_RD_ROM_START, /* /dev/rd0: ROM */
    DEV_RD_RAM_START, /* /dev/rd1: RAM */
};

/* implements both rd_read and rd_write */
int rd_transfer2(uint_fast8_t minor, uint_fast8_t rawflag, uint_fast8_t flag)
{
    used(flag);
                                                                                                                                    
    /* check device exists; do not allow writes to ROM */
    if (minor == RD_MINOR_ROM && rd_reverse) {
        udata.u_error = EROFS;
        return -1;
    } else {
        rd_src_address = dev_start[minor];

        if (rawflag) {
            if (d_blkoff(9))
                return -1;
            /* rawflag == 1, userspace transfer */
        }
        rd_dst_userspace = rawflag;

        rd_dst_address = (uint32_t)udata.u_dptr;
        rd_src_address += ((uint32_t)udata.u_block) << BLKSHIFT;

        if (rd_src_address >= dev_limit[minor]) {
           udata.u_error = EIO;
           return -1;
        }
    }
    rd_plt_copy();
    return rd_cpy_count;
}


int rd_read(uint_fast8_t minor, uint_fast8_t rawflag, uint_fast8_t flag)
{
    //kprintf("\nrd_read\n");
	rd_reverse = false;
    return rd_transfer2(minor, rawflag, flag);
}

int rd_write(uint_fast8_t minor, uint_fast8_t rawflag, uint_fast8_t flag)
{
    //kprintf("\nrd_write\n");
	rd_reverse = true;
    return rd_transfer2(minor, rawflag, flag);
}

#endif
