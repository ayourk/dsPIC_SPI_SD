/*******************************************************************************
*  skSDlib.c - ＭＭＣ／ＳＤカードアクセス関数ライブラリ                        *
*                                                                              *
*    SD_Init     - ＭＭＣ／ＳＤＣの初期化を行う処理                            *
*    SD_Open     - ファイルのオープンを行う処理                                *
*    SD_Close    - ファイルのクローズを行う処理                                *
*    SD_Write    - ファイルへ指定したバイト数だけ書き込む処理                  *
*    SD_Read     - ファイルから指定したバイト数だけ読み込む処理                *
*    SD_fGets    - ファイルから１行読み込む処理                                *
*    SD_Size     - ファイルのサイズを得る処理                                  *
*    SD_Position - ファイルの読書きポインタ値を得る処理                        *
*    SD_Seek     - ファイルの読書きポインタを移動させる処理                    *
*                                                                              *
*    メモ：SS ：13番ピン(RC2) SCK：14番ピン(RC3)  SDI：15番ピン(RC4)           *
*          SDO：16番ピン(RC5)を使用                                            *
*          標準SD・MicroSD(FAT16)/標準SDHC・MicroSDHC(FAT32)のみ読書きＯＫ     *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2012-04-30  きむ茶工房(きむしげ)  Create                            *
*  2.00    2012-05-03  きむ茶工房(きむしげ)  SDHCに対応                        *
*  2.01    2012-06-09  きむ茶工房(きむしげ)  標準SD/SDHCもOKコメントを変更     *
*  2.02    2014-02-02  きむ茶工房(きむしげ)  XC8 C Compiler 対応に変更         *
*  3.00    2014-09-01  ロッシさんからの報告  不具合修正                        *
*  3.01    2014-09-03  きむ茶工房(きむしげ)  ファイル名8文字不可不具合を修正   *
*  3.02    2014-09-03  きむ茶工房(きむしげ)  Seek()のフラグ4の処理を変更       *
* ============================================================================ *
*  PIC 16F1938 18F25K22                                                        *
*  (たぶんその他のPICでもSRAMが768byte以上有ればこの関数はそのまま利用可能と思う)*
*  MPLAB IDE(V8.63)                                                            *
*  MPLAB(R) XC8 C Compiler Version 1.00                                        *
*******************************************************************************/
#include <xc.h>
#include <string.h>
#include <libpic30.h>
//#include "skMonitorLCD.h"
//#include "skSPI.h"
//#include "skSD.h"


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


#ifndef _XTAL_FREQ
 // Unless already defined assume 8MHz system frequency
 // This definition is required to calibrate __delay_us() and __delay_ms()
 #define _XTAL_FREQ 8000000    // 使用するPIC等により動作周波数値を設定する
#endif

////////////////////////////////////////////////////////////////////////////////
// ＳＰＩ関連

#define CS   PORTBbits.RB2                        // カード選択信号

////////////////////////////////////////////////////////////////////////////////
// ＭＭＣ／ＳＤＣアクセス関連

#define CMD0   0x00                     // カードへのリセットコマンド
#define CMD1   0x01                     // MMCへの初期化コマンド
#define CMD8   0x08                     // 動作電圧の確認とSDCのバージョンチェック
#define CMD12  0x0C                     // データ読込みを停止させるコマンド
#define CMD13  0x0D                     // 書込みの状態を問い合わせるコマンド
#define CMD16  0x10                     // ブロックサイズの初期値設定コマンド
#define CMD17  0x11                     // シングルブロック読込み要求コマンド
#define CMD24  0x18                     // シングルブロック書込み要求コマンド
#define ACMD41 0x29                     // SDCへの初期化コマンド
#define CMD55  0x37                     // ACMD41/ACMD23とセットで使用するコマンド
#define CMD58  0x3A                     // OCRの読出しコマンド

#define SECTER_BYTES  512               // １セクタのバイト数

extern char MMCbuffer[SECTER_BYTES] ;          // ＭＭＣカード読書きバッファ
extern int  CardType ;                         // カードの種類
extern int  FatType ;                          // ＦＡＴの種類


////////////////////////////////////////////////////////////////////////////////
// ＦＡＴ関連

#define O_READ    0x01                  // ファイルの読込みオープン
#define O_RDWR    0x02                  // ファイルの読込みと書込みオープン
#define O_APPEND  0x04                  // ファイルの追加書込みオープン

//ＦＡＴファイルシステムのパラメータ
extern unsigned long Dir_Entry_StartP ;        // ディレクトリエントリの開始セクタ位置
extern unsigned int  DirEntry_SectorSU ;       // ディレクトリエントリのセクタ個数
extern unsigned long Data_Area_StartP ;        // データ領域の開始セクタ位置
extern unsigned long Fat_Area_StartP ;         // FAT領域の開始セクタ位置
extern unsigned int  Cluster1_SectorSU ;       // １クラスタあたりのセクタ数
extern unsigned long SectorsPerFatSU ;         // １組のFAT領域が占めるセクタ数

