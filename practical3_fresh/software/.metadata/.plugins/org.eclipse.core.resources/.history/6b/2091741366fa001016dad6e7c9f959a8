/*
 * 	FIFO queue for shared memory communication between niosII processors
 *
 *  	DATE		: 18-08-2016
 *      AUTHOR	: Isuru Nawinne
 *
 *	Structure of the FIFO:
 *	___________________________________________________________________________
 *	| 	full	| 	empty	| 	count		|	|	|	|	|	|	|	|	|
 *	| 	?	| 	?		| 	?		|	|	|	|	|	|	|	|	|
 *	___________________________________________________________________________
 * FIFO queue for shared memory communication between Nios II processors
 *
 * Structure in shared memory (relative to MEM_BASE):
 *  0x00 : full   (32-bit)
 *  0x04 : empty  (32-bit)
 *  0x08 : count  (32-bit)
 *  0x0C : data area begins (FIFO entries)
*/

#include "FIFO_1.h"

// Global offsets (relative to MEM_BASE)
int writep;
int readp;
int fullp;
int emptyp;
int countp;

static inline int fifo_data_end(void)
{
    // one-past-last valid byte offset for data area
    return (int)(STARTP + (CAPACITY * UNIT_SIZE));
}

void FIFO_1_INIT(void)
{
    // Header offsets
    fullp  = 0x0;
    emptyp = 0x4;
    countp = 0x8;

    // Local pointers start at first data slot
    writep = (int)STARTP;
    readp = (int)STARTP;

    // FIFO initially empty
    IOWR_32DIRECT(MEM_BASE, fullp,  0); // not full
    IOWR_32DIRECT(MEM_BASE, emptyp, 1); // empty
    IOWR_32DIRECT(MEM_BASE, countp, 0); // count = 0
}

void WRITE_FIFO_1(int *buffer)
{
    // 1) Wait while FIFO is full
    while (IORD_32DIRECT(MEM_BASE, fullp) != 0) {
        // busy wait
    }

    // 2) Write one entry (int) to FIFO
    // IMPORTANT: write the VALUE, not the pointer
    IOWR_32DIRECT(MEM_BASE, writep, (alt_u32)(*buffer));

    // 3) Advance write pointer (wrap around)
    writep += (int)UNIT_SIZE;
    if (writep >= fifo_data_end()) {
        writep = (int)STARTP;
    }

    // 4) Update count
    alt_u32 count = IORD_32DIRECT(MEM_BASE, countp);
    count++;
    IOWR_32DIRECT(MEM_BASE, countp, count);

    // 5) Update flags
    IOWR_32DIRECT(MEM_BASE, emptyp, 0);           // now not empty
    if (count >= CAPACITY) {
        IOWR_32DIRECT(MEM_BASE, fullp, 1);        // now full
    }
}

void READ_FIFO_1(int *buffer)
{
    // 1) Wait while FIFO is empty
    while (IORD_32DIRECT(MEM_BASE, emptyp) != 0) {
        // busy wait
    }

    // 2) Read one entry
    *buffer = (int)IORD_32DIRECT(MEM_BASE, readp);

    // 3) Advance read pointer (wrap around)
    readp += (int)UNIT_SIZE;
    if (readp >= fifo_data_end()) {
        readp = (int)STARTP;
    }

    // 4) Update count
    alt_u32 count = IORD_32DIRECT(MEM_BASE, countp);
    count--;
    IOWR_32DIRECT(MEM_BASE, countp, count);

    // 5) Update flags
    IOWR_32DIRECT(MEM_BASE, fullp, 0);            // after a read, cannot be full
    if (count == 0) {
        IOWR_32DIRECT(MEM_BASE, emptyp, 1);       // now empty
    }
}
