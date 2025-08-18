
#include <kernel.h>
#include <kdata.h>
#include <stdbool.h>
#include "rtc_reg.h"

void bq4845_write(uint_fast8_t reg, uint_fast8_t val)
{
    *(RTC_BASE + reg) = val;
}

uint_fast8_t bq4845_read(uint_fast8_t reg)
{
    return *(RTC_BASE + reg);
}

// eof
