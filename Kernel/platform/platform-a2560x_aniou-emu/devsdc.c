
/* there is an SD controller on older foenix machines
 * - for newer (like K2) there should be an SPI-based
 * alternative implementation created
 */

#include <kernel.h>
#include <blkdev.h>
#include <printf.h>
#include "sdc_reg.h"
#include "timers.h"

#define SDC_TIMEOUT_JF 30           /* Timeout in jiffies (1/60 second) */

bool devsdc_exist()
{
	return *SDC_VERSION_REG != 0;
}

bool devsdc_card_detected()
{
    return (*GABE_SDC_REG & GABE_SDC_PRESENT) != GABE_SDC_PRESENT;
}

int devsdc_flush( void )
{
    return 0;
}

/* from MCP */
short sdc_wait_busy()
{
    long timer_ticks;
    unsigned char status;

    timer_ticks = rtc_get_jiffies() + SDC_TIMEOUT_JF;
    do {
        if (rtc_get_jiffies() > timer_ticks) {
            return 1;
        }
        status = *SDC_TRANS_STATUS_REG;
    } while ((status & SDC_TRANS_BUSY) == SDC_TRANS_BUSY);

    return 0;
}

uint_fast8_t devsdc_transfer_sector(void)
{
	blkno_t lba_addr;

    if (!devsdc_card_detected()) {
    	udata.u_error = EIO;
        return 0;
    }

	if (! blk_op.is_read) {
    	udata.u_error = EROFS;
        return 0;
	}

	lba_addr = blk_op.lba << 1;    // in fact << 9, but it means that first byte is 0
    *SDC_SD_ADDR_7_0_REG   = 0;
    *SDC_SD_ADDR_15_8_REG  =  lba_addr       & 0xff;
    *SDC_SD_ADDR_23_16_REG = (lba_addr >> 8) & 0xff;
    *SDC_SD_ADDR_31_24_REG = 0;    // because blkno_t is 16bit

    *SDC_TRANS_TYPE_REG = SDC_TRANS_READ_BLK;   // Set the transaction type to READ
    *SDC_TRANS_CONTROL_REG = SDC_TRANS_START;   // Start the transaction

    if (sdc_wait_busy() != 0) {                 // Wait for the transaction to complete
        kprintf("sdc: wait busy timeout\n");
    	udata.u_error = EIO;
        return 0;
	}

    if (*SDC_TRANS_ERROR_REG != 0) {
        kprintf("sdc: error %2x occured\n", *SDC_TRANS_ERROR_REG);
    	udata.u_error = EIO;
        return 0;
	}

	uint16_t count;
	count = *SDC_RX_FIFO_DATA_CNT_HI << 8 | *SDC_RX_FIFO_DATA_CNT_LO;
	if (count != 512) {
        kprintf("sdc: want 512 bytes but got %d\n", count);
    	udata.u_error = EIO;
		return 0;
	}

	uint8_t *p = blk_op.addr;
	while (count--) {
		*p++ = *SDC_RX_FIFO_DATA_REG;
	}
	
	return 1;		// one block was read
}

void devsdc_init()
{
	blkdev_t *blk;

	kprintf("SD: ");
    if (!devsdc_exist()) {
        kprintf(" not found\n");
        return;
    }

    *SDC_TRANS_TYPE_REG = SDC_TRANS_INIT_SD;        // Start the INIT_SD transaction
    *SDC_TRANS_CONTROL_REG = SDC_TRANS_START;

    if (sdc_wait_busy() != 0) {
        kprintf(" init timeout\n");
        return;
    }

    if (*SDC_TRANS_ERROR_REG != 0) {
        kprintf(" error %2x occured\n", *SDC_TRANS_ERROR_REG);
        return;
    }

    blk = blkdev_alloc();
    blk->driver_data = 0;
    blk->transfer = devsdc_transfer_sector;
    blk->flush = devsdc_flush;
    blk->drive_lba_count=-1;        // we don't have a way to determine that by ctrl
    blkdev_scan(blk, 0);

    kprintf("ok\n");
}

// eof
