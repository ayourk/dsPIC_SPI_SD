/* ************************************************ *****************************
* skSDlib.c - MMC/SD card access function library *
* *
* SD_Init - Processing to initialize MMC/SDC *
* SD_Open - Process to open a file *
* SD_Close - Process to close the file *
* SD_Write - Process to write a specified number of bytes to a file *
* SD_Read - Process to read specified number of bytes from file *
* SD_fGets - Process to read one line from file *
* SD_Size - Process to get file size *
* SD_Position - Process to get file read/write pointer value *
* SD_Seek - Process to move file read/write pointer *
* *
* Memo: SS : Pin 13 (RC2) SCK: Pin 14 (RC3) SDI: Pin 15 (RC4) *
* SDO: Use pin 16 (RC5) *
* Only standard SD/MicroSD (FAT16)/standard SDHC/MicroSDHC (FAT32) can be read and written *
* ================================================= ===========================*
* VERSION DATE BY CHANGE/COMMENT *
* ------------------------------------------------- ---------------------------*
* 1.00 2012-04-30 Kimushige Create *
* 2.00 2012-05-03 Support for Kimushige SDHC *
* 2.01 2012-06-09 Kimushige Standard SD/SDHC OK comment changed *
* 2.02 2014-02-02 Changed to support Kimushige XC8 C Compiler *
* 3.00 2014-09-01 Reported by Mr. Rossi Bug fix *
* 3.01 2014-09-03 Kimushige fixed the problem that the file name cannot be 8 characters *
* 3.02 2014-09-03 Changed processing of flag 4 of Kimushige Seek() *
* ================================================= ===========================*
* PIC 16F1938 18F25K22 *
* (Maybe other PICs can use this function as it is if they have 768 bytes or more of SRAM)*
* MPLAB IDE(V8.63) *
* MPLAB(R) XC8 C Compiler Version 1.00 *
**************************************************** **************************** */
#include < xc.h > _ 
# include  < string.h >
#include < libpic30.h > _ 
// #include "skMonitorLCD.h"
// #include "skSPI.h"
// #include "skSD.h"


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


# ifndef _XTAL_FREQ
 // Unless already defined assume 8MHz system frequency
 // This definition is required to calibrate __delay_us() and __delay_ms()
# define  _XTAL_FREQ  8000000  // Set the operating frequency value according to the PIC used
# endif

//////////////////////////////////////////////////// _ ////////////////////////////////
// SPI related

# define  CS PORTBbits.RB2 // card select signal

//////////////////////////////////////////////////// _ ////////////////////////////////
// MMC/SDC access related

# define  CMD0  0x00  // reset command to card
# define  CMD1  0x01  // initialization command to MMC
# define  CMD8  0x08  // Confirm operating voltage and SDC version check
# define  CMD12  0x0C  // Command to stop reading data
# define  CMD13  0x0D  // Command to query write status
# define  CMD16  0x10  // block size initial value setting command
# define  CMD17  0x11  // single block read request command
# define  CMD24  0x18  // Single block write request command
# define  ACMD41  0x29  // initialization command to SDC
# define  CMD55  0x37  // command used in combination with ACMD41/ACMD23
# define  CMD58  0x3A  // OCR read command

# define  SECTER_BYTES  512  // number of bytes in one sector

extern  char MMCbuffer[SECTER_BYTES] ; // MMC card read/write buffer
extern  int CardType ; // card type
extern  int FatType ; // type of FAT


//////////////////////////////////////////////////// _ ////////////////////////////////
// FAT related

# define  O_READ  0x01  // open file for reading
# define  O_RDWR  0x02  // open file for reading and writing
# define  O_APPEND  0x04  // open file for appending

// FAT file system parameters
extern  unsigned  long Dir_Entry_StartP ; // start sector position of directory entry
extern  unsigned  int DirEntry_SectorSU ; // Sector count of directory entry
extern  unsigned  long Data_Area_StartP ; // start sector position of data area
extern  unsigned  long Fat_Area_StartP ; // Start sector position of FAT area
extern  unsigned  int Cluster1_SectorSU ; // number of sectors per cluster
extern  unsigned  long SectorsPerFatSU ; // Number of sectors occupied by one set of FAT areas

// file access information
struct FDS {
 unsigned  int Oflag ; // store open flag to access
 unsigned  int DirEntryIndex ; // Position of searched location for directory entry
 unsigned  long FileSize ; // file size
 unsigned  long FileSeekP ; // Position to read next file
 unsigned  long AppendSize ; // Size of file written additionally
 unsigned  long FirstFatno ; // Data storage destination FAT number
} ;
extern  struct FDS fds ;

