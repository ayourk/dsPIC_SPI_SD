/* ************************************************ *****************************
* skSDlib.h - Include file for functions that access MMC/SDC *
* *
* ================================================= ===========================*
* VERSION DATE BY CHANGE/COMMENT *
* ------------------------------------------------- ---------------------------*
* 1.00 2012-04-30 Kimucha Kobo Create *
**************************************************** **************************** */
# ifndef _SKSDLIB_H_
# define  _SKSDLIB_H_

// #include <htc.h> // needed for delay

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

char MMCbuffer[SECTER_BYTES] ; // MMC card read/write buffer
int CardType ; // card type
int FatType ; // Type of FAT


//////////////////////////////////////////////////// _ ////////////////////////////////
// FAT related

# define  O_READ  0x01  // open file for reading
# define  O_RDWR  0x02  // open file for reading and writing
# define  O_APPEND  0x04  // open file for appending

// FAT file system parameters
unsigned  long Dir_Entry_StartP ; // start sector position of directory entry
unsigned  int DirEntry_SectorSU ; // Number of sectors in directory entry
unsigned  long Data_Area_StartP ; // Start sector position of data area
unsigned  long Fat_Area_StartP ; // Start sector position of FAT area
unsigned  int Cluster1_SectorSU ; // number of sectors per cluster
unsigned  long SectorsPerFatSU ; // The number of sectors occupied by one set of FAT areas

// file access information
struct FDS {
 unsigned  int Oflag ; // store open flag to access
 unsigned  int DirEntryIndex ; // Position of searched location for directory entry
 unsigned  long FileSize ; // file size
 unsigned  long FileSeekP ; // Position to read next file
 unsigned  long AppendSize ; // Size of file written additionally
 unsigned  long FirstFatno ; // Data storage destination FAT number
} ;
struct FDS fds ;

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


# endif
