/*******************************************************************************
*  skSDlib.h - MMC/SDCのアクセスを行う関数のインクルードファイル               *
*                                                                              *
* ============================================================================ *
*   VERSION  DATE        BY             CHANGE/COMMENT                         *
* ---------------------------------------------------------------------------- *
*   1.00     2012-04-30  きむ茶工房     Create                                 *
*******************************************************************************/
#ifndef _SKSDLIB_H_
#define _SKSDLIB_H_

//#include <htc.h>              // delay用に必要

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

char MMCbuffer[SECTER_BYTES] ;          // ＭＭＣカード読書きバッファ
int  CardType ;                         // カードの種類
int  FatType ;                          // ＦＡＴの種類


////////////////////////////////////////////////////////////////////////////////
// ＦＡＴ関連

#define O_READ    0x01                  // ファイルの読込みオープン
#define O_RDWR    0x02                  // ファイルの読込みと書込みオープン
#define O_APPEND  0x04                  // ファイルの追加書込みオープン

//ＦＡＴファイルシステムのパラメータ
unsigned long Dir_Entry_StartP ;        // ディレクトリエントリの開始セクタ位置
unsigned int  DirEntry_SectorSU ;       // ディレクトリエントリのセクタ個数
unsigned long Data_Area_StartP ;        // データ領域の開始セクタ位置
unsigned long Fat_Area_StartP ;         // FAT領域の開始セクタ位置
unsigned int  Cluster1_SectorSU ;       // １クラスタあたりのセクタ数
unsigned long SectorsPerFatSU ;         // １組のFAT領域が占めるセクタ数

// ファイルのアクセス情報
struct FDS {
     unsigned int  Oflag ;              // アクセスするオープンフラグを保存
     unsigned int  DirEntryIndex ;      // ディレクトリエントリの検索した場所の位置
     unsigned long FileSize ;           // ファイルのサイズ
     unsigned long FileSeekP ;          // ファイルの次読み出す位置
     unsigned long AppendSize ;         // 追加書込みしたファイルのサイズ
     unsigned long FirstFatno ;         // データ格納先のFAT番号
} ;
struct FDS fds ;

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


#endif
