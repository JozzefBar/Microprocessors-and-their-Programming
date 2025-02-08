//***********************************************************
// Stopky s LED displejom a riadenÌm, 4x 7segmentov
//***********************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT0; //(1 << 0)
#define FS_L        P1OUT &= ~BIT0; //(1 << 0)

void fs_strobe(void);


void fs_strobe(void) //vygeneruj prepisovacÌ pulz na P1.0
    {
        FS_H; //P1OUT |= BIT0;
        asm("   nop");
        FS_L; //P1OUT &= ~BIT0;
    }



const unsigned char tvary_cislic[10] = {0x3F, 6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7F, 0x6F};
// ulozene do FLASH
// //"1"     6 = 0000 0110, hgfedCBa
// //"6"  0x7c = 0111 1100 -> 0111 1101 0x7d
// //"0"  0x3F = 0011 1111, hgFEDCBA

unsigned char jed_sek=0, des_sek=0, jed_min=0, des_min=0, RXin;

unsigned char i=0; //ktor˝ r·d sa vysiela


void main(void)
{

    WDTCTL = WDTPW + WDTHOLD;               // zastav watchdog

    i=1;


    P1OUT = 0x00;             // inicializ·cia P1, hlavne kvÙli prepisovaciemu pulzu
    P1DIR |= BIT0;  // nastav P1.0 na v˝stup, smer ostatn˝ch pinov sa nastavÌ pri pripojovanÌ perifÈrie USCI niûöie

    //nastavenie alternatÌvnej funkcie pinov. Piny P1.5 a P1.7 uû nebud˙ vic riadenÈ registrom P1OUT:

    P1SEL |= BIT7|BIT5;       //pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)
                            //sign·l UCB0MISO nepouûÌvame, tak pin P1.6 ostane v z·kladnej I/O funkcii, cez P1OUT
    P1SEL2 |= BIT7|BIT5;     //to istÈ: pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)


    // nastavenie modulu/periferie USCI pre mod SPI...
     // nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
     UCB0CTL1 |= UCSWRST; //vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


     //stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
     UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // bity do log.0, pre istotu
     UCB0CTL0 |= (UCCKPH|UCMSB|UCMST|UCSYNC);   //bity do log.1, MUSIME nastavit

     //zdroj signalu pre BRCLK:
     UCB0CTL1 &= ~(UCSSEL1); //bity do nuly,
     UCB0CTL1 |=(UCSSEL0); //bity do log.1, zdroj hodin je ACLK, 32kHz

     //delicka frekvencie BRCLK pre UCB0CLK - dva registre:
     // registre nie su inicializovane po resete, treba ich nastavit
     UCB0BR0 = 1; //delicka hodin, spodnych 8 bitov, delenie 1
     UCB0BR1 = 0; //delicka hodin, hornych 8 bitov

     UCB0CTL1 &= ~UCSWRST; //uvolni USCI z resetu, modul je pripraveny vysielat data

     // ******************** koniec nastavovania modulu USCI
     // teraz je modul USCI v mÛde SPI a je pripraven˝ vysielaù d·ta

    IE2 |= UCB0RXIE; //UCB0TXIE; //povol priznak od PRIJATIA dat, nie od zaciatku vysielania
     // pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR

//kontrola Ëinnosti kryöt·lovÈho oscil·tora LFXT, je potrebny ako pre casovac tak aj pre USCI

    P1OUT = 0x40;   // cervena led indikuje, ze LFXT este nebezi
    __bic_SR_register(OSCOFF);      // zapni oscilator LFXT1, len pre istoru
        do {
           IFG1 &= ~OFIFG;      //periodicky nunluj priznak nefunkcnosti oscilatora
           __delay_cycles(50);
           } while((IFG1&OFIFG));   //je stale nefunkcny?
    P1OUT = 0x00; // LFXT bezi, zhasni LED

//nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami bloku 4x8 bitov dat z USCI

    CCR0 = 3000;               // komparacny register CCR0 (TACCR0)
                               //startovacia hodnota
   TACTL = TASSEL_1 + MC_2;    // hodinovy signal pre casovac bude ACLK, 32kHz

// ************** koniec nastavovania modulu casovaca


   CCTL0 = CCIE;           // povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           //prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

   //CAST KODU PRE TLACIDLA

       //nastavenie riadiacich registrov prerusenia od potu P1
       P1IES = BIT1 | BIT2 | BIT3;   //reaguj na zostupnu hranu iba na P1.1 aj na .2 aj .3, tam je tlacitko
       //P1IES  &= ~ BIT3; //ak chcem nabeznu hranu.... (pustenie tl.)
       P1IFG = 0;      //nuluj vsetkych osem priznakov

       P1IE = BIT1 | BIT2 | BIT3;    //povol spustenie ISR len od pinu P1.1



   _BIS_SR(LPM0_bits + GIE);    // vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                                // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                                //  lebo napaja casovac aj seriovy port

    }

    // ISR od ËasovaËa, LEN od prÌznaku TACCTL0_CCIFG

