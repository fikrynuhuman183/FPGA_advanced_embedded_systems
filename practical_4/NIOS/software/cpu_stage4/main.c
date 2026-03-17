/*
 * Stage 4 — Quantization
 *
 * Ported from jencoder1d/quant.c (Tensilica Xtensa) to Nios II.
 * Replaces RECV1/RECV2/SEND with fifo_read_32/fifo_write_32.
 *
 * Data flow:
 *   Initialization:
 *     - Read num_mcus + quality_factor from fifo_1to4_param (was RECV1)
 *     - Build Lqt/Cqt/ILqt/ICqt tables
 *     - Send Lqt[64] then Cqt[64] to Stage 5 via fifo_4to5 (was SEND)
 *   Per MCU (3 blocks):
 *     - Read  64 DCT coeffs from fifo_3to4 (was RECV2)
 *     - Quantize and zigzag reorder
 *     - Write 64 quantized coeffs to fifo_4to5 (was SEND)
 */

#include <stdio.h>
#include "common/datatype.h"
#include "common/config.h"
#include "system.h"
#include "common/fifo_driver.h"

/* Zigzag reordering table */
static UINT8 zigzag_table[] =
{
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

UINT8  Lqt[BLOCK_SIZE];
UINT8  Cqt[BLOCK_SIZE];
UINT16 ILqt[BLOCK_SIZE];
UINT16 ICqt[BLOCK_SIZE];
INT16  Temp[BLOCK_SIZE];

static void drain_fifo_nonblocking(unsigned int data_base, unsigned int csr_base)
{
    while ((IORD_32DIRECT(csr_base, FIFO_CSR_STATUS) & FIFO_STATUS_EMPTY_MASK) == 0)
    {
        (void)IORD_32DIRECT(data_base, 0);
    }
}

static void flush_startup_fifos(void)
{
    drain_fifo_nonblocking(FIFO_1TO4_PARAM_OUT_BASE, FIFO_1TO4_PARAM_OUT_CSR_BASE);
    drain_fifo_nonblocking(FIFO_3TO4_OUT_BASE, FIFO_3TO4_OUT_CSR_BASE);
}

/* 16-step division for Q.15 format data */
UINT16 DSP_Division(UINT32 numer, UINT32 denom)
{
    UINT16 i;

    denom <<= 15;

    for (i = 16; i > 0; i--)
    {
        if (numer > denom)
        {
            numer -= denom;
            numer <<= 1;
            numer++;
        }
        else
            numer <<= 1;
    }

    return (UINT16)numer;
}

/* Build quantization tables from quality factor */
void initialize_quantization_tables(void)
{
    UINT16 i, index;
    UINT32 value;

    static UINT8 luminance_quant_table[] =
    {
        16, 11, 10, 16,  24,  40,  51,  61,
        12, 12, 14, 19,  26,  58,  60,  55,
        14, 13, 16, 24,  40,  57,  69,  56,
        14, 17, 22, 29,  51,  87,  80,  62,
        18, 22, 37, 56,  68, 109, 103,  77,
        24, 35, 55, 64,  81, 104, 113,  92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103,  99
    };

    static UINT8 chrominance_quant_table[] =
    {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
    };

    /* Read quality factor from parameter FIFO (was RECV1) */
    UINT32 quality_factor = fifo_read_32(FIFO_1TO4_PARAM_OUT_BASE, FIFO_1TO4_PARAM_OUT_CSR_BASE);

    for (i = 0; i < 64; i++)
    {
        index = zigzag_table[i];

        /* Luminance quantization table * quality factor */
        value = luminance_quant_table[i] * quality_factor;
        value = (value + 0x200) >> 10;

        if (value == 0)
            value = 1;
        else if (value > 255)
            value = 255;

        Lqt[index] = (UINT8)value;
        ILqt[i] = DSP_Division(0x8000, value);

        /* Chrominance quantization table * quality factor */
        value = chrominance_quant_table[i] * quality_factor;
        value = (value + 0x200) >> 10;

        if (value == 0)
            value = 1;
        else if (value > 255)
            value = 255;

        Cqt[index] = (UINT8)value;
        ICqt[i] = DSP_Division(0x8000, value);
    }

    /* Send Lqt[64] then Cqt[64] downstream to Stage 5 (was SEND) */
    for (i = 0; i < 64; i++)
        fifo_write_32(FIFO_4TO5_IN_BASE, FIFO_4TO5_IN_CSR_BASE, (unsigned int)Lqt[i]);

    for (i = 0; i < 64; i++)
        fifo_write_32(FIFO_4TO5_IN_BASE, FIFO_4TO5_IN_CSR_BASE, (unsigned int)Cqt[i]);
}

/* Quantize one 8x8 block with zigzag reordering */
void quantization(UINT16 *const quant_table_ptr)
{
    INT16 i;
    INT32 value;

    for (i = 0; i <= 63; i++)
    {
        /* Read one DCT coefficient from fifo_3to4 (was RECV2) */
        value = (INT16)(fifo_read_32(FIFO_3TO4_OUT_BASE, FIFO_3TO4_OUT_CSR_BASE) & 0xFFFF)
                * quant_table_ptr[i];
        value = (value + 0x4000) >> 15;

        Temp[zigzag_table[i]] = (INT16)value;
    }

    /* Write quantized + zigzag-reordered block to fifo_4to5 (was SEND) */
    for (i = 0; i <= 63; i++)
    {
        fifo_write_32(FIFO_4TO5_IN_BASE, FIFO_4TO5_IN_CSR_BASE, (unsigned int)(short)Temp[i]);
    }
}

int main(void)
{
    printf("Stage 4: Quantization started\n");
    flush_startup_fifos();

    while (1)
    {
        /* Read MCU count from parameter FIFO (was RECV1) */
        UINT32 count = fifo_read_32(FIFO_1TO4_PARAM_OUT_BASE, FIFO_1TO4_PARAM_OUT_CSR_BASE);

        /* Build quantization tables (reads quality_factor from same FIFO) */
        initialize_quantization_tables();

        /* Process all MCUs: 3 blocks each (Y=ILqt, Cb=ICqt, Cr=ICqt) */
        for (; count > 0; count--)
        {
            quantization(ILqt);
            quantization(ICqt);
            quantization(ICqt);
        }

        break;  /* single-image mode; remove for continuous */
    }

    printf("Stage 4: Done!\n");
    return 0;
}
