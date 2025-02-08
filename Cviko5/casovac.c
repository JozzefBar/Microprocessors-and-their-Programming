//**************************************************************************
// MPP cv.5
// Timer0_A3, prepinaj P1.0 a nezavisle aj P1.6, CCR0, kontinualny mod pocitadla, LFXT do SMCLK
//priorita spustenia viacerych ISR
//**************************************************************************

#include <msp430.h>


void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// zastav WDT

	P1DIR |= 0x41;					// piny P1.0 a P1.6 nastav ako vystupy
	P1OUT = 0x00;					// vynuluj vsetky bity, piny portu P1

	//Blok generovania hodinovych signalov
	//po resete je nastavene:

	//DCOCLK/1 -> MCLK -> CPU, (frekvencia cca 1000000 Hz)
	//LFXT/1   -> ACLK        (frekvencia presne 32768 Hz)

	// potrebujeme zmenit v module hodinoveho signalu:
	//LFXT/1 -> SMCLK
	BCSCTL2 |= SELS;


	// mozne nastavenie kondenzatorov oscilatora  LFXT
	//BCSCTL3 |=XCAP1; //kondenzatory pri krystali: xcap=10b -> 10 pF
	//BCSCTL3 &=~XCAP0; // po resete xcap=01b -> 6 pF, bolo malo

	P1OUT = 0x41;	// indikaacia, ze LFXT nebezi, program ost·va v nasledovnej sluËke
	__bic_SR_register(OSCOFF);		// zapnutie oscillatora LFXT1
		do {						// pre istotu
		   IFG1 &= ~OFIFG;
		   __delay_cycles(50);
		   } while((IFG1&OFIFG));	// cakanie na rozbeh LFXT
	P1OUT = 0x00; // LFXT bezi, zhasni LED-ky

	// potom samotne nastavenie:


	//nastavenie registrov casovaca	 Timer0_A3
	// pocitadlo TAR po resete nepocita, je neaktivne

	CCR0 = 3000;					// komparacny register CCR0
									//startovacia hodnota, od nej frekvencia
									// blikania LED nezavisi.


	//TACTL = TASSEL_1 | MC_2;		//zdroj hodÌn ACLK podla datasheatu, MC_2 tieû podla datashietu o ak˝ mod ide
	TACTL = TASSEL_2 | MC_2;        // skontrolovat parametre podla zadania, skomentovat v zavere
									// "Open declaration" pre TASSEL_1
		// ? je hodinovy signal SMCLK zdrojom impulzov pre pocitadlo TAR ?,
									// ? je start pocitania pocitadla v mode kontinualneho pocitania
									// (od  0 do 0xffff)?
									//  prikaz nuluje zvysne bity registra TACTL. Skontrolujem, co sa este
									//vsetko zmeni a ci si to mozem dovolit zmenit. Aky je stav registra TACTL po resete?
									// nuluje sa priznak TAIFG?


	CCTL0 = CCIE;					// povolenie prerusenia od priznaku nastatia rovnosti obsahu TAR a CCR0
									// mozem ostatne bity vynulovat? (znova: stav registra CCTL0 po resete )
	                                //v akom stave je priznakovy bit?

	CCTL1 = CCIE;

    //nastavenie riadiacich registrov prerusenia od potu P1
    P1IES = BIT1 | BIT2;   //reaguj na zostupnu hranu iba na P1.1 a P1.2, tam je tlacitko
    //P1IES  &= ~ BIT3; //ak chcem nabeznu hranu.... (pustenie tl.)
    P1IFG = 0;      //nuluj vsetkych osem priznakov
    P1IE = BIT1 | BIT2;    //povol spustenie ISR len od pinu P1.1

	_BIS_SR(LPM0_bits + GIE);		// prechod do modu LPM0, vsetky maskovatelne prer. povolit
									// vypneme len MCLK, LFXT oscilator musi ostat bezat, lebo napaaja casovac

}

// Obsluzny program  priznak TACCTL0_CCIFG (iba ten) modulu casovaca Timer0_A3

#pragma vector = TIMER0_A0_VECTOR		//skontrolujte nazov vektora
__interrupt void komp0 (void)
{

	P1OUT ^= 0x01;		// zmen log. stav na P1.0

	//LFXT1 32768 Hz
	CCR0 += 32768;					// pripocitaj hodnotu delta k obsahu CCR0
									// frekvencia zavisi od hodnoty delta
}
//nemusÌme vynulovaù prÌznak lebo rutina je jednoprÌznakov·

#pragma vector = TIMER0_A1_VECTOR        //skontrolujte nazov vektora
__interrupt void komp1 (void)
{

    P1OUT ^= 0x40;      // zmen log. stav na P1.6
            //BIT6

    //LFXT1 32768 Hz
    CCR1 += 32768/2;                  // pripocitaj hodnotu delta k obsahu CCR0
                                    // frekvencia zavisi od hodnoty delta
    CCTL1 &= ~CCIFG;             //musÌme vynulovaù prÌznak lebo rutina je viacprizn·kov·
}

#pragma vector = PORT1_VECTOR
__interrupt void nieco(void)
{
    //zelenÈ tlaËidlo P1.1
    if (P1IFG & BIT1)
    {
        //P1OUT ^= 0x01;      //zmen len zelenu LED
        CCTL0 ^= CCIE;
        P1OUT &= ~0x01;

        __delay_cycles(5000);

        P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
    }

    //ËervenÈ tlaËidlo P1.2
    if (P1IFG & BIT2)
    {
        //P1OUT ^= 0x40;      //zmen len cervenu LED
        CCTL1 ^= CCIE;
        P1OUT &= ~0x40;

        __delay_cycles(5000);
        P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.2
    }
}
