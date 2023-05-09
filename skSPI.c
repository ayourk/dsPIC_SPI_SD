/* ************************************************ *****************************
* skSPIlib.c - Function library for SPI communication *
* *
* SPI_Init - Processing to set and initialize SPI mode (master) *
* SPI_setDataMode - Processing to set SPI transfer mode *
* SPI_setClockDivider - Process to set SPI communication speed *
* SPI_transfer - Processing to send and receive data via SPI communication *
* *
* Note: SDI pin must be set to "digital input pin". *
* The SS (CS) pin used should be HIGH during initialization. *
* When using SSP2, declare "#define SPI_MSSP2_USE" in skI2Clib.h *
* ================================================= ===========================*
* VERSION DATE BY CHANGE/COMMENT *
* ------------------------------------------------- ---------------------------*
* 1.00 2012-04-30 Kimushige Create *
* 1.01 2012-12-29 Changed to support Kimushige XC8 C Compiler *
* 1.10 2013-02-18 Change Kimushige SPI_Init function *
* 1.11 2014-06-21 Kimushige changed comment of SPI_Init function *
* 2.00 2014-09-03 Kimushige Kobo 18F25K22 operation check, interrupt processing stopped *
* 2.10 2014-09-27 Change Kimushige SPI_Init function *
* 3.00 2015-04-05 Added Kimushige speed/transfer mode functions *
* 3.10 2015-04-20 Supports both Kimushige SSP1/SSP2 and 16F193x *
* 3.11 2015-05-23 Compatible with Kimushige 16F1825/1829 *
* ================================================= ===========================*
* PIC 12F1822 16F18xx 16F193x 18F25K22 *
* MPLAB IDE(V8.84) MPLAB X(V2.15) *
* MPLAB(R) XC8 C Compiler Version 1.00/1.32 *
**************************************************** **************************** */
#include < xc.h > _ 
#include " skSPI.h " _ 

