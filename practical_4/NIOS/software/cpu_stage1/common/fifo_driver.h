#ifndef FIFO_DRIVER_H
#define FIFO_DRIVER_H

/*
 * FIFO Driver for Nios II — Altera On-Chip FIFO Memory (Avalon-MM)
 *
 * Each FIFO has 4 separate Avalon-MM slave ports:
 *   - in       (data write)     -> data_base for writes
 *   - in_csr   (write-side CSR) -> csr_base  to check full status
 *   - out      (data read)      -> data_base for reads
 *   - out_csr  (read-side CSR)  -> csr_base  to check empty status
 *
 * All FIFOs are configured with a uniform 32-bit data width.
 */

#include <io.h>  /* Nios II HAL: IORD_32DIRECT, IOWR_32DIRECT */

/* CSR register offsets (from the CSR base address) */
#define FIFO_CSR_FILL_LEVEL  0x00  /* Current fill level (0..depth)         */
#define FIFO_CSR_STATUS      0x04  /* Status register                       */
#define FIFO_CSR_EVENT       0x08  /* Sticky event flags                    */
#define FIFO_CSR_IE          0x0C  /* Interrupt enable                      */
#define FIFO_CSR_ALMOSTFULL  0x10  /* Almost-full threshold                 */
#define FIFO_CSR_ALMOSTEMPTY 0x14  /* Almost-empty threshold                */

/* Status register bit masks (in_csr and out_csr use the same layout) */
#define FIFO_STATUS_FULL_MASK   0x01  /* Bit 0: full  */
#define FIFO_STATUS_EMPTY_MASK  0x02  /* Bit 1: empty */

/**
 * fifo_write_32 — Push a 32-bit value into a FIFO.
 * Blocks (spins) if the FIFO is full.
 *
 * @param data_base  Write-side data port base address (_IN_BASE)
 * @param csr_base   Write-side CSR port base address  (_IN_CSR_BASE)
 * @param value      32-bit value to push
 */
static inline void fifo_write_32(unsigned int data_base, unsigned int csr_base,
                                 unsigned int value) {
    while (IORD_32DIRECT(csr_base, FIFO_CSR_STATUS) & FIFO_STATUS_FULL_MASK) { }
    IOWR_32DIRECT(data_base, 0, value);
}

/**
 * fifo_read_32 — Pop a 32-bit value from a FIFO.
 * Blocks (spins) if the FIFO is empty.
 *
 * @param data_base  Read-side data port base address (_OUT_BASE)
 * @param csr_base   Read-side CSR port base address  (_OUT_CSR_BASE)
 * @return           32-bit value popped from FIFO
 */
static inline unsigned int fifo_read_32(unsigned int data_base,
                                        unsigned int csr_base) {
    while (IORD_32DIRECT(csr_base, FIFO_CSR_STATUS) & FIFO_STATUS_EMPTY_MASK) { }
    return IORD_32DIRECT(data_base, 0);
}

#endif /* FIFO_DRIVER_H */
