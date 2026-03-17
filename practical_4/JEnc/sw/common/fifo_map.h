#ifndef FIFO_MAP_H
#define FIFO_MAP_H

/*
 * FIFO base addresses — replace these with actual values from system.h
 * after Qsys generation.
 *
 * Each FIFO has FOUR base addresses:
 *   _IN_BASE      = write-side data slave  (producer writes data here)
 *   _IN_CSR_BASE  = write-side CSR slave   (producer checks full/fill level)
 *   _OUT_BASE     = read-side data slave   (consumer reads data here)
 *   _OUT_CSR_BASE = read-side CSR slave    (consumer checks empty/fill level)
 *
 * ALL FIFOs are 32-bit width.
 * Use fifo_write_32() / fifo_read_32() for all FIFOs.
 */

/* Stage 1 -> Stage 2: Y, Cb, Cr blocks (32-bit, depth 64) */
#define FIFO_1TO2_Y_IN_BASE        0x00021104
#define FIFO_1TO2_Y_IN_CSR_BASE    0x000210A0
#define FIFO_1TO2_Y_OUT_BASE       0x0000908C
#define FIFO_1TO2_Y_OUT_CSR_BASE   0x00009020
#define FIFO_1TO2_CB_IN_BASE       0x00021100
#define FIFO_1TO2_CB_IN_CSR_BASE   0x00021080
#define FIFO_1TO2_CB_OUT_BASE      0x00009088
#define FIFO_1TO2_CB_OUT_CSR_BASE  0x00009040
#define FIFO_1TO2_CR_IN_BASE       0x000210FC
#define FIFO_1TO2_CR_IN_CSR_BASE   0x00021060
#define FIFO_1TO2_CR_OUT_BASE      0x00009084
#define FIFO_1TO2_CR_OUT_CSR_BASE  0x00009000

/* Stage 2 -> Stage 3: level-shifted samples (32-bit, depth 64) */
#define FIFO_2TO3_IN_BASE          0x00009080
#define FIFO_2TO3_IN_CSR_BASE      0x00009060
#define FIFO_2TO3_OUT_BASE         0x00011044
#define FIFO_2TO3_OUT_CSR_BASE     0x00011000

/* Stage 3 -> Stage 4: DCT coefficients (32-bit, depth 64) */
#define FIFO_3TO4_IN_BASE          0x00011040
#define FIFO_3TO4_IN_CSR_BASE      0x00011020
#define FIFO_3TO4_OUT_BASE         0x00011068
#define FIFO_3TO4_OUT_CSR_BASE     0x00011020

/* Stage 4 -> Stage 5: quantized coefficients (32-bit, depth 64) */
#define FIFO_4TO5_IN_BASE          0x00011060
#define FIFO_4TO5_IN_CSR_BASE      0x00011040
#define FIFO_4TO5_OUT_BASE         0x0002108C
#define FIFO_4TO5_OUT_CSR_BASE     0x00021020

/* Stage 5 -> Stage 6: encoded byte stream (32-bit, depth 64) */
#define FIFO_5TO6_DATA_IN_BASE      0x00021084
#define FIFO_5TO6_DATA_IN_CSR_BASE  0x00021060
#define FIFO_5TO6_DATA_OUT_BASE     0x00009048
#define FIFO_5TO6_DATA_OUT_CSR_BASE 0x00009060

/* Stage 5 -> Stage 6: done token (32-bit, depth 1) */
#define FIFO_5TO6_DONE_IN_BASE      0x00021080
#define FIFO_5TO6_DONE_IN_CSR_BASE  0x00021040
#define FIFO_5TO6_DONE_OUT_BASE     0x00009044
#define FIFO_5TO6_DONE_OUT_CSR_BASE 0x00009020

/* Stage 1 -> Stage 4: parameters (32-bit, depth 2) */
#define FIFO_1TO4_PARAM_IN_BASE      0x000210F8
#define FIFO_1TO4_PARAM_IN_CSR_BASE  0x00021040
#define FIFO_1TO4_PARAM_OUT_BASE     0x00011064
#define FIFO_1TO4_PARAM_OUT_CSR_BASE 0x00011000

/* Stage 1 -> Stage 5: parameters (32-bit, depth 3) */
#define FIFO_1TO5_PARAM_IN_BASE      0x000210F4
#define FIFO_1TO5_PARAM_IN_CSR_BASE  0x00021020
#define FIFO_1TO5_PARAM_OUT_BASE     0x00021088
#define FIFO_1TO5_PARAM_OUT_CSR_BASE 0x00021000

/* Stage 1 -> Stage 6: filename (32-bit, depth 100) */
#define FIFO_1TO6_FNAME_IN_BASE      0x000210F0
#define FIFO_1TO6_FNAME_IN_CSR_BASE  0x00021000
#define FIFO_1TO6_FNAME_OUT_BASE     0x00009040
#define FIFO_1TO6_FNAME_OUT_CSR_BASE 0x00009000

#endif /* FIFO_MAP_H */
