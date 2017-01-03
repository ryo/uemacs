/*
 * $Id: kanji.c,v 1.24 2017/01/02 15:17:50 ryo Exp $
 */
#include <stdio.h>
#include <string.h>

#ifdef ICONV
#include <iconv.h>
#endif

#include "estruct.h"
#include "etype.h"
#include "kanji.h"

static int nindex(char *, int, char *);
static unsigned short sjis_hira2kata(unsigned short);
static int check_utf8(char *, unsigned int);

int
chkana(unsigned char ch)
{
	if (ch < 0xa0)
		return 0;
	if (ch < 0xe0)
		return 1;
	return 0;
}



int
chctype(unsigned char ch)
{
	if (ch < 0x80)
		return CT_ANK;

	if (ch < 0xa0)
		return CT_KJ1;

	if (ch < 0xe0)
		return CT_ANK;

	return CT_KJ1;
}


int
nthctype(unsigned char *str, int nth)
{
	int i;
	unsigned char c;
	int nctype = CT_ILGL;

	for (i = 0; i <= nth; i++) {
		c = *str++;
		if (c == '\0') {
			return CT_ILGL;
		}

		if (nctype == CT_KJ1)
			nctype = CT_KJ2;
		else
			nctype = chctype(c);
	}

	return nctype;
}

int
nptrctype(unsigned char *str, unsigned char *ptr)
{
	return nthctype(str, ptr - str);
}

unsigned short
sjis2euc(unsigned short code)
{
	unsigned char h, l;

	h = (code >> 8) & 0xff;
	l = code & 0xff;

	/* GAIJI or RESERVED */
	if (h > 0xf0)
		return 0xa2a8;

	if (h < 0xa0)
		h -= 0x71;
	else
		h -= 0xb1;

	h = (h<<1) + 1;

	if (l > 0x9e) {
		l -= 0x7e;
		h += 1;
	} else if (l >= 0x80) {
		l -= 0x20;
	} else {
		l -= 0x1f;
	}

	h |= 0x80;
	l |= 0x80;

	return (h << 8) + l;
}


unsigned short
euc2sjis(unsigned short code)
{
	unsigned char h, l;

	h = code >> 8;
	if (h == 0x8e) {
		return code & 0xff;
	}
	if (h == 0x8f) {
		return 0x81A6; /* 3byte EUC (3th KANJI) */
	}

	h &= 0x7f;
	l = code & 0x7f;

	if (h & 1)
		l += 0x1f;
	else
		l += 0x7d;

	if (l >= 0x7f)
		l += 1;

	h = ((h - 0x21)>>1) + 0x81;
	if (h >= 0xa0)
		h += 0x40;

	return (h << 8) + l;
}


unsigned short
euc2jis(unsigned short code)
{
	return code & 0x7f7f;
}


unsigned short
jis2euc(unsigned short code)
{
	return code | 0x8080;
}


unsigned short
sjis2jis(unsigned short code)
{
	return euc2jis(sjis2euc(code));
}


unsigned short
jis2sjis(unsigned short code)
{
	return euc2sjis(jis2euc(code));
}

/*
 * convert INTERNAL code to some KANJI code
 */
unsigned short
kanji_convcode(int code, unsigned char c1, unsigned char c2)
{
	unsigned short c;

	c = (c1 << 8) + c2;

	switch (code) {
	case KANJI_JIS:
		if (c1 == 0 && chkana(c2)) {
			c = c2 & 0x7f;
		} else {
			c = sjis2jis(c);
		}
		break;
	case KANJI_SJIS:
		/* nothing to do */
		break;
	case KANJI_EUC:
		if (c1 == 0 && chkana(c2))
			c = 0x8e00 + c2;
		else
			c = sjis2euc(c);
		break;
	case KANJI_ASCII:
		/* 8bit through. not convert */
	default:
		c = (c1 << 8) + c2;
		break;
	}
	return c;
}


static int
nindex(char *str, int len, char *key)
{
	int i, ii;
	int l = strlen(key);

	for (i = 0; i < len; i++) {
		for (ii = 0; ii < l; ii++) {
			if (str[i + ii] != key[ii])
				break;

			if (ii + 1 == l)
				return i;
		}
	}
	return -1;
}


