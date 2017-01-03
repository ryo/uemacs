/*
 * $Id: file.c,v 1.43 2017/01/02 15:17:50 ryo Exp $
 *
 * file.c: for MicroEMACS
 *
 * The routines in this file handle the reading, writing and lookup of disk
 * files.  All of details about the reading and writing of the disk are in
 * "fileio.c".
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "kanji.h"





/*
 * Read a file into the current
 * buffer. This is really easy; all you do is
 * find the name of the file, and call the standard
 * "read a file into the current buffer" code.
 * Bound to "C-X C-R".
 */
int
fileread(int f, int n)	/* defualt and numeric arguments (unused) */
{
	char *fname;		/* file name to read */

	if (restflag)		/* don't allow this command if restricted */
		return resterr();

	if ((fname = gtfilename(TEXT131)) == NULL)
		/* "Read file" */
		return FALSE;
	return readin(fname, TRUE);
}

/*
 * Insert a file into the current
 * buffer. This is really easy; all you do it
 * find the name of the file, and call the standard
 * "insert a file into the current buffer" code.
 * Bound to "C - X C - I".
 */
int
insfile(int f, int n)
{
	char *fname;		/* file name */

	if (restflag)		/* don't allow this command if restricted */
		return resterr();
	if (!checkmodify())
		return FALSE;

	if ((fname = gtfilename(TEXT132)) == NULL)
		/* "Insert file" */
		return FALSE;
	return ifile(fname);
}

/*
 * Select a file for editing.
 * Look around to see if you can find the
 * fine in another buffer; if you can find it
 * just switch to the buffer. If you cannot find
 * the file, create a new buffer, read in the
 * text, and switch to the new buffer.
 * Bound to C - X C - F.
 */
int
filefind(int f, int n)
{
	char *fname;		/* file user wishes to find file name */

	if (restflag)		/* don't allow this command if restricted */
		return resterr();

	if ((fname = gtfilename(TEXT133)) == NULL)
		/* "Find file" */
		return FALSE;

	if (ifsplit)
		splitwind(FALSE, 0);

	return getfile(fname, TRUE);
}


int
viewfile(int f, int n)
{				/* visit a file in VIEW mode */
	char *fname;		/* file user wishes to find file name */
	int s;			/* status return */

	if (restflag)		/* don't allow this command if restricted */
		return resterr();

	if ((fname = gtfilename(TEXT134)) == NULL)
		/* "View file" */
		return FALSE;
	s = getfile(fname, FALSE);
	if (s) {		/* if we succeed, put it in view mode */
		curwp->w_bufp->b_mode |= MDVIEW;
		upmode();
	}
	return s;
}

#if CRYPT
int
resetkey(void)
{				/* reset the encryption key if needed */
	int s;			/* return status */

	/* turn off the encryption flag */
	cryptflag = FALSE;

	/* if we are in crypt mode */
	if (curbp->b_mode & MDCRYPT) {
		if (curbp->b_key[0] == 0) {
			s = setekey(FALSE, 0);
			if (s != TRUE)
				return s;
		}
		/* let others know... */
		cryptflag = TRUE;

		/* and set up the key to be used! */
		/* de-encrypt it */
		p_crypt((char *) NULL, 0);
		p_crypt(curbp->b_key, strlen(curbp->b_key));

		/* re-encrypt it...seeding it to start */
		p_crypt((char *) NULL, 0);
		p_crypt(curbp->b_key, strlen(curbp->b_key));
	}
	return TRUE;
}
#endif

/*
 *  fname: name of file to read
 *  lockfl: check for file locks?
 */
