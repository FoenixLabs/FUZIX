/*
 *	Set these top setings according to your board if different
 */

#define CONFIG_VT
#define CONFIG_FONT8X8

/* that platforms can use multiple resolutions */
signed char a2560k_b_text_width();
signed char a2560k_b_text_height();
signed char a2560k_b_text_stride();

#define VT_WIDTH  a2560k_b_text_stride()
#define VT_RIGHT  a2560k_b_text_width()
#define VT_BOTTOM a2560k_b_text_height()

/* Enable to make ^Z dump the inode table for debug */
#undef CONFIG_IDUMP
/* Enable to make ^A drop back into the monitor */
#undef CONFIG_MONITOR
/* Profil syscall support (not yet complete) */
#undef CONFIG_PROFIL

#define CONFIG_32BIT
#define CONFIG_LEVEL_2

#define CONFIG_MULTI
#define CONFIG_FLAT
#define CONFIG_SPLIT_ID
#define CONFIG_PARENT_FIRST
/* It's not that meaningful but we currently chunk to 512 bytes */
#define CONFIG_BANKS 	(65536/512)

#define CONFIG_LARGE_IO_DIRECT(x)	1

#define CONFIG_SPLIT_UDATA
#define UDATA_SIZE	1024
#define UDATA_BLKS	2

#define TICKSPERSEC 10   /* Ticks per second */

#define BOOT_TTY (512 + 1)   /* Set this to default device for stdio, stderr */
                            /* In this case, the default is the first TTY device */
                            /* Temp FIXME set to serial port for debug ease */

/* We need a tidier way to do this from the loader */
#define CMDLINE	NULL	  /* Location of root dev name */

/* Device parameters */
#define NUM_DEV_TTY 2
#define TTYDEV   BOOT_TTY /* Device used by kernel for messages, panics */

/* Could be bigger but we need to add hashing first and it's not clearly
   a win with a CF card anyway */
#define NBUFS    16       /* Number of block buffers */
#define NMOUNTS	 4	  /* Number of mounts at a time */

#define MAX_BLKDEV 5 /* IDE SD ROM RAM */
#define CONFIG_IDE

#define CONFIG_INPUT
#define CONFIG_INPUT_GRABMAX    3

/* in fact: bq4802, but they are enough compatible */
#define CONFIG_RTC_BQ4845
#define CONFIG_RTC
#define CONFIG_RTC_FULL
#define CONFIG_RTC_INTERVAL 1

/* Size for a slightly bigger setup than the little 8bit boxes */
#define PTABSIZE	125
#define OFTSIZE		160
#define ITABSIZE	176
#define UFTSIZE		16

/* #define BOOTDEVICENAMES "hd#,,,,,,,,rd"*/
#define BOOTDEVICENAMES "hd#"
/* undefine, if want to be asked */
#define BOOTDEVICE 1

/* Memory backed devices
 *
 * ramdisk works and system may be even booted from it, by passing rd1
 * device in bootloader - but current SRAM controller in a2560x may be
 * a little unstable:
 *
 * gadget             — 4.01.2025, 05:33
 * FWIW, the SDRAM does not work reliably on my GenX, so I've just been 
 * making due* with the 4MB of SRAM while waiting for an FPGA update to 
 * fix it. 
 *
 * The Cyber Mistress — 4.01.2025, 04:32
 * I believe the SDRAM is working in the K Classic and possibly the Kxx, 
 * but I am not sure and again, it is not super efficiently instantiated.
 * I really plug all of those on the side of the table... 
 * So, there is work to be done.

#define CONFIG_RAMDISK
#define DEV_RD_ROM_PAGES 0      // size of the ROM disk (/dev/rd0) in 4KB pages
#define DEV_RD_RAM_PAGES 10240  // size of the RAM disk (/dev/rd1) in 4KB pages 

#define DEV_RD_ROM_START (uint32_t)0x02400000
#define DEV_RD_RAM_START (uint32_t)0x02000000
#define DEV_RD_ROM_SIZE  ((uint32_t)DEV_RD_ROM_PAGES << 12)
#define DEV_RD_RAM_SIZE  ((uint32_t)DEV_RD_RAM_PAGES << 12)

*/

/* eof */
