/*
 * Stage 1 — RGB → YCbCr + Parameter Distribution
 *
 * Ported from jencoder1a (Tensilica Xtensa) to Nios II.
 *
 * Reads a BMP image using bmp_extract(), converts each 8x8 MCU from
 * RGB to YCbCr, and pushes the data into the downstream FIFOs.
 *
 * FIFO mapping (SEND → fifo_write_32):
 *   SEND1(Y)              → fifo_write_32(FIFO_1TO2_Y_IN_BASE,       FIFO_1TO2_Y_IN_CSR_BASE,       ...)
 *   SEND2(Cb)             → fifo_write_32(FIFO_1TO2_CB_IN_BASE,      FIFO_1TO2_CB_IN_CSR_BASE,      ...)
 *   SEND3(Cr)             → fifo_write_32(FIFO_1TO2_CR_IN_BASE,      FIFO_1TO2_CR_IN_CSR_BASE,      ...)
 *   SEND4(num_mcus/qual)  → fifo_write_32(FIFO_1TO4_PARAM_IN_BASE,   FIFO_1TO4_PARAM_IN_CSR_BASE,   ...)
 *   SEND5(w/h/num_mcus)   → fifo_write_32(FIFO_1TO5_PARAM_IN_BASE,   FIFO_1TO5_PARAM_IN_CSR_BASE,   ...)
 *   SEND6(ch)             → fifo_write_32(FIFO_1TO6_FNAME_IN_BASE,   FIFO_1TO6_FNAME_IN_CSR_BASE,   ...)
 *
 * Base addresses are taken from the BSP-generated system.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sys/alt_stdio.h"
#include "system.h"         /* BSP-generated: all _BASE / _CSR_BASE defines */

#include "./common/datatype.h"
#include "./common/jdatatype.h"
#include "./common/config.h"
#include "./common/fifo_driver.h"

#include "bmp_extract.h"

/* -----------------------------------------------------------------------
 * Global arrays (same role as in reference encoder.c / readYUV.c).
 * Only Y1/CB/CR are needed; Y2/Y3/Y4 are for 4:2:0 / 4:2:2 which are not
 * used here.
 * --------------------------------------------------------------------- */
INT16  Y1[BLOCK_SIZE];
INT16  CB[BLOCK_SIZE];
INT16  CR[BLOCK_SIZE];

/* Function pointer — set to read_444_format in initialization() */
void (*read_format)(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure, UINT8 *input_ptr);

/* -----------------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------------- */
static void   initialization(JPEG_ENCODER_STRUCTURE *jpeg,
                              UINT32 image_format,
                              UINT32 image_width,
                              UINT32 image_height);

static UINT8 *encode_image(UINT8 *input_ptr, UINT8 *output_ptr,
                            UINT32 quality_factor,
                            UINT32 image_format,
                            UINT32 image_width,
                            UINT32 image_height);

static void   read_444_format(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure,
                               UINT8 *input_ptr);

static void print_fifo_status(const char *tag, unsigned int csr_base)
{
    printf("%s fill=%lu status=0x%08lx\n",
           tag,
           (unsigned long)IORD_32DIRECT(csr_base, FIFO_CSR_FILL_LEVEL),
           (unsigned long)IORD_32DIRECT(csr_base, FIFO_CSR_STATUS));
}

static void flush_startup_fifos(void)
{
    /* Stage 1 is producer-only; output FIFOs can only be drained by consumers. */
    printf("S1 flush: producer-only stage, no local FIFO drain\n");
}

/* -----------------------------------------------------------------------
 * initialization()
 *
 * Mirrors encoder.c:initialization() from jencoder1a.
 * Hardcoded to 4:4:4 (RGB) — only 8×8 MCUs, 3 bytes/pixel.
 * --------------------------------------------------------------------- */
