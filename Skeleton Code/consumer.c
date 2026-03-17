/*
 * 	Template of consumer application using FIFO
 *
 *  	DATE		: 18-08-2016
 *      AUTHOR	: Isuru Nawinne
 */


#include "FIFO_1.h"


void delay(int n);
int consumer();


int main()
{
	consumer();
	
	while(1) { }

	return 0;
}


int consumer()
{
	printf("Consumer starting..\n");

	FIFO_1_INIT(); // Initialize the producer-consumer fifo

	int j = 0;
	int k = 5;
	while(k<500)
	{
		READ_FIFO_1(&j);  // Read from the producer-consumer fifo
		if(j==k)
			printf("Consumer succefully received [%i]\n",j);

		k+=10;
	}

	printf("Consumer finished..\n");
	return 0;
}


void delay(int n)
{
	int i;
	for(i=n; i>0; i--)
	{
		continue;
	}
}