/*
 * $Id: kanji.h,v 1.11 2009/11/18 07:57:28 ryo Exp $
 */


#define	BOM_UTF8	"\357\273\277"
#define	BOM_UTF16BE	"\376\377"
#define	BOM_UTF16LE	"\377\376"

enum {
	KANJI_ASCII = 0,
	KANJI_JIS,
	KANJI_EUC,
	KANJI_SJIS,
	KANJI_UTF8,
	KANJI_UTF16LE,
	KANJI_UTF16BE
};

#define	CT_ANK	0
#define	CT_KJ1	1
#define	CT_KJ2	2
#define	CT_ILGL	-1

unsigned short kanji_convcode(int, unsigned char, unsigned char);
unsigned int kanji_test(char *, unsigned int, int);
int kanji_convert(int, unsigned char *, unsigned int);

unsigned short euc2sjis(unsigned short);
unsigned short sjis2euc(unsigned short);
unsigned short sjis2jis(unsigned short);
unsigned short euc2jis(unsigned short);
unsigned short jis2euc(unsigned short);
unsigned short jis2sjis(unsigned short);

unsigned short sjis_zen2han(unsigned short);
unsigned short euc_zen2han(unsigned short);
unsigned short sjis_han2zen(unsigned short);
unsigned short euc_han2zen(unsigned short);

int kanji1st(int, unsigned char);