# define  _30F6014A
/* ************************************************ *****************************
* SPI_Init(mode, divider, sdo) *
* Processing to set and initialize SPI mode *
* Stops interrupt processing and looks at "SSP1IF" directly *
* *
* mode : Set SPI transfer mode (combination of clock polarity and clock phase) *
* SPI_MODE0 = Polarity (0: LOW) Phase (0: at idle 0V, transfer with change from 0V to 5V) *
* SPI_MODE1 = Polarity (0: LOW) Phase (1: at idle 0V, transfer with change from 5V to 0V) *
* SPI_MODE2 = Polarity (1: HIGH) Phase (0: at idle 5V, transfer with change from 5V to 0V) *
* SPI_MODE3 = Polarity (1: HIGH) Phase (1: at idle 5V, transfer with change from 0V to 5V) *
*divider : Set SPI communication speed*
* SPI_CLOCK_DIV4 = Fosc/4 *
* SPI_CLOCK_DIV16 = Fosc/16 *
* SPI_CLOCK_DIV64 = Fosc/64 *
* SPI_CLOCK_DIVT2 = 1/2 the output of TMR2 *
* SPI_CLOCK_DIVADD = FOSC/((SSPxADD + 1)*4) *
* sdo : Specify the SDO transmission pin number to use *
**************************************************** **************************** */
void  SPI_Init ( void )
{
 // char con , stat ;

#if defined (_12F1822)
SDOSEL = 0 ; // Set pin 7 (RA0) as SDO transmit pin
 if (sdo == 3 ) SDOSEL = 1 ; // Set pin 3 (RA4) as SDO transmit pin
# endif

#if defined (_16F1823)
SDOSEL = 0 ; // Use pin 8 (RC2) as SDO transmit pin
 if (sdo == 3 ) SDOSEL = 1 ; // Set pin 3 (RA4) as SDO transmit pin
# endif

#if defined (_16F1825)
SDO1SEL = 0 ; // Use pin 8 (RC2) as SDO1 transmit pin
 if (sdo == 3 ) SDO1SEL = 1 ; // Set pin 3 (RA4) as SDO1 send pin
# endif

# if defined(_16F1826) || defined(_16F1827)
SDO1SEL = 0 ; // Use pin 8 (RB2) as SDO1 transmit pin
 if (sdo == 15 ) SDO1SEL = 1 ; // Set 15th pin (RA6) as SDO1 send pin
# endif

#if defined (_16F1829)
SDO2SEL = 0 ; // Set pin 15 (RC1) as SDO2 transmit pin
 if (sdo == 2 ) SDO2SEL = 1 ; // Set pin 2 (RA5) as SDO2 transmit pin
# endif
/*
con = 0b00100000 ; // clock polarity set to FOSC/4 clock in LOW master mode
stat = 0b00000000 ; // send data on falling clock phase
con = con | divider ; // set the specified clock speed
if (mode == SPI_MODE1 || mode == SPI_MODE3) {
stat = stat | 0x40 ; // Set specified clock phase
}
if (mode == SPI_MODE2 || mode == SPI_MODE3) {
con = con | 0x10 ; // set specified clock polarity
}
SPI_SSPCON1 = con ;
SPI_SSPSTAT = stat ;
SPI_SSPIF = 0 ; // Initialize SPI interrupt flag
PEIE = 1 ; // Peripheral interrupt enabled
GIE = ​​1 ; // Allow all interrupt processing
*/

#if defined (_30F6014A)
SPI1CONbits.MSTEN = 1 ; _
SPI1STATbits.SPIROV = 0 ; _
SPI1CONbits.CKE = 0 ; _
SPI1CONbits.CKP = 1 ; _
SPI1CONbits.DISSDO = 0 ; _
SPI1CONbits.FRMEN = 0 ; _
SPI1CONbits. MODE16 = 0 ;
SPI1CONbits.SMP = 0 ; _
SPI1CONbits.SSEN = 0 ; _
SPI1CONbits.PPRE = 0 ; _
SPI1CONbits.SPRE = 7 ; _
SPI1STATbits.SPIEN = 1 ; _
# endif

}
/* ************************************************ *****************************
* SPI_setDataMode(mode) *
* Processing to set SPI transfer mode *
* mode : Set SPI transfer mode (combination of clock polarity and clock phase) *
**************************************************** **************************** */

/*
void SPI_setDataMode(char mode)
{
if (mode == SPI_MODE1 || mode == SPI_MODE3) {
SPI_SSPSTAT = SPI_SSPSTAT | 0x40 ; // Set the specified clock phase to 1
} else {
SPI_SSPSTAT = SPI_SSPSTAT & 0xBF ; // set the specified clock phase to 0
}
if (mode == SPI_MODE2 || mode == SPI_MODE3) {
SPI_SSPCON1 = SPI_SSPCON1 | 0x10 ; // Set the specified clock polarity to 1
} else {
SPI_SSPCON1 = SPI_SSPCON1 & 0xEF ; // set the specified clock polarity to 0
}
}
*/

/* ************************************************ *****************************
* SPI_setClockDivider(divider) *
* Process for setting SPI communication speed *
*divider : Set SPI communication speed*
* SPI_CLOCK_DIVADD = FOSC/((SSPxADD + 1)*4) *
* rate : Value to be set in SSPxADD *
**************************************************** **************************** */
/*
void SPI_setClockDivider(char divider,char rate)
{
if (divider == SPI_CLOCK_DIVADD) {
SPI_SSPADD = rate ;
}
SPI_SSPCON1 = (SPI_SSPCON1 & 0xF0) | divider ; // Sets the specified clock speed
}
*/
/* ************************************************ *****************************
* ans = SPI_transfer(dt) *
* Processing for sending and receiving data via SPI communication *
* *
* dt : Specify 8-bit data to send *
* ans : Returns 8-bit received data *
**************************************************** **************************** */
char  SPI_transfer ( char dt)
{
SPI1BUF = dt ; // Send data
 while (SPI1STATbits. SPITBF == 0 ) ; // Wait for reception
IFS0bits.SPI1IF = 0 ; // Clear flag
 return SPI1BUF ; // Receive data
}