static void initialization(JPEG_ENCODER_STRUCTURE *jpeg,
                            UINT32 image_format,
                            UINT32 image_width,
                            UINT32 image_height)
{
    UINT16 mcu_width  = 8;
    UINT16 mcu_height = 8;

    (void)image_format; /* always 4:4:4 for BMP */

    jpeg->mcu_width  = mcu_width;
    jpeg->mcu_height = mcu_height;

    jpeg->horizontal_mcus = (UINT16)((image_width  + mcu_width  - 1) >> 3);
    jpeg->vertical_mcus   = (UINT16)((image_height + mcu_height - 1) >> 3);

    read_format = read_444_format;

    jpeg->rows_in_bottom_mcus = (UINT16)(image_height -
                                 (jpeg->vertical_mcus - 1) * mcu_height);
    jpeg->cols_in_right_mcus  = (UINT16)(image_width  -
                                 (jpeg->horizontal_mcus - 1) * mcu_width);

    jpeg->length_minus_mcu_width = (UINT16)((image_width - mcu_width) * 3);
    jpeg->length_minus_width     = (UINT16)((image_width - jpeg->cols_in_right_mcus) * 3);

    jpeg->mcu_width_size = (UINT16)(mcu_width * 3);

    /* Advance needed at end of each row of MCUs to skip to next MCU row */
    jpeg->offset = (UINT16)((image_width * (mcu_height - 1) -
                   (mcu_width - jpeg->cols_in_right_mcus)) * 3);

    jpeg->ldc1 = 0;
    jpeg->ldc2 = 0;
    jpeg->ldc3 = 0;
}

/* -----------------------------------------------------------------------
 * read_444_format()
 *
 * Mirrors readYUV.c:read_444_format() from jencoder1a.
 *
 * Reads one 8×8 MCU from the raw (post-bmp_rearrange_comps) RGB bitmap,
 * converts each pixel to YCbCr, and pushes 64 values for each component
 * into the corresponding FIFOs (was SEND1/SEND2/SEND3).
 *
 * Edge MCUs (right / bottom) are padded by replicating the last pixel.
 * --------------------------------------------------------------------- */
