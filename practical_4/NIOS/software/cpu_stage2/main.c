/*
 * Stage 2 — Level Shift (subtract 128 from each sample)
 *
 * Ported from jencoder1b/main.c (Tensilica Xtensa) to Nios II.
 * Replaces RECV1/RECV2/RECV3/SEND with fifo_read_32/fifo_write_32.
 *
 * Data flow per MCU:
 *   Read  64 Y  from fifo_1to2_Y,  subtract 128, write to fifo_2to3
 *   Read  64 Cb from fifo_1to2_Cb, subtract 128, write to fifo_2to3
 *   Read  64 Cr from fifo_1to2_Cr, subtract 128, write to fifo_2to3
 *
 * Runs in an infinite loop — Stage 1 keeps feeding MCUs.
 */

#include <stdio.h>
#include <io.h>
#include "system.h"
#include "datatype.h"
#include "fifo_driver.h"

/* Print FIFO fill level and status for one CSR */
static void lf(const char *t, unsigned int c)
{
    printf(" %s:%u,%x\n", t,
           IORD_32DIRECT(c, FIFO_CSR_FILL_LEVEL),
           IORD_32DIRECT(c, FIFO_CSR_STATUS));
}

static UINT32 drain_fifo_nonblocking(unsigned int data_base, unsigned int csr_base)
{
    UINT32 drained = 0;

    while ((IORD_32DIRECT(csr_base, FIFO_CSR_STATUS) & FIFO_STATUS_EMPTY_MASK) == 0)
    {
        (void)IORD_32DIRECT(data_base, 0);
        drained++;
    }

    return drained;
}

static void flush_startup_fifos(void)
{
    printf("S2 flush Y=%lu Cb=%lu Cr=%lu\n",
           (unsigned long)drain_fifo_nonblocking(FIFO_1TO2_Y_OUT_BASE, FIFO_1TO2_Y_OUT_CSR_BASE),
           (unsigned long)drain_fifo_nonblocking(FIFO_1TO2_CB_OUT_BASE, FIFO_1TO2_CB_OUT_CSR_BASE),
           (unsigned long)drain_fifo_nonblocking(FIFO_1TO2_CR_OUT_BASE, FIFO_1TO2_CR_OUT_CSR_BASE));
}

void levelshift(void)
{
    INT16 i, j;
    UINT32 mc = 0;

    while (1)
    {
        mc++;

        /* Log before reading this MCU */
        printf("S2 M%lu>\n", (unsigned long)mc);
        lf("Y", FIFO_1TO2_Y_OUT_CSR_BASE);
        lf("Cb", FIFO_1TO2_CB_OUT_CSR_BASE);
        lf("Cr", FIFO_1TO2_CR_OUT_CSR_BASE);
        lf("W", FIFO_2TO3_IN_CSR_BASE);

        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_Y_OUT_BASE, FIFO_1TO2_Y_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CB_OUT_BASE, FIFO_1TO2_CB_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CR_OUT_BASE, FIFO_1TO2_CR_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Log after MCU done */
        printf("S2 M%lu ok\n", (unsigned long)mc);
        lf("W", FIFO_2TO3_IN_CSR_BASE);
    }
}

int main(void)
{
    printf("S2:LvlShift\n");
    flush_startup_fifos();
    lf("Y", FIFO_1TO2_Y_OUT_CSR_BASE);
    lf("Cb", FIFO_1TO2_CB_OUT_CSR_BASE);
    lf("Cr", FIFO_1TO2_CR_OUT_CSR_BASE);
    lf("W", FIFO_2TO3_IN_CSR_BASE);
    levelshift();
    return 0;
}
