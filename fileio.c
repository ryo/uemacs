/*
 * $Id: fileio.c,v 1.23 2017/01/02 15:17:50 ryo Exp $
 *
 * fileio.c: Low level file i/o routines MicroEMACS 3.10
 *
 * The routines in this file read and write ASCII files from the disk. All of
 * the knowledge about files are here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ICONV
#include <iconv.h>
#endif

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "kanji.h"

void ffputc_kanji(unsigned char,int,FILE *);

FILE *ffp;		/* File pointer, all functions. */
static int eofflag;		/* end-of-file flag */

/*
 * Open a file for reading.
 */
int
ffropen(char *fn)
{
	if ((ffp = fopen(fn, "r")) == NULL)
		return FIOFNF;
	eofflag = FALSE;
	return FIOSUC;
}

/*
 * Open a file for writing. Return TRUE if all is well, and FALSE on error
 * (cannot create).
 */
int
ffwopen(char *fn)
{
	if ((ffp = fopen(fn, "w")) == NULL) {
		mlwrite(TEXT155);
		/* "Cannot open file for writing" */
		return FIOERR;
	}
	return FIOSUC;
}

/*
 * Close a file. Should look at the status in all systems.
 */
int
ffclose(void)
{
	/* free this since we do not need it anymore */
	if (fline) {
		FREE(fline);
		fline = NULL;
	}

	if (fclose(ffp) != FALSE) {
		mlwrite(TEXT156);
		/* "Error closing file" */
		return FIOERR;
	}
	return FIOSUC;
}


void
ffputc_kanji(unsigned char c, int kcode, FILE *fp)
{
	static int current_set = -1;	/* for JIS */
	static int multibyte = 0;
	static unsigned char code1;

	if (multibyte) {
		/* 2nd byte */

		unsigned short output;

		output = kanji_convcode(kcode,code1,c);

		if (current_set != 0) {
			switch (kcode) {
			case KANJI_JIS:
				fputs("\e$B",fp);	/* set G0 */
				break;
			default:
				break;
			}
			current_set = 0;
		}

		fputc(output >> 8  ,fp);
		fputc(output & 0xff,fp);

		multibyte = 0;

	} else {
		if (chkana(c)) {
			/* hankaku kana */
			unsigned short output;

			if (current_set != 1) {
				switch (kcode) {
				case KANJI_JIS:
					fputs("\e(I",fp);	/* set G1 */
					break;
				default:
					break;
				}
				current_set = 1;
			}

			output = kanji_convcode(kcode,0,c);

			if (output >> 8)
				fputc(output >> 8  ,fp);	/* for EUC */
			fputc(output & 0xff,fp);
		} else {
			if (c >= 0x80) {
				code1 = c;
				multibyte = 1;
			} else {
				if (current_set != -1) {
					switch (kcode) {
					case KANJI_JIS:
						fputs("\e(B",fp);	/* unset G0/G1 */
						break;
					default:
						break;
					}
					current_set = -1;
				}
				fputc(c, fp);
			}
		}
	}
}


/*
 * Write a line to the already opened file. The "buf" points to the buffer,
 * and the "nbuf" is its length, less the free newline. Return the status.
 * Check only at the newline.
 */
int
ffputline(int nline, BUFFER *bp, char *buf, int nbuf, int crlf)
{
	int i;

#if CRYPT
	char c;			/* character to translate */

	if (cryptflag) {
		for (i = 0; i < nbuf; ++i) {
			c = buf[i];
			p_crypt(&c, 1);
			putc(c, ffp);
		}
	} else {
#endif
		switch (bp->b_kanjicode) {
#ifdef ICONV
		case KANJI_UTF8:
		case KANJI_UTF16BE:
		case KANJI_UTF16LE:
			{
				iconv_t cd;
				char *tmpbuf;
				const char *psrc;
				char *pdst;
				size_t srclen, dstlen;

				if ((nline == 0) && (bp->b_bom)) {
					switch (bp->b_kanjicode) {
					case KANJI_UTF8:
						fwrite(BOM_UTF8, 3, 1, ffp);
						break;
					case KANJI_UTF16BE:
						fwrite(BOM_UTF16BE, 2, 1, ffp);
						break;
					case KANJI_UTF16LE:
						fwrite(BOM_UTF16LE, 2, 1, ffp);
						break;
					}
				}

				psrc = buf;
				srclen = nbuf;
				pdst = tmpbuf = MALLOC(nbuf * 4);
				dstlen = nbuf * 4;

				switch (bp->b_kanjicode) {
				case KANJI_UTF8:
					cd = iconv_open("UTF-8", "CP932");
					break;
				case KANJI_UTF16LE:
					cd = iconv_open("UTF-16LE", "CP932");
					break;
				case KANJI_UTF16BE:
					cd = iconv_open("UTF-16BE", "CP932");
					break;
				}

				iconv(cd, &psrc, &srclen, &pdst, &dstlen);
				*pdst = '\0';
				iconv_close(cd);

				fwrite(tmpbuf, pdst - tmpbuf, 1, ffp);

				FREE(tmpbuf);

			}
			break;
#endif /* ICONV */
		default:
			for (i = 0; i < nbuf; ++i)
				ffputc_kanji(buf[i], bp->b_kanjicode, ffp);
			break;
		}
#if CRYPT
	}
#endif

	switch (crlf) {
	case 2:
		putc('\r', ffp);
	default:
	case 1:
		putc('\n', ffp);
	case 0:
		break;
	}

	if (ferror(ffp)) {
		mlwrite(TEXT157);
		/* "Write I/O error" */
		return FIOERR;
	}
	return FIOSUC;
}


/*
 * Read a line from a file, and store the bytes in the supplied buffer. The
 * "nbuf" is the length of the buffer. Complain about long lines and lines
 * at the end of the file that don't have a newline present. Check for I/O
 * errors too. Return status.
 */
int
ffgetline(void)
{
	int c;			/* current character read */
	int i;			/* current index into fline */

	/* if we are at the end...return it */
	if (eofflag)
		return FIOEOF;

	/* dump fline if it ended up too big */
	if (flen > NSTRING && fline != NULL) {
		FREE(fline);
		fline = NULL;
	}
	/* if we don't have an fline, allocate one */
	if (fline == NULL)
		if ((fline = MALLOC(flen = NSTRING)) == NULL)
			return FIOMEM;

	/* read the line in */
	i = 0;

	while (((c = getc(ffp)) != EOF) && (c != '\n')) {
		fline[i++] = c;
		if (i >= flen) {
			fline = REALLOC(fline, flen + NSTRING);
			if (fline == NULL)
				return FIOMEM;
			flen += NSTRING;
		}
	}

	/* test for any errors that may have occured */
	if (c == EOF) {
		if (ferror(ffp)) {
			mlwrite(TEXT158);
			/* "File read error" */
			return FIOERR;
		}
		if (i != 0)
			eofflag = TRUE;
		else
			return FIOEOF;
	}
	/* terminate and decrypt the string */
	fline[i] = 0;
#if CRYPT
	if (cryptflag)
		p_crypt(fline, strlen(fline));
#endif
	return i;
}

/* does <fname> exist on disk? */
int
fexist(char *fname)	/* file to check for existance */

{
	FILE *fp;

	/* try to open the file for reading */
	fp = fopen(fname, "r");

	/* if it fails, just return false! */
	if (fp == NULL)
		return FALSE;

	/* otherwise, close it and report true */
	fclose(fp);
	return TRUE;
}