static int
check_utf8(char *p, unsigned int left)
{
	unsigned char c;
	int i;
	int nseq;
	int is_utf8;

	if (left == 0)
		return 0;

	is_utf8 = 0;

	while (left != 0) {
		c = *p++;
		left--;

		if ((c & 0x80) == 0) {			/* 7bit */
			continue;
		} else if ((c & 0xe0) == 0xc0) {	/* 11bit */
			nseq = 1;
		} else if ((c & 0xf0) == 0xe0) {	/* 16bit */
			nseq = 2;
		} else if ((c & 0xf8) == 0xf0) {	/* 21bit */
			nseq = 3;
		} else if ((c & 0xfc) == 0xf8) {	/* 26bit */
			nseq = 4;
		} else if ((c & 0xfe) == 0xfc) {	/* 31bit */
			nseq = 5;
		} else {
			return 0;	/* illegal sequence */
		}

		if (left < nseq)
			return 0;
		for (i = 0; i < nseq; i++) {
			if ((p[i] & 0xc0) != 0x80)
				return 0;
		}
		p += nseq;
		left -= nseq;

		is_utf8 = 1;
	}

	return is_utf8;
}


unsigned int
kanji_test(char *str, unsigned int len, int hint)
{
	int i;
	unsigned char *p;
	unsigned char ch, nch;
	int flag;

	if (hint == KANJI_EUC)
		return hint;

	if ((hint == KANJI_UTF8) ||
	    (hint == KANJI_UTF16BE) ||
	    (hint == KANJI_UTF16LE))
		return hint;

	if (check_utf8(str, len)) {
		hint = KANJI_UTF8;
		return hint;
	}

	for (i = 0, flag = 0, p = (unsigned char *)str; i < len; i++) {
		if (*p++ & 0x80) {
			flag = 1;
			break;
		}
	}

	if (flag) {
		/* EUC or SJIS ? */
		if (hint == KANJI_SJIS)
			return KANJI_SJIS;

		for (i = 0; i < len; i++) {
			ch = str[i];
			nch = (i < len) ? str[i + 1] : 0x00;
			if (0x80 <= ch && ch <= 0x9f && ch != 0x8e && ch != 0x8f) {
				return KANJI_SJIS;
			}

			if (ch == 0x8e) {
				if ((nch < 0xa1) || (0xe0 < nch)) {
					return KANJI_SJIS;
				}
			}
		}
		return KANJI_EUC;

	} else {
		/* JIS or ASCII ? */
		if (nindex(str, len, "\e$") >= 0 ||
		    nindex(str, len, "\e(") >= 0 ||
		    nindex(str, len, "\e)") >= 0 ||
		    nindex(str, len, "\e*") >= 0 ||
		    nindex(str, len, "\e+") >= 0 ||
		    nindex(str, len, "\e,") >= 0 ||
		    nindex(str, len, "\e-") >= 0 ||
		    nindex(str, len, "\e.") >= 0 ||
		    nindex(str, len, "\e/") >= 0) {

			return KANJI_JIS;
		}
	}

	if (hint)
		return hint;

	return KANJI_ASCII;
}



/*
 * convert from any code to internal (SJIS) code
 */
int
kanji_convert(int kanjicode, unsigned char *str, unsigned int len)
{
	int i, wi;
	unsigned short c;

	switch (kanjicode) {
	default:
	case KANJI_ASCII:
		/* noting */
		break;
	case KANJI_JIS:
		/* not yet */
		break;
	case KANJI_SJIS:
		/* nothing */
		break;
	case KANJI_EUC:
		for (i = 0, wi = 0; i < len; i++, wi++) {
			if (str[i] >= 0x80) {
				if (i + 1 >= len) {
					/* XXX */
					str[wi] = '$';
				} else {
					c = euc2sjis(str[i] * 256 + str[i + 1]);
					i++;
					if (c >= 0x100) {
						str[wi]   = (c >> 8) & 0xff;
						str[wi + 1] = c & 0xff;
						wi++;
					} else {
						str[wi] = c;
					}
				}
			} else {
				str[wi] = str[i];
			}
		}
		return wi;
#ifdef ICONV
	case KANJI_UTF8:
	case KANJI_UTF16BE:
	case KANJI_UTF16LE:
		{
			iconv_t cd;
			char *tmpbuf;
			char const *psrc;
			char *pdst;
			size_t srclen, dstlen;

			psrc = (char *)str;
			srclen = len;
			pdst = tmpbuf = MALLOC(len * 2);
			dstlen = len * 2;

			switch (kanjicode) {
			case KANJI_UTF8:
				cd = iconv_open("CP932", "UTF-8");
				break;
			case KANJI_UTF16BE:
				cd = iconv_open("CP932", "UTF-16BE");
				break;
			case KANJI_UTF16LE:
				cd = iconv_open("CP932", "UTF-16LE");
				break;
			}

			iconv(cd, &psrc, &srclen, &pdst, &dstlen);
			*pdst = '\0';
			iconv_close(cd);

			memcpy(str, tmpbuf, strlen(tmpbuf));
			len = strlen(tmpbuf);
			str[len] = '\0';

			FREE(tmpbuf);
		}
		break;
#endif /* ICONV */
	}

	return len;
}


