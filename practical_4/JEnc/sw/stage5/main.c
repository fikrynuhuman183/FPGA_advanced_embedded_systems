/*
 * Stage 5 — Huffman Encoding + JPEG Header/Marker Generation
 *
 * Ported from jencoder1e/huffman.c (Tensilica Xtensa) to Nios II.
 * Replaces RECV1/RECV2/SEND1/SEND2 with fifo_read_32/fifo_write_32.
 *
 * Data flow:
 *   Initialization:
 *     - Read width, height, count from fifo_1to5_param (was RECV1)
 *     - Read Lqt[64] and Cqt[64] from fifo_4to5 (was RECV2)
 *     - Write JPEG header markers to fifo_5to6_data (was SEND1)
 *   Per MCU:
 *     - Read  64 quantized coefficients from fifo_4to5 (was RECV2)
 *     - Huffman encode (DC differential + AC run-length)
 *     - Write encoded bytes to fifo_5to6_data (was SEND1)
 *   Termination:
 *     - Write EOI marker, then done token via fifo_5to6_done (was SEND2)
 */

#include <stdio.h>
#include "datatype.h"
#include "system.h"
#include "fifo_driver.h"

/* ======== Huffman data tables (from huffdata.h) ======== */

static UINT16 luminance_dc_code_table[] =
{
    0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
    0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
};

static UINT16 luminance_dc_size_table[] =
{
    0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
};

static UINT16 chrominance_dc_code_table[] =
{
    0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
    0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
};

static UINT16 chrominance_dc_size_table[] =
{
    0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
    0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
};

static UINT16 luminance_ac_code_table[] =
{
    0x000A,
    0x0000, 0x0001, 0x0004, 0x000B, 0x001A, 0x0078, 0x00F8, 0x03F6, 0xFF82, 0xFF83,
    0x000C, 0x001B, 0x0079, 0x01F6, 0x07F6, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88,
    0x001C, 0x00F9, 0x03F7, 0x0FF4, 0xFF89, 0xFF8A, 0xFF8b, 0xFF8C, 0xFF8D, 0xFF8E,
    0x003A, 0x01F7, 0x0FF5, 0xFF8F, 0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95,
    0x003B, 0x03F8, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D,
    0x007A, 0x07F7, 0xFF9E, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5,
    0x007B, 0x0FF6, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD,
    0x00FA, 0x0FF7, 0xFFAE, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5,
    0x01F8, 0x7FC0, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD,
    0x01F9, 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6,
    0x01FA, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
    0x03F9, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8,
    0x03FA, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1,
    0x07F8, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA,
    0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4,
    0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
    0x07F9
};

static UINT16 luminance_ac_size_table[] =
{
    0x0004,
    0x0002, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x000A, 0x0010, 0x0010,
    0x0004, 0x0005, 0x0007, 0x0009, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0006, 0x0009, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0007, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0008, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000B
};

static UINT16 chrominance_ac_code_table[] =
{
    0x0000,
    0x0001, 0x0004, 0x000A, 0x0018, 0x0019, 0x0038, 0x0078, 0x01F4, 0x03F6, 0x0FF4,
    0x000B, 0x0039, 0x00F6, 0x01F5, 0x07F6, 0x0FF5, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B,
    0x001A, 0x00F7, 0x03F7, 0x0FF6, 0x7FC2, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F, 0xFF90,
    0x001B, 0x00F8, 0x03F8, 0x0FF7, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96,
    0x003A, 0x01F6, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E,
    0x003B, 0x03F9, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6,
    0x0079, 0x07F7, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE,
    0x007A, 0x07F8, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6,
    0x00F9, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
    0x01F7, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8,
    0x01F8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1,
    0x01F9, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA,
    0x01FA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3,
    0x07F9, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEb, 0xFFEC,
    0x3FE0, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5,
    0x7FC3, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
    0x03FA
};

static UINT16 chrominance_ac_size_table[] =
{
    0x0002,
    0x0002, 0x0003, 0x0004, 0x0005, 0x0005, 0x0006, 0x0007, 0x0009, 0x000A, 0x000C,
    0x0004, 0x0006, 0x0008, 0x0009, 0x000B, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0005, 0x0008, 0x000A, 0x000C, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0006, 0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0008, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000E, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x000A
};

static UINT8 bitsize[] =
{
    0, 1, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8
};

/* ======== Marker data table (from markdata.h) ======== */

