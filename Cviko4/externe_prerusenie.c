#include <msp430.h>
int i,j;
void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// zastav watchdog

	// nastavenie registrov portu P1
	P1DIR = BIT0|BIT6;	//0x41			// pimy P1.0 a P1.6 nastav na vystup
	P1OUT = 0x00;					// obe LED (a vsetky ostatne piny) na 0V
	//P1REN = 0xff; // na P1.0 a P1.6, tam su LED, tam rezistory nepovoluj, lebo budu iba slabo svietit
	                // ma P1REN prednost pred P1DIR? (P1REN = 0xBE; )
	            // a tak isto, kde su  tlacitka a enkoder (P1.1, .2, .3 a .4),
	            //nemoze byt pull down a radsej ani pull up
	P1REN = 0xA0; //0b10100000;     //resitor enable
	//P1REN = 0b10110110; //B6, ak bolo len 1 tl na P1.3
	//P1REN = 0b11110111; //REN vsade okrem tl na P1.3
	                    // odber CPU maly, ale LED svietia slabo. Preto P1REN = 0b10110110


	// nastavenie registrov portu P2
	P2REN=0x3f; //povolit rezistory, aby definovali log uroven,
	            //P2 maa vyvedenych len 6 pinov
	P2OUT=0x00; //pull down, P2DIR je nulovy, vstup, to je v poriadku


	//nastavenie riadiacich registrov prerusenia od potu P1
	P1IES = BIT1 | BIT2 | BIT3;	//reaguj na zostupnu hranu iba na P1.1, tam je tlacitko
	//P1IES  &= ~ BIT3;	//ak chcem nabeznu hranu.... (pustenie tl.)
	P1IFG = 0;		//nuluj vsetkych osem priznakov   //dobre urobit predtym nez povolim prerusenie

	P1IE = BIT1 | BIT2 | BIT3;	//povol spustenie ISR len od pinu P1.1
	
	_BIS_SR(GIE);	//povol vsetky maskovatelne prerusenia      //global interrupt enable
					//od tohto okamihu je mozne spustit ISR

	//spotreba bez LPM4 rezimu
	//main - 340 uA
	//prerusenie - 352 uA

	//spotreba v LMP4 rezime
	//main - 0.1 uA
	//prerusenie - 352 uA

	_BIS_SR(GIE + LPM4_bits);// uved CPU do nizkoprikonoveho rezimu LPM4
    //_BIS_SR(GIE + CPUOFF + OSCOFF + SCG0 + SCG1);// uved CPU do nizkoprikonoveho rezimu LPM4

	while(1);		//nekonecna slucka
}
//#pragma vector = 2 // aj takto je mozne definovat vektor prerusenia od portu P1
					// pozri v: .../ccsv5/ccs_base/msp430/include/msp430g2231.h
                    // F3 - "Open declaration"

#pragma vector = PORT1_VECTOR
__interrupt void nieco (void)
{
	//P1IFG &= ~BIT1;	//viacpriznakova ISR, mazanie prizanku P1IFG.3 programom
/*
	P1OUT = 1;         //zapni zelenu (aj vypni vsetko ostatne)
	for (j=16; j > 0; j--){
			for (i = 20000; i > 0; i--);	//programove oneskorenie
			P1OUT ^= 0x41;          //zmen stav na p1.0 a P.6
			}
	P1OUT = 0x00;				//vypni obe LED

	P1IFG &= ~BIT1;     //ked vynulujeme priznak az na konci nie je mozne spustit rutinu znova pocas blikania lediek
*/
    if (P1IFG & BIT1)
    {
        P1OUT ^= 0x01;      //zmen len zelenu LED
        //P1OUT ^= BIT0;    // to iste co hore

        __delay_cycles(5000);

        P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
    }

    if (P1IFG & BIT2)
    {
        P1OUT ^= 0x40;      //zmen len cervenu LED

        __delay_cycles(5000);
        P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.2

    }

    if (P1IFG & BIT3)   //P1.3 - enkoder A
    {
        if (P1IN & BIT4)     //testujeme stav P1.4 - enkoder B
        {

            P1OUT ^= 0x01;  //zmen zelenu LED
        }
        else
        {
            P1OUT ^= 0x40;  //zmen cervenu LED
        }

        __delay_cycles(5000);
        P1IFG &= ~BIT3;     //nuluj len priznak P1IFG.3
    }



}


//080-436
//800-20800
