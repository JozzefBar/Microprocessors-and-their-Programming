//***********************************************************
// Zbernica I2C, obvod TCA9534, nie PCF8574_PCF8574A
//***********************************************************

#include <msp430.h>

unsigned char i,j,outLED, co;
unsigned char adrreg[]={3, 1}; //  adresy registrov TCA9534 {confreg, outreg}
unsigned char datareg[]={0, 0}; //data do registrov TCA9534 {vsetky piny do vystupu TCA9534, zapni vsetky led}



void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;   // zastav watchdog
    outLED=1;
    i=1;
    j=1; //adresa bola poslanaa, j=2 -daata boli poslane

    P1DIR = 1; //P1.0 vystup (zel LED)

    //nastavenie alternativnej funkcie pinov
    // Piny P1.6 and P1.7 dalej nebudu riadene bitmi registra P1OUT:

    P1SEL |= BIT6|BIT7;  //pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)
                         //pin P1.5 {hodiny pri SPI) nepouzivame
    P1SEL2 |= BIT6|BIT7;     //to istee aj tu: pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)

    P2DIR |= BIT5; //vystup pre modru RGB LED
    P2DIR |= BIT1; //vystup pre cervenu RGB LED
    P2OUT &= ~(BIT5|BIT1); //zhasni modru a cervenu LED


	//inicializacia periferie I2C_USCI

    //uved modul do resetu, potom: I2C Master, zdroj hodiin,
    // adresa slave-u, smer daat, ......, uvolni USCI z resetu:

    // pred robenim zmien v USCI module, uved ho do stavu resetu:
    UCB0CTL1 |= UCSWRST; //uvedenie do resetu, pre istoru, po PUC je uz modul v resete




            //****** nastavenie bitov v registri UCB0CTL0 *****//

    //stav UCB0CTL0 po resete: 0x01, UCSYNC bit = 1, synchroonny mod -
    // - je riadeny hodinovym signalom 

    //bity registra UCB0CTL0, ktore je potrebne nulovat (pre istotu)
    UCB0CTL0 &= ~(UCA10|UCSLA10|UCMM); // tieto bity nuluj, pre istotu
    //UCA10 - vlastna addresa UCSI je 7bit-ova
    //UCSLA10 - adresa slave-u je 7-bit-ova
    //UCMM - nie multi-master, blok porovnavania adresy je neaktivny

    //bity registra UCB0CTL0, ktore je potrebne nastavit do log.1
    UCB0CTL0 |= (UCMST|UCMODE1|UCMODE0|UCSYNC);   //bity do log.1, musia byt nastavene
    
    //UCMST - master mode
    //UCMODE1|UCMODE0 - I2C mode
    //UCSYNC - synchroonny rezim, pre istotu
    

            //****** nastavenie bitov v registri UCB0CTL1 *****//

    //obsah registra UCB0CTL1 po resete: 0x01, USCI modul je drzany v stave resetu,
    // //v I2C moode mame rovnako nazvany register UCB0CTL0,
    //ale jeho styri horne bity maju uplne inyy vyznam ako v SPI moode!!!
    // v pripade registra UCB0CTL1, je v nom viacej nastavovacich bitov, ktore NEboli
    // pouzite v rezime SPI

    //bity registra UCB0CTL1 ktore su sice nulove ale pre istotu ich nulujeme
    UCB0CTL1 &= ~(UCSSEL1|UCTXNACK|UCTXSTP|UCTXSTT); //nulovanie bitov a sucasne musi byt modul stale v resete (UCSWRST bit=1)
    //UCTXNACK - potvrdzuj normalne, ACK bit
    //UCTXSTP negeneruj STOP stav, neskuor ho pouzijeme
    //UCTXSTT negeneruj START stav, neskuor ho pouzijeme


    //bity registra UCB0CTL1, ktore je potrebne nastavit do log.1
    UCB0CTL1 |=UCSSEL0|UCSSEL1|UCTR|UCSWRST;
    // zdroj hodin SMCLK, 1MHz, (UCSSEL1,0 = 1)
    // UCTR=1 - modul USCI bude vysielat data do externeho obvodu slave
    // (po vyslani adresy slave modul vysle R/*W bit =0)
    // UCSWRST=1 - drz modul v resete, pre istotu

    //registre delicky hodinoveho signalu:
    // musia byt nastavene po PUC resete, lebo nie su automaticky inicializovane
    UCB0BR0 = 30; //delicka, spodnych 8 bitov, delenie 8, minimalne dovolene je :4
    UCB0BR1 = 0; //delicka, hornych 8 bitov,
    // signal SCL bude mat frekvenciu 1000000/8=125000Hz. nie,
    // ale 112000 Hz (merane osciloskopom)

        //***** nastavenie adresy slave-u, register UCBxI2CSA
    // SA Slave Address TCA9534

    UCB0I2CSA=0x21;

    //prÌklad tvorby adresy
    // A6 A5 A4 A3 A2 A1 A0
    //  0  1  0  0  0  0  1
    //  0  1  0    0  0  0  1 ->  0x21
    
        //uvolnenie modulu USCI z resetu****

    UCB0CTL1 &= ~UCSWRST; //uvolni modul USCI z resetu

    //vyber priznakov
    IFG2 &=~(0xC); //vynuluj USCI B0 RX and TX priznaky
    IE2 |= UCB0TXIE; //povol priznak po starte vysielania dat/addresy
    // jeden vektor pre prijem, UCB0TXIFG aj vysielnie, UCB0RXIFG 

    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // nuluj vsetky stavove priznaky USCI I2C
    //UCB0I2CIE |= UCNACKIE|UCSTPIE|UCSTTIE|UCALIE;// povol vsetky stavove priznaky I2C
    UCB0I2CIE |= UCNACKIE; // povol len priznak nepotvrdenia
    // USCI je uvolnene davno z resetu

    
