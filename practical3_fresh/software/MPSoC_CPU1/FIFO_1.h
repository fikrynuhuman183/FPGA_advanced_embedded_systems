/*
 * 	FIFO header for shared memory communication between niosII processors
 *
 *  	DATE		: 18-08-2016
 *      AUTHOR	: Isuru Nawinne
*
*	Structure of the FIFO:
*	___________________________________________________________________________
*	| 	full	| 	empty	| 	count		|	|	|	|	|	|	|	|	|
*	| 	?	    | 	?		| 	?	     	|	|	|	|	|	|	|	|	|
*	___________________________________________________________________________
*/


#ifndef FIFO_1_H_
#define FIFO_1_H_

#include "io.h"
#include "alt_types.h"

/*
 * FIFO layout in shared memory (relative to MEM_BASE):
 * 0x00 : full  (u32)
 * 0x04 : empty (u32)
 * 0x08 : count (u32)
 * 0x0C : data[0]
 * 0x10 : data[1]
 * ...
 */

// SET THIS to your shared FIFO partition base address (check system.h / BSP memory map)
#define MEM_BASE   (0x00027530u)

// Each entry is one 32-bit int (producer/consumer use int)
#define UNIT_SIZE  4u

// Start of data area = 3 words header
#define STARTP     0xCu

// Number of int entries stored
// Example safe value: 256 entries = 256*4 = 1024 bytes data + 12 header = 1036 bytes (fits in 2KB)
#define CAPACITY   16u

// Offsets (relative to MEM_BASE)
extern int writep;   // byte offset to next write slot (local to writer CPU)
extern int readpp;   // byte offset to next read slot  (local to reader CPU)
extern int fullp;    // offset of full flag in shared mem
extern int emptyp;   // offset of empty flag in shared mem
extern int countp;   // offset of count in shared mem

void FIFO_1_INIT(void);
void WRITE_FIFO_1(int *buffer);
void READ_FIFO_1(int *buffer);

#endif /* FIFO_1_H_ */
