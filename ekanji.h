/*
----------------------------------------
	KANJI.H: MicroEMACS 3.10
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/ekanji.h,v 1.3 1991/09/02 11:56:22 SALT Exp $
----------------------------------------
*/

#ifndef CT_ANK
#define CT_ANK	0
#define CT_KJ1	1
#define CT_KJ2	2
#define CT_ILGL	(-1)
#endif

#define ANK 	0
#define KANJI	1
#define KANJI1	1
#define KANJI2	2
#define SPC 	3
#define BREAK	4

char *jstrmatch(char *, char *);
char *jstrchr(char *, int);
char *jstrrev(char *);
int countbyte(int);
int hantozen(int);
int isalnmkana(int);
int jstrlen(char *);
int nthctype(char *, int);
int zentohan(int);
