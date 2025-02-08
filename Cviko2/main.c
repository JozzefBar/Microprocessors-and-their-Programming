/*
 * Mikropocitace a ich programovanie
 * cvicenie c. 2 softvérový test tlaèítka na porte
*/

#include  <msp430.h>

void onesk(unsigned int i); //deklaracia funkcie onesk

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;  // Zastav casovac watchdog-u
  P1DIR |= 0x41;             // Nastav piny P1.0 and P1.6 do vystupneho modu
  P1DIR &= 0xF7;			 // Nastav pin P1.3 do vstupnej funkcie    pomocou & resetujeme bity do nuly
  	  	  	  	  	  	  	 //(zbytocny prikaz. Preco?) - po resete su nastavené na 0
  unsigned int j = 0; //kolko bitovy je short integer?
  for (;;) //nekonecny cyklus
  {
  	P1OUT = 0x00;

  	while(P1IN & BIT3); //test pustene tl, test na log.1 z P1.3
  				//stlacil som
  	P1OUT = 0x40;

  	onesk(2); //nasledujuci test spusti až pochvili, preco?
  	while(~P1IN & BIT3); //test stlacene tl    opak/negácia
  				//pustil som
  				// led stale svieti
  	P1OUT = 0x41;
  	onesk(2);
  	while(P1IN & BIT3); //test pustene tl
  	           //stlacil som
  	P1OUT = 0x01; //LED zhasne

  	onesk(2);
	while(~P1IN & BIT3);//test stlacene tl
	 	 	 	//pustil som
				// led stale zhasnuta

	onesk(2);
	while(P1IN & BIT3); //test pustene tl
				// stlacil som
				// led stale zhasnuta
	P1OUT = 0x00;

	onesk(2);
	while(~P1IN & BIT3);
	P1OUT = 0x40;
	onesk(2);
	while(P1IN & BIT3);
	//onesk(2); netreba, je tam 'for'
  	for(j=0;j<3;j++)
  	{
  		P1OUT = 0x41;
  		onesk(50000);
  		//50000 je na signed integer
  		// uz prilis velka
  		// na unsigned integer este nie
  		P1OUT = 0x00;
  		onesk(50000);
  	}
  	while(~P1IN & BIT3); // test stlacene
  	onesk(2);
  	while(P1IN & BIT3);
  	P1OUT = 0x40;
  	onesk(2);
  	while(~P1IN & BIT3);
  	onesk(2);
  }
}

// Cakacia funkcia
void onesk(unsigned int i)   //definicia funkcie onesk
{
	do {(i--);
	    asm(" nop"); //musí by 1 medzera pred nop aby to chápal ako inštrukciu a nie ako návestie, instrukcia ktora nerobí niè
	}
    while (i != 0);
}