int
getfile(char *fname, int lockfl)
{
	BUFFER *bp;
	LINE *lp;
	int i;
	int s;
	int cmark;		/* current mark */
	char bname[NBUFN];	/* buffer name to put file */

#if MSDOS
	mklower(fname);		/* msdos isn't case sensitive */
#endif
	for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
		if ((bp->b_flag & BFINVS) == 0 && strcmp(bp->b_fname, fname) == 0) {
			swbuffer(bp);
			lp = curwp->w_dotp;
			i = curwp->w_ntrows / 2;
			while (i-- && lback(lp) != curbp->b_linep)
				lp = lback(lp);
			curwp->w_linep = lp;
			curwp->w_flag |= WFMODE | WFHARD;
			mlwrite(TEXT135);
			/* "[Old buffer]" */
			return TRUE;
		}
	}
	makename(bname, fname);	/* New buffer name.	 */
	while ((bp = bfind(bname, FALSE, 0)) != NULL) {
		/* old buffer name conflict code */
		s = mlreply(TEXT136, bname, NBUFN);
		/* "Buffer name: " */
		if (s == ABORT)	/* ^G to just quit	 */
			return s;
		if (s == FALSE) {	/* CR to clobber it	 */
			makename(bname, fname);
			break;
		}
		unqname(bname);
	}
	if (bp == NULL && (bp = bfind(bname, TRUE, 0)) == NULL) {
		mlwrite(TEXT137);
		/* "Cannot create buffer" */
		return FALSE;
	}
	if (--curbp->b_nwnd == 0) {	/* Undisplay.		 */
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		for (cmark = 0; cmark < NMARKS; cmark++) {
			curbp->b_markp[cmark] = curwp->w_markp[cmark];
			curbp->b_marko[cmark] = curwp->w_marko[cmark];
		}
		curbp->b_fcol = curwp->w_fcol;
	}
	curbp = bp;		/* Switch to it.	 */
	curwp->w_bufp = bp;
	curbp->b_nwnd++;

	if (readin(fname, lockfl)) {	/* Read it in.		 */
		exechook(postreadhook);
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
 * Read file "fname" into the current buffer, blowing away any text
 * found there.  Called by both the read and find commands.  Return
 * the final status of the read.  Also called by the mainline, to
 * read in a file specified on the command line as an argument.
 * The command in $readhook is called after the buffer is set up
 * and before it is read.
 *
 *  fname: name of file to read
 *  lockfl: check for file locks?
 */
int
readin(char *fname, int lockfl)
{
	LINE *lp;
	LINE *lp1;
	LINE *lp2;
	int i;
	WINDOW *wp;
	BUFFER *bp;
	int s;
	int nbytes;
	int nline;
	int cmark;		/* current mark */
	char mesg[NSTRING];
	struct stat statbuf;
	int crlf = 0;

	bp = curbp;			/* Cheap.		 */
	if ((s = bclear(bp)) != TRUE)	/* Might be old.	 */
		return s;
	bp->b_flag &= ~(BFINVS | BFCHG);

	strcpy(bp->b_fname, fname);

	/* let a user macro get hold of things...if he wants */
	exechook(readhook);

#if CRYPT
	/* set up for decryption */
	s = resetkey();
	if (s != TRUE)
		return s;
#endif

	/* turn off ALL keyboard translation in case we get a dos error */
	TTkclose();

	if ((s = ffropen(fname)) == FIOERR)	/* Hard file open.	 */
		goto out;


	curbp->b_mtime = 0;

	if (s == FIOFNF) {	/* File not found.	 */
		mlwrite(TEXT138);
		/* "[New file]" */
		goto out;
	}

	stat(fname, &statbuf);
	curbp->b_mtime = statbuf.st_mtime;

	if (S_ISDIR(statbuf.st_mode)) {
		DIR *d;
		struct dirent *dp;
		char *tmp_linebuf;
		struct stat tmp_statbuf;
		int pathlen = strlen(fname);
		int need_slash = 0;

		d = opendir(fname);

		if (fname[pathlen - 1] != '/')
			need_slash = 1;

		tmp_linebuf = MALLOC(pathlen + NSTRING);

		while ((dp = readdir(d))) {
			if (strcmp(".", dp->d_name) == 0)
				continue;

			strncpy(tmp_linebuf, fname, pathlen);
			tmp_linebuf[pathlen] = '\0';
			if (need_slash) {
				strcat(tmp_linebuf, "/");
			}
			strcat(tmp_linebuf, dp->d_name);
			stat(tmp_linebuf, &tmp_statbuf);

			strcat(tmp_linebuf, ":1:");

			i = strlen(tmp_linebuf);
			if (i < 8)
				strcat(tmp_linebuf,"\t");
			if (i < 16)
				strcat(tmp_linebuf,"\t");
			if (i < 24)
				strcat(tmp_linebuf,"\t");
			if (i >= 24)
				strcat(tmp_linebuf," ");

			i = strlen(tmp_linebuf);

#if 0
			{
				char *flag = "";
				struct tm *tmp_tm;

				if (S_ISDIR(tmp_statbuf.st_mode))  flag = "/";
				if (S_ISLNK(tmp_statbuf.st_mode))  flag = "@";
				if (S_ISFIFO(tmp_statbuf.st_mode)) flag = "|";
				if (S_ISSOCK(tmp_statbuf.st_mode)) flag = "=";

				tmp_tm = localtime(&tmp_statbuf.st_mtime);

				sprintf(&tmp_linebuf[i],
				        "%04d-%02d-%02d %02d:%02d:%02d %10llu %s%s",
				        tmp_tm->tm_year + 1900,
				        tmp_tm->tm_mon + 1,
				        tmp_tm->tm_mday,
				        tmp_tm->tm_hour,
				        tmp_tm->tm_min,
				        tmp_tm->tm_sec,
				        tmp_statbuf.st_size,
				        dp->d_name, flag);
			}
#endif

			nbytes = strlen(tmp_linebuf);
			if ((lp1 = lalloc(nbytes)) == NULL) {
				s = FIOMEM;	/* Keep message on the display */
				break;
			}

			bytecopy((char *)lp1->l_text, tmp_linebuf, nbytes);


			lp2 = lback(curbp->b_linep);
			lp2->l_fp = lp1;
			lp1->l_fp = curbp->b_linep;
			lp1->l_bp = lp2;
			curbp->b_linep->l_bp = lp1;

			++nline;
		}
		FREE(tmp_linebuf);


		closedir(d);
		ffclose();
	} else {
		/* read the file in */
		mlwrite(TEXT139);
		/* "[Reading file]" */
		nline = 0;
		while ((nbytes = ffgetline()) >= 0) {
			if (nbytes && fline[nbytes - 1] == '\r') {
				crlf = 1;
				nbytes -= 1;
			}

			curbp->b_kanjicode = kanji_test(fline, nbytes, curbp->b_kanjicode);

			if ((lp1 = lalloc(nbytes)) == NULL) {
				s = FIOMEM;	/* Keep message on the	 */
				break;	/* display.		 */
			}
			lp2 = lback(curbp->b_linep);
			lp2->l_fp = lp1;
			lp1->l_fp = curbp->b_linep;
			lp1->l_bp = lp2;
			curbp->b_linep->l_bp = lp1;
			for (i = 0; i < nbytes; ++i)
				lputc(lp1, i, fline[i]);
			++nline;
		}
		ffclose();		/* Ignore errors.	 */
		strcpy(mesg, "[");
		if (s == FIOERR) {
			strcat(mesg, TEXT141);
			/* "I/O ERROR, " */
			curbp->b_flag |= BFTRUNC;
		}
		if (s == FIOMEM) {
			strcat(mesg, TEXT142);
			/* "OUT OF MEMORY, " */
			curbp->b_flag |= BFTRUNC;
		}
		strcat(mesg, TEXT140);
		/* "Read " */
		strcat(mesg, int_asc(nline));
		strcat(mesg, TEXT143);
		/* " line" */
		if (nline > 1)
			strcat(mesg, "s");
		strcat(mesg, "]");
		mlwrite(mesg);
	}

	if (!is_writable(fname) || S_ISDIR(statbuf.st_mode)) {
		bp->b_mode |= MDVIEW;
	}

	if (crlf)
		bp->b_mode |= MDCRLF;


	/* bom check */
	lp = lforw(curbp->b_linep);
	if (strncmp(BOM_UTF8, (char *)lp->l_text, 3) == 0) {
		curbp->b_kanjicode = KANJI_UTF8;
		lp->l_used -= 3;
		memcpy(lp->l_text, lp->l_text + 3, lp->l_used);
		lp->l_text[lp->l_used] = '\0';
		curbp->b_bom = TRUE;
	} else if (strncmp(BOM_UTF16BE, (char *)lp->l_text, 2) == 0) {
		curbp->b_kanjicode = KANJI_UTF16BE;
		lp->l_used -= 2;
		memcpy(lp->l_text, lp->l_text + 2, lp->l_used);
		lp->l_text[lp->l_used] = '\0';
		curbp->b_bom = TRUE;
	} else if (strncmp(BOM_UTF16LE, (char *)lp->l_text, 2) == 0) {
		curbp->b_kanjicode = KANJI_UTF16LE;
		lp->l_used -= 2;
		memcpy(lp->l_text, lp->l_text + 2, lp->l_used);
		lp->l_text[lp->l_used] = '\0';
		curbp->b_bom = TRUE;
	}


	lp = lforw(curbp->b_linep);
	while (lp != curbp->b_linep) {
		lp->l_used = kanji_convert(curbp->b_kanjicode, lp->l_text, lp->l_used);
		lp = lforw(lp);
	}


out:
	TTkopen();		/* open the keyboard again */

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_linep = lforw(curbp->b_linep);
			wp->w_dotp = lforw(curbp->b_linep);
			wp->w_doto = 0;
			for (cmark = 0; cmark < NMARKS; cmark++) {
				wp->w_markp[cmark] = NULL;
				wp->w_marko[cmark] = 0;
			}
			wp->w_flag |= WFMODE | WFHARD;
		}
	}
	if (s == FIOERR || s == FIOFNF)	/* False if error.	 */
		return FALSE;

	return TRUE;
}

