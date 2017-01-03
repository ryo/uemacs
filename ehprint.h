/*
----------------------------------------
	HPRINT.H: MicroEMACS 3.10
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/ehprint.h,v 1.2 1992/02/15 07:19:02 SALT Exp SALT $
----------------------------------------
*/

/*
----------------------------------------
	H_PRINTÉÇÉWÉÖÅ[ÉãÇÃêÈåæ
----------------------------------------
*/

int H_CUROFF(void);
int H_CURON(void);
int H_LOCATE(int, int);
void H_CURINT(int);
void H6_CURSET(void);
void H_DENSITY(int);
void H_ERA(int);
void H_ERA63(void);
void H_INIT(void);
void H_MAKE_HALF(int, char *, char *);
void H_PRINT3(int, int, char *, char *, char *, char *, char *, char *, int);
void H_SCROLL(int, int, int, int, int);
void LDIRL(VIDEO *, VIDEO *, int);
void TXRASCOPY(int, int, int);
