/*
 * Stage 3 — 2D DCT (8x8 Discrete Cosine Transform)
 *
 * Ported from jencoder1c/dct.c (Tensilica Xtensa) to Nios II.
 * Replaces RECV/SEND with fifo_read_32/fifo_write_32.
 *
 * Data flow per block:
 *   Read  64 INT16 from fifo_2to3  (level-shifted samples)
 *   Perform 8-point row DCT, then 8-point column DCT
 *   Write 64 INT16 to fifo_3to4    (DCT coefficients)
 *
 * Called 3 times per MCU (Y, Cb, Cr).
 * Uses a static array instead of malloc for embedded safety.
 */

#include <stdio.h>
#include "common/datatype.h"
#include "system.h"
#include "common/fifo_driver.h"

static INT16 dct_data[64];
static INT16 *data;

static void drain_fifo_nonblocking(unsigned int data_base, unsigned int csr_base)
{
    while ((IORD_32DIRECT(csr_base, FIFO_CSR_STATUS) & FIFO_STATUS_EMPTY_MASK) == 0)
    {
        (void)IORD_32DIRECT(data_base, 0);
    }
}

static void flush_startup_fifos(void)
{
    drain_fifo_nonblocking(FIFO_2TO3_OUT_BASE, FIFO_2TO3_OUT_CSR_BASE);
}

void DCT(void)
{
    UINT16 i;
    INT32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

    /* All values are shifted left by 10 and rounded off to nearest integer */
    static const UINT16 c1 = 1420;  /* cos PI/16 * root(2)  */
    static const UINT16 c2 = 1338;  /* cos PI/8  * root(2)  */
    static const UINT16 c3 = 1204;  /* cos 3PI/16 * root(2) */
    static const UINT16 c5 = 805;   /* cos 5PI/16 * root(2) */
    static const UINT16 c6 = 554;   /* cos 3PI/8  * root(2) */
    static const UINT16 c7 = 283;   /* cos 7PI/16 * root(2) */

    static const UINT16 s1 = 3;
    static const UINT16 s2 = 10;
    static const UINT16 s3 = 13;

    data = dct_data;

    /* Read 64 values from input FIFO */
    for (i = 0; i < 64; i++)
    {
        data[i] = (INT16)(fifo_read_32(FIFO_2TO3_OUT_BASE, FIFO_2TO3_OUT_CSR_BASE) & 0xFFFF);
    }

    /* Row DCT: 8 rows of 8 elements */
    for (i = 8; i > 0; i--)
    {
        x8 = data[0] + data[7];
        x0 = data[0] - data[7];

        x7 = data[1] + data[6];
        x1 = data[1] - data[6];

        x6 = data[2] + data[5];
        x2 = data[2] - data[5];

        x5 = data[3] + data[4];
        x3 = data[3] - data[4];

        x4 = x8 + x5;
        x8 -= x5;

        x5 = x7 + x6;
        x7 -= x6;

        data[0] = (INT16)(x4 + x5);
        data[4] = (INT16)(x4 - x5);

        data[2] = (INT16)((x8 * c2 + x7 * c6) >> s2);
        data[6] = (INT16)((x8 * c6 - x7 * c2) >> s2);

        data[7] = (INT16)((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> s2);
        data[5] = (INT16)((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> s2);
        data[3] = (INT16)((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> s2);
        data[1] = (INT16)((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> s2);

        data += 8;
    }

    data -= 64;

    /* Column DCT: 8 columns of 8 elements (stride = 8) */
    for (i = 8; i > 0; i--)
    {
        x8 = data[0] + data[56];
        x0 = data[0] - data[56];

        x7 = data[8] + data[48];
        x1 = data[8] - data[48];

        x6 = data[16] + data[40];
        x2 = data[16] - data[40];

        x5 = data[24] + data[32];
        x3 = data[24] - data[32];

        x4 = x8 + x5;
        x8 -= x5;

        x5 = x7 + x6;
        x7 -= x6;

        data[0]  = (INT16)((x4 + x5) >> s1);
        data[32] = (INT16)((x4 - x5) >> s1);

        data[16] = (INT16)((x8 * c2 + x7 * c6) >> s3);
        data[48] = (INT16)((x8 * c6 - x7 * c2) >> s3);

        data[56] = (INT16)((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> s3);
        data[40] = (INT16)((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> s3);
        data[24] = (INT16)((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> s3);
        data[8]  = (INT16)((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> s3);

        data++;
    }

    data -= 8;

    /* Write 64 DCT coefficients to output FIFO */
    for (i = 0; i < 64; i++)
    {
        fifo_write_32(FIFO_3TO4_IN_BASE, FIFO_3TO4_IN_CSR_BASE, (unsigned int)(short)data[i]);
    }
}

int main(void)
{
    printf("Stage 3: DCT started\n");
    flush_startup_fifos();

    while (1)
    {
        /* 3 blocks per MCU: Y, Cb, Cr */
        DCT();
        DCT();
        DCT();
    }

    return 0;
}
