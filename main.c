/* ************************************************ *****************************
* SD2.c - MMC/SDC card (SPI) write test *
* *
* SS: Pin 13 (RC2) SCK: Pin 14 (RC3) SDI: Pin 15 (RC4) SDO: Pin 16 (RC5) *
* ================================================= ===========================*
* VERSION DATE BY CHANGE/COMMENT *
* ------------------------------------------------- ---------------------------*
* 1.00 2012-01-30 Kimushige Create *
* 2.00 2014-02-02 Kimcha Koubou (Kimushige) Supported by changing "skMonitorLCD.c" *
* 2.10 2014-09-27 Kimushige interrupt processing stopped by changing "skSDlib.c" *
* 3.00 2015-10-26 Kimushige rewrite for MPLAB X/XC8 V1.32 *
* ================================================= ===========================*
* PIC 16F1938 *
* MPLAB X(v2.15) *
* MPLAB(R) XC8 C Compiler Version 1.32 *
**************************************************** **************************** */
#include <xc.h>
# include <string.h>
#include <libpic30.h>
// #include "skMonitorLCD.h"
// #include "skSPI.h"
// #include "skSD.h"

# define  _XTAL_FREQ  8000000  // for delay (when operating at 8MHz clock)

/*
// set configuration 1
#pragma config FOSC = INTOSC // use internal clock (INTIO)
#pragma config WDTE = OFF // No watchdog timer (OFF)
#pragma config PWRTE = ON // Start the program 64ms after power on (ON)
#pragma config MCLRE = OFF // Don't use external reset signal, use digital input (RE3) pin (OFF)
#pragma config CP = OFF // Don't protect program memory (OFF)
#pragma config CPD = OFF // Data memory is not protected (OFF)
#pragma config BOREN = ON // Power supply voltage drop constant monitoring function ON (ON)
#pragma config CLKOUTEN = OFF // use CLKOUT pin on RA6 pin (OFF)
#pragma config IESO = OFF // No startup by external/internal clock switching (OFF)
#pragma config FCMEN = OFF // No external clock monitoring (OFF)
// setting configuration 2
#pragma config WRT = OFF // Do not protect Flash memory (OFF)
#pragma config VCAPEN = OFF // Do not use capacitor for low voltage regulator (OFF)
#pragma config PLLEN = OFF // Do not operate the operating clock at 32MHz (4xPLL) (OFF)
#pragma config STVREN = ON // Reset when stack overflows or underflows (ON)
#pragma config BORV = HI // Power supply voltage drop constant monitoring voltage (2.5V) setting (HI)
#pragma config LVP = OFF // Do not use low voltage programming function (OFF)
*/

// FOSC
# pragma config FOSFPR = FRC // Oscillator (Internal Fast RC (No change to Primary Osc Mode bits))
# pragma config FCKSMEN = CSW_FSCM_OFF // Clock Switching and Monitor (Sw Disabled, Mon Disabled)
// FWDTs
# pragma config FWPSB = WDTPSB_16 // WDT Prescaler B (1:16)
# pragma config FWPSA = WDTPSA_512 // WDT Prescaler A (1:512)
# pragma config WDT = WDT_OFF // Watchdog Timer (Disabled)
// FBORPOR
# pragma config FPWRT = PWRT_64 // POR Timer Value (64ms)
# pragma config BODENV = BORV_27 // Brown Out Voltage (2.7V)
# pragma config BOREN = PBOR_ON // PBOR Enable (Enabled)
# pragma config MCLRE = MCLR_DIS // Master Clear Enable (Disabled)
// FBS
# pragma config BWRP = WR_PROTECT_BOOT_OFF // Boot Segment Program Memory Write Protect (Boot Segment Program Memory may be written)
# pragma config BSS = NO_BOOT_CODE // Boot Segment Program Flash Memory Code Protection (No Boot Segment)
# pragma config EBS = NO_BOOT_EEPROM // Boot Segment Data EEPROM Protection (No Boot EEPROM)
# pragma config RBS = NO_BOOT_RAM // Boot Segment Data RAM Protection (No Boot RAM)
// FSS
# pragma config SWRP = WR_PROT_SEC_OFF // Secure Segment Program Write Protect (Disabled)
# pragma config SSS = NO_SEC_CODE // Secure Segment Program Flash Memory Code Protection (No Secure Segment)
# pragma config ESS = NO_SEC_EEPROM // Secure Segment Data EEPROM Protection (No Segment Data EEPROM)
# pragma config RSS = NO_SEC_RAM // Secure Segment Data RAM Protection (No Secure RAM)
// FGS
# pragma config GWRP = GWRP_OFF  // General Code Segment Write Protect (Disabled)
# pragma config GCP = GSS_OFF // General Segment Code Protection (Disabled)
// FICD
# pragma config ICS = ICS_PGD // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

