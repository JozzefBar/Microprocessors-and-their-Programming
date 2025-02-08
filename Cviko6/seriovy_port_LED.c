//***********************************************************
// SPI komunikacia s modulom USCI, LED stlpec
//***********************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT0; //(1 << 0)
#define FS_L        P1OUT &= ~BIT0; //(1 << 0)

void fs_strobe(void);
unsigned char i,RXin,outLED;

void fs_strobe(void) //Generuj SW zapisovaci pulz 74HC595, signal STCP, pin P1.0
    {
        FS_H; //P1OUT |= BIT0;
        asm(" nop");
        FS_L; //P1OUT &= ~BIT0;
    }


void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;      // Zastav casovac watchdogu
    outLED=1;
    i=1;

	// inicializaci P1
    P1OUT = 0x00;
    P1DIR |= BIT7|BIT5|BIT0;  // nastav piny 0,5,7 do vystupu, pin 6 je RX, vstup
                              //ale aj tak sa to nastavi pri nast. alternat. f-cii pinov

    //nastavenie alternativnej funkcie pinov.
    //Piny P1.5 and P1.7 uz dalej nebudu riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;       //pripoj vyvod USCI UCB0MOSI (bude  vystup dat) na pin P1.7 (BIT7)
                              //a vystup hodin. sig. UCB0CLK na pin P1.5 (BIT5)
                            //vyvod MISO, P1.6 nebudeme pouzivat, ostane riadeny reg-om P1OUT
    P1SEL2 |= BIT7|BIT5;     //to iste, MOSI na P1.7, UCBOCLK na P1.5


    // nastavenie modulu/periferie USCI pre mod SPI...
    // nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST; //vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


    //stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // bity do log.0, pre istotu
    //UCB0CTL0 |= (UCCKPH|UCCKPL|UCMSB|UCMST|UCSYNC);   //bity do log.1, MUSIME nastavit
    UCB0CTL0 |= (UCCKPL|UCMSB|UCMST|UCSYNC);   //zmena fazy clk

    //zdroj signalu pre BRCLK:
    UCB0CTL1 &= ~(UCSSEL1); //bity do nuly,
    UCB0CTL1 |=UCSSEL0; //bity do log.1, zdroj hodin je ACLK, 32kHz
    //iny sposob vyberu zdroja signalu pre BRCLK
    //UCB0CTL1 |= (UCSSEL_1+UCSWRST);

    //delicka frekvencie signalu BRCLK pre signal UCB0CLK - dva registre:
    // registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 1; //delicka hodin, spodnych 8 bitov, delenie 1
    UCB0BR1 = 0; //delicka hodin, hornych 8 bitov

    UCB0CTL1 &= ~UCSWRST; //uvolni USCI z resetu, modul je pripraveny vysielat data

    // ******************** koniec nastavovania modulu USCI


    //priznak zaciatku vysielania, UCB0TXIFG je po uvolneni z resetu rovny log.1
    //priznak konca prijmu dat, UCB0RXIFG je =0.
    // ISR potrebujem spustit v case, az ked sa data vyslu,
    //pouzijem k tomu priznak prjatia dat, UCB0RXIFG

    IE2 |= UCB0RXIE; //UCB0TXIE; //povol priznak od PRIJATIA dat, nie od zaciatku vysielania.
    // Pokial som nezacal vysielat, tak priznak UCB0RXIFG sa nema ako nastavit a spustit ISR

//*********************   nastavenie modulu casovaca

    //najprv start oscilatora LFXT, je potrebny ako pre casovac tak aj pre USCI
    P1OUT = 0x40;   // cervena led indikuje, ze LFXT este nebezi
    __bic_SR_register(OSCOFF);      // zapni oscilator LFXT1, len pre istoru
        do {
           IFG1 &= ~OFIFG;      //periodicky nunluj priznak nefunkcnosti oscilatora
           __delay_cycles(50);
           } while((IFG1&OFIFG));   //je stale nefunkcny?
    P1OUT = 0x00; // LFXT bezi, zhasni LED

    //nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami dat z USCI
    CCR0 = 3000;               // komparacny register CCR0 (TACCR0)
                               //startovacia hodnota

   TACTL = TASSEL_1 + MC_2;    // hodinovy signal pre casovac bude ACLK
                               // MC_2 - pocitaj dookola, od 0 to 0xffffh a znova od 0
                               // v prikaze je "=" vsetky ostatne bity su nulovane
                               // prikaz tiez sucasne nuluje priznak TAIFG

   // ************** koniec nastavovania modulu casovaca

   CCTL0 = CCIE;           // povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           //prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

   _BIS_SR(LPM0_bits + GIE);    // vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                                // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                                //  lebo napaja modul casovaca a aj modul serioveho portu

    }



// ISR od casovaca, priznak TACCTL0_CCIFG
#pragma vector = TIMER0_A0_VECTOR
__interrupt void rovnost (void)
{

    // ISR sa spusti na zaklade rovnosti hodnoty registrov TAR and CCR0.
    // ked ISR bezi, rovnost uz nemusi platit, TAR pocita stale dalej
    switch (i)
    {
        case 1:
            outLED=outLED<<1; //zasviet LED o jednu vyssie
            if(outLED==0) // ak je 1 mimo 8 bitov
            {
                outLED=0x40; // vrat na predposlednu poziciu
                i=2;
            }
        break;
        case 2:
            outLED=outLED>>1; //zasviet LED o jednu nizsie
            if(outLED==0) // ak je 1 mimo 8 bitov
            {
                outLED=2; // vrat na predposlednu poziciu z druheho konca
                i=1;
            }
        break;
    }

    //FS_H; //P1OUT |= BIT0;
      //      asm(" nop");
    //FS_L; //P1OUT &= ~BIT0;

    UCB0TXBUF = ~outLED; //start vysielania dat

    //UCB0TXBUF = 0b01110110; //0x76 pre prednasku
    //UCB0TXBUF = 0b01110111; //0x77 ide aj pri CKPL,CKPH 0,1 aj 1,0

    //pri zapise do UCB0TXBUF dojde k nulovaniu priznaku UCB0TXIFG.
    //Kedze sa ale predtym nevysielalo a shift register je prazdny, dojde k okamzitemu kopirovaniu dat
    // z UCB0TXBUF do shift registra. Register UCB0TXBUF je opat "prazdny" co hlasi
    //opatovnym nastavenim priznaku UCB0TXIFG znova do log.1.
    //V tomto kode ale priznak UCB0TXIFG nepouzivame.


    CCR0 += 11;  // dlzka pauzy medzi dvoma vysielaniami, parameter delta.
                    // Ako casto sa meni stav LED stlpca
                    // CCR0 += 11;

}

#pragma vector = USCIAB0RX_VECTOR   //ISR sa pusti, ak vsetky data boli prijate/vyslane
__interrupt void po_prijati (void)
{

    fs_strobe(); //vygeneruj zapisovaci pulz pre externy 74HC595

    /*FS_H; //P1OUT |= BIT0; //vygeneruj vysielaci pulz pre externy 74HC595
       asm(" nop");
    FS_L; //P1OUT &= ~BIT0;
*/


    //RXin=UCB0RXBUF;       // spravne nulovanie priznaku: precitanim UCB0RXBUF nulujem priznak UCB0RXIFG
    IFG2 &=~(UCB0RXIFG);    // nespravne nulovanie priznaku, pri dalsom vysielani sa nastavi bit UCOE,
                            // Overrun Error, hlásiaci, že som neprecital som prijate data z UCB0RXBUF

}