static UINT16 markerdata[] =
{
    /* dht */
    0xFFC4, 0x1A2, 0x00,

    /* luminance dc (2 - 16) + 1 */
    0x0105, 0x0101, 0x00101, 0x0101, 0x0000, 0x00000, 00000, 00000,

    /* luminance dc (2 - 12) + 1 */
    0x0102, 0x0304, 0x0506, 0x0708, 0x090A, 0x0B01,

    /* chrominance dc (1 - 16) */
    0x0003, 0x0101, 0x0101, 0x0101, 0x0101, 0x0100, 0x0000, 0x0000,

    /* chrominance dc (1 - 12) */
    0x0001, 0x00203, 0x0405, 0x0607, 0x0809, 0x00A0B,

    /* luminance ac 1 + (1 - 15) */
    0x1000, 0x0201, 0x0303, 0x0204, 0x0305, 0x0504, 0x0400, 0x0001,

    /* luminance ac 1 + (1 - 162) + 1 */
    0x7D01, 0x0203, 0x0004, 0x1105, 0x1221, 0x3141, 0x0613, 0x5161,
    0x0722, 0x7114, 0x3281, 0x91A1, 0x0823, 0x42B1, 0xC115, 0x52D1,
    0xF024, 0x3362, 0x7282, 0x090A, 0x1617, 0x1819, 0x1A25, 0x2627,
    0x2829, 0x2A34, 0x3536, 0x3738, 0x393A, 0x4344, 0x4546, 0x4748,
    0x494A, 0x5354, 0x5556, 0x5758, 0x595A, 0x6364, 0x6566, 0x6768,
    0x696A, 0x7374, 0x7576, 0x7778, 0x797A, 0x8384, 0x8586, 0x8788,
    0x898A, 0x9293, 0x9495, 0x9697, 0x9899, 0x9AA2, 0xA3A4, 0xA5A6,
    0xA7A8, 0xA9AA, 0xB2B3, 0xB4B5, 0xB6B7, 0xB8B9, 0xBAC2, 0xC3C4,
    0xC5C6, 0xC7C8, 0xC9CA, 0xD2D3, 0xD4D5, 0xD6D7, 0xD8D9, 0xDAE1,
    0xE2E3, 0xE4E5, 0xE6E7, 0xE8E9, 0xEAF1, 0xF2F3, 0xF4F5, 0xF6F7,
    0xF8F9, 0xFA11,

    /* chrominance ac (1 - 16) */
    0x0002, 0x0102, 0x0404, 0x0304, 0x0705, 0x0404, 0x0001, 0x0277,

    /* chrominance ac (1 - 162) */
    0x0001, 0x0203, 0x1104, 0x0521, 0x3106, 0x1241, 0x5107, 0x6171,
    0x1322, 0x3281, 0x0814, 0x4291, 0xA1B1, 0xC109, 0x2333, 0x52F0,
    0x1562, 0x72D1, 0x0A16, 0x2434, 0xE125, 0xF117, 0x1819, 0x1A26,
    0x2728, 0x292A, 0x3536, 0x3738, 0x393A, 0x4344, 0x4546, 0x4748,
    0x494A, 0x5354, 0x5556, 0x5758, 0x595A, 0x6364, 0x6566, 0x6768,
    0x696A, 0x7374, 0x7576, 0x7778, 0x797A, 0x8283, 0x8485, 0x8687,
    0x8889, 0x8A92, 0x9394, 0x9596, 0x9798, 0x999A, 0xA2A3, 0xA4A5,
    0xA6A7, 0xA8A9, 0xAAB2, 0xB3B4, 0xB5B6, 0xB7B8, 0xB9BA, 0xC2C3,
    0xC4C5, 0xC6C7, 0xC8C9, 0xCAD2, 0xD3D4, 0xD5D6, 0xD7D8, 0xD9DA,
    0xE2E3, 0xE4E5, 0xE6E7, 0xE8E9, 0xEAF2, 0xF3F4, 0xF5F6, 0xF7F8,
    0xF9FA
};

/* ======== Bit-packing state (global, persists across blocks) ======== */
static UINT32 lcode = 0;
static UINT16 bitindex = 0;
static INT16  ldc1 = 0, ldc2 = 0, ldc3 = 0;

/* ======== Helper: write one byte to output FIFO (was SEND1) ======== */
static inline void send_byte(UINT8 byte)
{
    fifo_write_32(FIFO_5TO6_DATA_IN_BASE, FIFO_5TO6_DATA_IN_CSR_BASE, (unsigned int)byte);
}

