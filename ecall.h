/*
----------------------------------------
	ECALL.H: MicroEMACS 3.10
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/ecall.h,v 1.3 1992/01/04 13:11:18 SALT Exp SALT $
----------------------------------------
*/

/*
----------------------------------------
	DOS コールの定義
----------------------------------------
*/

#define _MAX_DRIVE	2
#define	_MAX_PATH	65
#define	_MAX_NAME1	8
#define	_MAX_NAME2	10
#define	_MAX_NAME	(_MAX_NAME1 + _MAX_NAME2 + 1)
#define	_MAX_EXTEND	5
#define	_MAX_FILENAME	(_MAX_NAME + _MAX_EXTEND - 1)

struct FREEINF
  {
    unsigned short free, max;
    unsigned short sec, byte;
  };

struct FILBUF
  {
    char os[21];
    char atr;
    short time, date;
    int filelen;
    char name[_MAX_FILENAME];
  };

struct NAMESTBUF
  {
    char flg, drive;
    char path[_MAX_PATH];
    char name1[_MAX_NAME1];
    char ext[3], name2[_MAX_NAME2];
  };

/* from myu header */
struct NAMECKBUF
  {
    char   drive[_MAX_DRIVE];
    char   path[_MAX_PATH];
    char   name[_MAX_NAME];
    char   ext[_MAX_EXTEND];
  };

int BREAKCK (int);
int CHGDRV (int);
int CLOSE (int);
int CREATE (char *, int);
int CURDRV (void);
int C_FNKMOD (int);
int C_WIDTH (int);
int DELETE (char *);
int DSKFRE (int, struct FREEINF *);
int FILES (struct FILBUF *, char *, int);
int GETDATE (void);
int GETENV (char *, char *, char *);
int GETTIME (void);
int KEYSNS (void);
int KFLUSHIO (int);
int K_KEYBIT (int);
int K_KEYINP (void);
int K_KEYSNS (void);
int K_SFTSNS (void);
int NAMECK (char *, struct NAMECKBUF *);
int NAMESTS (char *, struct NAMESTBUF *);
int NFILES (struct FILBUF *);
int CHMOD (char *, int);
int OPEN (char *, int);
int READ (int, char *, int);
int SEEK (int, int, int);
int SETENV (char *, char *, char *);
int VERNUM (void);
int WRITE (int, char *, int);
void (*INTVCS (int, void (*) (void))) (void);
void CHANGE_PR (void);
void FNCKEYGT (int, char *);
void FNCKEYST (int, char *);
void K_INSMOD (int);
int SUPER (int);

/*
----------------------------------------
	IOCS コールの定義
----------------------------------------
*/

struct XLINEPTR
  {
    short vram_page;
    short x, y, x1;
    short line_style;
  };

struct YLINEPTR
  {
    short vram_page;
    short x, y, y1;
    short line_style;
  };

int B_SFTSNS (void);
int B_BPEEK (int);
int B_COLOR (int);
int B_CONSOL (int, int, int, int);
int B_LOCATE (int, int);
int B_PRINT (char *);
int B_PUTC (int);
int B_PUTMES (int, int, int, int, char *);
int B_WPOKE (int, short);
int CRTMOD (int);
int MS_CURGT (void);
int MS_CURST (int, int);
int MS_GETDT (void);
int MS_LIMIT (int, int, int, int);
int ONTIME (void);
int SKEY_MOD (int, int, int);
int TPALET (int, int);
int TPALET2 (int, int);
void B_CLR_AL (void);
void LEDMOD (int, int);
void MS_CUROF (void);
void MS_CURON (void);
void MS_INIT (void);
void OS_CUROF (void);
void OS_CURON (void);
void TXXLINE (struct XLINEPTR *);
void TXYLINE (struct YLINEPTR *);
void TXRASCPY (int, int, int);
