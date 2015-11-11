/*******************************************************************************
*  SD2.c − ＭＭＣ／ＳＤＣカード(SPI)の書き込みテスト                          *
*                                                                              *
*  SS:13番ピン(RC2) SCK：14番ピン(RC3)　SDI：15番ピン(RC4)　SDO：16番ピン(RC5) *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2012-01-30  きむ茶工房(きむしげ)  Create                            *
*  2.00    2014-02-02  きむ茶工房(きむしげ)  "skMonitorLCD.c"の変更により対応  *
*  2.10    2014-09-27  きむ茶工房(きむしげ)  "skSDlib.c"変更で割込み処理止めた *
*  3.00    2015-10-26  きむ茶工房(きむしげ)  MPLAB X・XC8 V1.32用に書換え      *
* ============================================================================ *
*  PIC 16F1938                                                                 *
*  MPLAB X(v2.15)                                                              *
*  MPLAB(R) XC8  C Compiler Version 1.32                                       *
*******************************************************************************/
#include <xc.h>
#include <string.h>
#include <libpic30.h>
//#include "skMonitorLCD.h"
//#include "skSPI.h"
//#include "skSD.h"

#define _XTAL_FREQ  8000000	// delay用(クロック8MHzで動作時)

/*
// コンフィギュレーション１の設定
#pragma config FOSC = INTOSC  // 内部ｸﾛｯｸを使用する(INTIO)
#pragma config WDTE = OFF     // ｳｵｯﾁﾄﾞｯｸﾞﾀｲﾏｰ無し(OFF)
#pragma config PWRTE = ON     // 電源ONから64ms後にﾌﾟﾛｸﾞﾗﾑを開始する(ON)
#pragma config MCLRE = OFF    // 外部ﾘｾｯﾄ信号は使用せずにﾃﾞｼﾞﾀﾙ入力(RE3)ﾋﾟﾝとする(OFF)
#pragma config CP = OFF       // ﾌﾟﾛｸﾞﾗﾑﾒﾓﾘｰを保護しない(OFF)
#pragma config CPD = OFF      // ﾃﾞｰﾀﾒﾓﾘｰを保護しない(OFF)
#pragma config BOREN = ON     // 電源電圧降下常時監視機能ON(ON)
#pragma config CLKOUTEN = OFF // CLKOUTﾋﾟﾝをRA6ﾋﾟﾝで使用する(OFF)
#pragma config IESO = OFF     // 外部・内部ｸﾛｯｸの切替えでの起動はなし(OFF)
#pragma config FCMEN = OFF    // 外部ｸﾛｯｸ監視しない(OFF)
// コンフィギュレーション２の設定
#pragma config WRT = OFF      // Flashﾒﾓﾘｰを保護しない(OFF)
#pragma config VCAPEN = OFF   // 低電圧レギュレータ用のキャパシタは使用しない(OFF)
#pragma config PLLEN = OFF    // 動作クロックを32MHz(4xPLL)では動作させない(OFF)
#pragma config STVREN = ON    // スタックがオーバフローやアンダーフローしたらリセットをする(ON)
#pragma config BORV = HI      // 電源電圧降下常時監視電圧(2.5V)設定(HI)
#pragma config LVP = OFF      // 低電圧プログラミング機能使用しない(OFF)
*/

// FOSC
#pragma config FOSFPR = FRC             // Oscillator (Internal Fast RC (No change to Primary Osc Mode bits))
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)
// FWDT
#pragma config FWPSB = WDTPSB_16        // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_512       // WDT Prescaler A (1:512)
#pragma config WDT = WDT_OFF            // Watchdog Timer (Disabled)
// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config BODENV = BORV_27         // Brown Out Voltage (2.7V)
#pragma config BOREN = PBOR_ON          // PBOR Enable (Enabled)
#pragma config MCLRE = MCLR_DIS         // Master Clear Enable (Disabled)
// FBS
#pragma config BWRP = WR_PROTECT_BOOT_OFF// Boot Segment Program Memory Write Protect (Boot Segment Program Memory may be written)
#pragma config BSS = NO_BOOT_CODE       // Boot Segment Program Flash Memory Code Protection (No Boot Segment)
#pragma config EBS = NO_BOOT_EEPROM     // Boot Segment Data EEPROM Protection (No Boot EEPROM)
#pragma config RBS = NO_BOOT_RAM        // Boot Segment Data RAM Protection (No Boot RAM)
// FSS
#pragma config SWRP = WR_PROT_SEC_OFF   // Secure Segment Program Write Protect (Disabled)
#pragma config SSS = NO_SEC_CODE        // Secure Segment Program Flash Memory Code Protection (No Secure Segment)
#pragma config ESS = NO_SEC_EEPROM      // Secure Segment Data EEPROM Protection (No Segment Data EEPROM)
#pragma config RSS = NO_SEC_RAM         // Secure Segment Data RAM Protection (No Secure RAM)
// FGS
#pragma config GWRP = GWRP_OFF          // General Code Segment Write Protect (Disabled)
#pragma config GCP = GSS_OFF            // General Segment Code Protection (Disabled)
// FICD
#pragma config ICS = ICS_PGD            // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