/* ======== PUTBITS macro (bit-packing with byte-stuffing) ======== */
#define PUTBITS                                                             \
{                                                                           \
    bits_in_next_word = (INT16)(bitindex + numbits - 32);                   \
    if (bits_in_next_word < 0)                                              \
    {                                                                       \
        lcode = (lcode << numbits) | data;                                  \
        bitindex += numbits;                                                \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        lcode = (lcode << (32 - bitindex)) | (data >> bits_in_next_word);   \
        out = (UINT8)(lcode >> 24);                                         \
        send_byte(out);                                                     \
        if (out == 0xff) send_byte(0);                                      \
        out = (UINT8)(lcode >> 16);                                         \
        send_byte(out);                                                     \
        if (out == 0xff) send_byte(0);                                      \
        out = (UINT8)(lcode >> 8);                                          \
        send_byte(out);                                                     \
        if (out == 0xff) send_byte(0);                                      \
        out = (UINT8)lcode;                                                 \
        send_byte(out);                                                     \
        if (out == 0xff) send_byte(0);                                      \
        lcode = data;                                                       \
        bitindex = bits_in_next_word;                                       \
    }                                                                       \
}

/* ======== write_markers ========
 * Writes JPEG header (SOI, DQT, DHT, SOF, SOS) to output FIFO.
 * Reads Lqt[64] and Cqt[64] from Stage 4 via fifo_4to5.
 */
void write_markers(UINT32 image_width, UINT32 image_height)
{
    UINT16 i, header_length;
    UINT8 number_of_components;

    /* Start of image marker */
    send_byte(0xFF);
    send_byte(0xD8);

    /* Quantization table marker */
    send_byte(0xFF);
    send_byte(0xDB);

    /* Quantization table length */
    send_byte(0x00);
    send_byte(0x84);

    /* Pq, Tq */
    send_byte(0x00);

    /* Lqt table — read 64 values from Stage 4 (was RECV2) */
    for (i = 0; i < 64; i++)
        send_byte((UINT8)(fifo_read_32(FIFO_4TO5_OUT_BASE, FIFO_4TO5_OUT_CSR_BASE) & 0xFF));

    /* Pq, Tq */
    send_byte(0x01);

    /* Cqt table — read 64 values from Stage 4 (was RECV2) */
    for (i = 0; i < 64; i++)
        send_byte((UINT8)(fifo_read_32(FIFO_4TO5_OUT_BASE, FIFO_4TO5_OUT_CSR_BASE) & 0xFF));

    /* Huffman table (DHT) */
    for (i = 0; i < 210; i++)
    {
        send_byte((UINT8)(markerdata[i] >> 8));
        send_byte((UINT8)markerdata[i]);
    }

    number_of_components = 3;

    /* Frame header (SOF) */

    /* Start of frame marker */
    send_byte(0xFF);
    send_byte(0xC0);

    header_length = (UINT16)(8 + 3 * number_of_components);

    /* Frame header length */
    send_byte((UINT8)(header_length >> 8));
    send_byte((UINT8)header_length);

    /* Precision (P) */
    send_byte(0x08);

    /* Image height */
    send_byte((UINT8)(image_height >> 8));
    send_byte((UINT8)image_height);

    /* Image width */
    send_byte((UINT8)(image_width >> 8));
    send_byte((UINT8)image_width);

    /* Nf */
    send_byte(number_of_components);

    send_byte(0x01);
    send_byte(0x11);
    send_byte(0x00);

    send_byte(0x02);
    send_byte(0x11);
    send_byte(0x01);

    send_byte(0x03);
    send_byte(0x11);
    send_byte(0x01);

    /* Scan header (SOS) */

    /* Start of scan marker */
    send_byte(0xFF);
    send_byte(0xDA);

    header_length = (UINT16)(6 + (number_of_components << 1));

    /* Scan header length */
    send_byte((UINT8)(header_length >> 8));
    send_byte((UINT8)header_length);

    /* Ns */
    send_byte(number_of_components);

    send_byte(0x01);
    send_byte(0x00);

    send_byte(0x02);
    send_byte(0x11);

    send_byte(0x03);
    send_byte(0x11);

    send_byte(0x00);
    send_byte(0x3F);
    send_byte(0x00);
}

/* ======== huffman ========
 * Huffman-encodes one 8x8 block (64 quantized coefficients).
 * Reads coefficients from fifo_4to5.
 * Writes encoded bytes to fifo_5to6_data.
 */
