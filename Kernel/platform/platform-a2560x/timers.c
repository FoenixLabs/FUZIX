
#include <kernel.h>
#include <printf.h>
#include "timers_reg.h"
#include "interrupt.h"

/*
 * Return the number of jiffies (1/60 of a second) since last reset time
 * TIMER3 and 4 are clocked by Start Of Frame clocks in Vicky III channel
 * A and B
 */
long rtc_get_jiffies()
{
    return *TIMER_VALUE_3;
}

/*
 * Initialize the timers and their interrupts
 */
void timers_init() {
    *TIMER_TCR0 = 0;    // Reset timers 0, 1, and 2
    //*TIMER_TCR1 = 0;    // Reset timers 3, and 4 (if 4 is available)

    // Set timer 0 to count up and auto clear
    *TIMER_VALUE_0   = 0U;
    *TIMER_COMPARE_0 = 3300000;  // fire at 1/10 sec (normally 33 000 000 Hz per second)
    *TIMER_TCR0      = TCR_ENABLE_0 | TCR_CNTUP_0 | TCR_CLEAR_0 | TCR_INE_0;
    *TIMER_TCR0      = TCR_ENABLE_0 | TCR_CNTUP_0 | TCR_INE_0;

    kprintf("timer ctrl %x value %d compare %d\n", *TIMER_TCR0, *TIMER_VALUE_0, *TIMER_COMPARE_0);
    kprintf("timer ctrl %x value %d compare %d\n", *TIMER_TCR0, *TIMER_VALUE_1, *TIMER_COMPARE_1);
    kprintf("timer ctrl %x value %d compare %d\n", *TIMER_TCR0, *TIMER_VALUE_2, *TIMER_COMPARE_2);

    long timer_ticks;
    timer_ticks = rtc_get_jiffies() + 50;
    do {
            kprintf("jiffies value %d\n", rtc_get_jiffies() );
    } while (rtc_get_jiffies() > timer_ticks);

    kprintf("timer ctrl %x value %d compare %d\n", *TIMER_TCR0, *TIMER_VALUE_0, *TIMER_COMPARE_0);

    int_enable(INT_TIMER0); 
    return;
}