//nastavenie modulu casovaca a oscilator LFXT

//test start oscilatora LFXT
    P1OUT = 0x01;   // zelena LED svieti ak LFXT nebezi
    __bic_SR_register(OSCOFF);      
        do {                        
           IFG1 &= ~OFIFG;
           __delay_cycles(50);
           } while((IFG1&OFIFG));   
    P1OUT = 0x00; // zhasni LED, LFXT bezi

//nastavenie casovaca
    CCR0 = 3000;       

   TACTL = TASSEL_1 + MC_2;    // ACLK je zdrojom hodin
                               
   CCTL0 = CCIE;           // povol prerusenie, zmaz priznak

   //****** koniec nastavenie casovaca ***

   //** konfiguruje prijimac I2C
   UCB0CTL1 |=UCTXSTT;   //start vysielania
       j=1; // najprv addresu slave
       co=0; //bude confreg a vsetky piny do vystupu

   _BIS_SR(LPM0_bits + GIE);       

}

    
#pragma vector = TIMER0_A0_VECTOR       
__interrupt void porov (void)
{
// vyrob novy stav LED slpca

    outLED=outLED>>1; //zasviet LED o jednu nizsie
    if(outLED==0)outLED=0x80; // ak je 1 mimo 8 bitov

    //UCB0CTL1 |=UCTXSTP;
    UCB0CTL1 |=UCTXSTT;   //start vysielania
    j=1; // najprv addresu slave
    co=1; //bude outreg a outLED
    datareg[1]=~outLED;

    CCR0 += 3276; //8;
    
    P1OUT ^=1;///zmen zelenu LED
    P2OUT &= ~(BIT5+BIT1); //zhasni modru a cervenu RGB LED

}

#pragma vector = USCIAB0TX_VECTOR   //USCI_B0 I2C vysiel/prij addr/data!!!!
__interrupt void adresa_data (void)  // moze sa spustit  po priznakoch UCB0TXIFG a UCB0RXIFG
// povoleny je len UCB0TXIFG
{


    P2OUT ^= BIT5; //zmen modru LED
switch (j){
case 1:         // po zacati vysielania adresy sa prvy raz spusti aby naplnila prv˝m bajtom d·t register UCB0TXBUF.
    j=2;
    UCB0TXBUF=adrreg[co]; //ktory register TCA9534
    break;

case 2: // Druhy raz sa spusti, ked USCI zaËne vysielat prv˝ch 8 bitov d·t, aby sa znova
        //naplnil UCB0TXBUF druhym bajtom d·t.

    j=3;
    UCB0TXBUF=datareg[co]; // data do registra TCA9534
    break;


case 3:     //TretÌ raz sa spustÌ, ked sa druhy bajt skopÌruje do shift reg v USCI,
            //a teda sa zacne vysielanie druheho bajtu d·t.
            //Vtedy je ten spravny cas
            // nastavit generovanie STOP, ktore sa ale neprejavi okamzite, ale az po vyslani pr·ve vysielanÈho bajtu.
            // Do TXBUF sme v treùom behu nic nezapisali tak sa vygeneruje ten prednastaven˝ STOP stav
    j=1;
    UCB0CTL1 |=UCTXSTP; //az sa data vyslu, vysli stop
    break;

}



    IFG2 &=~(0x0C); //nuluj priznaky USCIB0 RX/TX adresa/data

}
#pragma vector = USCIAB0RX_VECTOR   //USCI_B0 I2C status, nie priznak UCB0RXIFG, !sme v mÛde I2C perifÈrUSCI
__interrupt void status (void)  // moze sa spustit ak UCALIFG, UCNACKIFG, ICSTTIFG, UCSTPIFG
// je povoleny iba UCNACKIFG
{
    UCB0CTL1 |=UCTXSTP; //adresa nerozpoznana vysli stop
    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // vynuluj vsetky priznaky
    P2OUT = BIT1; //zapni cervenu RGB LED

}










