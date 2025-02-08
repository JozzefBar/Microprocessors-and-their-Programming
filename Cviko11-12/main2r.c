//***********************************************************
// Mikropocítaèe a ich programovanie
// Cvicenie c. 11 a 12
// Riadenie inteligentneho alfanumerickeho displeja DEM16217 - dvojriadkovy
// MSP-EXP430G2 _LaunchPad_Extended_Board_2
//***********************************************************

#include <msp430.h>
#include "evb2.h"

const unsigned char text1[] = "Mikropocitace a ich"; //"Mikropoc" "\xa0" "tace a ich";
const unsigned char text2[] = "   programovanie   ";

void main(void)
{
	char i = 0;
	
	evb2_io_init();
	
	lcd_init();
	lcd_clear();
	lcd_puts(&text1[0]);
	lcd_goto(0x40);
	lcd_puts(&text2[0]);
	LCD_RS_L;				
	lcd_write(0x0C);
	
	while(1)
	{
		for(i=0;i<16;i++)
		{
			__delay_cycles(250000);
			lcd_write(0x1C);
		}
		for(i=0;i<35;i++)
		{
			__delay_cycles(250000);
			lcd_write(0x18);
		}
		for(i=0;i<19;i++)
		{
			__delay_cycles(250000);
			lcd_write(0x1C);
		}
	}
}
