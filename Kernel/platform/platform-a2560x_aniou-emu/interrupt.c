
/* ripped-off from original FoenixMCP
 * Copyright (c) 2021, Peter J. Weingartner
 *
 * modifications: 2025 Piotr Meyer
 */


#include "interrupt.h"

/*
 * Return the mask bit for the interrupt number
 *
 * For the m68000 machines, this will just be the bit corresponding to the lower nibble
 */
unsigned short int_mask(unsigned short n)
{
        return (1 << (n & 0x0f));
}

/*
 * Return the group number for the interrupt number
 *
 * For the m68000 machines, this will just be the high nibble of the number
 */
unsigned short int_group(unsigned short n) 
{
    return ((n >> 4) & 0x0f);
} 

/*
 * Acknowledge an interrupt (clear out its pending flag)
 *
 * Inputs:
 * n = the number of the interrupt: n[7..4] = group number, n[3..0] = individual number.
 */
void int_clear(unsigned short n)
{
    /* Find the group (the relevant interrupt mask register) for the interrupt */
    unsigned short group = int_group(n);

    /* Find the mask for the interrupt */
    unsigned short mask = int_mask(n);
    unsigned short new_mask = PENDING_GRP0[group] | mask;
    
    /* Set the bit for the interrupt to mark it as cleared */
    PENDING_GRP0[group] = new_mask;
}   

/*
 * Enable an interrupt
 *
 * Interrupt number is made by the group number and number within the group.
 * For instance, the RTC interrupt would be 0x1F and the Channel A SOF interrupt would be 0x00.
 * And interrupt number of 0xFF specifies that all interrupts should be disabled.                                                   
 *
 * Inputs:
 * n = the number of the interrupt: n[7..4] = group number, n[3..0] = individual number.
 */
void int_enable(unsigned short n) {
    /* Find the group (the relevant interrupt mask register) for the interrupt */
    unsigned short group = int_group(n);

    /* Find the mask for the interrupt */
    unsigned short mask = int_mask(n);
    unsigned short new_mask = MASK_GRP0[group] & ~mask;

    /* Clear the mask bit for the interrupt in the correct MASK register */
    MASK_GRP0[group] = new_mask;
}


// eof