/*
 * Take a file name, and from it
 * fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system.
 * I suppose that this information could be put in
 * a better place than a line of code.
 * Returns a pointer into fname indicating the end of the file path; i.e.,
 * 1 character BEYOND the path name.
 */
char *
makename(char *bname, char *fname)
{
	char *cp1;
	char *cp2;
	char *pathp;

	cp1 = &fname[0];
	while (*cp1 != 0)
		++cp1;

#if MSDOS
	while (cp1 != &fname[0] && cp1[-1] != ':' && cp1[-1] != '\\' && cp1[-1] != '/')
		--cp1;
#endif
#if BSD
	while (cp1 != &fname[0] && cp1[-1] != '/')
		--cp1;
#endif
	/* cp1 is pointing to the first real filename char */
	pathp = cp1;

	cp2 = &bname[0];
	while (cp2 != &bname[NBUFN - 1] && *cp1 != 0 && *cp1 != ';')
		*cp2++ = *cp1++;
	*cp2 = 0;
	return pathp;
}

/* make sure a buffer name is unique */
int
unqname(char *name)	/* name to check on */
{
	char *sp;

	/* check to see if it is in the buffer list */
	while (bfind(name, 0, FALSE) != NULL) {

		/* go to the end of the name */
		sp = name;
		while (*sp)
			++sp;
		if (sp == name || (*(sp - 1) < '0' || *(sp - 1) > '8')) {
			*sp++ = '0';
			*sp = 0;
		} else
			*(--sp) += 1;
	}
	return TRUE;
}