// ファイルのアクセス情報
struct FDS {
     unsigned int  Oflag ;              // アクセスするオープンフラグを保存
     unsigned int  DirEntryIndex ;      // ディレクトリエントリの検索した場所の位置
     unsigned long FileSize ;           // ファイルのサイズ
     unsigned long FileSeekP ;          // ファイルの次読み出す位置
     unsigned long AppendSize ;         // 追加書込みしたファイルのサイズ
     unsigned long FirstFatno ;         // データ格納先のFAT番号
} ;
extern struct FDS fds ;

// ＦＡＴファイルシステム(FAT12/FAT16)のパラメータ構造体(512バイト)
struct FAT_PARA {
     unsigned char jump[3] ;            // ブート用のジャンプコード
     unsigned char oemId[8] ;           // フォーマット時のボリューム名
     unsigned int  BytesPerSector ;     // １セクタあたりのバイト数、通常は512バイト
     unsigned char SectorsPerCluster ;  // １クラスタあたりのセクタ数
     unsigned int  ReservedSectorCount ;// ブートセクタ以降の予約領域のセクタ数
     unsigned char FatCount ;           // FATの組数(バックアップFAT数)、通常は２組
     unsigned int  RootDirEntryCount ;  // ディレクトリの作成可能個数、通常は512個
     unsigned int  TotalSectors16 ;     // 全領域のセクター総数(FAT12/FAT16用)
     unsigned char MediaType ;          // FAT領域の先頭の値、通常は0xF8
     unsigned int  SectorsPerFat16 ;    // １組のFAT領域が占めるセクタ数(FAT12/FAT16用)
     unsigned int  SectorsPerTrack ;    // １トラックあたりのセクタ数
     unsigned int  HeadCount ;          // ヘッド数
     unsigned long HidddenSectors ;     // 隠蔽されたセクタ数
     unsigned long TotalSectors32 ;     // 全領域のセクター総数(FAT32用)
     unsigned long SectorsPerFat32 ;    // １組のFAT領域が占めるセクタ数(FAT32用)
     unsigned int  FlagsFat32 ;         // FATの有効無効等の情報フラグ
     unsigned int  VersionFat32 ;
     unsigned long RootDirClusterFat32 ;// ディレクトリのスタートクラスタ(FAT32用)
     unsigned char Dumy[6] ;
     unsigned char FileSystemType[8] ;  // FATの種類("FAT12/16")(FAT32は20バイト下に有る)
     unsigned char BootCode[448] ;      // ブートコード領域
     unsigned char BootSectorSig0 ;     // 0x55
     unsigned char BootSectorSig1 ;     // 0xaa
} ;

// ディレクトリエントリーの構造体(32バイト)
struct DIR_ENTRY {
     unsigned char FileName[11] ;       // ファイル名(8)+拡張子(3)
     unsigned char Attributes ;         // ファイルの属性
     unsigned char ReservedNT ;         // Windows NT 用 予約領域
     unsigned char CreationTimeTenths ; // ファイル作成時間の1/10秒単位をあらわす
     unsigned int  CreationTime ;       // ファイルの作成時間(hhhhhmmmmmmsssss)
     unsigned int  CreationDate ;       // ファイルの作成日(yyyyyyymmmmddddd)
     unsigned int  LastAccessDate ;     // 最終のアクセス日
     unsigned int  FirstClusterHigh ;   // データ格納先のFAT番号上位２バイト
     unsigned int  LastWriteTime ;      // 最終のファイル書込み時間
     unsigned int  LastWriteDate ;      // 最終のファイル書込み日
     unsigned int  FirstClusterLow ;    // データ格納先のFAT番号下位２バイト
     unsigned long FileSize ;           // ファイルのサイズ
} ;

////////////////////////////////////////////////////////////////////////////////
//
int SD_Init() ;
int SD_Open(const char *filename,int oflag) ;
void SD_Close() ;
int SD_Write(char *buf,int nbyte) ;
int SD_Read(char *buf,int nbyte) ;
int SD_fGets(char *buf,int nbyte) ;
unsigned long SD_Size() ;
unsigned long SD_Position() ;
unsigned long SD_Seek(unsigned long offset,int sflag) ;



