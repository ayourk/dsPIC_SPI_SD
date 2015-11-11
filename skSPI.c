/*******************************************************************************
*  skSPIlib.c - ＳＰＩ通信を行う関数ライブラリ                                 *
*                                                                              *
*    SPI_Init            - ＳＰＩモード(マスター)の設定と初期化を行う処理      *
*    SPI_setDataMode     - ＳＰＩの転送モード設定を行う処理                    *
*    SPI_setClockDivider - ＳＰＩの通信速度設定を行う処理                      *
*    SPI_transfer        - ＳＰＩ通信でのデータ送信とデータ受信を行う処理      *
*                                                                              *
*    メモ：SDIピンは必ず「デジタル入力ピン」に設定を行って下さい。             *
*          使用するSS(CS)ピンは初期化時にHIGHにします。                        *
*          SSP2を利用する場合は、skI2Clib.hに"#define SPI_MSSP2_USE"を宣言する *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2012-04-30  きむ茶工房(きむしげ)  Create                            *
*  1.01    2012-12-29  きむ茶工房(きむしげ)  XC8 C Compiler 対応に変更         *
*  1.10    2013-02-18  きむ茶工房(きむしげ)  SPI_Init関数を変更                *
*  1.11    2014-06-21  きむ茶工房(きむしげ)  SPI_Init関数のコメントを変更      *
*  2.00    2014-09-03  きむ茶工房(きむしげ)  18F25K22の動作確認,割込み処理止めた*
*  2.10    2014-09-27  きむ茶工房(きむしげ)  SPI_Init関数を変更                *
*  3.00    2015-04-05  きむ茶工房(きむしげ)  速度/転送mode関数を追加           *
*  3.10    2015-04-20  きむ茶工房(きむしげ)  SSP1/SSP2両方と16F193xに対応      *
*  3.11    2015-05-23  きむ茶工房(きむしげ)  16F1825/1829に対応                *
* ============================================================================ *
*  PIC 12F1822 16F18xx 16F193x 18F25K22                                        *
*  MPLAB IDE(V8.84) MPLAB X(V2.15)                                             *
*  MPLAB(R) XC8 C Compiler Version 1.00/1.32                                   *
*******************************************************************************/
#include <xc.h>
#include "skSPI.h"