/*
 * Ask for a file name, and write the
 * contents of the current buffer to that file.
 * Update the remembered file name and clear the
 * buffer changed flag. This handling of file names
 * is different from the earlier versions, and
 * is more compatable with Gosling EMACS than
 * with ITS EMACS. Bound to "C-X C-W".
 */
int
filewrite(int f, int n)
{
	int s;
	char fname[NFILEN];

	if (restflag)		/* don't allow this command if restricted */
		return resterr();

	if ((s = mlreply(TEXT144, fname, NFILEN)) != TRUE)
		/* "Write file: " */
		return s;

	if ((s = writeout(fname)) == TRUE) {
		strcpy(curbp->b_fname, fname);
		curbp->b_flag &= ~BFCHG;
		/* Update mode lines.	 */
		upmode();
	}
	return s;
}

/*
 * Save the contents of the current
 * buffer in its associatd file. Do nothing
 * if nothing has changed (this may be a bug, not a
 * feature). Error if there is no remembered file
 * name for the buffer. Bound to "C-X C-S". May
 * get called by "C-Z".
 */
int
filesave(int f, int n)
{
	struct stat statbuf;
	int s;

	if (!checkmodify())
		return FALSE;

	if ((curbp->b_flag & BFCHG) == 0)	/* Return, no changes.	 */
		return TRUE;

	if (curbp->b_fname[0] == 0) {	/* Must have a name.	 */
		mlwrite(TEXT145);
		/* "No file name" */
		return FALSE;
	}

	/* complain about truncated files */
	if ((curbp->b_flag & BFTRUNC) != 0) {
		if (mlyesno(TEXT146) == FALSE) {
			/* "Truncated file..write it out" */
			mlwrite(TEXT8);
			/* "[Aborted]" */
			return FALSE;
		}
	}

	if (stat(curbp->b_fname, &statbuf)==0) {
		if (curbp->b_mtime != statbuf.st_mtime) {
			if (mlyesno(TEXT218) == FALSE) {
				/* "File has changed since visited or saved. Really save ?" */
				mlwrite(TEXT8);
				/* "[Aborted]" */
				return FALSE;
			}
		}
	}

	/* complain about narrowed buffers */
	if ((curbp->b_flag & BFNAROW) != 0) {
		if (mlyesno(TEXT147) == FALSE) {
			/* "Narrowed Buffer..write it out" */
			mlwrite(TEXT8);
			/* "[Aborted]" */
			return FALSE;
		}
	}

	if ((s = writeout(curbp->b_fname)) == TRUE) {
		curbp->b_flag &= ~BFCHG;
		/* Update mode lines.	 */
		upmode();
	}
	return s;
}