/*******************************************************************************
*  メインの処理                                                                *
*******************************************************************************/
int main(){
    
	union {
	     char c[2] ;
	     int  i ;
	} ans ;
     char dt[3][22] = {
          "PIC16F1938\r\n",
          "MicroSD\r\n",
          "Arduino Duemilanove\r\n"
     } ;
	
     OSCCON     = 0b01110010 ; // 内部クロックは8MHzとする
//     ANSELA     = 0b00000000 ; // AN0-AN4は使用しない全てデジタルI/Oとする
//     ANSELB     = 0b00000000 ; // AN8-AN13は使用しない全てデジタルI/Oとする
//     ANSELC     = 0b00000000 ; // AN14-AN19アナログは使用しない、デジタルI/Oに割当
//     TRISA      = 0b00000000 ; // ピン(RA)は全て出力に割当てる(0:出力 1:入力)
//     TRISB      = 0b00000000 ; // ピン(RB)は全て出力に割当てる
//     TRISC      = 0b00010000 ; // ピン(RC)はRC4(ＳＤＩ)のみ入力に割当てる
//     PORTA      = 0b00000000 ; // RA出力ピンの初期化(全てLOWにする)
//     PORTB      = 0b00000000 ; // RB出力ピンの初期化(全てLOWにする)
//     PORTC      = 0b00000000 ; // RC出力ピンの初期化(全てLOWにする)
    TRISD = 0; // Set all port D pins as outputs
    TRISA  = 0x00 ;     // 1で入力 0で出力 RA0-RA7全て出力に設定(RA5は入力専用)
    TRISF  = 0b0000000011000000 ;     // 1:in 0:out SDI1(RF7:in) SDO1(RF8:out) SCK1(RF6:in) SS(RF2:out)
    PORTA  = 0x00 ;     // 出力ピンの初期化(全てLOWにする)
    PORTF  = 0x0000 ;     // 出力ピンの初期化(全てLOWにする)
    
//     MonitorInit() ;           // ＬＣＤモニターに送信出来る様に初期化する

     // SPIの初期化を行う(クロック極性:HIGH　クロック位相:立上り　通信速度:Fosc/64)
     SPI_Init() ;

//     MonitorCls(0) ;           // ＬＣＤモニター画面の消去
     __delay32(9000000); // 3s delay
     
     // MMC/SDCの初期化を行う
     ans.i = SD_Init() ;
     if (ans.i == 0) {
          // ファイルを新規に作成してオープンする
          ans.i = SD_Open("ABC.TXT",O_RDWR) ;
          if (ans.i == 0) {
//               MonitorPuts("The write start") ;
               // ファイルへ１行ずつ書き込む
               ans.i = SD_Write(dt[0],strlen(dt[0])) ;
               ans.i = SD_Write(dt[1],strlen(dt[1])) ;
               ans.i = SD_Write(dt[2],strlen(dt[2])) ;
               // ファイルのクローズ
               SD_Close() ;
//               MonitorPutc(0x11) ;
               if (ans.i != -1) /*MonitorPuts("Successful End!!")*/ ;
               else             /*MonitorPuts("Abnormal Write!!")*/ ;
          } else {
//               MonitorPutc(0x11) ;
//               MonitorPuts("Error Open!!") ;      // オープンエラー
          }
     } else {
//          MonitorPutc(0x11) ;
//          MonitorPuts("Error Init ") ;            // 初期化エラー
//          MonitorPuth(ans.c[1]) ;
//          MonitorPuth(ans.c[0]) ;
     }

     while(1) ;                                   // 処理中断 
     
     return 0;
}
