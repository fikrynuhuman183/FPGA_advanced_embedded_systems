/*************************************************************************************
 * CO503 Advanced Embedded Systems
 * 
 * Date		: 16-08-2016
 * Author 	: Isuru Nawinne
 *
 *************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <io.h>
#include <unistd.h>

#define LED_BASE 0x00081020 // INSERT BASE ADDRESS OF "led_out" PIO DEVICE FROM QSYS
#define OFFSET 0x00000000

/* 8-bit "loop counter" variable. */
static uint8_t count;


int main(void)
{ 
    count = 0;

    /* 0x00-0xFF counting loop. */
    while( 1 ) 
    {
        usleep(100000); // Wait for about 0.1 seconds

        IOWR_8DIRECT(LED_BASE,OFFSET,count); // Write the value of "count" to the "led_out" PIO device

        if( count == 0xFF )
        {
        	usleep(1000000); // If done counting, wait about 1 second...
        	count = 0;
        }

        count++;
    }

    return 0;
}
