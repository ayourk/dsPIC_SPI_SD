/* ************************************************ *****************************
* skSPIlib.h - Include file for functions that perform SPI communication *
* (PIC 12F1822 16F182x 16F193x 18F25K22) *
* *
* ================================================= ===========================*
* VERSION DATE BY CHANGE/COMMENT *
* ------------------------------------------------- ---------------------------*
* 1.00 2012-04-30 Kimucha Kobo Create *
* 1.11 2014-06-21 Kimushige changed the comment of SPI_MODE *
* 2.00 2014-09-03 Kimushige Kobo 18F25K22 operation check, interrupt processing stopped *
* 3.00 2015-04-05 Added Kimushige speed/transfer mode functions *
* 3.10 2015-04-20 Supports both Kimushige SSP1/SSP2 and 16F193x *
**************************************************** **************************** */
# ifndef _SKSPILIB_H_
# define  _SKSPILIB_H_


// constant definition
#ifndef LOW _
# define  LOW  0
# endif
#ifndef HIGH _
#define HIGH 1 _  
# endif

# define  SPI_MODE0  0  // Clock polarity (0: LOW) Clock phase (0: idle 0V, transfer with change from 0V to 5V)
# define  SPI_MODE1  1  // Clock polarity (0: LOW) Clock phase (1: at idle 0V, transfer with change from 5V to 0V)
# define  SPI_MODE2  2  // Clock polarity (1: HIGH) Clock phase (0: idle 5V, transfer with change from 5V to 0V)
# define  SPI_MODE3  3  // Clock polarity (1: HIGH) Clock phase (1: idle 5V, transfer with change from 0V to 5V)
# define  SPI_CLOCK_DIV4  0x0  // communication speed Fosc/4
# define  SPI_CLOCK_DIV16  0x1  // communication speed Fosc/16
# define  SPI_CLOCK_DIV64  0x2  // communication speed Fosc/64
# define  SPI_CLOCK_DIVT2  0x3  // baud rate 1/2 of TMR2 output
# define  SPI_CLOCK_DIVADD  0xA  // Baud rate FOSC/((SSPxADD + 1)*4)

// #define SPI_MSSP2_USE // Uncomment when using MSSP2.
// MSSP module register definition
/*
#if defined(_16F1933)||defined(_16F1934)||defined(_16F1936)||\
defined(_16F1937)||defined(_16F1938)||defined(_16F1939)
// Definition when using MSSP for 16F193x
#define SPI_SSPCON1 SSPCON1
#define SPI_SSPSTAT SSPSTAT
#define SPI_SSPADD SSPADD
#define SPI_SSPBUF SSPBUF
#define SPI_SSPIF SSPIF
#else
#ifdef SPI_MSSP2_USE
// Definition when using MSSP2
#define SPI_SSPCON1 SSP2CON1
#define SPI_SSPSTAT SSP2STAT
#define SPI_SSPADD SSP2ADD
#define SPI_SSPBUF SSP2BUF
#define SPI_SSPIF SSP2IF
#else
// Definition when using MSSP1
#define SPI_SSPCON1 SSP1CON1
#define SPI_SSPSTAT SSP1STAT
#define SPI_SSPADD SSP1ADD
#define SPI_SSPBUF SSP1BUF
#define SPI_SSPIF SSP1IF
#endif
#endif
*/

// function prototype declaration
void  SPI_Init ( void ) ;
// void SPI_setDataMode(char mode) ;
// void SPI_setClockDivider(char divider,char rate) ;
char  SPI_transfer ( char dt) ;


# endif