#pragma vector = TIMER0_A0_VECTOR
__interrupt void komp0 (void)
{
  // ISR sa spustila lebo bol obsah registrov TAR a CCR0 rovnak˝ (teraz uû nemusÌ tak byù)

    //jed_sek++;        //zvysovanie
    jed_sek--;          //znizovanie, pretecenie funguje aj naopak 1--0--255...
    //pre dekrementovanie
    if(jed_sek == 255) {jed_sek = 9; des_sek--;}
    if(des_sek == 255) {des_sek = 5; jed_min--;}
    if(jed_min == 255) {jed_min = 9; des_min--;}
    if(des_min == 255) {des_min = 5; }

    //pre inkrementovanie (podmienky predtym nikdy nenastavia hodnoty aby sa zapli tieto - nemusime zakomentovat)
    if (jed_sek==10){jed_sek=0,des_sek++;}
    if (des_sek==6){des_sek=0,jed_min++;}
    if (jed_min==10){jed_min=0,des_min++;}
    if (des_min==6){des_min=0;}



    UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
    //do registra vloû prvok z poæa podæa hodnoty ËÌslice r·dy jedotiek sek˙nd
    //ötart prenosu prv˝ch 8 bitov
    i=1; // ktor˝ r·d bude poslan˝ ako nasleduj˙ci - des_sek

    CCR0 += 32768; //spusti ISR od ËasovaËa kaûd˙ sekundu

}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void after_sent (void) //prvy krat sa spusti az ked sa vysle tvar sekund
{
    switch(i)
            {
            case 1:
                UCB0TXBUF =  ~(tvary_cislic[des_sek]);
                i=2; // ktor˝ r·d bude poslan˝ ÔalöÌ - jed_min)
            break;

            case 2:
                UCB0TXBUF =  ~(tvary_cislic[jed_min]);
                i=3; // ktor˝ r·d bude poslan˝ ÔalöÌ - des_min)
            break;

            case 3:
                UCB0TXBUF =  ~(tvary_cislic[des_min]);
                i=4; // Ëo bude poslanÈ ako Ôalöie  - prepisovacÌ pulz
            break;

            case 4:
                fs_strobe(); //prepisovacÌ pulz

                i=5;    // ak i=5 ziadny case neplati len zmaz priznak
            break;
            }

    RXin=UCB0RXBUF; // precitanim UCB0RXBUF nulujem priznak UCB0RXIFG

}

#pragma vector = PORT1_VECTOR
__interrupt void nieco(void)
{
    if (P1IFG & BIT1)
    {
        CCTL0 ^= CCIE;              //vypneme prerusenie alebo zapneme pri stlaceni cize zastavime alebo pustime cas

        __delay_cycles(5000);

        P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
    }

    if (P1IFG & BIT2)           //vynulujeme stopky;
    {
        jed_sek = 0;
        des_sek = 0;
        jed_min = 0;
        des_min = 0;

        __delay_cycles(5000);
        P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.2

    }

    if (P1IFG & BIT3)           //pri toceni enkoderom bud zmensenie alebo zvacsenie sekund
    {
        if (P1IN & BIT4)
        {

            jed_sek++;

            if (jed_sek==10){jed_sek=0,des_sek++;}
            if (des_sek==6){des_sek=0,jed_min++;}
            if (jed_min==10){jed_min=0,des_min++;}
            if (des_min==6){des_min=0;}

            UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
            //do registra vloû prvok z poæa podæa hodnoty ËÌslice r·dy jedotiek sek˙nd
            //ötart prenosu prv˝ch 8 bitov
            i=1; // ktor˝ r·d bude poslan˝ ako nasleduj˙ci - des_sek
        }
        else
        {
            jed_sek--;

            if(jed_sek == 255) {jed_sek = 9; des_sek--;}
            if(des_sek == 255) {des_sek = 5; jed_min--;}
            if(jed_min == 255) {jed_min = 9; des_min--;}
            if(des_min == 255) {des_min = 5; }

        }


        UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
        //do registra vloû prvok z poæa podæa hodnoty ËÌslice r·dy jedotiek sek˙nd
        //ötart prenosu prv˝ch 8 bitov
        i=1; // ktor˝ r·d bude poslan˝ ako nasleduj˙ci - des_sek


        __delay_cycles(5000);
        P1IFG &= ~BIT3;     //nuluj len priznak P1IFG.3
    }
}