int
kanji1st(int kanjicode, unsigned char c)
{
	switch (kanjicode) {
	case KANJI_EUC:
		if (c & 0x80)
			return 1;
		break;
	case KANJI_SJIS:
		if (c >= 0xe0)
			return 1;
		if (c >= 0xa0)
			return 0;
		if (c >= 0x80)
			return 1;
		break;
	default:
		break;
	}

	return 0;
}




static unsigned short sjis_han2zen_map[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

	0x8140, 0x8149, 0x8168, 0x8194, 0x8190, 0x8193, 0x8195, 0x8166,
	0x8169, 0x816A, 0x8196, 0x817B, 0x8143, 0x817C, 0x8144, 0x815E,

	0x824F, 0x8250, 0x8251, 0x8252, 0x8253, 0x8254, 0x8255, 0x8256,
	0x8257, 0x8258, 0x8146, 0x8147, 0x8183, 0x8181, 0x8184, 0x8148,

	0x8197, 0x8260, 0x8261, 0x8262, 0x8263, 0x8264, 0x8265, 0x8266,
	0x8267, 0x8268, 0x8269, 0x826A, 0x826B, 0x826C, 0x826D, 0x826E,

	0x826F, 0x8270, 0x8271, 0x8272, 0x8273, 0x8274, 0x8275, 0x8276,
	0x8277, 0x8278, 0x8279, 0x816D, 0x815F, 0x816E, 0x814F, 0x8151,

	0x8165, 0x8281, 0x8282, 0x8283, 0x8284, 0x8285, 0x8286, 0x8287,
	0x8288, 0x8289, 0x828A, 0x828B, 0x828C, 0x828D, 0x828E, 0x828F,

	0x8290, 0x8291, 0x8292, 0x8293, 0x8294, 0x8295, 0x8296, 0x8297,
	0x8298, 0x8299, 0x829A, 0x816F, 0x8162, 0x8170, 0x8160, 0x0000,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

	0x8140, 0x8142, 0x8175, 0x8176, 0x8141, 0x8145, 0x8392, 0x8340,
	0x8342, 0x8344, 0x8346, 0x8348, 0x8383, 0x8385, 0x8387, 0x8362,

	0x815B, 0x8341, 0x8343, 0x8345, 0x8347, 0x8349, 0x834A, 0x834C,
	0x834E, 0x8350, 0x8352, 0x8354, 0x8356, 0x8358, 0x835A, 0x835C,

	0x835E, 0x8360, 0x8363, 0x8365, 0x8367, 0x8369, 0x836A, 0x836B,
	0x836C, 0x836D, 0x836E, 0x8371, 0x8374, 0x8377, 0x837A, 0x837D,

	0x837E, 0x8380, 0x8381, 0x8382, 0x8384, 0x8386, 0x8388, 0x8389,
	0x838A, 0x838B, 0x838C, 0x838D, 0x838F, 0x8393, 0x814A, 0x814B,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

struct _sjis_zen2han_dakuten_map {
	unsigned short sjiscode;
	unsigned char han1;
	unsigned char han2;
};

struct _sjis_zen2han_dakuten_map sjis_zen2han_dakuten_map[] = {
	{	0x834b,	0xb6, 0xde	},	/* �� -> ���� */
	{	0x834d,	0xb7, 0xde	},	/* �� -> ���� */
	{	0x834f,	0xb8, 0xde	},	/* �� -> ���� */
	{	0x8351,	0xb9, 0xde	},	/* �� -> ���� */
	{	0x8353,	0xba, 0xde	},	/* �� -> ���� */
	{	0x8355,	0xbb, 0xde	},	/* �� -> ���� */
	{	0x8357,	0xbc, 0xde	},	/* �� -> ���� */
	{	0x8359,	0xbd, 0xde	},	/* �� -> ���� */
	{	0x835b,	0xbe, 0xde	},	/* �� -> ���� */
	{	0x835d,	0xbf, 0xde	},	/* �� -> ���� */
	{	0x835f,	0xc0, 0xde	},	/* �� -> ���� */
	{	0x8361,	0xc1, 0xde	},	/* �� -> ���� */
	{	0x8364,	0xc2, 0xde	},	/* �� -> �� */
	{	0x8366,	0xc3, 0xde	},	/* �� -> �Î� */
	{	0x8368,	0xc4, 0xde	},	/* �� -> �Ď� */
	{	0x836f,	0xca, 0xde	},	/* �� -> �ʎ� */
	{	0x8372,	0xcb, 0xde	},	/* �� -> �ˎ� */
	{	0x8375,	0xcc, 0xde	},	/* �� -> �̎� */
	{	0x8378,	0xcd, 0xde	},	/* �� -> �͎� */
	{	0x837b,	0xce, 0xde	},	/* �� -> �Ύ� */
	{	0x8370,	0xca, 0xdf	},	/* �� -> �ʎ� */
	{	0x8373,	0xcb, 0xdf	},	/* �� -> �ˎ� */
	{	0x8376,	0xcc, 0xdf	},	/* �� -> �̎� */
	{	0x8379,	0xcd, 0xdf	},	/* �� -> �͎� */
	{	0x837c,	0xce, 0xdf	},	/* �� -> �Ύ� */
	{	0x8394,	0xb3, 0xde	},	/* �� -> ���� */
	{	0x0000,	0x00, 0x00	}	/* TERMINATE! */
};


struct _sjis_hira2kata_map {
	unsigned short	hira;
	unsigned short	kata;
};

struct _sjis_hira2kata_map sjis_hira2kata_map[] = {
	{	0x829f, 0x8340	}, /* ������ */
	{	0x82a0, 0x8341	}, /* ������ */
	{	0x82a1, 0x8342	}, /* ������ */
	{	0x82a2, 0x8343	}, /* ������ */
	{	0x82a3, 0x8344	}, /* ������ */
	{	0x82a4, 0x8345	}, /* ������ */
	{	0x82a5, 0x8346	}, /* ������ */
	{	0x82a6, 0x8347	}, /* ������ */
	{	0x82a7, 0x8348	}, /* ������ */
	{	0x82a8, 0x8349	}, /* ������ */
	{	0x82a9, 0x834a	}, /* ������ */
	{	0x82aa, 0x834b	}, /* ������ */
	{	0x82ab, 0x834c	}, /* ������ */
	{	0x82ac, 0x834d	}, /* ������ */
	{	0x82ad, 0x834e	}, /* ������ */
	{	0x82ae, 0x834f	}, /* ������ */
	{	0x82af, 0x8350	}, /* ������ */
	{	0x82b0, 0x8351	}, /* ������ */
	{	0x82b1, 0x8352	}, /* ������ */
	{	0x82b2, 0x8353	}, /* ������ */
	{	0x82b3, 0x8354	}, /* ������ */
	{	0x82b4, 0x8355	}, /* ������ */
	{	0x82b5, 0x8356	}, /* ������ */
	{	0x82b6, 0x8357	}, /* ������ */
	{	0x82b7, 0x8358	}, /* ������ */
	{	0x82b8, 0x8359	}, /* ������ */
	{	0x82b9, 0x835a	}, /* ������ */
	{	0x82ba, 0x835b	}, /* ������ */
	{	0x82bb, 0x835c	}, /* ������ */
	{	0x82bc, 0x835d	}, /* ������ */
	{	0x82bd, 0x835e	}, /* ������ */
	{	0x82be, 0x835f	}, /* ������ */
	{	0x82bf, 0x8360	}, /* ������ */
	{	0x82c0, 0x8361	}, /* �¢��� */
	{	0x82c1, 0x8362	}, /* �â��� */
	{	0x82c2, 0x8363	}, /* �Ģ��� */
	{	0x82c3, 0x8364	}, /* �Ţ��� */
	{	0x82c4, 0x8365	}, /* �Ƣ��� */
	{	0x82c5, 0x8366	}, /* �Ǣ��� */
	{	0x82c6, 0x8367	}, /* �Ȣ��� */
	{	0x82c7, 0x8368	}, /* �ɢ��� */
	{	0x82c8, 0x8369	}, /* �ʢ��� */
	{	0x82c9, 0x836a	}, /* �ˢ��� */
	{	0x82ca, 0x836b	}, /* �̢��� */
	{	0x82cb, 0x836c	}, /* �͢��� */
	{	0x82cc, 0x836d	}, /* �΢��� */
	{	0x82cd, 0x836e	}, /* �Ϣ��� */
	{	0x82ce, 0x836f	}, /* �Т��� */
	{	0x82cf, 0x8370	}, /* �Ѣ��� */
	{	0x82d0, 0x8371	}, /* �Ң��� */
	{	0x82d1, 0x8372	}, /* �Ӣ��� */
	{	0x82d2, 0x8373	}, /* �Ԣ��� */
	{	0x82d3, 0x8374	}, /* �բ��� */
	{	0x82d4, 0x8375	}, /* �֢��� */
	{	0x82d5, 0x8376	}, /* �ע��� */
	{	0x82d6, 0x8377	}, /* �آ��� */
	{	0x82d7, 0x8378	}, /* �٢��� */
	{	0x82d8, 0x8379	}, /* �ڢ��� */
	{	0x82d9, 0x837a	}, /* �ۢ��� */
	{	0x82da, 0x837b	}, /* �ܢ��� */
	{	0x82db, 0x837c	}, /* �ݢ��� */
	{	0x82dc, 0x837d	}, /* �ޢ��� */
	{	0x82dd, 0x837e	}, /* �ߢ��� */
	{	0x82de, 0x8380	}, /* �ࢪ�� */
	{	0x82df, 0x8381	}, /* �ᢪ�� */
	{	0x82e0, 0x8382	}, /* �⢪�� */
	{	0x82e1, 0x8383	}, /* �㢪�� */
	{	0x82e2, 0x8384	}, /* �䢪�� */
	{	0x82e3, 0x8385	}, /* �墪�� */
	{	0x82e4, 0x8386	}, /* �梪�� */
	{	0x82e5, 0x8387	}, /* �碪�� */
	{	0x82e6, 0x8388	}, /* �袪�� */
	{	0x82e7, 0x8389	}, /* �颪�� */
	{	0x82e8, 0x838a	}, /* �ꢪ�� */
	{	0x82e9, 0x838b	}, /* �뢪�� */
	{	0x82ea, 0x838c	}, /* �좪�� */
	{	0x82eb, 0x838d	}, /* ���� */
	{	0x82ed, 0x838f	}, /* ��� */
	{	0x82f0, 0x8392	}, /* �򢪥� */
	{	0x82f1, 0x8393	}, /* �󢪥� */
	{	0x0000, 0x0000	}
};

static unsigned short
sjis_hira2kata(unsigned short sjis)
{
	int i;
	for (i = 0; sjis_hira2kata_map[i].hira; i++) {
		if (sjis_hira2kata_map[i].hira == sjis)
			return sjis_hira2kata_map[i].kata;
	}
	return sjis;
}


static unsigned short
table_zen2han(unsigned short code, unsigned short *table)
{
	int i;
	unsigned short ch = 0;
	for (i = 0x20; i <= 0xff; i++) {
		if (table[i] == code) {
			ch = i;
			break;
		}
	}
	return ch;
}


unsigned short
sjis_zen2han(unsigned short sjis)
{
	int i;
	unsigned short hc;

	sjis = sjis_hira2kata(sjis);
	hc = table_zen2han(sjis, sjis_han2zen_map);

	if (hc)
		return hc;

	for (i = 0; sjis_zen2han_dakuten_map[i].sjiscode; i++) {
		if (sjis_zen2han_dakuten_map[i].sjiscode == sjis) {
			return sjis_zen2han_dakuten_map[i].han1 * 256 + sjis_zen2han_dakuten_map[i].han2;
		}
	}

	return 0;
}

static unsigned short
table_han2zen(unsigned short c, unsigned short *table)
{
	unsigned short mb;

	mb = table[c];
	if (mb)
		return mb;

	return c;
}

unsigned short
sjis_han2zen(unsigned short c)
{
	return table_han2zen(c, sjis_han2zen_map);
}

