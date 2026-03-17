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

	// Write the data to FIFO

	// Update the write pointer

	// Update "count" in shared mem

	// Update the "full?" and "empty?" flags accordingly
	// Set the full flag if FIFO is now full
	// Reset the empty flag if FIFO now has 1 enrty
}


void READ_FIFO_1(int *buffer)
{
	// Wait if the fifo is empty

	// Read the data

	// Update the read pointer

	// Update "count" in shared mem

	// Update the "full?" and "empty?" flags accordingly
	// Set the empty flag if FIFO is now empty
	// Reset the full flag if FIFO now has 1 enrty less than capacity
}



//Initialization
void FIFO_1_INIT()
{
	writep = ???; // Initially the FIFO is empty, so start writing at the first slot
	readp  = ???;
	fullp   = ?????; // SET THIS OFFSET (If there are previous FIFOs in shared memory, use Prev Fifo's STARTP + Prev Fifo's size)
	emptyp   = fullp + ?????;
	countp  = emptyp + ?????;

	// Assigning values for the flags.
	IOWR_32DIRECT(MEM_BASE, fullp, ?);
	IOWR_32DIRECT(MEM_BASE, emptyp, ?); // The fifo is empty at the start
	IOWR_32DIRECT(MEM_BASE, countp, ?); // The fifo is empty at the start
}
