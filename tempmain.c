/* mipslabmain.c

   This file written 2015 by Axel Isaksson,
   modified 2015, 2017 by F Lundevall

   Latest update 2017-04-21 by F Lundevall

   For copyright and licensing, see file COPYING */
#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */

void *stdin, *stdout, *stderr;

#define MEASURING_TIME  4450000
int globalTime = 1;

int getButton(void){
  return ((PORTD >> 5) & 7);
}

int main(void) {
        /*
	  This will set the peripheral bus clock to the same frequency
	  as the sysclock. That means 80 MHz, when the microcontroller
	  is running at 80 MHz. Changed 2017, as recommended by Axel.
	*/
	SYSKEY = 0xAA996655;  /* Unlock OSCCON, step 1 */
	SYSKEY = 0x556699AA;  /* Unlock OSCCON, step 2 */
	while(OSCCON & (1 << 21)); /* Wait until PBDIV ready */
	OSCCONCLR = 0x180000; /* clear PBDIV bit <0,1> */
	while(OSCCON & (1 << 21));  /* Wait until PBDIV ready */
	SYSKEY = 0x0;  /* Lock OSCCON */

	// OSCCON &= ~0x180000;
	// OSCCON |= 0x080000;


	/* Set up output pins */
	AD1PCFG = 0xFFFF;
	ODCE = 0x0;
	TRISECLR = 0xFF;
	PORTE = 0x0;

	/* Output pins for display signals */
	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;

	/* Set up input pins */
	TRISDSET = (1 << 8);
	TRISFSET = (1 << 1);

	/* Set up SPI as master */
	SPI2CON = 0;
	SPI2BRG = 4;
	/* SPI2STAT bit SPIROV = 0; */
	SPI2STATCLR = 0x40;
	/* SPI2CON bit CKP = 1; */
        SPI2CONSET = 0x40;
	/* SPI2CON bit MSTEN = 1; */
	SPI2CONSET = 0x20;
	/* SPI2CON bit ON = 1; */
	SPI2CONSET = 0x8000;

	display_init();


	tempInit(); /* Do any lab-specific initialization */
	char globalTimeBuf[32];

	while( 1 )
	{

	  while(getSwitch() & 0x8)
	  {
		sprintf(globalTimeBuf, "PERIOD:%d SEC", globalTime);
		  if(getButton() & 4 && globalTime > 1)
		  {
		    globalTime--;
		  }

		  if(getButton() & 2 && globalTime < 999)
		  {
		    globalTime++;
		  }

		  display_string(0,"TIME MEASURMENT");
		  display_string(2,globalTimeBuf);
		  display_string(1,"");
		  display_string(3,"");
		  display_update();
		  quicksleep(MEASURING_TIME/10);

	  }

	  work(); /* Do lab-specific things again and again */
	int i = 0;
	while(i < globalTime)
  		{
      	  quicksleep(MEASURING_TIME);
		  i++;
		  if(getSwitch() & 0x8)
		  {
			break;
		  }
 		}
	}
	return 0;
}