void huffman(UINT16 component)
{
    UINT16 i;
    UINT16 *DcCodeTable, *DcSizeTable, *AcCodeTable, *AcSizeTable;

    INT16 Coeff, LastDc;
    UINT16 AbsCoeff, HuffCode, HuffSize, RunLength = 0, DataSize = 0, index;

    INT16 bits_in_next_word;
    UINT16 numbits;
    UINT32 data;

    UINT8 out;

    /* Read DC coefficient from fifo_4to5 (was RECV2) */
    Coeff = (INT16)(fifo_read_32(FIFO_4TO5_OUT_BASE, FIFO_4TO5_OUT_CSR_BASE) & 0xFFFF);

    if (component == 1)
    {
        DcCodeTable = luminance_dc_code_table;
        DcSizeTable = luminance_dc_size_table;
        AcCodeTable = luminance_ac_code_table;
        AcSizeTable = luminance_ac_size_table;

        LastDc = ldc1;
        ldc1 = Coeff;
    }
    else
    {
        DcCodeTable = chrominance_dc_code_table;
        DcSizeTable = chrominance_dc_size_table;
        AcCodeTable = chrominance_ac_code_table;
        AcSizeTable = chrominance_ac_size_table;

        if (component == 2)
        {
            LastDc = ldc2;
            ldc2 = Coeff;
        }
        else
        {
            LastDc = ldc3;
            ldc3 = Coeff;
        }
    }

    Coeff -= LastDc;

    AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;

    while (AbsCoeff != 0)
    {
        AbsCoeff >>= 1;
        DataSize++;
    }

    HuffCode = DcCodeTable[DataSize];
    HuffSize = DcSizeTable[DataSize];

    Coeff &= (1 << DataSize) - 1;
    data = (HuffCode << DataSize) | Coeff;
    numbits = HuffSize + DataSize;

    PUTBITS

    /* AC coefficients: 63 remaining */
    for (i = 63; i > 0; i--)
    {
        /* Read next AC coefficient from fifo_4to5 (was RECV2) */
        if ((Coeff = (INT16)(fifo_read_32(FIFO_4TO5_OUT_BASE, FIFO_4TO5_OUT_CSR_BASE) & 0xFFFF)) != 0)
        {
            while (RunLength > 15)
            {
                RunLength -= 16;
                data = AcCodeTable[161];
                numbits = AcSizeTable[161];
                PUTBITS
            }

            AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;

            if (AbsCoeff >> 8 == 0)
                DataSize = bitsize[AbsCoeff];
            else
                DataSize = bitsize[AbsCoeff >> 8] + 8;

            index = RunLength * 10 + DataSize;
            HuffCode = AcCodeTable[index];
            HuffSize = AcSizeTable[index];

            Coeff &= (1 << DataSize) - 1;
            data = (HuffCode << DataSize) | Coeff;
            numbits = HuffSize + DataSize;

            PUTBITS
            RunLength = 0;
        }
        else
            RunLength++;
    }

    if (RunLength != 0)
    {
        data = AcCodeTable[0];
        numbits = AcSizeTable[0];
        PUTBITS
    }
}

/* ======== close_bitstream ========
 * Flushes remaining bits and writes EOI marker.
 */
void close_bitstream(void)
{
    UINT16 i, count;
    UINT8 *ptr;
    UINT8 out;

    if (bitindex > 0)
    {
        lcode <<= (32 - bitindex);
        count = (bitindex + 7) >> 3;

        ptr = (UINT8 *)&lcode + 3;

        for (i = count; i > 0; i--)
        {
            out = *ptr--;
            send_byte(out);
            if (out == 0xff)
                send_byte(0);
        }
    }

    /* End of image marker */
    send_byte(0xFF);
    send_byte(0xD9);
}

/* ======== main ======== */
int main(void)
{
    printf("Stage 5: Huffman Encoding started\n");

    while (1)
    {
        lcode = 0;
        bitindex = 0;
        ldc1 = 0; ldc2 = 0; ldc3 = 0;

        /* Read parameters from fifo_1to5_param (was RECV1) */
        UINT32 width  = fifo_read_32(FIFO_1TO5_PARAM_OUT_BASE, FIFO_1TO5_PARAM_OUT_CSR_BASE);
        UINT32 height = fifo_read_32(FIFO_1TO5_PARAM_OUT_BASE, FIFO_1TO5_PARAM_OUT_CSR_BASE);
        UINT32 count  = fifo_read_32(FIFO_1TO5_PARAM_OUT_BASE, FIFO_1TO5_PARAM_OUT_CSR_BASE);

        /* Write JPEG header markers (reads Lqt/Cqt from Stage 4) */
        write_markers(width, height);

        /* Encode all MCUs: 3 blocks each (Y=1, Cb=2, Cr=3) */
        for (; count > 0; count--)
        {
            huffman(1);
            huffman(2);
            huffman(3);
        }

        /* Close bitstream (flush bits + EOI marker) */
        close_bitstream();

        /* Signal done to Stage 6 (was SEND2(1)) */
        fifo_write_32(FIFO_5TO6_DONE_IN_BASE, FIFO_5TO6_DONE_IN_CSR_BASE, 1);

        break;  /* single-image mode; remove for continuous */
    }

    printf("Stage 5: Done!\n");
    return 0;
}
