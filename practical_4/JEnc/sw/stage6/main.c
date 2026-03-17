/*
 * Stage 6 — File Output (Write JPEG byte stream to file)
 *
 * Ported from jencoder1f/main.c (Tensilica Xtensa) to Nios II.
 * Replaces RECV1/RECV2/RECV3 with fifo_read_32.
 *
 * Data flow:
 *   1. Read filename characters from fifo_1to6_fname (was RECV1)
 *   2. Open output file
 *   3. Read encoded bytes from fifo_5to6_data (was RECV2)
 *      until EOI marker (0xFF 0xD9) is detected
 *   4. Close file
 *
 * NOTE: File I/O uses Nios II HAL (fopen/fputc). For a first test
 * without actual file I/O, you can count bytes and printf the count.
 * For production, write to an SDRAM buffer and transfer to host.
 */

#include <stdio.h>
#include <stdlib.h>
#include "datatype.h"
#include "system.h"
#include "fifo_driver.h"

int main(void)
{
    FILE *fptr;
    UINT8 out;
    INT8 filename[50];
    UINT8 idx;

    printf("Stage 6: File Output started\n");

    while (1)
    {
        idx = 0;

        /* Read filename from fifo_1to6_fname, char by char (was RECV1) */
        filename[idx] = (INT8)(fifo_read_32(FIFO_1TO6_FNAME_OUT_BASE, FIFO_1TO6_FNAME_OUT_CSR_BASE) & 0xFF);

        /* If first char is NUL, that signals end of all files */
        if (filename[idx] == '\0')
        {
            printf("Stage 6: No more files, exiting\n");
            break;
        }

        while (filename[idx] != '\0')
        {
            idx++;
            filename[idx] = (INT8)(fifo_read_32(FIFO_1TO6_FNAME_OUT_BASE, FIFO_1TO6_FNAME_OUT_CSR_BASE) & 0xFF);
        }

        /* Append .jpg extension */
        filename[idx++] = '.';
        filename[idx++] = 'j';
        filename[idx++] = 'p';
        filename[idx++] = 'g';
        filename[idx]   = '\0';

        fptr = fopen(filename, "wb");
        if (!fptr) {
            printf("ERROR: Cannot open %s for writing\n", filename);
            /* Still need to drain the data FIFO to avoid deadlock */
            while (1) {
                out = (UINT8)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE, FIFO_5TO6_DATA_OUT_CSR_BASE) & 0xFF);
                if (out == 0xFF) {
                    out = (UINT8)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE, FIFO_5TO6_DATA_OUT_CSR_BASE) & 0xFF);
                    if (out == 0xD9) break;
                }
            }
            continue;
        }

        printf("Stage 6: Output file: %s\n", filename);

        /* Read encoded bytes and write to file until EOI marker */
        while (1)
        {
            out = (UINT8)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE, FIFO_5TO6_DATA_OUT_CSR_BASE) & 0xFF);
            fputc(out, fptr);
            if (out == 0xFF)
            {
                out = (UINT8)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE, FIFO_5TO6_DATA_OUT_CSR_BASE) & 0xFF);
                fputc(out, fptr);
                if (out == 0xD9)
                    break;  /* EOI marker detected */
            }
        }

        fclose(fptr);
        printf("Stage 6: Done writing %s\n", filename);
        break;  /* single-image mode; remove for continuous */
    }

    printf("Stage 6: Finished!\n");
    return 0;
}