// FAT file system (FAT12/FAT16) parameter structure (512 bytes)
struct FAT_PARA {
 unsigned  char jump[ 3 ] ; // Jump code for boot
 unsigned  char oemId[ 8 ] ; // Volume name when formatting
 unsigned  int BytesPerSector ; // number of bytes per sector, typically 512 bytes
 unsigned  char SectorsPerCluster ; // number of sectors per cluster
 unsigned  int ReservedSectorCount ; // Number of sectors in reserved area after boot sector
 unsigned  char FatCount ; // Number of FAT pairs (number of backup FATs), usually 2 pairs
 unsigned  int RootDirEntryCount ; // Number of directories that can be created, usually 512
 unsigned  int TotalSectors16 ; // Total number of sectors in all areas (for FAT12/FAT16)
 unsigned  char MediaType ; // first value in FAT area, usually 0xF8
 unsigned  int SectorsPerFat16 ; // Number of sectors occupied by one set of FAT area (for FAT12/FAT16)
 unsigned  int SectorsPerTrack ; // number of sectors per track
 unsigned  int HeadCount ; // Number of heads
 unsigned  long HiddenSectors ; // number of hidden sectors
 unsigned  long TotalSectors32 ; // Total number of sectors in all areas (for FAT32)
 unsigned  long SectorsPerFat32 ; // Number of sectors occupied by one set of FAT area (for FAT32)
 unsigned  int FlagsFat32 ; // Information flags such as FAT enable/disable
 unsigned  int VersionFat32 ;
 unsigned  long RootDirClusterFat32 ; // Directory start cluster (for FAT32)
 unsigned  char Dumy[ 6 ];
 unsigned  char FileSystemType[ 8 ] ; // FAT type ("FAT12/16") (FAT32 is 20 bytes below)
 unsigned  char BootCode[ 448 ] ; // boot code area
 unsigned  char BootSectorSig0 ; // 0x55
 unsigned  char BootSectorSig1 ; // 0xaa
} ;

// Directory entry structure (32 bytes)
struct DIR_ENTRY {
 unsigned  char FileName[ 11 ] ; // File name (8) + extension (3)
 unsigned  char Attributes ; // file attributes
 unsigned  char ReservedNT ; // reserved area for Windows NT
 unsigned  char CreationTimeTenths ; // File creation time in tenths of a second
 unsigned  int CreationTime ; // File creation time (hhhhmmmmmmsssss)
 unsigned  int CreationDate ; // File creation date (yyyyyyymmmmddddd)
 unsigned  int LastAccessDate ; // last access date
 unsigned  int FirstClusterHigh ; // FAT number upper 2 bytes of data storage destination
 unsigned  int LastWriteTime ; // last file write time
 unsigned  int LastWriteDate ; // last file write date
 unsigned  int FirstClusterLow ; // Lower 2 bytes of FAT number of data storage destination
 unsigned  long FileSize ; // file size
} ;

//////////////////////////////////////////////////// _ ////////////////////////////////
//
int  SD_Init () ;
int  SD_Open ( const  char *filename, int oflag) ;
void  SD_Close ();
int  SD_Write ( char *buf, int nbyte) ;
int  SD_Read ( char *buf, int nbyte) ;
int  SD_fGets ( char *buf, int nbyte) ;
unsigned  long  SD_Size () ;
unsigned  long  SD_Position () ;
unsigned  long  SD_Seek ( unsigned  long offset, int sflag) ;



