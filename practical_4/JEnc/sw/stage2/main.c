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

#include <io.h>             /* IORD_32DIRECT */
#include "sys/alt_stdio.h"  /* alt_printf — tiny, no full libc printf */
#include "system.h"         /* BSP-generated: all _BASE defines */
#include "datatype.h"
#include "fifo_driver.h"

/*
 * Lightweight FIFO CSR log — uses alt_printf (%x only, no %u/%d).
 * Prints: tag fill status event   (all hex)
 */
static void logf(const char *tag, unsigned int csr)
{
    unsigned int f = IORD_32DIRECT(csr, FIFO_CSR_FILL_LEVEL);
    unsigned int s = IORD_32DIRECT(csr, FIFO_CSR_STATUS);
    unsigned int e = IORD_32DIRECT(csr, FIFO_CSR_EVENT);
    alt_printf(" %s f:%x s:%x e:%x\n", tag, f, s, e);
}

void levelshift(void)
{
    INT16  i, j;
    UINT32 mc = 0;

    while (1)
    {
        mc++;

        /* Pre-read status */
        alt_printf("S2 M%x rd\n", mc);
        logf("Yi", FIFO_1TO2_Y_OUT_CSR_BASE);
        logf("Cb", FIFO_1TO2_CB_OUT_CSR_BASE);
        logf("Cr", FIFO_1TO2_CR_OUT_CSR_BASE);
        logf("Wo", FIFO_2TO3_IN_CSR_BASE);

        /* Y block */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_Y_OUT_BASE, FIFO_1TO2_Y_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Cb block */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CB_OUT_BASE, FIFO_1TO2_CB_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Cr block */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CR_OUT_BASE, FIFO_1TO2_CR_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Post-MCU status */
        alt_printf("S2 M%x ok\n", mc);
        logf("Wo", FIFO_2TO3_IN_CSR_BASE);
    }
}

int main(void)
{
    alt_putstr("\nS2:LevelShift\n");
    alt_printf("Y_o=%x Y_oc=%x\n",  FIFO_1TO2_Y_OUT_BASE,  FIFO_1TO2_Y_OUT_CSR_BASE);
    alt_printf("Cb_o=%x Cb_oc=%x\n", FIFO_1TO2_CB_OUT_BASE, FIFO_1TO2_CB_OUT_CSR_BASE);
    alt_printf("Cr_o=%x Cr_oc=%x\n", FIFO_1TO2_CR_OUT_BASE, FIFO_1TO2_CR_OUT_CSR_BASE);
    alt_printf("W=%x Wc=%x\n",       FIFO_2TO3_IN_BASE,     FIFO_2TO3_IN_CSR_BASE);
    alt_putstr("Init:\n");
    logf("Yi", FIFO_1TO2_Y_OUT_CSR_BASE);
    logf("Cb", FIFO_1TO2_CB_OUT_CSR_BASE);
    logf("Cr", FIFO_1TO2_CR_OUT_CSR_BASE);
    logf("Wo", FIFO_2TO3_IN_CSR_BASE);
    alt_putstr("Waiting S1...\n");

    levelshift();
    return 0;
}
