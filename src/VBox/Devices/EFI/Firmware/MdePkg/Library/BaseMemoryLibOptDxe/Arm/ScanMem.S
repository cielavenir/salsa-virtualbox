// Copyright (c) 2010-2011, Linaro Limited
// All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// Written by Dave Gilbert <david.gilbert@linaro.org>
//
// This memchr routine is optimised on a Cortex-A9 and should work on
// all ARMv7 processors.   It has a fast past for short sizes, and has
// an optimised path for large data sets; the worst case is finding the
// match early in a large data set.
//


// 2011-02-07 david.gilbert@linaro.org
//    Extracted from local git a5b438d861
// 2011-07-14 david.gilbert@linaro.org
//    Import endianness fix from local git ea786f1b
// 2011-12-07 david.gilbert@linaro.org
//    Removed unneeded cbz from align loop

// this lets us check a flag in a 00/ff byte easily in either endianness
#define CHARTSTMASK(c) 1<<(c*8)

    .text
    .thumb
    .syntax unified

    .type ASM_PFX(InternalMemScanMem8), %function
ASM_GLOBAL ASM_PFX(InternalMemScanMem8)
ASM_PFX(InternalMemScanMem8):
    // r0 = start of memory to scan
    // r1 = length
    // r2 = character to look for
    // returns r0 = pointer to character or NULL if not found
    uxtb    r2, r2        // Don't think we can trust the caller to actually pass a char

    cmp     r1, #16       // If it's short don't bother with anything clever
    blt     20f

    tst     r0, #7        // If it's already aligned skip the next bit
    beq     10f

    // Work up to an aligned point
5:
    ldrb    r3, [r0],#1
    subs    r1, r1, #1
    cmp     r3, r2
    beq     50f           // If it matches exit found
    tst     r0, #7
    bne     5b            // If not aligned yet then do next byte

10:
    // At this point, we are aligned, we know we have at least 8 bytes to work with
    push    {r4-r7}
    orr     r2, r2, r2, lsl #8  // expand the match word across to all bytes
    orr     r2, r2, r2, lsl #16
    bic     r4, r1, #7    // Number of double words to work with
    mvns    r7, #0        // all F's
    movs    r3, #0

15:
    ldmia   r0!, {r5,r6}
    subs    r4, r4, #8
    eor     r5, r5, r2    // Get it so that r5,r6 have 00's where the bytes match the target
    eor     r6, r6, r2
    uadd8   r5, r5, r7    // Parallel add 0xff - sets the GE bits for anything that wasn't 0
    sel     r5, r3, r7    // bytes are 00 for none-00 bytes, or ff for 00 bytes - NOTE INVERSION
    uadd8   r6, r6, r7    // Parallel add 0xff - sets the GE bits for anything that wasn't 0
    sel     r6, r5, r7    // chained....bytes are 00 for none-00 bytes, or ff for 00 bytes - NOTE INVERSION
    cbnz    r6, 60f
    bne     15b           // (Flags from the subs above) If not run out of bytes then go around again

    pop     {r4-r7}
    and     r2, r2, #0xff // Get r2 back to a single character from the expansion above
    and     r1, r1, #7    // Leave the count remaining as the number after the double words have been done

20:
    cbz     r1, 40f       // 0 length or hit the end already then not found

21: // Post aligned section, or just a short call
    ldrb    r3, [r0], #1
    subs    r1, r1, #1
    eor     r3, r3, r2    // r3 = 0 if match - doesn't break flags from sub
    cbz     r3, 50f
    bne     21b           // on r1 flags

40:
    movs    r0, #0        // not found
    bx      lr

50:
    subs    r0, r0, #1    // found
    bx      lr

60: // We're here because the fast path found a hit - now we have to track down exactly which word it was
    // r0 points to the start of the double word after the one that was tested
    // r5 has the 00/ff pattern for the first word, r6 has the chained value
    subs    r0, r0, #3
    cmp     r5, #0
    it      eq
    moveq.n r5, r6        // the end is in the 2nd word
    it      ne
    subne.n r0, r0, #4    // or 2nd byte of 1st word

    // r0 currently points to the 3rd byte of the word containing the hit
    tst     r5, #CHARTSTMASK(0)     // 1st character
    bne     61f
    adds    r0, r0, #1
    tst     r5, #CHARTSTMASK(1)     // 2nd character
    bne     61f
    adds    r0, r0 ,#1
    tst     r5, #(3 << 15)          // 2nd & 3rd character
    // If not the 3rd must be the last one
    it      eq
    addeq.n r0, r0, #1

61:
    pop     {r4-r7}
    subs    r0, r0, #1
    bx      lr