/* ************************************************ *****************************
* Main process *
**************************************************** **************************** */
int  main (){

 union {
 char c[ 2 ] ;
 int i ;
} ans;
 char dt[ 3 ][ 22 ] = {
 " PIC16F1938 \r\n " ,
 " MicroSD \r\n " ,
 " Arduino Duemilanove \r\n "
} ;

OSCCON = 0b01110010 ; // Internal clock is 8MHz
// ANSELA = 0b00000000 ; // AN0-AN4 are all unused digital I/O
// ANSELB = 0b00000000 ; // AN8-AN13 are all unused digital I/O
// ANSELC = 0b00000000 ; // AN14-AN19 analog not used, assigned to digital I/O
// TRISA = 0b00000000 ; // Assign all pins (RA) to outputs (0: output 1: input)
// TRISB = 0b00000000 ; // Assign all pins (RB) to outputs
// TRISC = 0b00010000 ; // Pin (RC) assigns only RC4 (SDI) to input
// PORTA = 0b00000000 ; // Initialize RA output pins (all low)
// PORTB = 0b00000000 ; // Initialize RB output pins (all LOW)
// PORTC = 0b00000000 ; // Initialize RC output pins (all low)
TRISD = 0 ; // Set all port D pins as outputs
TRISA = 0x00 ; // 1 for input 0 for output RA0-RA7 all set to output (RA5 is input only)
TRISF = 0b0000000011000000 ; // 1:in 0:out SDI1(RF7:in) SDO1(RF8:out) SCK1(RF6:in) SS(RF2:out)
PORTA = 0x00 ; // Initialize output pins (set all to LOW)
PORTF = 0x0000 ; // Initialize output pins (set all to LOW)

// MonitorInit() ; // Initialize to send to LCD monitor

 // Initialize SPI (Clock polarity: HIGH Clock phase: Rise Communication speed: Fosc/64)
 SPI_Init ();

// MonitorCls(0) ; // Clear the LCD monitor screen
 __delay32 ( 9000000 ); // 3s delay

 // Initialize MMC/SDC
ans. i = SD_Init () ;
 if (ans. i == 0 ) {
 // create and open a new file
ans.i = SD_Open ( " ABC.TXT " ,O_RDWR) ;
 if (ans. i == 0 ) {
// MonitorPuts("The write start");
 // write to file line by line
ans. i = SD_Write (dt[ 0 ], strlen (dt[ 0 ])) ;
ans. i = SD_Write (dt[ 1 ], strlen (dt[ 1 ])) ;
ans. i = SD_Write (dt[ 2 ], strlen (dt[ 2 ])) ;
 // close the file
 SD_Close ();
// MonitorPutc(0x11) ;
 if (ans. i != - 1 ) /* MonitorPuts("Successful End!!") */ ;
 else  /* MonitorPuts("Abnormal Write!!") */ ;
} else {
// MonitorPutc(0x11) ;
// MonitorPuts("Error Open!!") ; // open error
}
} else {
// MonitorPutc(0x11) ;
// MonitorPuts("Error Init ") ; // initialization error
// MonitorPath(ans.c[1]);
// MonitorPut(ans.c[0]) ;
}

 while ( 1 ) ; // Interrupt processing

 return  0 ;
}