/*
 * This function performs the details of file writing. It uses
 * the file management routines in the "fileio.c" package. The
 * number of lines written is displayed. Several errors are
 * posible, and cause writeout to return a FALSE result. When
 * $ssave is TRUE,  the buffer is written out to a temporary
 * file, and then the old file is unlinked and the temporary
 * renamed to the original name.  Before the file is written,
 * a user specifyable routine (in $writehook) can be run.
 */
int
writeout(char *fn)	/* name of file to write current buffer to */
{
	LINE *lp;		/* line to scan while writing */
	char *sp;		/* temporary string pointer */
	int nline;		/* number of lines written */
	int status;		/* return status */
	int sflag;		/* are we safe saving? */
	char tname[NFILEN];	/* temporary file name */
	char buf[NSTRING];	/* message buffer */
	char realname[NFILEN];
	struct stat statbuf;

	static char prevname[NSTRING];
	static int writeoutcnt = 0;

	strcpy(realname, fn);
	if (!lstat(realname, &statbuf)) {
		if ((statbuf.st_mode & S_IFLNK) == S_IFLNK) {
			char tmpname[NSTRING];
			char *p = rindex(realname, '/');
			int i;

			if (!p)
				p = realname;
			else
				p++;	/* '/' */

			i = readlink(realname, tmpname, NSTRING);
			if (i < 0)
				i = 0;
			tmpname[i]='\0';

			strcpy(p, tmpname);
		}
	}
	/* let a user macro get hold of things...if he wants */
	exechook(writehook);

	/* determine if we will use the save method */
	sflag = FALSE;

	if (ssave && fexist(realname)) {
		sflag = TRUE;

		if (!stat(realname, &statbuf)) {
			int a;
			a = statbuf.st_mode;
			if (!(a & S_IWUSR)) {
				if (!writeoutcnt) {
					mlwrite("You have no permission to write file");
					writeoutcnt++;
					return FALSE;
				} else {
					writeoutcnt = 0;
					chmod(realname, a | S_IWUSR);
					mlwrite("Force writing");
				}
			}
		}
	}
	writeoutcnt = 0;
	strcpy(prevname, realname);

#if CRYPT
	/* set up for file encryption */
	status = resetkey();
	if (status != TRUE)
		return status;
#endif

	/* turn off ALL keyboard translation in case we get a dos error */
	TTkclose();

	/* Perform Safe Save..... */
	if (sflag) {
		/* duplicate original file name, and find where to trunc it */
		sp = tname + (makename(tname, realname) - realname) + 1;
		strcpy(tname, realname);

		/* create a unique name, using random numbers */
		do {
			*sp = 0;
			strcat(tname, int_asc(ernd()));
		} while (fexist(tname));

		/* open the temporary file */
		status = ffwopen(tname);
	} else {
		if (makbak) {
			char backname[NSTRING];
			strcpy(backname, realname);
			strcat(backname, "~");
			mlwrite("[Writing backup file...]");
			filecopy(realname, backname);
			mlwrite("[Wrote backup file]");
		}
		status = ffwopen(realname);
	}

	/* if the open failed.. clean up and abort */
	if (status != FIOSUC) {
		TTkopen();
		return FALSE;
	}

	/* write the current buffer's lines to the open disk file */
	mlwrite(TEXT148);	/* tell us that we're writing */
	/* "[Writing...]" */
	lp = lforw(curbp->b_linep);	/* start at the first line. */
	nline = 0;		/* track the Number of lines */

	while (lp != curbp->b_linep) {
		int crlf = (curbp->b_mode & MDCRLF) ? 2 : 1;
		if (!curbp->b_eofreturn && (lforw(lp) == curbp->b_linep))
			crlf = 0;

		if ((status = ffputline(nline, curbp, (char *)&lp->l_text[0], llength(lp), crlf) != FIOSUC))
			break;
		++nline;

		if (!(nline & 0x7f))
			mlwrite("[Writing %d lines]", nline);

		lp = lforw(lp);
	}

	/* report on status of file write */
	*buf = 0;
	status |= ffclose();
	if (status == FIOSUC) {
		/* report on success (or lack therof) */
		strcpy(buf, TEXT149);
		/* "[Wrote " */
		strcat(buf, int_asc(nline));
		strcat(buf, TEXT143);
		/* " line" */
		if (nline > 1)
			strcat(buf, "s");

		if (sflag) {
			/* erase original file */
			/* rename temporary file to original name */

			if (!write_inode_think(realname, tname)) {

			} else {
				strcat(buf, TEXT150);
				/* ", saved as " */
				strcat(buf, tname);
				status = FIODEL;	/* failed */
			}
		}
		strcat(buf, "]");
		mlwrite(buf);
	}

	stat(realname, &statbuf);
	curbp->b_mtime = statbuf.st_mtime;

	exechook(postwritehook);

	/* reopen the keyboard, and return our status */
	TTkopen();
	return (status == FIOSUC);
}