static void read_444_format(JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure,
                             UINT8 *input_ptr)
{
    INT32  i, j;
    UINT8  R, G, B;
    INT32  Y, Cb, Cr;

    UINT16 rows = jpeg_encoder_structure->rows;
    UINT16 cols = jpeg_encoder_structure->cols;
    UINT16 incr = jpeg_encoder_structure->incr;

    /* ---- Process valid rows ---- */
    for (i = rows; i > 0; i--)
    {
        /* Valid columns in this row */
        for (j = cols; j > 0; j--)
        {
            R = (UINT8)*input_ptr++;
            G = (UINT8)*input_ptr++;
            B = (UINT8)*input_ptr++;

            Y  =  (77 * R + 150 * G +  29 * B) >> 8;
            Cb = ((-43 * R -  85 * G + 128 * B) >> 8) + 128;
            Cr = ((128 * R - 107 * G -  21 * B) >> 8) + 128;

            if (Y  < 0)   Y  = 0;   else if (Y  > 255) Y  = 255;
            if (Cb < 0)   Cb = 0;   else if (Cb > 255) Cb = 255;
            if (Cr < 0)   Cr = 0;   else if (Cr > 255) Cr = 255;

            /* was SEND1(Y), SEND2(Cb), SEND3(Cr) */
            fifo_write_32(FIFO_1TO2_Y_IN_BASE,
                          FIFO_1TO2_Y_IN_CSR_BASE,
                          (unsigned int)Y);
            fifo_write_32(FIFO_1TO2_CB_IN_BASE,
                          FIFO_1TO2_CB_IN_CSR_BASE,
                          (unsigned int)Cb);
            fifo_write_32(FIFO_1TO2_CR_IN_BASE,
                          FIFO_1TO2_CR_IN_CSR_BASE,
                          (unsigned int)Cr);
        }

        /* Pad remaining columns (right-edge MCU) by replicating last pixel */
        j = 8 - cols;
        if (j > 0)
        {
            R = (UINT8)*(input_ptr - 3);
            G = (UINT8)*(input_ptr - 2);
            B = (UINT8)*(input_ptr - 1);

            Y  =  (77 * R + 150 * G +  29 * B) >> 8;
            Cb = ((-43 * R -  85 * G + 128 * B) >> 8) + 128;
            Cr = ((128 * R - 107 * G -  21 * B) >> 8) + 128;

            if (Y  < 0)   Y  = 0;   else if (Y  > 255) Y  = 255;
            if (Cb < 0)   Cb = 0;   else if (Cb > 255) Cb = 255;
            if (Cr < 0)   Cr = 0;   else if (Cr > 255) Cr = 255;

            for (; j > 0; j--)
            {
                fifo_write_32(FIFO_1TO2_Y_IN_BASE,
                              FIFO_1TO2_Y_IN_CSR_BASE,
                              (unsigned int)Y);
                fifo_write_32(FIFO_1TO2_CB_IN_BASE,
                              FIFO_1TO2_CB_IN_CSR_BASE,
                              (unsigned int)Cb);
                fifo_write_32(FIFO_1TO2_CR_IN_BASE,
                              FIFO_1TO2_CR_IN_CSR_BASE,
                              (unsigned int)Cr);
            }
        }

        input_ptr += incr;
    }

    /* ---- Pad remaining rows (bottom-edge MCU) by replicating last row ---- */
    /*
     * After the loop above, input_ptr has been advanced by incr one extra
     * time. Step it back so it points just past the last valid row.
     */
    input_ptr -= incr;

    for (i = 8 - rows; i > 0; i--)
    {
        /* Re-read the last valid row from the buffer */
        UINT8 *row_start = input_ptr - (cols * 3);

        for (j = 0; j < (INT32)cols; j++)
        {
            R = (UINT8)row_start[j * 3 + 0];
            G = (UINT8)row_start[j * 3 + 1];
            B = (UINT8)row_start[j * 3 + 2];

            Y  =  (77 * R + 150 * G +  29 * B) >> 8;
            Cb = ((-43 * R -  85 * G + 128 * B) >> 8) + 128;
            Cr = ((128 * R - 107 * G -  21 * B) >> 8) + 128;

            if (Y  < 0)   Y  = 0;   else if (Y  > 255) Y  = 255;
            if (Cb < 0)   Cb = 0;   else if (Cb > 255) Cb = 255;
            if (Cr < 0)   Cr = 0;   else if (Cr > 255) Cr = 255;

            fifo_write_32(FIFO_1TO2_Y_IN_BASE,
                          FIFO_1TO2_Y_IN_CSR_BASE,
                          (unsigned int)Y);
            fifo_write_32(FIFO_1TO2_CB_IN_BASE,
                          FIFO_1TO2_CB_IN_CSR_BASE,
                          (unsigned int)Cb);
            fifo_write_32(FIFO_1TO2_CR_IN_BASE,
                          FIFO_1TO2_CR_IN_CSR_BASE,
                          (unsigned int)Cr);
        }

        /* Pad right-edge columns within this padded row too */
        j = 8 - cols;
        if (j > 0)
        {
            R = (UINT8)row_start[(cols - 1) * 3 + 0];
            G = (UINT8)row_start[(cols - 1) * 3 + 1];
            B = (UINT8)row_start[(cols - 1) * 3 + 2];

            Y  =  (77 * R + 150 * G +  29 * B) >> 8;
            Cb = ((-43 * R -  85 * G + 128 * B) >> 8) + 128;
            Cr = ((128 * R - 107 * G -  21 * B) >> 8) + 128;

            if (Y  < 0)   Y  = 0;   else if (Y  > 255) Y  = 255;
            if (Cb < 0)   Cb = 0;   else if (Cb > 255) Cb = 255;
            if (Cr < 0)   Cr = 0;   else if (Cr > 255) Cr = 255;

            for (; j > 0; j--)
            {
                fifo_write_32(FIFO_1TO2_Y_IN_BASE,
                              FIFO_1TO2_Y_IN_CSR_BASE,
                              (unsigned int)Y);
                fifo_write_32(FIFO_1TO2_CB_IN_BASE,
                              FIFO_1TO2_CB_IN_CSR_BASE,
                              (unsigned int)Cb);
                fifo_write_32(FIFO_1TO2_CR_IN_BASE,
                              FIFO_1TO2_CR_IN_CSR_BASE,
                              (unsigned int)Cr);

                print_fifo_status("S1 W:Y ", FIFO_1TO2_Y_IN_CSR_BASE);
                print_fifo_status("S1 W:Cb", FIFO_1TO2_CB_IN_CSR_BASE);
                print_fifo_status("S1 W:Cr", FIFO_1TO2_CR_IN_CSR_BASE);
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * encode_image()
 *
 * Mirrors encoder.c:encode_image() from jencoder1a.
 *
 * 1. Initialises the JPEG_ENCODER_STRUCTURE.
 * 2. Pushes control parameters to Stage 4 (num_mcus, quality) and
 *    Stage 5 (width, height, num_mcus)  — was SEND4/SEND5.
 * 3. Iterates over all MCUs calling read_format() for each one.
 * --------------------------------------------------------------------- */
static UINT8 *encode_image(UINT8 *input_ptr, UINT8 *output_ptr,
                            UINT32 quality_factor,
                            UINT32 image_format,
                            UINT32 image_width,
                            UINT32 image_height)
{
    UINT16 i, j;
    JPEG_ENCODER_STRUCTURE JpegStruct;
    JPEG_ENCODER_STRUCTURE *jpeg_encoder_structure = &JpegStruct;
    UINT32 num_mcus;

    if (image_format == RGB)
        image_format = FOUR_FOUR_FOUR;

    /* Set up encoder structure and read_format function pointer */
    initialization(jpeg_encoder_structure, image_format, image_width, image_height);

    num_mcus = (UINT32)(jpeg_encoder_structure->vertical_mcus) *
               (UINT32)(jpeg_encoder_structure->horizontal_mcus);

    /* ---- Send parameters to Stage 4 (was SEND4) ---- */
    fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, FIFO_1TO4_PARAM_IN_CSR_BASE,
                  num_mcus);
    fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, FIFO_1TO4_PARAM_IN_CSR_BASE,
                  quality_factor);

    /* ---- Send parameters to Stage 5 (was SEND5) ---- */
    fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, FIFO_1TO5_PARAM_IN_CSR_BASE,
                  image_width);
    fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, FIFO_1TO5_PARAM_IN_CSR_BASE,
                  image_height);
    fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, FIFO_1TO5_PARAM_IN_CSR_BASE,
                  num_mcus);

    /* ---- Process all MCUs ---- */
    for (i = 1; i <= jpeg_encoder_structure->vertical_mcus; i++)
    {
        /* Number of valid rows in this MCU row */
        if (i < jpeg_encoder_structure->vertical_mcus)
            jpeg_encoder_structure->rows = jpeg_encoder_structure->mcu_height;
        else
            jpeg_encoder_structure->rows = jpeg_encoder_structure->rows_in_bottom_mcus;

        for (j = 1; j <= jpeg_encoder_structure->horizontal_mcus; j++)
        {
            /* Number of valid columns in this MCU */
            if (j < jpeg_encoder_structure->horizontal_mcus)
            {
                jpeg_encoder_structure->cols = jpeg_encoder_structure->mcu_width;
                jpeg_encoder_structure->incr = jpeg_encoder_structure->length_minus_mcu_width;
            }
            else
            {
                jpeg_encoder_structure->cols = jpeg_encoder_structure->cols_in_right_mcus;
                jpeg_encoder_structure->incr = jpeg_encoder_structure->length_minus_width;
            }

            /* Convert this MCU block and push via FIFOs */
            read_format(jpeg_encoder_structure, input_ptr);

            /* Advance to the next MCU in this row */
            input_ptr += jpeg_encoder_structure->mcu_width_size;
        }

        /* Advance past the rows consumed by this MCU row */
        input_ptr += jpeg_encoder_structure->offset;
    }

    return output_ptr;
}