////////////////////////////////////////////////////////////////////////////////
// ＭＭＣ／ＳＤＣのカードを選択、CS信号のHIGH/LOWを出力する処理
void cs_select(int flag){
    
     CS = flag ;
     SPI_transfer(0xff) ;     // 確実にＣＳを切替える為にクロックを送信しておく
}
////////////////////////////////////////////////////////////////////////////////
// レディ状態のチェックを行う処理
int ready_check()
{
     int c ;
     
     c = 0 ;
     while (SPI_transfer(0xff) != 0xff && c<=500) {
          c++ ;
          __delay32(100) ;
     }
     if (c>=500) return 1 ;    // タイムアウト
     else        return 0 ;    // ＯＫです
}
////////////////////////////////////////////////////////////////////////////////
// ＭＭＣ／ＳＤＣへコマンドを送る処理
int send_command(unsigned char cmd, unsigned long arg)
{
     int x , ans ;

     cs_select(LOW) ;
     ans = ready_check() ;
     if (ans != 0) return 0xff00 ;           // カードは Busy らしい
     // コマンドの送信を行う
     SPI_transfer(cmd | 0x40) ;                             // コマンドの送信
     for (x=24 ; x >= 0 ; x -= 8) SPI_transfer(arg >> x) ;  // パラメータの送信
     x = 0XFF ;
     if (cmd == CMD0) x = 0X95 ;
     if (cmd == CMD8) x = 0X87 ;
     SPI_transfer(x) ;                                      // CRCの送信
     // コマンドの返答を待つ処理を行う
     x = 0 ;
     do {
          ans = SPI_transfer(0xff) ;
          x++;
     } while(ans & 0x80 && x < 256) ;
    if (x >= 256) ans = (cmd|0x40) << 8 ;    // レスポンスタイムアウト

    return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// 指定のセクタ位置からデータを１ブロック(シングル・ライト)書込む処理
int sblock_write(unsigned long sector,char *buff)
{
     int i , ans ;

     if (CardType != 0x13) sector <<= 9 ;         // SDHCでない
     ans = send_command(CMD24, sector) ;          // シングル・リードコマンドの発行
     if (ans == 0) {
          SPI_transfer(0xfe) ;                    // データトークンの送信
          for (i=0 ; i<SECTER_BYTES ; i++) {
               SPI_transfer(*buff) ;              // データ部の送信
               buff++ ;
          }
          SPI_transfer(0xff) ;                    // CRCの部分を送信
          SPI_transfer(0xff) ;
          ans = SPI_transfer(0xff) ;              // レスポンスの受信
          ans = ans & 0x1F ;
		if (ans == 0x05) {
               ans = ready_check() ;
               if (ans != 0) ans = 0x8800 ;       // カードは Busy らしい
               else {
                    // 書込みが正常に行われたか問い合わせを行う
                    ans = send_command(CMD13, sector) ;
                    i  = SPI_transfer(0xff) ;
                    if (ans == 0) {
                         if (i != 0) ans = 0x8900|ans ; // 書込み失敗している
                    }
               }
          } else ans = 0x8800 | ans ;             // 書き込みエラー
     } else ans = CMD24 << 8 | ans ;              // CMD24エラー
     cs_select(HIGH) ;

     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// 指定のセクタ位置からデータを１ブロック(シングル・リード)読込む処理
int sblock_read(unsigned long sector,char *buff)
{
     int i , ans ;

     if (CardType != 0x13) sector <<= 9 ;         // SDHCでない
     ans = send_command(CMD17, sector) ;          // シングル・リードコマンドの発行
     if (ans == 0) {
          i = 0 ;
          // 返信データの１バイト目を待つ
          while(1) {
               ans = SPI_transfer(0xff) ;
               i++ ;
               if (ans != 0xff || i>=1000) {
                    if (i>=1000) ans = 0x8600 ;   // データトークンタイムアウト
                    break ;
               }
               __delay32(100) ;
          }
          // ２バイト目以降の受信
          if (ans == 0xfe) {
               for (i=0 ; i<SECTER_BYTES ; i++) {
                    *buff = SPI_transfer(0xff) ;
                     buff++ ;
               }
               SPI_transfer(0xff) ;               // CRCの部分を受信
               SPI_transfer(0xff) ;
               ans = 0 ;
          } else ans = 0x8600 | ans ;             // 読み込みエラー
     } else ans = CMD17 << 8 | ans ;              // CMD17エラー
     cs_select(HIGH) ;

     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// ＦＡＴファイルシステムのパラメータを読込む処理
int fat_para_read()
{
     union {
          unsigned char c[4] ;
          unsigned long l ;
     } dt ;
     struct FAT_PARA *fat ;
     int i , ans ;

     // カードの先頭から５１２バイト読み出す
     ans = sblock_read(0,MMCbuffer) ;
     if (ans == 0) {
          // BPB(ブートセクタ)までのオフセット値を取出す
          for (i=0 ; i<4 ; i++) {
               dt.c[i] = MMCbuffer[i+0x1c6] ;
          }
          // ＦＡＴファイルシステムのパラメータを読込む
          ans = sblock_read(dt.l,MMCbuffer) ;
          if (ans == 0) {
               fat = (struct FAT_PARA *)MMCbuffer ;
               // １クラスタあたりのセクタ数
               Cluster1_SectorSU = fat->SectorsPerCluster ;
               // １組のFAT領域が占めるセクタ数
               if (fat->SectorsPerFat16 != 0) {
                    FatType = 2 ;
                    SectorsPerFatSU = fat->SectorsPerFat16 ;
               } else {
                    FatType = 4 ;
                    SectorsPerFatSU = fat->SectorsPerFat32 ;
               }
               // ＦＡＴ領域の開始セクタ位置
               Fat_Area_StartP = fat->ReservedSectorCount + dt.l ;
               // ルートディレクトリエントリの開始セクタ位置
               Dir_Entry_StartP = Fat_Area_StartP + (SectorsPerFatSU * fat->FatCount) ;
               // データ領域の開始セクタ位置
               DirEntry_SectorSU = fat->RootDirEntryCount / (SECTER_BYTES / sizeof(struct DIR_ENTRY)) ;
               Data_Area_StartP  = Dir_Entry_StartP + DirEntry_SectorSU ;
               if (FatType == 4) DirEntry_SectorSU = Cluster1_SectorSU ;   // FAT32
          }
     }
     return ans ;
}
/*******************************************************************************
*  ans = SD_Init()                                                             *
*    ＭＭＣ／ＳＤＣの初期化を行う処理                                          *
*    標準タイプSD/SDHCとMicroSD/SDHCのFAT16/FAT32のみ利用可能                  *
*                                                                              *
*    ans  : 正常＝０  異常＝０以外のエラーコード値                             *
*******************************************************************************/
int SD_Init()
{
     unsigned long arg ;
     unsigned char r7[4] ;
     unsigned int i , ans ;

     CS       = 1 ;
     CardType = 0 ;

     // CLKを74クロック以上送信し、カードをコマンド待ち状態にする
     for (i = 0 ; i < 10 ; i++) SPI_transfer(0xFF) ;

     // カードにリセットコマンド(CMD0)を送信する(SPIモードに移行)
     ans = send_command(CMD0, 0) ;
     if (ans == 1) {
//          MonitorPuts("CMD0") ;
          // ＳＤＣ用の初期化処理を行う
          arg = 0 ;
          // 動作電圧の確認とカードのバージョンチェック
          ans = send_command(CMD8, 0x1AA) ;
          if (ans == 1) {
               for (i=0 ; i<4 ; i++) r7[i] = SPI_transfer(0xff) ;
               if (r7[3] == 0xAA) {
                    CardType = 0x12 ;                  // SDCver.2のカード
                    arg = 0X40000000 ;
               } else ans = 0x8200 ;                   // CMD8のエラー
          } else {
               if (ans & 0x4) CardType = 0x11 ;        // SDCver.1のカード
          }
          if (CardType != 0) {
//               MonitorPuts(":8") ;
               // ＳＤＣ用の初期化コマンド(ACMD41)を発行する
               i = 0 ;
               while(1) {
                    ans = send_command(CMD55, 0) ;
                    ans = send_command(ACMD41, arg) ;
                    i++ ;
                    if (ans == 0 || i>=2000) {
                         if (i>=2000) ans = 0x8300|ans ;    // ACMD41タイムアウト
                         break ;
                    }
                    __delay32(100) ;
               }
//               if (ans == 0) MonitorPuts(":A41") ;
               if (ans == 0 && CardType == 0x12) {
                    // Ver.2ならOCRの読出しコマンド(CMD58)を発行する
                    ans = send_command(CMD58, 0) ;
                    if (ans == 0) {
//                         MonitorPuts(":58") ;
                         for (i=0 ; i<4 ; i++) r7[i] = SPI_transfer(0xff) ;
                         if (r7[0] & 0x40) CardType = 0x13 ;// SDHCのカード
                    } else ans = CMD58 << 8 | ans ;         // CMD58エラー
               }
          } else {
               // ＭＭＣ用の初期化コマンド(CMD1)を発行する
               i = 0 ;
               while(1) {
                    ans = send_command(CMD1, 0) ;
                    i++ ;
                    if (ans != 1 || i>=2000) {
                         if (i>=2000) ans = 0x8100 ;   // 初期化タイムアウト
                         break ;
                    }
                    __delay32(100) ;
               }
               if (ans == 0) {
                    CardType = 0x01 ;                  // MMCのカード
//                    MonitorPuts(":1") ;
               } else {
                    if (ans != 0x8100) ans = CMD1 << 8 | ans ;   // CMD1エラー
               }
          }
          if (ans == 0) {
               // ブロックサイズの初期値を５１２バイトに再設定する
               ans = send_command(CMD16, SECTER_BYTES) ;
               if (ans != 0) ans = CMD16 << 8 | ans ;  // CMD16のエラー
//               else MonitorPuts(":16") ;
          }
     } else ans = CMD0 << 8 | ans ;                    // CMD0エラー
     cs_select(HIGH) ;

     // 初期化が正常に終了したらＦＡＴファイルシステムのパラメータを読込む
     if (ans == 0) {
          ans = fat_para_read() ;
     }

//          MonitorPuts("CardType = ") ;
//          MonitorPuth(CardType,HEX) ;

     // ファイルのアクセス情報を初期化する
     fds.Oflag = 0 ;

     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// 指定ファイルのエントリ情報を得る、なければエントリの空の場所を得る処理
int search_file(char *filename,struct FDS *fds)
{
     struct DIR_ENTRY *inf ;
     int i , j , c , x , ans ;

     ans = c = 0 ;
     fds->DirEntryIndex = 0 ;
     // ディレクトリエントリの全セクタ分だけ繰り返す
     for (i=0 ; i<DirEntry_SectorSU ; i++) {
          // ディレクトリエントリのデータを読込む
          ans = sblock_read(Dir_Entry_StartP+i,MMCbuffer) ;
          if (ans == 0) {
               // １セクタ内のエントリの個数分だけ繰り返す
               x = SECTER_BYTES / sizeof(struct DIR_ENTRY) ;
               for (j=0 ; j<x ; j++ ) {
                    c++ ;
                    // ファイルのエントリを調べる
                    inf = (struct DIR_ENTRY *)&MMCbuffer[j*sizeof(struct DIR_ENTRY)] ;
                    if (inf->FileName[0] == 0x2e) continue ;// サブディレクトリ
                    if (inf->FileName[0] == 0xe5) {
                         if (fds->DirEntryIndex == 0) fds->DirEntryIndex = c ;
                         continue ;                         // 削除されたエントリ
                    }
                    if (inf->FileName[0] == 0x00) {
                         if (fds->DirEntryIndex == 0) fds->DirEntryIndex = c ;
                         i = DirEntry_SectorSU ;
                         ans = 2 ;                          // 検索されなかった
                         break ;                            // 空のエントリ
                    }
                    // ファイルの属性を調べる
                    if (inf->Attributes != 0x20) continue ; // 普通のファイルでない
                    // ファイル名を比較する
                    if (memcmp(inf->FileName,filename,11) == 0) {
                         fds->DirEntryIndex = c ;
                         // ファイルのアクセス情報を設定する
                         fds->FileSeekP = 0 ;               // ファイルの読込む位置は０
                         fds->FileSize = inf->FileSize ;    // ファイルのサイズ
                         // データ格納先のFAT番号を記録
                         fds->FirstFatno = inf->FirstClusterHigh ;
                         fds->FirstFatno = (fds->FirstFatno<<16) | inf->FirstClusterLow ;
                         i = DirEntry_SectorSU ;
                         ans = 1 ;
                         break ;                            // 正常終了
                    }
               }
          } else ans = -1 ;
     }

     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// 空きのＦＡＴ番号を探す処理
unsigned long search_fat()
{
     unsigned long ans ;
     unsigned int i ;
     int j , x , k ;

     ans = 0 ;
     // １組の総ＦＡＴ数分繰り返す
     for (i=0 ; i<SectorsPerFatSU ; i++) {
          if (sblock_read(Fat_Area_StartP+i,MMCbuffer) == 0) {
               for (j=0 ; j<SECTER_BYTES ; j=j+FatType) {
                    // 空きのＦＡＴが有ったぞぉ
                    x = 0 ;
                    for (k=0 ; k<FatType ; k++) x = x | MMCbuffer[j+k] ;
                    if (x == 0) {
                         MMCbuffer[j]   = 0xff ;
                         MMCbuffer[j+1] = 0xff ;
                         if (FatType == 4) {    // FAT32
                              MMCbuffer[j+2] = 0xff ;
                              MMCbuffer[j+3] = 0x0f ;
                         }
                         // 予約更新
                         if (sblock_write(Fat_Area_StartP+i,MMCbuffer) == 0) {
                              // ２組目のFATも書込む
                              sblock_write((Fat_Area_StartP+i)+SectorsPerFatSU,MMCbuffer) ;
                              ans = (i * SECTER_BYTES + j) / FatType ;
                         }
                         i = SectorsPerFatSU ;
                         break ;  // 検索終了
                    }
               }
          } else break ;
     }
     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// ディレクトリエントリの更新処理
int direntry_make(unsigned long no,char *filename,struct FDS *fds)
{
     struct DIR_ENTRY *inf ;
     unsigned long p ;
     unsigned int x , y ;
     int ans ;

     ans = -1 ;
     // ディレクトリエントリを読込む
     x = fds->DirEntryIndex - 1 ;
     y = SECTER_BYTES / sizeof(struct DIR_ENTRY) ;
     p = Dir_Entry_StartP + (x / y) ;
     if (sblock_read(p,MMCbuffer) == 0) {
          inf = (struct DIR_ENTRY *)&MMCbuffer[(x % y) * sizeof(struct DIR_ENTRY)] ;
          if (no != 0) {
               memset(inf,0x00,sizeof(struct DIR_ENTRY)) ;
               // ファイル名を設定する
               memcpy(inf->FileName,filename,11) ;
               // ファイルの属性を設定する
               inf->Attributes = 0x20 ;
               // ファイルの新規作成
               inf->FileSize = 0 ;
               // ファイルの作成時間を設定する
               
               // ファイルの作成日を設定する
               
               // アクセス日を設定する
               
               // データ格納先のFAT番号を設定する
               inf->FirstClusterHigh = (unsigned int)(no >> 16) ;
               inf->FirstClusterLow  = (unsigned int)(no & 0x0000ffff) ;
               // ファイルのアクセス情報を設定する
               fds->FileSeekP = 0 ;
               fds->FileSize  = 0 ;
               // データ格納先のFAT番号を記録
               fds->FirstFatno = inf->FirstClusterHigh ;
               fds->FirstFatno = (fds->FirstFatno<<16) | inf->FirstClusterLow ;
          } else {
               // ファイルのサイズを更新する
               inf->FileSize = inf->FileSize + fds->AppendSize ;
               // ファイル書込み日時を設定する
               
               // アクセス日を設定する
               
          }
          // ディレクトリエントリの更新
          ans = sblock_write(p,MMCbuffer) ;
     }
     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// ファイル名の成形を行う処理
void filename_check(char *c,const char *filename)
{
     int i ;

     memset(c,0x20,11) ;
     for (i=0 ; i<8 ; i++) {
          if (*filename == '.') {
               c = c + (8-i) ;
               filename++ ;
               break ;
          }
          *c = *filename ;
          c++ ;
          filename++ ;
     }
     if (i > 7) filename++ ;
     for (i=0 ; i<3 ; i++) {
          if (*filename == 0x00) break ;
          *c = *filename ;
          c++ ;
          filename++ ;
     }
}
/*******************************************************************************
*  ans = SD_Open(filename,oflag)                                               *
*    ファイルのオープンを行う処理                                              *
*    filename : ファイル名(xxxxxxxx.xxx)を指定します                           *
*               拡張子は必ず指定、また英文字は大文字を指定 。                  *
*    oflag    : アクセスのフラグを指定します                                   *
*               O_READ   読み込み専用(ファイルが存在しない時はエラー)          *
*               O_APPEND 追加書込み専用(ファイルの最後に追加します)            *
*               O_RDWR   読み書き可能(ファイルが無い時は作成されます)          *
*                                                                              *
*    ans  : 正常＝０  異常＝−１                                               *
*******************************************************************************/
int SD_Open(const char *filename,int oflag)
{
     unsigned long no ;
     char c[11] ;
     int  ans , ret ;

     ret = -1 ;
     if (fds.Oflag != 0) return ret ;        // 既にオープンされている
     fds.Oflag = 0 ;
     // 指定のファイル名を成形する
     filename_check(c,filename) ;
     // 指定されたファイルを検索する
     ans = search_file(c,(struct FDS *)&fds) ;
     if (ans > 0) {
          if (oflag == O_READ && ans == 1) ret = 0 ;
          if (oflag == O_APPEND && ans == 1) {
               // ファイルのアクセス位置をファイルの最後＋１に設定する
               fds.FileSeekP = fds.FileSize ;
               ret = 0 ;
          }
          if (oflag == O_RDWR && (ans == 1 || ans == 2)) {
               if (ans == 2) {
                    // 新規ファイルの作成を行う
                    if (fds.DirEntryIndex != 0) {
                         // 空きのFATを探して確保する
                         no = search_fat() ;
                         if (no != 0) {
                              // ディレクトリエントリの作成を行う
                              if (direntry_make(no,c,(struct FDS *)&fds) == 0) ret = 0 ;
                         }
                    }
               } else ret = 0 ;
          }
          if (ret == 0) {
               // ファイルにアクセス可能とする
               fds.Oflag = oflag ;
               fds.AppendSize = 0 ;
          }
     }

     return ret ;
}
/*******************************************************************************
*  SD_Close()                                                                  *
*    ファイルのクローズを行う処理                                              *
*    データを追加した時のファイルサイズがこの関数で書き込まれます。            *
*******************************************************************************/
void SD_Close()
{
     // 書き込みオープン時の処理
     if (fds.Oflag & (O_APPEND | O_RDWR)) {
          if (fds.AppendSize == 0) return ;
          // ディレクトリエントリの更新を行う
          direntry_make(0,0,(struct FDS *)&fds) ;
     }
     // ファイルのアクセス情報を初期化する
     fds.Oflag = 0 ;
}
////////////////////////////////////////////////////////////////////////////////
// 指定されたFAT番号の次のFAT番号を得る処理
unsigned long next_fat_read(unsigned long fatno,struct FDS *fds)
{
     union {
          unsigned char c[4] ;
          unsigned long i ;
     } no ;
     unsigned long p , x , y , ans ;
     int j ;

     // ＦＡＴ領域のデータを読込む
     p = Fat_Area_StartP + (fatno / (SECTER_BYTES/FatType)) ;
     ans = sblock_read(p,MMCbuffer) ;
     if (ans == 0) {
          x = (fatno % (SECTER_BYTES/FatType)) * FatType ;
          no.i = 0 ;
          for (j=0 ; j<FatType ; j++) no.c[j] = MMCbuffer[x+j] ;
          // 次のチェーン先FAT番号を得る
          ans = no.i ;
          if (FatType == 4) y = 0x0fffffff ;  // FAT32
          else              y = 0xffff ;
          // 次のチェーン先がない時は新規にチェーン先ＦＡＴを作成する
          if (y == ans) {
               ans = search_fat() ;     // 新ＦＡＴ番号の空きを探す
               if (ans != 0) {
                    // チェーン元のＦＡＴ情報を更新する
                    if (sblock_read(p,MMCbuffer) == 0) {
                         no.i = ans ;
                         for (j=0 ; j<FatType ; j++) MMCbuffer[x+j] = no.c[j] ;
                         if (sblock_write(p,MMCbuffer) == 0) {
                              // ２組目のFATも書込む
                              sblock_write(p + SectorsPerFatSU,MMCbuffer) ;
                         } else ans = 0 ;
                    } else ans = 0 ;
               }
          }
     } else ans = 0 ;

     return ans ;
}
////////////////////////////////////////////////////////////////////////////////
// シーク位置からFAT番号(クラスタ番号)を算出する処理
void fatno_seek_conv(unsigned long *fatno,struct FDS *fds)
{
     unsigned int  p ;
     int  i ;

     // データのシーク位置から読出論理セクタ位置を算出し何番目の論理クラスタ位置か？
     p = (fds->FileSeekP / SECTER_BYTES) / Cluster1_SectorSU ;
     // FAT領域より論理クラスタから実際のFAT番号(クラスタ番号)を算出する
     *fatno = fds->FirstFatno ;
     for (i=0 ; i<p ; i++) {
          // 次のチェーン先FAT番号を読込んでおく
          *fatno = next_fat_read(*fatno,(struct FDS *)fds) ;
     }
}
////////////////////////////////////////////////////////////////////////////////
// この関数が実際にファイルから指定したバイト数だけ読み込む処理
// type : 1=SD_Readからのコール  2=SD_fGetsからのコール  3=SD_Writeからのコール
int sd_rdwr(char *buf,int nbyte,int type)
{
     unsigned long dtSP ;
     unsigned long fatno ;
     unsigned int  p , x ;
     int  i , c , ans ;

     // シーク位置からFAT番号(クラスタ番号)を算出
     fatno_seek_conv(&fatno,(struct FDS *)&fds) ;
     if (fatno == 0) return -1 ;                       // FAT領域読込みエラー
     // データの先頭セクタ位置の算出
     dtSP = Data_Area_StartP + ((fatno - 2) * Cluster1_SectorSU) ;
     p = (fds.FileSeekP / SECTER_BYTES) % Cluster1_SectorSU ; // クラスタ内のセクタ位置
     // データ領域からファイル内容を読出す
     ans = sblock_read(dtSP+p,MMCbuffer) ;
     if (ans == 0) {
          x = fds.FileSeekP % SECTER_BYTES ;
          c = 0 ;
          // 指定バイト数ぶん繰り返す
          for (i=0 ; i<nbyte ; i++ ) {
               if (type == 3) MMCbuffer[x] = *buf ;    // 書込み
               else           *buf = MMCbuffer[x] ;    // 読込み
               c++ ;
               x++ ;
               fds.FileSeekP++ ;
               if (fds.FileSeekP >= fds.FileSize) {
                    if (type < 3) break ;              // 最後まで読み込んだ
                    fds.AppendSize++ ;                 // データが追加された分だけカウント
               }
               if (c >= SECTER_BYTES) break ;          // SECTER_BYTESだけ処理した
               if (type == 2 && *buf == 0x0a) break ;  // ＬＦなら終了
               // 次のセクタにデータがまたがった場合の処理
               if (x >= SECTER_BYTES) {
                    // 書込み要求ならここまでのデータを書込む
                    if (type == 3) {
                         ans = sblock_write(dtSP+p,MMCbuffer) ;
                         if (ans != 0) {
                              ans = -1 ;               // データ領域書込みエラー
                              break ;
                         }
                    }
                    p++ ;
                    if (p >= Cluster1_SectorSU) {
                         // 次のクラスタにデータが有るようである
                         // シーク位置から次のFAT番号(クラスタ番号)を算出
                         fatno_seek_conv(&fatno,(struct FDS *)&fds) ;
                         if (fatno == 0) {
                              ans = -1 ;               // FAT領域読込みエラー
                              break ;
                         }
                         // データの先頭セクタ位置の算出
                         dtSP = Data_Area_StartP + ((fatno - 2) * Cluster1_SectorSU) ;
                         p = (fds.FileSeekP / SECTER_BYTES) % Cluster1_SectorSU ; // クラスタ内のセクタ位置
                    }
                    // 次のブロックを読込む
                    ans = sblock_read(dtSP+p,MMCbuffer) ;
                    if (ans == 0) x = 0 ;
                    else {
                         ans = -1 ;                    // データ領域読込みエラー
                         break ;
                    }
               }
               buf++ ;
          }
          // 書込み要求ならデータを書込む
          if (x != 0 && ans != -1 && type == 3) {
               ans = sblock_write(dtSP+p,MMCbuffer) ;
               if (ans != 0) ans = -1 ;                // データ領域書込みエラー
          }
          if (ans != -1) ans = c ;                     // 読書込んだバイト数を返す
     } else ans = -1 ;                                 // データ領域読込みエラー

     return ans ;
}
/*******************************************************************************
*  ans = SD_Write(buf,nbyte)                                                   *
*    ファイルへ指定したバイト数だけ書き込む処理                                *
*    ファイルの先頭から書込まれます、書込む位置を変える場合はSD_Seekを使用     *
*    *buf  : 書込むデータを格納した配列変数を指定します                        *
*    nbyte : 書込むデータのバイト数を指定します                                *
*                                                                              *
*    ans  : 正常＝書込んだバイト数  異常＝−１                                 *
*******************************************************************************/
int SD_Write(char *buf,int nbyte)
{
     if (!(fds.Oflag & (O_APPEND | O_RDWR))) return -1 ; // 書込みオープンでないエラー
     return sd_rdwr(buf,nbyte,3) ;
}
/*******************************************************************************
*  ans = SD_Read(buf,nbyte)                                                    *
*    ファイルから指定したバイト数だけ読み込む処理                              *
*    ファイルの先頭から読込まれます、読込む位置を変える場合はSD_Seekを使用     *
*    *buf  : 読込んだデータを格納する配列変数を指定します                      *
*    nbyte : 読込むデータのバイト数を指定します                                *
*            ファイルの最後(EOF)の場合、nbyteに満たなくてもそこで停止します    *
*                                                                              *
*    ans  : 正常＝読込んだバイト数  異常＝−１  ＥＯＦ＝０                     *
*******************************************************************************/
int SD_Read(char *buf,int nbyte)
{
     if (!(fds.Oflag & (O_READ | O_RDWR))) return -1 ; // 読込みオープンでないエラー
     if (fds.FileSeekP >= fds.FileSize)    return  0 ; // EOF
     return sd_rdwr(buf,nbyte,1) ;
}
/*******************************************************************************
*  ans = SD_fGets(buf,nbyte)                                                   *
*    ファイルから１行読み込む処理                                              *
*    0x0A(LF)に出会うまで読込みます、だから 0x0d(CR)0x0a(LF) もOKです。        *
*    CRとLFはデータとしてbufに入ります、bufの確保するデータサイズに注意です。  *
*    また、LFに出会わなければnbyteまで読込みます。                             *
*    *buf  : 読込むデータを格納した配列変数を指定します                        *
*    nbyte : 読込むデータのバイト数を指定します                                *
*                                                                              *
*    ans  : 正常＝読込んだバイト数  異常＝−１  ＥＯＦ＝０                     *
*******************************************************************************/
int SD_fGets(char *buf,int nbyte)
{
     if (!(fds.Oflag & (O_READ | O_RDWR))) return -1 ; // 読込みオープンでないエラー
     if (fds.FileSeekP >= fds.FileSize)    return  0 ; // EOF
     return sd_rdwr(buf,nbyte,2) ;
}
/*******************************************************************************
*  ans = SD_Size()                                                             *
*    ファイルのサイズを得る処理(現在SDに書込まれているサイズを返す)            *
*                                                                              *
*    ans  : 正常＝ファイルのサイズ  異常＝0xffffffff                           *
*******************************************************************************/
unsigned long SD_Size()
{
     if (fds.Oflag == 0) return 0xffffffff ;
     return fds.FileSize + fds.AppendSize ;
}
/*******************************************************************************
*  ans = SD_Position()                                                         *
*    ファイルの読書きポインタ値を得る処理                                      *
*                                                                              *
*    ans  : 正常＝現在位置付けされているファイルポインタ値  異常＝0xffffffff   *
*******************************************************************************/
unsigned long SD_Position()
{
     if (fds.Oflag == 0) return 0xffffffff ;
     return fds.FileSeekP ;
}
/*******************************************************************************
*  ans = SD_Seek(offset,sflag)                                                 *
*    ファイルの読書きポインタを移動させる処理                                  *
*    offset : 位置付けする場所(オフセット値)を指定します                       *
*    sflag  : 位置付けするオフセット値の方向を示すフラグを指定します           *
*             1 = ファイルの先頭からのオフセット値ですよ                       *
*             2 = 現在の場所から後ろ方向に位置付するオフセット値ですよ         *
*             3 = 現在の場所から前方向に位置付するオフセット値ですよ           *
*             4 = ファイル最後のバイトから後ろ方向に位置付けします             *
*                                                                              *
*    ans  : 正常＝移動先のファイルポインタ値  異常＝0xffffffff                 *
*******************************************************************************/
unsigned long SD_Seek(unsigned long offset,int sflag)
{
     unsigned long x , ans ;

     ans = 0xffffffff ;
     if (!(fds.Oflag & (O_READ | O_RDWR))) return ans ; // READ/RDWRオープン以外はエラー
     if (fds.AppendSize != 0) return ans ;              // データ追加中はシーク出来ない
     switch (sflag) {
        case 1:// 先頭から位置付する
               if (offset < fds.FileSize) ans = fds.FileSeekP = offset ;
               break ;
        case 2:// 現在の場所から後ろ方向に位置付する
               x = fds.FileSeekP + offset ;
               if (x < fds.FileSize) ans = fds.FileSeekP = x ;
               break ;
        case 3:// 現在の場所から前方向に位置付する
               x = fds.FileSize - fds.FileSeekP ;
               if (x >= offset) ans = fds.FileSeekP = fds.FileSeekP - offset ;
               break ;
        case 4:// ファイルの最後のバイトから後ろ方向に位置付けします
               if (fds.FileSize != 0) ans = fds.FileSeekP = (fds.FileSize-1) + offset ;
               else                   ans = fds.FileSeekP = 0 ;
               break ;
     }
     return ans ;
}