#define _30F6014A
/*******************************************************************************
*  SPI_Init(mode,divider,sdo)                                                  *
*    ＳＰＩモードの設定と初期化を行う処理                                      *
*    割り込みでの処理を止めて直接"SSP1IF"を見る様にしています                  *
*                                                                              *
*    mode :   SPIの転送モードを設定します(クロック極性とクロック位相の組合せ)  *
*             SPI_MODE0 = 極性(0:LOW)  位相(0:アイドル0Vで、0V->5Vに変化で転送)*
*             SPI_MODE1 = 極性(0:LOW)  位相(1:アイドル0Vで、5V->0Vに変化で転送)*
*             SPI_MODE2 = 極性(1:HIGH) 位相(0:アイドル5Vで、5V->0Vに変化で転送)*
*             SPI_MODE3 = 極性(1:HIGH) 位相(1:アイドル5Vで、0V->5Vに変化で転送)*
*    divider :SPIの通信速度を設定します                                        *
*             SPI_CLOCK_DIV4   = Fosc/4                                        *
*             SPI_CLOCK_DIV16  = Fosc/16                                       *
*             SPI_CLOCK_DIV64  = Fosc/64                                       *
*             SPI_CLOCK_DIVT2  = TMR2の出力の1/2                               *
*             SPI_CLOCK_DIVADD = FOSC/((SSPxADD + 1)*4)                        *
*    sdo :    使用するSDO送信のピン番号を指定する                              *
*******************************************************************************/
void SPI_Init(void)
{
     //char con , stat ;

#if defined(_12F1822)
     SDOSEL = 0 ;                  // 7番ピン(RA0)をSDO送信ピンとする
     if (sdo == 3) SDOSEL = 1 ;    // 3番ピン(RA4)をSDO送信ピンとする
#endif

#if defined(_16F1823) 
     SDOSEL = 0 ;                  // 8番ピン(RC2)をSDO送信ピンとする
     if (sdo == 3) SDOSEL = 1 ;    // 3番ピン(RA4)をSDO送信ピンとする
#endif

#if defined(_16F1825) 
     SDO1SEL = 0 ;                 // 8番ピン(RC2)をSDO1送信ピンとする
     if (sdo == 3) SDO1SEL = 1 ;   // 3番ピン(RA4)をSDO1送信ピンとする
#endif

#if defined(_16F1826) || defined(_16F1827)
     SDO1SEL = 0 ;                 //  8番ピン(RB2)をSDO1送信ピンとする
     if (sdo == 15) SDO1SEL = 1 ;  // 15番ピン(RA6)をSDO1送信ピンとする
#endif

#if defined(_16F1829) 
     SDO2SEL = 0 ;                 // 15番ピン(RC1)をSDO2送信ピンとする
     if (sdo == 2) SDO2SEL = 1 ;   //  2番ピン(RA5)をSDO2送信ピンとする
#endif
/*
     con  = 0b00100000 ;       // クロック極性はLOW　マスタモードでFOSC/4のクロックに設定
     stat = 0b00000000 ;       // クロック位相は立下りでデータを送信
     con  = con | divider ;    // 指定のクロック速度を設定する
     if (mode == SPI_MODE1 || mode == SPI_MODE3) {
          stat = stat | 0x40 ; // 指定のクロック位相を設定する
     }
     if (mode == SPI_MODE2 || mode == SPI_MODE3) {
          con = con | 0x10 ;   // 指定のクロック極性を設定する
     }
     SPI_SSPCON1 = con ;
     SPI_SSPSTAT = stat ;
     SPI_SSPIF   = 0 ;         // ＳＰＩの割込みフラグを初期化する
     PEIE  = 1 ;               // 周辺装置割込み有効
     GIE   = 1 ;               // 全割込み処理を許可する
*/

#if defined(_30F6014A)
    SPI1CONbits.MSTEN = 1;
    SPI1STATbits.SPIROV = 0;
    SPI1CONbits.CKE = 0;
    SPI1CONbits.CKP = 1;
    SPI1CONbits.DISSDO = 0;
    SPI1CONbits.FRMEN = 0;
    SPI1CONbits.MODE16 = 0;
    SPI1CONbits.SMP = 0;
    SPI1CONbits.SSEN = 0;
    SPI1CONbits.PPRE = 0;
    SPI1CONbits.SPRE = 7;
    SPI1STATbits.SPIEN = 1;
#endif
    
}
/*******************************************************************************
*  SPI_setDataMode(mode)                                                       *
*    ＳＰＩの転送モード設定を行う処理                                          *
*    mode :   SPIの転送モードを設定します(クロック極性とクロック位相の組合せ)  *
*******************************************************************************/

/*
void SPI_setDataMode(char mode)
{
     if (mode == SPI_MODE1 || mode == SPI_MODE3) {
          SPI_SSPSTAT = SPI_SSPSTAT | 0x40 ; // 指定のクロック位相を１設定する
     } else {
          SPI_SSPSTAT = SPI_SSPSTAT & 0xBF ; // 指定のクロック位相を０設定する
     }
     if (mode == SPI_MODE2 || mode == SPI_MODE3) {
          SPI_SSPCON1 = SPI_SSPCON1 | 0x10 ; // 指定のクロック極性を１設定する
     } else {
          SPI_SSPCON1 = SPI_SSPCON1 & 0xEF ; // 指定のクロック極性を０設定する
     }
}
*/

/*******************************************************************************
*  SPI_setClockDivider(divider)                                                *
*    ＳＰＩの通信速度設定を行う処理                                            *
*    divider :SPIの通信速度を設定します                                        *
*             SPI_CLOCK_DIVADD = FOSC/((SSPxADD + 1)*4)                        *
*    rate    :SSPxADDに設定する値                                              *
*******************************************************************************/
/*
void SPI_setClockDivider(char divider,char rate)
{
     if (divider == SPI_CLOCK_DIVADD) {
          SPI_SSPADD = rate ;
     }
     SPI_SSPCON1 = (SPI_SSPCON1 & 0xF0)  | divider ;    // 指定のクロック速度を設定する
}
*/
/*******************************************************************************
*  ans = SPI_transfer(dt)                                                      *
*    ＳＰＩ通信でのデータ送信とデータ受信を行う処理                            *
*                                                                              *
*    dt  : ８ビットの送信するデータを指定します                                *
*    ans : ８ビットの受信したデータを返します                                  *
*******************************************************************************/
char SPI_transfer(char dt)
{
     SPI1BUF = dt ;            // データの送信
     while(SPI1STATbits.SPITBF == 0) ;      // 受信待ち
     IFS0bits.SPI1IF = 0 ;              // フラグクリア
     return SPI1BUF ;          // データの受信
}