/* -----------------------------------------------------------------------
 * main()
 *
 * Prompts the user for a BMP filename and JPEG quality over JTAG UART,
 * reads the BMP via the host filesystem using bmp_extract(), sends the
 * output filename to Stage 6, then calls encode_image() to push all
 * YCbCr MCU data and parameters into the downstream FIFOs.
 *
 * The BMP file must be accessible to the Nios II host filesystem
 * (typically placed in the "files/" sub-directory of the application
 * project and referenced as "/mnt/host/files/<name>.bmp").
 * --------------------------------------------------------------------- */
int main(void)
{
    char     input_filename[150];
    char     filepath[200];
    char     quality_str[16];
    char    *nl;
    Bmp_data pic_data;
    UINT8   *output_ptr = NULL;
    UINT32   quality_factor;
    const char *name_ptr;

    /* Disable stdout buffering so printf output reaches JTAG UART immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n==============================\n");
    printf("  JPEG MPSoC - Stage 1\n");
    printf("  RGB -> YCbCr + Param Push\n");
    printf("==============================\n\n");

    flush_startup_fifos();

    while (1)
    {
        /* ---- Prompt for BMP filename ---- */
        printf("Enter BMP filename (without path, e.g. image.bmp), or 'quit': ");
        if (fgets(input_filename, sizeof(input_filename), stdin) == NULL)
            break;
        nl = strchr(input_filename, '\n');
        if (nl) *nl = '\0';

        if (strcmp(input_filename, "quit") == 0 ||
            strcmp(input_filename, "exit") == 0)
        {
            break;
        }

        /* ---- Prompt for JPEG quality factor ---- */
        printf("Enter JPEG quality (1-100): ");
        if (fgets(quality_str, sizeof(quality_str), stdin) == NULL)
            break;
        quality_factor = (UINT32)atoi(quality_str);
        if (quality_factor < 1 || quality_factor > 100)
        {
            quality_factor = 50;
            printf("Invalid quality; defaulting to 50.\n");
        }

        /*
         * Build the full path for the host filesystem.
         * Files should be placed in the "files/" directory of the Nios II
         * application project; Nios II host filesystem mounts this under
         * /mnt/host.
         */
        sprintf(filepath, "/mnt/host/files/%s", input_filename);
        printf("Opening: %s\n", filepath);

        /* ---- Read BMP from host filesystem ---- */
        if (bmp_extract(filepath, &pic_data) != 0)
        {
            printf("Error: failed to load '%s'. Try again.\n\n", filepath);
            continue;
        }

        printf("Image loaded: %lux%lu px, quality=%lu\n\n",
               (unsigned long)pic_data.header->BMPWidth,
               (unsigned long)pic_data.header->BMPHeight,
               (unsigned long)quality_factor);

        /*
         * Send output filename to Stage 6 (was SEND6 in jencoder1a main.c).
         * Transmit characters up to (but not including) the '.' extension
         * separator, then send '\0' as terminator.
         */
        name_ptr = input_filename;
        while (*name_ptr != '.' && *name_ptr != '\0')
        {
            fifo_write_32(FIFO_1TO6_FNAME_IN_BASE,
                          FIFO_1TO6_FNAME_IN_CSR_BASE,
                          (unsigned int)(unsigned char)*name_ptr);
            print_fifo_status("S1 W:FNm", FIFO_1TO6_FNAME_IN_CSR_BASE);
            name_ptr++;
        }
        fifo_write_32(FIFO_1TO6_FNAME_IN_BASE,
                      FIFO_1TO6_FNAME_IN_CSR_BASE,
                      (unsigned int)'\0');
        print_fifo_status("S1 W:FNm", FIFO_1TO6_FNAME_IN_CSR_BASE);

        /*
         * Encode the image:
         *   - sends num_mcus + quality to Stage 4
         *   - sends width, height, num_mcus to Stage 5
         *   - sends 64 Y / Cb / Cr values per MCU to Stage 2
         *
         * pic_data.bitmap already points to the rearranged (top-left,
         * R-G-B ordered) pixel data after bmp_extract() completes.
         */
        printf("Encoding...\n");
        output_ptr = encode_image(
            (UINT8 *)pic_data.bitmap,
            output_ptr,
            quality_factor,
            FOUR_FOUR_FOUR,
            (UINT32)pic_data.header->BMPWidth,
            (UINT32)pic_data.header->BMPHeight
        );

        printf("Stage 1: encoding complete.\n\n");

        /* Free the malloc'd BMP buffer from bmp_extract() */
        free(pic_data.buffer);
        pic_data.buffer = NULL;
    }

    /*
     * Signal end-of-stream to Stage 6 by sending a single '\0' character
     * as the first byte of a new filename (same as original SEND6(0)).
     */
    fifo_write_32(FIFO_1TO6_FNAME_IN_BASE,
                  FIFO_1TO6_FNAME_IN_CSR_BASE,
                  (unsigned int)0);

    printf("Stage 1: all images processed. Halting.\n");

    /* Spin — downstream stages may still be processing */
    while (1) { }

    return 0;
}
