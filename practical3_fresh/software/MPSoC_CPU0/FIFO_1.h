/*
 * 	FIFO header for shared memory communication between niosII processors
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


//CHANGE THESE NAMES FOR DIFFERENT FIFOS
#ifndef FIFO_1_H_
#define FIFO_1_H_


#include <io.h>
#include "system.h"
#include <stdio.h>


#define MEM_BASE 0x00000000 // Base address of the memory device. SET THIS PROPERLY
#define CAPACITY 16 // Depth of the fifo. SET THIS AS REQUIRED
#define UNIT_SIZE 4 // Size of an entry in bytes
#define STARTP 0x00109500 // Start of the actual fifo storage area. (3 words from the start of "full", just after "count") This address is relative to the MEM_BASE. (Prev Fifo's STARTP + 0x14 + Prev Fifo's size)


// Pointers associated with the FIFO. These are offsets to MEM_BASE
 int writep; // Write pointer. Points to the next slot to be written in. Used and modified only by the writing CPU
 int readp; // Read pointer. Points to the next slot to be read. Used and modified only by the reading CPU
 int fullp; // Points to "full?" flag in shared memory
 int emptyp; // points to "empty?" flag in shared memory
 int countp; // Points to "count" in shared memory


// CALL THIS FUNCTION AT THE BEGINING OF BOTH PROGRAMS USING THIS FIFO. CHANGE THE NAME FOR DIFFERENT FIFOS
void FIFO_1_INIT();


//CHANGE THE NAME FOR DIFFERENT FIFOS
void WRITE_FIFO_1(int *buffer);

//CHANGE THE NAME FOR DIFFERENT FIFOS
void READ_FIFO_1(int *buffer);


#endif /* FIFO_1_H_ */
