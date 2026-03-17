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
*/


#include "FIFO_1.h"


void WRITE_FIFO_1(int *buffer)
{
	// Wait if the fifo is full
	while (IORD_32DIRECT(MEM_BASE, fullp) == 1){

	}
	// Write the data to FIFO
	IOWR_32DIRECT(MEM_BASE, writep, *buffer);
	// Update the write pointer
	writep = writep + UNIT_SIZE;

	// Update "count" in shared mem
	int count = IORD_32DIRECT(MEM_BASE, countp);
	count ++;
	IOWR_32DIRECT(MEM_BASE, countp, count);

	// Update the "full?" and "empty?" flags accordingly
	if(count>= CAPACITY){
		IOWR_32DIRECT(MEM_BASE, fullp,1);
	}
	// Set the full flag if FIFO is now full
	// Reset the empty flag if FIFO now has 1 enrty
	IOWR_32DIRECT(MEM_BASE, emptyp,0);
}


//void READ_FIFO_1(int *buffer)
//{
//	// Wait if the fifo is empty
//
//	// Read the data
//
//	// Update the read pointer
//
//	// Update "count" in shared mem
//
//	// Update the "full?" and "empty?" flags accordingly
//	// Set the empty flag if FIFO is now empty
//	// Reset the full flag if FIFO now has 1 enrty less than capacity
//}



//Initialization
void FIFO_1_INIT()
{
	writep = STARTP + 3*UNIT_SIZE ; // Initially the FIFO is empty, so start writing at the first slot
//	readp  = ???;
	fullp   =   STARTP; // SET THIS OFFSET (If there are previous FIFOs in shared memory, use Prev Fifo's STARTP + Prev Fifo's size)
	emptyp   = fullp + UNIT_SIZE;
	countp  = emptyp + UNIT_SIZE;

	// Assigning values for the flags.
	IOWR_32DIRECT(MEM_BASE, fullp, 0);
	IOWR_32DIRECT(MEM_BASE, emptyp, 1); // The fifo is empty at the start
	IOWR_32DIRECT(MEM_BASE, countp, 0); // The fifo is empty at the start
}