//////////////////////////////////////////////////// _ ////////////////////////////////
// Process to select MMC/SDC card and output HIGH/LOW of CS signal
void  cs_select ( int flag) {

CS = flag;
 SPI_transfer ( 0xff ) ; // Transmit the clock to switch CS reliably
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to check the ready status
int  ready_check ()
{
 intc ;

c = 0 ;
 while ( SPI_transfer ( 0xff ) != 0xff && c<= 500 ) {
c++;
 __delay32 ( 100 ) ;
}
 if (c>= 500 ) return  1 ; // timeout
 else  return  0 ; // ok
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to send commands to MMC/SDC
int  send_command ( unsigned  char cmd, unsigned  long arg)
{
 int x , ans ;

 cs_select (LOW);
ans = ready_check () ;
 if (ans != 0 ) return  0xff00 ; // card seems to be busy
 // send command
 SPI_transfer (cmd | 0x40 ) ; // Send command
 for (x= 24 ; x >= 0 ; x -= 8 ) SPI_transfer (arg >> x) ; // Transmit parameters
x = 0XFF ;
 if (cmd == CMD0) x = 0X95 ;
 if (cmd == CMD8) x = 0X87 ;
 SPI_transfer (x) ; // Send CRC
 // wait for command response
x = 0 ;
 do {
ans = SPI_transfer ( 0xff ) ;
x++;
} while (ans & 0x80 && x < 256 ) ;
 if (x >= 256 ) ans = (cmd| 0x40 ) << 8 ; // response timeout

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to write one block (single write) of data from the specified sector position
int  sblock_write ( unsigned  long sector, char *buff)
{
 int i , ans ;

 if (CardType != 0x13 ) sector <<= 9 ; // Not SDHC
ans = send_command (CMD24, sector) ; // Issue a single read command
 if (ans == 0 ) {
 SPI_transfer ( 0xfe ) ; // Send data token
 for (i= 0 ; i<SECTER_BYTES ; i++) {
 SPI_transfer (*buff) ; // Send the data part
buff++;
}
 SPI_transfer ( 0xff ) ; // Transmit the CRC part
 SPI_transfer ( 0xff ) ;
ans = SPI_transfer ( 0xff ) ; // Receive response
ans = ans & 0x1F ;
 if (ans == 0x05 ) {
ans = ready_check () ;
 if (ans != 0 ) ans = 0x8800 ; // card seems busy
 else {
 // Inquire if the write was successful
ans = send_command (CMD13, sector) ;
i = SPI_transfer ( 0xff ) ;
 if (ans == 0 ) {
 if (i != 0 ) ans = 0x8900 |ans ; // write failed
}
}
} else ans = 0x8800 | ans ; // write error
} else ans = CMD24 << 8 | ans ; // CMD24 error
 cs_select (HIGH);

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to read one block of data (single read) from the specified sector position
int  sblock_read ( unsigned  long sector, char *buff)
{
 int i , ans ;

 if (CardType != 0x13 ) sector <<= 9 ; // Not SDHC
ans = send_command (CMD17, sector) ; // Issue a single read command
 if (ans == 0 ) {
i = 0 ;
 // wait for the first byte of the reply data
 while ( 1 ) {
ans = SPI_transfer ( 0xff ) ;
i++;
 if (ans != 0xff || i>= 1000 ) {
 if (i>= 1000 ) ans = 0x8600 ; // data token timeout
 break ;
}
 __delay32 ( 100 ) ;
}
 // receive the second and subsequent bytes
 if (ans == 0xfe ) {
 for (i= 0 ; i<SECTER_BYTES ; i++) {
*buff = SPI_transfer ( 0xff ) ;
buff++;
}
 SPI_transfer ( 0xff ) ; // Receive the CRC part
 SPI_transfer ( 0xff ) ;
ans = 0 ;
} else ans = 0x8600 | ans ; // read error
} else ans = CMD17 << 8 | ans ; // CMD17 error
 cs_select (HIGH);

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to read the parameters of the FAT file system
int  fat_para_read ()
{
 union {
 unsigned  char c[ 4 ];
 unsigned  long l;
} dt;
 struct FAT_PARA *fat;
 int i , ans ;

 // read 512 bytes from the beginning of the card
ans = sblock_read ( 0 ,MMCbuffer);
 if (ans == 0 ) {
 // Get offset value to BPB (boot sector)
 for (i= 0 ; i< 4 ; i++) {
dt.c [ i] = MMCbuffer[i+ 0x1c6 ] ;
}
 // read the parameters of the FAT file system
ans = sblock_read (dt. l ,MMCbuffer) ;
 if (ans == 0 ) {
fat = ( struct FAT_PARA *) MMCbuffer ;
 // number of sectors per cluster
Cluster1_SectorSU = fat -> SectorsPerCluster ;
 // Number of sectors occupied by one set of FAT area
 if (fat-> SectorsPerFat16 != 0 ) {
FatType = 2 ;
SectorsPerFatSU = fat -> SectorsPerFat16 ;
} else {
FatType = 4 ;
SectorsPerFatSU = fat -> SectorsPerFat32 ;
}
 // start sector position of FAT area
Fat_Area_StartP = fat-> ReservedSectorCount + dt. l ;
 // start sector location of root directory entry
Dir_Entry_StartP = Fat_Area_StartP + (SectorsPerFatSU * fat-> FatCount ) ;
 // start sector position of data area
DirEntry_SectorSU = fat-> RootDirEntryCount / (SECTER_BYTES / sizeof ( struct DIR_ENTRY)) ;
Data_Area_StartP = Dir_Entry_StartP + DirEntry_SectorSU ;
 if (FatType == 4 ) DirEntry_SectorSU = Cluster1_SectorSU ; // FAT32
}
}
 return ans;
}
/* ************************************************ *****************************
* ans = SD_Init() *
* Processing to initialize MMC/SDC *
* Available only for standard type SD/SDHC and MicroSD/SDHC FAT16/FAT32 *
* *
* ans : Normal = 0 Abnormal = error code value other than 0 *
**************************************************** **************************** */
int  SD_Init ()
{
 unsigned  long arg;
 unsigned  char r7[ 4 ];
 unsigned  int i , ans ;

CS = 1 ;
CardType = 0 ;

 // Transmit CLK for 74 clocks or more to put the card in command wait state
 for (i = 0 ; i < 10 ; i++) SPI_transfer ( 0xFF ) ;

 // Send a reset command (CMD0) to the card (shift to SPI mode)
ans = send_command (CMD0, 0 ) ;
 if (ans == 1 ) {
// MonitorPuts("CMD0") ;
 // Perform initialization processing for SDC
arg = 0 ;
 // Confirm operating voltage and card version check
ans = send_command (CMD8, 0x1AA ) ;
 if (ans == 1 ) {
 for (i= 0 ; i< 4 ; i++) r7[i] = SPI_transfer ( 0xff ) ;
 if (r7[ 3 ] == 0xAA ) {
CardType = 0x12 ; // SDCver.2 card
arg = 0X40000000 ;
} else ans = 0x8200 ; // CMD8 error
} else {
 if (ans & 0x4 ) CardType = 0x11 ; // SDCver.1 card
}
 if (CardType != 0 ) {
// MonitorPuts(":8") ;
 // Issue initialization command (ACMD41) for SDC
i = 0 ;
 while ( 1 ) {
ans = send_command (CMD55, 0 ) ;
ans = send_command (ACMD41, arg) ;
i++;
 if (ans == 0 || i>= 2000 ) {
 if (i>= 2000 ) ans = 0x8300 |ans ; // ACMD41 timeout
 break ;
}
 __delay32 ( 100 ) ;
}
// if (ans == 0) MonitorPuts(":A41") ;
 if (ans == 0 && CardType == 0x12 ) {
 // Issue OCR read command (CMD58) for Ver.2
ans = send_command (CMD58, 0 ) ;
 if (ans == 0 ) {
// MonitorPuts(":58") ;
 for (i= 0 ; i< 4 ; i++) r7[i] = SPI_transfer ( 0xff ) ;
 if (r7[ 0 ] & 0x40 ) CardType = 0x13 ; // SDHC card
} else ans = CMD58 << 8 | ans ; // CMD58 error
}
} else {
 // Issue initialization command (CMD1) for MMC
i = 0 ;
 while ( 1 ) {
ans = send_command (CMD1, 0 ) ;
i++;
 if (ans != 1 || i>= 2000 ) {
 if (i>= 2000 ) ans = 0x8100 ; // initialization timeout
 break ;
}
 __delay32 ( 100 ) ;
}
 if (ans == 0 ) {
CardType = 0x01 ; // MMC card
// MonitorPuts(":1") ;
} else {
 if (ans != 0x8100 ) ans = CMD1 << 8 | ans ; // CMD1 error
}
}
 if (ans == 0 ) {
 // Reset the initial block size to 512 bytes
ans = send_command (CMD16, SECTER_BYTES) ;
 if (ans != 0 ) ans = CMD16 << 8 | ans ; // CMD16 error
// else MonitorPuts(":16") ;
}
} else ans = CMD0 << 8 | ans ; // CMD0 error
 cs_select (HIGH);

 // read the parameters of the FAT file system when the initialization is completed successfully
 if (ans == 0 ) {
ans = fat_para_read ();
}

// MonitorPuts("CardType = ") ;
// MonitorPut(CardType, HEX) ;

 // Initialize file access information
fds.Oflag = 0 ; _

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Get the entry information of the specified file, if not, get the empty location of the entry
int  search_file ( char *filename, struct FDS *fds)
{
 struct DIR_ENTRY *inf ;
 int i , j , c , x , ans ;

ans = c = 0 ;
fds -> DirEntryIndex = 0 ;
 // repeat for all sectors of the directory entry
 for (i= 0 ; i<DirEntry_SectorSU ; i++) {
 // read directory entry data
ans = sblock_read (Dir_Entry_StartP+i,MMCbuffer) ;
 if (ans == 0 ) {
 // Repeat for the number of entries in one sector
x = SECTER_BYTES / sizeof ( struct DIR_ENTRY) ;
 for (j= 0 ; j<x ; j++ ) {
c++;
 // examine file entries
inf = ( struct DIR_ENTRY *)&MMCbuffer[j* sizeof ( struct DIR_ENTRY)] ;
 if (inf-> FileName [ 0 ] == 0x2e ) continue ; // subdirectory
 if (inf-> FileName [ 0 ] == 0xe5 ) {
 if ( fds-> DirEntryIndex == 0 ) fds-> DirEntryIndex = c ;
 continue ; // Deleted entry
}
 if (inf-> FileName [ 0 ] == 0x00 ) {
 if ( fds-> DirEntryIndex == 0 ) fds-> DirEntryIndex = c ;
i = DirEntry_SectorSU ;
ans = 2 ; // not found
 break ; // empty entry
}
 // check file attributes
 if (inf-> Attributes != 0x20 ) continue ; // not a normal file
 // compare file names
 if ( memcmp (inf-> FileName ,filename, 11 ) == 0 ) {
fds -> DirEntryIndex = c ;
 // set file access information
fds-> FileSeekP = 0 ; // File read position is 0
fds-> FileSize = inf-> FileSize ; // file size
 // Record the FAT number of the data storage destination
fds -> FirstFatno = inf -> FirstClusterHigh ;
fds-> FirstFatno = (fds-> FirstFatno << 16 ) | inf-> FirstClusterLow ;
i = DirEntry_SectorSU ;
ans = 1 ;
 break ; // Normal end
}
}
} else ans = - 1 ;
}

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to search for an empty FAT number
unsigned  long  search_fat ()
{
 unsigned  long ans;
 unsigned  int i ;
 int j , x , k ;

ans = 0 ;
 // Repeat for the total number of FATs in one set
 for (i= 0 ; i<SectorsPerFatSU ; i++) {
 if ( sblock_read (Fat_Area_StartP+i,MMCbuffer) == 0 ) {
 for (j= 0 ; j<SECTER_BYTES ; j=j+FatType) {
 // There was an empty FAT
x = 0 ;
 for (k= 0 ; k<FatType ; k++) x = x | MMCbuffer[j+k] ;
 if (x == 0 ) {
MMCbuffer[j] = 0xff ;
MMCbuffer[j+ 1 ] = 0xff ;
 if (FatType == 4 ) { // FAT32
MMCbuffer[j+ 2 ] = 0xff ;
MMCbuffer[j+ 3 ] = 0x0f ;
}
 // reservation update
 if ( sblock_write (Fat_Area_StartP+i,MMCbuffer) == 0 ) {
 // also write the second FAT
 sblock_write ((Fat_Area_StartP+i)+SectorsPerFatSU,MMCbuffer) ;
ans = (i * SECTER_BYTES + j) / FatType ;
}
i = SectorsPerFatSU;
 break ; // finish search
}
}
} else  break ;
}
 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Directory entry update process
int  direntry_make ( unsigned  long no, char *filename, struct FDS *fds)
{
 struct DIR_ENTRY *inf ;
 unsigned  long p ;
 unsigned  int x , y ;
 int ans;

ans = - 1 ;
 // read the directory entry
x = fds -> DirEntryIndex - 1 ;
y = SECTER_BYTES / sizeof ( struct DIR_ENTRY) ;
p = Dir_Entry_StartP + (x / y) ;
 if ( sblock_read (p,MMCbuffer) == 0 ) {
inf = ( struct DIR_ENTRY *)&MMCbuffer[(x % y) * sizeof ( struct DIR_ENTRY)] ;
 if (no != 0 ) {
 memset (inf, 0x00 , sizeof ( struct DIR_ENTRY)) ;
 // set file name
 memcpy (inf-> FileName ,filename, 11 ) ;
 // set file attributes
inf -> Attributes = 0x20 ;
 // create a new file
inf -> FileSize = 0 ;
 // set file creation time

 // set the creation date of the file

 // set access date

 // Set the FAT number of the data storage destination
inf-> FirstClusterHigh = ( unsigned  int )(no >> 16 ) ;
inf-> FirstClusterLow = ( unsigned  int )(no & 0x0000ffff ) ;
 // set file access information
fds -> FileSeekP = 0 ;
fds -> FileSize = 0 ;
 // Record the FAT number of the data storage destination
fds -> FirstFatno = inf -> FirstClusterHigh ;
fds-> FirstFatno = (fds-> FirstFatno << 16 ) | inf-> FirstClusterLow ;
} else {
 // update file size
inf -> FileSize = inf -> FileSize + fds -> AppendSize ;
 // set file write date and time

 // set access date

}
 // update directory entry
ans = sblock_write (p,MMCbuffer) ;
}
 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to format the file name
void  filename_check ( char *c, const  char *filename)
{
 int i ;

 memset (c, 0x20 , 11 ) ;
 for (i= 0 ; i< 8 ; i++) {
 if (*filename == ' . ' ) {
c = c + ( 8 -i);
filename++;
 break ;
}
*c = *filename;
c++;
filename++;
}
 if (i > 7 ) filename++;
 for (i= 0 ; i< 3 ; i++) {
 if (*filename == 0x00 ) break ;
*c = *filename;
c++;
filename++;
}
}
/* ************************************************ *****************************
* ans = SD_Open(filename,oflag) *
* Processing to open the file *
* filename : Specify the file name (xxxxxxxx.xxx) *
* Be sure to specify the extension, and specify uppercase letters. *
* oflag : Specifies the access flag *
* O_READ read only (error if file does not exist) *
* O_APPEND Append write only (append to end of file) *
* O_RDWR read/write (file will be created if it doesn't exist) *
* *
* ans : Normal = 0 Abnormal = -1 *
**************************************************** **************************** */
int  SD_Open ( const  char *filename, int oflag)
{
 unsigned  long no;
 char c[ 11 ] ;
 int ans , ret ;

ret = - 1 ;
 if (fds. Oflag != 0 ) return ret ; // Already opened
fds.Oflag = 0 ; _
 // format the specified file name
 filename_check (c, filename) ;
 // find the specified file
ans = search_file (c,( struct FDS *)&fds) ;
 if (ans > 0 ) {
 if (oflag == O_READ && ans == 1 ) ret = 0 ;
 if (oflag == O_APPEND && ans == 1 ) {
 // Set the file access position to the end of the file + 1
fds.FileSeekP = fds.FileSize ; _
ret = 0 ;
}
 if (oflag == O_RDWR && (ans == 1 || ans == 2 )) {
 if (ans == 2 ) {
 // create a new file
 if ( fds.DirEntryIndex ! = 0 ) {
 // Find and secure an empty FAT
no = search_fat ();
 if (no != 0 ) {
 // create a directory entry
 if ( direntry_make (no,c,( struct FDS *)&fds) == 0 ) ret = 0 ;
}
}
} else ret = 0 ;
}
 if (ret == 0 ) {
 // make the file accessible
fds.Oflag = oflag ;
fds.AppendSize = 0 ; _
}
}

 return ret ;
}
/* ************************************************ *****************************
* SD_Close() *
* Processing to close the file *
* The file size when adding data is written by this function. *
**************************************************** **************************** */
void  SD_Close ()
{
 // processing when open for writing
 if (fds.Oflag & ( O_APPEND | O_RDWR)) {
 if ( fds. AppendSize == 0 ) return ;
 // update the directory entry
 direntry_make ( 0 , 0 ,( struct FDS *)&fds) ;
}
 // Initialize file access information
fds.Oflag = 0 ; _
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Processing to get the FAT number next to the specified FAT number
unsigned  long  next_fat_read ( unsigned  long fatno, struct FDS *fds)
{
 union {
 unsigned  char c[ 4 ];
 unsigned  long i ;
} no;
 unsigned  long p , x , y , ans ;
 intj ;

 // read the data in the FAT area
p = Fat_Area_StartP + (fatno / (SECTER_BYTES/FatType)) ;
ans = sblock_read (p,MMCbuffer) ;
 if (ans == 0 ) {
x = (fatno % (SECTER_BYTES/FatType)) * FatType ;
no. i = 0 ;
 for (j= 0 ; j<FatType ; j++) no.c [ j] = MMCbuffer[x+j] ;
 // get the next destination FAT number
ans = no. i ;
 if (FatType == 4 ) y = 0x0fffffff ; // FAT32
 else y = 0xffff ;
 // If there is no next chain destination, create a new chain destination FAT
 if (y == ans) {
ans = search_fat () ; // Find new FAT number available
 if (ans != 0 ) {
 // Update the FAT information of the chain source
 if ( sblock_read (p,MMCbuffer) == 0 ) {
no. i = ans ;
 for (j= 0 ; j<FatType ; j++) MMCbuffer[x+j] = no. c [j] ;
 if ( sblock_write (p,MMCbuffer) == 0 ) {
 // also write the second FAT
 sblock_write (p + SectorsPerFatSU,MMCbuffer) ;
} else ans = 0 ;
} else ans = 0 ;
}
}
} else ans = 0 ;

 return ans;
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// Calculating the FAT number (cluster number) from the seek position
void  fatno_seek_conv ( unsigned  long *fatno, struct FDS *fds)
{
 unsigned  int p ;
 int i ;

 // Calculate the read logical sector position from the data seek position, and what logical cluster position is it?
p = (fds-> FileSeekP / SECTER_BYTES) / Cluster1_SectorSU ;
 // Calculate the actual FAT number (cluster number) from the logical cluster from the FAT area
*fatno = fds -> FirstFatno ;
 for (i= 0 ; i<p ; i++) {
 // read the next chain destination FAT number
*fatno = next_fat_read (*fatno,( struct FDS *)fds) ;
}
}
//////////////////////////////////////////////////// _ ////////////////////////////////
// The process by which this function actually reads the specified number of bytes from the file
// type : 1=Call from SD_Read 2=Call from SD_fGets 3=Call from SD_Write
int  sd_rdwr ( char *buf, int nbyte, int type)
{
 unsigned  long dtSP ;
 unsigned  long fat no;
 unsigned  int p , x ;
 int i , c , ans ;
 
 // Calculate the FAT number (cluster number) from the seek position
 fatno_seek_conv (&fatno,( struct FDS *)&fds) ;
 if (fatno == 0 ) return - 1 ; // FAT area read error
 // Calculate the first sector position of data
dtSP = Data_Area_StartP + ((fatno - 2 ) * Cluster1_SectorSU) ;
p = (fds. FileSeekP / SECTER_BYTES) % Cluster1_SectorSU ; // Sector position within cluster
 // read file contents from data area
ans = sblock_read (dtSP+p,MMCbuffer) ;
 if (ans == 0 ) {
x = fds.FileSeekP % SECTER_BYTES ;
c = 0 ;
 // Repeat for the specified number of bytes
 for (i= 0 ; i<nbyte ; i++ ) {
 if (type == 3 ) MMCbuffer[x] = *buf ; // write
 else *buf = MMCbuffer[x] ; // read
c++;
x++ ;
fds.FileSeekP ++ ;
 if ( fds.FileSeekP > = fds.FileSize ) {
 if (type < 3 ) break ; // read to the end
fds. AppendSize ++ ; // counts as much data as added
}
 if (c >= SECTER_BYTES) break ; // processed only SECTER_BYTES
 if (type == 2 && *buf == 0x0a ) break ; // exit if LF
 // Processing when data spans the next sector
 if (x >= SECTER_BYTES) {
 // If it is a write request, write the data up to here
 if (type == 3 ) {
ans = sblock_write (dtSP+p,MMCbuffer) ;
 if (ans != 0 ) {
ans = - 1 ; // data area write error
 break ;
}
}
p++;
 if (p >= Cluster1_SectorSU) {
 // It seems that there is data in the next cluster
 // Calculate the next FAT number (cluster number) from the seek position
 fatno_seek_conv (&fatno,( struct FDS *)&fds) ;
 if (fatno == 0 ) {
ans = - 1 ; // FAT area read error
 break ;
}
 // Calculate the first sector position of data
dtSP = Data_Area_StartP + ((fatno - 2 ) * Cluster1_SectorSU) ;
p = (fds. FileSeekP / SECTER_BYTES) % Cluster1_SectorSU ; // Sector position within cluster
}
 // read the next block
ans = sblock_read (dtSP+p,MMCbuffer) ;
 if (ans == 0 ) x = 0 ;
 else {
ans = - 1 ; // data area read error
 break ;
}
}
buf++ ;
}
 // write data if write request
 if (x != 0 && ans != - 1 && type == 3 ) {
ans = sblock_write (dtSP+p,MMCbuffer) ;
 if (ans != 0 ) ans = - 1 ; // data area write error
}
 if (ans != - 1 ) ans = c ; // return the number of bytes read
} else ans = - 1 ; // data area read error
 
 return ans;
}
/* ************************************************ *****************************
* ans = SD_Write(buf,nbyte) *
* Processing to write the specified number of bytes to the file *
* The file is written from the beginning, use SD_Seek to change the writing position *
* *buf : Specify the array variable that stores the data to be written *
* nbyte : Specifies the number of bytes of data to write *
* *
* ans : Normal = Number of bytes written Abnormal = -1 *
**************************************************** **************************** */
int  SD_Write ( char *buf, int nbyte)
{
 if (!(fds. Oflag & (O_APPEND | O_RDWR))) return - 1 ; // error not open for write
 return  sd_rdwr (buf, nbyte, 3 ) ;
}
/* ************************************************ *****************************
* ans = SD_Read(buf,nbyte) *
* Processing to read only the specified number of bytes from the file *
* Read from the beginning of the file, use SD_Seek to change the read position *
* *buf : Specify the array variable to store the read data *
* nbyte : Specifies the number of bytes of data to read *
* At end of file (EOF), stop there even if less than nbyte *
* *
* ans : Normal = Number of bytes read Abnormal = -1 EOF = 0 *
**************************************************** **************************** */
int  SD_Read ( char *buf, int nbyte)
{
 if (!(fds. Oflag & (O_READ | O_RDWR))) return - 1 ; // error not open for reading
 if (fds.FileSeekP > = fds.FileSize ) return 0 ; // EOF 
 return  sd_rdwr (buf, nbyte, 1 ) ;
}
/* ************************************************ *****************************
* ans = SD_fGets(buf,nbyte) *
* Processing to read one line from a file *
* Read until 0x0A(LF) is encountered, so 0x0d(CR)0x0a(LF) is also OK. *
* CR and LF are entered in buf as data, so pay attention to the data size of buf. *
* Also, if LF is not encountered, read up to nbyte. *
* *buf : Specify the array variable that stores the read data *
* nbyte : Specifies the number of bytes of data to read *
* *
* ans : Normal = Number of bytes read Abnormal = -1 EOF = 0 *
**************************************************** **************************** */
int  SD_fGets ( char *buf, int nbyte)
{
 if (!(fds. Oflag & (O_READ | O_RDWR))) return - 1 ; // error not open for reading
 if (fds.FileSeekP > = fds.FileSize ) return 0 ; // EOF 
 return  sd_rdwr (buf, nbyte, 2 ) ;
}
/* ************************************************ *****************************
* ans = SD_Size() *
* Processing to get the file size (returns the size currently written to SD) *
* *
* ans : Normal = File size Abnormal = 0xffffffff *
**************************************************** **************************** */
unsigned  long  SD_Size ()
{
 if ( fds.Oflag == 0 ) return 0xffffffff  ;
 return fds.FileSize + fds.AppendSize ; _
}
/* ************************************************ *****************************
* ans = SD_Position() *
* Process to get file read/write pointer value *
* *
* ans : Normal = File pointer value currently positioned Abnormal = 0xffffffff *
**************************************************** **************************** */
unsigned  long  SD_Position ()
{
 if ( fds.Oflag == 0 ) return 0xffffffff  ;
 return fds.FileSeekP ; _
}
/* ************************************************ *****************************
* ans = SD_Seek(offset, sflag) *
* Processing to move the file read/write pointer *
* offset : Specifies the position (offset value) to position *
* sflag : Specifies the flag indicating the direction of the offset value to be positioned *
* 1 = offset from the beginning of the file *
* 2 = Offset value to position backwards from the current location *
* 3 = the offset value to position forward from the current location *
* 4 = Position backwards from the last byte of the file *
* *
* ans : Normal = Destination file pointer value Abnormal = 0xffffffff *
**************************************************** **************************** */
unsigned  long  SD_Seek ( unsigned  long offset, int sflag)
{
 unsigned  long x , ans ;

ans = 0xffffffff ;
 if (!(fds. Oflag & (O_READ | O_RDWR))) return ans ; // error except READ/RDWR open
 if (fds. AppendSize != 0 ) return ans ; // You cannot seek while data is being added
 switch (sflag) {
 case  1 : // position from the beginning
 if (offset < fds.FileSize ) ans = fds.FileSeekP = offset ;
 break ;
 case  2 : // position backwards from the current location
x = fds.FileSeekP + offset;
 if (x < fds.FileSize ) ans = fds.FileSeekP = x ;
 break ;
 case  3 : // position forward from the current location
x = fds.FileSize - fds.FileSeekP ;
 if (x >= offset) ans = fds.FileSeekP = fds.FileSeekP - offset ;
 break ;
 case  4 : // position backwards from the last byte of the file
 if (fds.FileSize != 0 ) ans = fds.FileSeekP = ( fds.FileSize - 1 ) + offset ;
 else ans = fds.FileSeekP = 0 ;
 break ;
}
 return ans;
}
