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
#include <io.h>             /* IORD_32DIRECT */
#include "system.h"         /* BSP-generated: all _BASE defines */
#include "datatype.h"
#include "fifo_driver.h"

/* -----------------------------------------------------------------------
 * FIFO CSR diagnostic helper
 *
 * Reads fill-level (offset 0x00), status (0x04), and event (0x08)
 * from a FIFO CSR base and prints them.  "tag" is a short label such
 * as "Y_OUT" or "2to3_IN".
 * --------------------------------------------------------------------- */
static void log_fifo_csr(const char *tag, unsigned int csr_base)
{
    unsigned int fill  = IORD_32DIRECT(csr_base, FIFO_CSR_FILL_LEVEL);
    unsigned int stat  = IORD_32DIRECT(csr_base, FIFO_CSR_STATUS);
    unsigned int event = IORD_32DIRECT(csr_base, FIFO_CSR_EVENT);

    printf("  [%s] fill=%u  status=0x%02x (full=%u empty=%u)  event=0x%02x\n",
           tag, fill, stat,
           (stat & FIFO_STATUS_FULL_MASK)  ? 1u : 0u,
           (stat & FIFO_STATUS_EMPTY_MASK) ? 1u : 0u,
           event);
}

void levelshift(void)
{
    INT16  i, j;
    UINT32 mcu_count = 0;

    while (1)
    {
        mcu_count++;

        /* ---------- Log input FIFO status BEFORE reading ---------- */
        printf("S2: MCU #%lu  -- pre-read FIFO status:\n",
               (unsigned long)mcu_count);
        log_fifo_csr("Y_OUT ",  FIFO_1TO2_Y_OUT_CSR_BASE);
        log_fifo_csr("Cb_OUT",  FIFO_1TO2_CB_OUT_CSR_BASE);
        log_fifo_csr("Cr_OUT",  FIFO_1TO2_CR_OUT_CSR_BASE);
        log_fifo_csr("2to3_IN", FIFO_2TO3_IN_CSR_BASE);

        /* Y block: 64 samples */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_Y_OUT_BASE, FIFO_1TO2_Y_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Log after Y block */
        printf("S2: MCU #%lu  -- post-Y  FIFO status:\n",
               (unsigned long)mcu_count);
        log_fifo_csr("Y_OUT ",  FIFO_1TO2_Y_OUT_CSR_BASE);
        log_fifo_csr("2to3_IN", FIFO_2TO3_IN_CSR_BASE);

        /* Cb block: 64 samples */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CB_OUT_BASE, FIFO_1TO2_CB_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* Log after Cb block */
        printf("S2: MCU #%lu  -- post-Cb FIFO status:\n",
               (unsigned long)mcu_count);
        log_fifo_csr("Cb_OUT",  FIFO_1TO2_CB_OUT_CSR_BASE);
        log_fifo_csr("2to3_IN", FIFO_2TO3_IN_CSR_BASE);

        /* Cr block: 64 samples */
        for (j = 0; j < 64; j++)
        {
            i = (INT16)(fifo_read_32(FIFO_1TO2_CR_OUT_BASE, FIFO_1TO2_CR_OUT_CSR_BASE) & 0xFFFF);
            i -= 128;
            fifo_write_32(FIFO_2TO3_IN_BASE, FIFO_2TO3_IN_CSR_BASE, (unsigned int)(short)i);
        }

        /* ---------- Log output FIFO status AFTER full MCU ---------- */
        printf("S2: MCU #%lu  -- post-Cr (MCU done) FIFO status:\n",
               (unsigned long)mcu_count);
        log_fifo_csr("Cr_OUT",  FIFO_1TO2_CR_OUT_CSR_BASE);
        log_fifo_csr("2to3_IN", FIFO_2TO3_IN_CSR_BASE);
        printf("S2: MCU #%lu  complete (192 samples level-shifted)\n\n",
               (unsigned long)mcu_count);
    }
}

int main(void)
{
    /* Disable stdout buffering so printf reaches JTAG UART immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n==============================\n");
    printf("  Stage 2: Level Shift\n");
    printf("==============================\n");
    printf("FIFO base addresses (from system.h):\n");
    printf("  FIFO_1TO2_Y_OUT_BASE      = 0x%08x\n", (unsigned)FIFO_1TO2_Y_OUT_BASE);
    printf("  FIFO_1TO2_Y_OUT_CSR_BASE  = 0x%08x\n", (unsigned)FIFO_1TO2_Y_OUT_CSR_BASE);
    printf("  FIFO_1TO2_CB_OUT_BASE     = 0x%08x\n", (unsigned)FIFO_1TO2_CB_OUT_BASE);
    printf("  FIFO_1TO2_CB_OUT_CSR_BASE = 0x%08x\n", (unsigned)FIFO_1TO2_CB_OUT_CSR_BASE);
    printf("  FIFO_1TO2_CR_OUT_BASE     = 0x%08x\n", (unsigned)FIFO_1TO2_CR_OUT_BASE);
    printf("  FIFO_1TO2_CR_OUT_CSR_BASE = 0x%08x\n", (unsigned)FIFO_1TO2_CR_OUT_CSR_BASE);
    printf("  FIFO_2TO3_IN_BASE         = 0x%08x\n", (unsigned)FIFO_2TO3_IN_BASE);
    printf("  FIFO_2TO3_IN_CSR_BASE     = 0x%08x\n", (unsigned)FIFO_2TO3_IN_CSR_BASE);
    printf("\nInitial FIFO status:\n");
    log_fifo_csr("Y_OUT ",  FIFO_1TO2_Y_OUT_CSR_BASE);
    log_fifo_csr("Cb_OUT",  FIFO_1TO2_CB_OUT_CSR_BASE);
    log_fifo_csr("Cr_OUT",  FIFO_1TO2_CR_OUT_CSR_BASE);
    log_fifo_csr("2to3_IN", FIFO_2TO3_IN_CSR_BASE);
    printf("\nWaiting for data from Stage 1...\n\n");

    levelshift();
    return 0;
}