/*
 * The command allows the user
 * to modify the file name associated with
 * the current buffer. It is like the "f" command
 * in UNIX "ed". The operation is simple; just zap
 * the name in the BUFFER structure, and mark the windows
 * as needing an update. You can type a blank line at the
 * prompt if you wish.
 */
int
filename(int f, int n)
{
	int s;
	char fname[NFILEN];

	if (restflag)		/* don't allow this command if restricted */
		return resterr();
	if ((s = mlreply(TEXT151, fname, NFILEN)) == ABORT)
		/* "Name: " */
		return s;
	if (s == FALSE)
		strcpy(curbp->b_fname, "");
	else
		strcpy(curbp->b_fname, fname);
	/* Update mode lines.	 */
	upmode();
	curbp->b_mode &= ~MDVIEW;	/* no longer read only mode */
	return TRUE;
}

/*
 * Insert file "fname" into the current
 * buffer, Called by insert file command. Return the final
 * status of the read.
 */
int
ifile(char *fname)
{
	LINE *lp0;
	LINE *lp1;
	LINE *lp2;
	int i;
	BUFFER *bp;
	int s;
	int nbytes;
	int nline;
	int cmark;		/* current mark */
	char mesg[NSTRING];

	bp = curbp;		/* Cheap.		 */

	/* first change ? */
	if ((bp->b_flag & BFCHG) == 0) {
		exechook(modifyhook);
	}
	bp->b_flag |= BFCHG;	/* we have changed	 */
	bp->b_flag &= ~BFINVS;	/* and are not temporary */
	if ((s = ffropen(fname)) == FIOERR)	/* Hard file open.	 */
		goto out;
	if (s == FIOFNF) {	/* File not found.	 */
		mlwrite(TEXT152);
		/* "[No such file]" */
		return FALSE;
	}
	mlwrite(TEXT153);
	/* "[Inserting file]" */

#if CRYPT
	s = resetkey();
	if (s != TRUE)
		return s;
#endif
	/* back up a line and save the mark here */
	curwp->w_dotp = lback(curwp->w_dotp);
	curwp->w_doto = 0;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		curwp->w_markp[cmark] = curwp->w_dotp;
		curwp->w_marko[cmark] = 0;
	}

	nline = 0;
	while ((nbytes = ffgetline()) >= 0) {
		if ((lp1 = lalloc(nbytes)) == NULL) {
			s = FIOMEM;	/* Keep message on the	 */
			break;	/* display.		 */
		}
		lp0 = curwp->w_dotp;	/* line previous to insert */
		lp2 = lp0->l_fp;/* line after insert */

		/* re-link new line between lp0 and lp2 */
		lp2->l_bp = lp1;
		lp0->l_fp = lp1;
		lp1->l_bp = lp0;
		lp1->l_fp = lp2;

		/* and advance and write out the current line */
		curwp->w_dotp = lp1;
		for (i = 0; i < nbytes; ++i)
			lputc(lp1, i, fline[i]);
		++nline;
	}
	ffclose();		/* Ignore errors.	 */
	curwp->w_markp[0] = lforw(curwp->w_markp[0]);
	strcpy(mesg, "[");
	if (s == FIOERR) {
		strcat(mesg, TEXT141);
		/* "I/O ERROR, " */
		curbp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, TEXT142);
		/* "OUT OF MEMORY, " */
		curbp->b_flag |= BFTRUNC;
	}
	strcat(mesg, TEXT154);
	/* "Inserted " */
	strcat(mesg, int_asc(nline));
	strcat(mesg, TEXT143);
	/* " line" */
	if (nline > 1)
		strcat(mesg, "s");
	strcat(mesg, "]");
	mlwrite(mesg);

out:
	/* advance to the next line and mark the window for changes */
	curwp->w_dotp = lforw(curwp->w_dotp);
	curwp->w_flag |= WFHARD | WFMODE;

	/* copy window parameters back to the buffer structure */
	curbp->b_dotp = curwp->w_dotp;
	curbp->b_doto = curwp->w_doto;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		curbp->b_markp[cmark] = curwp->w_markp[cmark];
		curbp->b_marko[cmark] = curwp->w_marko[cmark];
	}
	curbp->b_fcol = curwp->w_fcol;

	if (s == FIOERR)	/* False if error.	 */
		return FALSE;
	return TRUE;
}
