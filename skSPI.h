/*******************************************************************************
*  skSPIlib.h - ＳＰＩ通信を行う関数のインクルードファイル                     *
*               (PIC 12F1822 16F182x 16F193x 18F25K22)                         *
*                                                                              *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2012-04-30  きむ茶工房            Create                            *
*  1.11    2014-06-21  きむ茶工房(きむしげ)  SPI_MODEのコメントを変更          *
*  2.00    2014-09-03  きむ茶工房(きむしげ)  18F25K22の動作確認,割込み処理止めた*
*  3.00    2015-04-05  きむ茶工房(きむしげ)  速度/転送mode関数を追加           *
*  3.10    2015-04-20  きむ茶工房(きむしげ)  SSP1/SSP2両方と16F193xに対応      *
*******************************************************************************/
#ifndef _SKSPILIB_H_
#define _SKSPILIB_H_


// 定数の定義
#ifndef LOW
#define LOW  0
#endif
#ifndef HIGH
#define HIGH 1
#endif

#define SPI_MODE0 0           // クロック極性(0:LOW)  クロック位相(0:アイドル0Vで、0V->5Vに変化で転送)
#define SPI_MODE1 1           // クロック極性(0:LOW)  クロック位相(1:アイドル0Vで、5V->0Vに変化で転送)
#define SPI_MODE2 2           // クロック極性(1:HIGH) クロック位相(0:アイドル5Vで、5V->0Vに変化で転送)
#define SPI_MODE3 3           // クロック極性(1:HIGH) クロック位相(1:アイドル5Vで、0V->5Vに変化で転送)
#define SPI_CLOCK_DIV4   0x0  // 通信速度  Fosc/4
#define SPI_CLOCK_DIV16  0x1  // 通信速度  Fosc/16
#define SPI_CLOCK_DIV64  0x2  // 通信速度  Fosc/64
#define SPI_CLOCK_DIVT2  0x3  // 通信速度  TMR2の出力の1/2
#define SPI_CLOCK_DIVADD 0xA  // 通信速度  FOSC/((SSPxADD + 1)*4)

//#define SPI_MSSP2_USE         // MSSP2を利用する場合はコメントを外します。
// ＭＳＳＰモジュールのレジスター定義
/*
#if defined(_16F1933)||defined(_16F1934)||defined(_16F1936)||\
    defined(_16F1937)||defined(_16F1938)||defined(_16F1939)
// 16F193x用のMSSPを利用する場合の定義
#define SPI_SSPCON1         SSPCON1
#define SPI_SSPSTAT         SSPSTAT
#define SPI_SSPADD          SSPADD
#define SPI_SSPBUF          SSPBUF
#define SPI_SSPIF           SSPIF
#else
#ifdef  SPI_MSSP2_USE
// MSSP2を利用する場合の定義
#define SPI_SSPCON1         SSP2CON1
#define SPI_SSPSTAT         SSP2STAT
#define SPI_SSPADD          SSP2ADD
#define SPI_SSPBUF          SSP2BUF
#define SPI_SSPIF           SSP2IF
#else
// MSSP1を利用する場合の定義
#define SPI_SSPCON1         SSP1CON1
#define SPI_SSPSTAT         SSP1STAT
#define SPI_SSPADD          SSP1ADD
#define SPI_SSPBUF          SSP1BUF
#define SPI_SSPIF           SSP1IF
#endif
#endif
*/

// 関数のプロトタイプ宣言
void SPI_Init(void) ;
//void SPI_setDataMode(char mode) ;
//void SPI_setClockDivider(char divider,char rate) ;
char SPI_transfer(char dt) ;


#endif
