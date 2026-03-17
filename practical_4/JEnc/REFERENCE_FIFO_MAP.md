# Reference Design: FIFO Connectivity and Depths

From `JEnc_vlog_Final-2012-09-20/target/jencoder1.c` (ADDFIFO calls). Use this to replicate the same topology in Qsys.

## FIFO List (Reference)

| FIFO name   | Depth | Producer (Stage) | Consumer (Stage) | Content |
|-------------|-------|------------------|------------------|---------|
| fifo_a2b1   | 64    | 1 (6o OUT1)      | 2 (3i1o IN1)     | Y block (64 values) |
| fifo_a2b2   | 64    | 1 (6o OUT2)      | 2 (3i1o IN2)     | Cb block |
| fifo_a2b3   | 64    | 1 (6o OUT3)      | 2 (3i1o IN3)     | Cr block |
| fifo_a2d    | 2     | 1 (6o OUT4)      | 4 (2i1oA IN1)    | e.g. num_mcus, quality |
| fifo_a2e    | 3     | 1 (6o OUT5)      | 5 (2i2o IN1)     | e.g. image params |
| fifo_a2f    | 100   | 1 (6o OUT6)      | 6 (3i IN1)       | Filename string |
| fifo_b2c     | 64    | 2 (3i1o OUT)     | 3 (1i1o IN)      | Level-shifted block (64) |
| fifo_c2d     | 64    | 3 (1i1o OUT)     | 4 (2i1oA IN2)    | DCT block (64) |
| fifo_d2e     | 64    | 4 (2i1oA OUT)    | 5 (2i2o IN2)     | Quantized block (64) |
| fifo_e2f1    | 64    | 5 (2i2o OUT1)    | 6 (3i IN2)       | Encoded bytes |
| fifo_e2f2    | 1     | 5 (2i2o OUT2)    | 6 (3i IN3)       | Done/sync token |

**Note:** Stage 3 runs DCT **three times per macro-block** (Y, Cb, Cr), so the main loop in `encoder.c` sends 3×64 level-shifted samples to Stage 2→3, and Stage 3 processes 3 blocks and sends 3×64 to Stage 4. Your FIFO depths can stay 64 if you pass one 8×8 block at a time; increase depth if you want to buffer multiple blocks.

## Stage ↔ Reference folder

| Stage | Folder       | Main file(s)   | TIE include (replace with your FIFO driver) |
|-------|--------------|----------------|---------------------------------------------|
| 1     | jencoder1a   | main.c, encoder.c, readYUV.c | portqueue6o.h (6 outputs) |
| 2     | jencoder1b   | main.c         | portqueue3i1o.h (3 in, 1 out) |
| 3     | jencoder1c   | dct.c          | portqueue1i1o.h (1 in, 1 out) |
| 4     | jencoder1d   | quant.c, main  | portqueue2i1oA.h (2 in, 1 out) |
| 5     | jencoder1e   | huffman.c      | portqueue2i2o.h (2 in, 2 out) |
| 6     | jencoder1f   | main.c         | portqueue3i.h (3 inputs) |

Use this together with `PROJECT_GUIDE_StepByStep.md` when building your Nios II system and porting the C code.
