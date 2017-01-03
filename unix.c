/*
 * $Id: unix.c,v 1.50 2017/01/02 15:17:50 ryo Exp $
 *
 * unix.c: Operating specific I/O and Spawning functions under UNIX V7,
 * BSD4.2/3, System V, SUN OS and SCO XENIX for MicroEMACS 3.10
 * (C)opyright 1988 by Daniel M. Lawrence
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>		/* to get at the typeahead */
#include <sys/socket.h>
#include <errno.h>
#include <paths.h>

#include "estruct.h"
#include "etype.h"
#include "kanji.h"
#include "edef.h"
#include "elang.h"


#ifdef USE_SGTTY

#include <sgtty.h>	/* for stty/gtty functions */
struct sgttyb ostate;		/* saved tty state */
struct sgttyb nstate;		/* values for editor mode */
struct tchars otchars;		/* Saved terminal special character set */
struct tchars ntchars = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#else /* USE_SGTTY */

#include <sys/termios.h>
#ifdef __linux__
#elif defined(__CYGWIN__)
#else
#include <sys/filio.h>
#endif

struct termios ostate;		/* saved tty state */
struct termios nstate;		/* values for editor mode */

#endif /* USE_SGTTY */


static char *tilde_expand(char *);
static void ttputc_kanji(unsigned char);

#define	TBUFSIZ	128
char tobuf[TBUFSIZ];		/* terminal output buffer */



int
getwindow_lines(void)
{
	struct winsize winsz;
	if (ioctl(0, TIOCGWINSZ, &winsz) < 0)
		return 0;
	else {
		if (winsz.ws_row <= 2)
			return 0;
		else
			return winsz.ws_row - 1;
	}
}

int
getwindow_columns(void)
{
	struct winsize winsz;
	if (ioctl(0, TIOCGWINSZ, &winsz) < 0)
		return 0;
	else {
		if (winsz.ws_col <= 2)
			return 0;
		else
			return winsz.ws_col;
	}
}


/*
 * This function is called once to set up the terminal device streams.
 * On VMS, it translates TT until it finds the terminal, then assigns
 * a channel to it and sets it raw. On CPM it is a no-op.
 */
int
ttopen(void)
{
#ifdef USE_SGTTY
	ioctl(0, TIOCGETP, &ostate);	/* save old state */
	ioctl(0, TIOCGETP, &nstate);	/* get base of new state */
	nstate.sg_flags |= RAW;
	nstate.sg_flags &= ~(ECHO | CRMOD);	/* no echo for now... */
	ioctl(0, TIOCSETP, &nstate);	/* set mode */
	ioctl(0, TIOCGETC, &otchars);	/* Save old characters */
	ioctl(0, TIOCSETC, &ntchars);	/* Place new character into K */
#else
	tcgetattr(0, &ostate);
	tcgetattr(0, &nstate);

	nstate.c_iflag &= ~(IMAXBEL|IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	nstate.c_oflag &= ~OPOST;
	nstate.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	nstate.c_cflag &= ~(CSIZE|PARENB);
	nstate.c_cflag |= CS8;
	nstate.c_cc[VMIN] = 1;
	nstate.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &nstate);
#endif

	/*
	 * provide a smaller terminal output buffer so that the type ahead
	 * detection works better (more often)
	 */
	setbuffer(stdout, &tobuf[0], TBUFSIZ);

	/*
	 * on all screens we are not sure of the initial position of the
	 * cursor
	 */
	ttrow = 99;
	ttcol = 99;

	return TRUE;
}

/*
 * This function gets called just before we go back home to the command
 * interpreter.
 */
int
ttclose(void)
{
#ifdef USE_SGTTY
	ioctl(0, TIOCSETP, &ostate);
	ioctl(0, TIOCSETC, &otchars);	/* Place old character into K */
#else
	tcsetattr(0, TCSANOW, &ostate);
#endif

	return TRUE;
}

/*
 * Write a character to the display. On VMS, terminal output is buffered, and
 * we just put the characters in the big array, after checking for overflow.
 * On CPM terminal I/O unbuffered, so we just write the byte out. Ditto on
 * MS-DOS (use the very very raw console output routine).
 */
static void
ttputc_kanji(unsigned char c)
{
	static int current_set = -1;	/* for JIS */
	static int multibyte = 0;
	static unsigned char code1;

	if (multibyte) {
		/* 2nd byte */

		unsigned short output;

		output = kanji_convcode(dispkanji, code1, c);

		if (current_set != 0) {
			switch (dispkanji) {
			case KANJI_JIS:
				fputs("\e$B", stdout);	/* set G0 */
				break;
			default:
				break;
			}
			current_set = 0;
		}

		fputc(output >> 8  , stdout);
		fputc(output & 0xff, stdout);

		multibyte = 0;

	} else {
		if (chkana(c)) {
			/* hankaku kana */
			unsigned short output;

			if (current_set != 1) {
				switch (dispkanji) {
				case KANJI_JIS:
					fputs("\e(I", stdout);	/* set G1 */
					break;
				default:
					break;
				}
				current_set = 1;
			}

			output = kanji_convcode(dispkanji, 0, c);

			if (output >> 8)
				fputc(output >> 8  , stdout);	/* for EUC */
			fputc(output & 0xff, stdout);
		} else {
			if (c >= 0x80) {
				code1 = c;
				multibyte = 1;
			} else {
				if (current_set != -1) {
					switch (dispkanji) {
					case KANJI_JIS:
						fputs("\e(B", stdout);	/* unset G0/G1 */
						break;
					default:
						break;
					}
					current_set = -1;
				}
				fputc(c, stdout);
			}
		}
	}
}



int
ttputc(unsigned char c)
{
#if 1
	ttputc_kanji(c);
#else
	fputc(c, stdout);
#endif
	return TRUE;
}

/*
 * Flush terminal buffer. Does real work where the terminal output is buffered
 * up. A no-operation on systems where byte at a time terminal I/O is done.
 */
int
ttflush(void)
{
	fflush(stdout);
	return TRUE;
}

/*
 * TTGETC:	Read a character from the terminal, performing no editing and
 * doing no echo at all. More complex in VMS that almost anyplace else, which
 * figures. Very simple on CPM, because the system can do exactly what you
 * want.
 */

static char prereadbuf[1024];
static int prereadlen = 0;
static int prereadidx = 0;

int
tthaveinput(void)
{
	return prereadlen;
}

int
ttgetc(void)
{
	char c;

	if (prereadlen == 0) {
		prereadlen = read(0, prereadbuf, 1024);
		if (prereadlen <= 0) {
			/* tty is closed? */
			exit(5);
		}
		prereadidx = 0;
	}

	if (prereadlen) {
		c = prereadbuf[prereadidx++];

		if (prereadidx >= prereadlen) {
			prereadidx = 0;
			prereadlen = 0;
		}
	}
	return 255 & (int) c;
}


#if TYPEAH
/*
 * typahead:	Check to see if any characters are already in the keyboard
 * buffer
 */

int
typahead(void)
{
	int x;			/* holds # of pending chars */

	if ((x = tthaveinput()))
		return x;

	return (ioctl(0, FIONREAD, &x) < 0) ? 0 : x;
}
#endif

/*
 * Create a subjob with a copy of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint. Bound to "^X C". The message at the start in VMS puts out a newline.
 * Under some (unknown) condition, you don't get one free when DCL starts up.
 */
int
spawncli(int f, int n)
{
	char *cp;

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	movecursor(term.t_nrow, 0);	/* Seek to last line.   */
	TTflush();
	TTclose();		/* stty to old settings */
	if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
		system(cp);
	else
#if BSD
		system("exec /bin/csh");
#else
		system("exec /bin/sh");
#endif
	sgarbf = TRUE;
	/* sleep(2);	 */
	TTopen();
	return TRUE;
}

#if BSD
/* suspend MicroEMACS and wait to wake up */
int
bktoshell(int f, int n)
{
	vttidy();

	kill(0, SIGTSTP);

	TTopen();
	curwp->w_flag = WFHARD;
	sgarbf = TRUE;

	return TRUE;
}
#endif

/*
 * Run a one-liner in a subjob. When the command returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X !".
 */
int
spawn(int f, int n)
{
	int s;
	char line[NLINE];

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	if ((s = mlreply("!", line, NLINE)) != TRUE)
		return s;
	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	system(line);
	TTopen();
	TTflush();
	/* if we are interactive, pause here */
	if (clexec == FALSE) {
		mlputs(TEXT6);
		/* "\r\n\n[End]" */
		tgetc();
	}
	sgarbf = TRUE;
	return TRUE;
}

/*
 * Run an external program with arguments. When it returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X $".
 */
int
execprg(int f, int n)
{
	int s;
	char line[NLINE];

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	if ((s = mlreply("!", line, NLINE)) != TRUE)
		return s;
	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	system(line);
	TTopen();
	mlputs(TEXT188);	/* Pause. */
	/* "[End]" */
	TTflush();
	while ((s = tgetc()) != '\r' && s != ' ');
	sgarbf = TRUE;
	return TRUE;
}

int
nowaitexecprg(int f, int n)
{
	int s;
	int pid;
	char line[NLINE];
	int fd;
	char *argv[] = { "sh", "-c", line, 0 };
	extern char **environ;
	int status = 255;

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	if ((s = mlreply("!", line, NLINE)) != TRUE)
		return s;

	if ((pid = fork()) < 0) {
		return FALSE;
	} else if (pid == 0) {
		fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		execve("/bin/sh", argv, environ);
		_exit(1);
	} else {
		wait(&status);
	}
	if (status == 0)
		return TRUE;
	return TRUE;	/* XXX */
}

static int
pclose2(FILE *fh)
{
	int status = 255;

	fclose(fh);
	wait(&status);

	if (status == 0)
		return TRUE;
	return FALSE;
}

static FILE *
popen2(const char *command, const char *type)
{
	FILE *iop;
	const char * volatile xtype = type;
	int pdes[2], pid, serrno;
	volatile int twoway;

	if (strchr(xtype, '+')) {
		twoway = 1;
		type = "r+";
		if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pdes) < 0)
			return NULL;
	} else  {
		twoway = 0;
		if ((*xtype != 'r' && *xtype != 'w') || xtype[1] ||
		    (pipe(pdes) < 0)) {
			errno = EINVAL;
			return NULL;
		}
	}

	switch (pid = fork()) {
	case -1:			/* Error. */
		serrno = errno;
		close(pdes[0]);
		close(pdes[1]);
		errno = serrno;
		return NULL;
		/* NOTREACHED */
	case 0:				/* Child. */
		if (*xtype == 'r') {
			(void)close(pdes[0]);
			if (pdes[1] != STDOUT_FILENO) {
				(void)dup2(pdes[1], STDOUT_FILENO);
				(void)dup2(pdes[1], STDERR_FILENO);
				(void)close(pdes[1]);
			}
			if (twoway)
				(void)dup2(STDOUT_FILENO, STDIN_FILENO);
		} else {
			(void)close(pdes[1]);
			if (pdes[0] != STDIN_FILENO) {
				(void)dup2(pdes[0], STDIN_FILENO);
				(void)dup2(pdes[0], STDERR_FILENO);
				(void)close(pdes[0]);
			}
		}

		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		_exit(127);
		/* NOTREACHED */
	}

	/* Parent; assume fdopen can't fail. */
	if (*xtype == 'r') {
		iop = fdopen(pdes[0], xtype);
		close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], xtype);
		close(pdes[0]);
	}

	return iop;
}

int
wcom(int f, int n)
{
	int s;
	char line[NLINE];
	FILE *pfp;
	unsigned int nline = 0;
	int kcode = 0;
	int len;

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	if ((s = mlreply("!", line, NLINE)) != TRUE)
		return s;

	pfp = popen2(line, "r");
	while (1) {
		char buf[NSTRING];
		char *p = fgets(buf, NSTRING - 1, pfp);
		if (!p)
			break;

		len = strlen(buf);
		kcode = kanji_test(buf, len, kcode);
		kanji_convert(kcode, (unsigned char *)buf, len);

		if (len) {
			if (buf[len - 1] == '\n') {
				chomp(buf);
				linstr(buf);
				lnewline();
				nline++;
			} else {
				linstr(buf);
			}
		} else {
			lnewline();
			nline++;
		}

		if ((nline < 10) || !(nline % 10)) {
			update(TRUE);
		}
	}
	pclose2(pfp);

	ttopen();

	return TRUE;
}



/*
 * Pipe a one line command into a window
 * Bound to ^X @
 */
int
pipecmd(int f, int n)
{
	int s;			/* return status from CLI */
	WINDOW *wp;		/* pointer to new window */
	BUFFER *bp;		/* pointer to buffer to zot */
	char line[NLINE];	/* command line send to shell */
	static char bname[] = "command";

	static char filnam[NFILEN] = "command";

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	/* get the command to pipe in */
	if ((s = mlreply("@", line, NLINE)) != TRUE)
		return s;

	/* get rid of the command output buffer if it exists */
	if ((bp = bfind(bname, FALSE, 0)) != FALSE) {
		/* try to make sure we are off screen */
		wp = wheadp;
		while (wp != NULL) {
			if (wp->w_bufp == bp) {
				onlywind(FALSE, 1);
				break;
			}
			wp = wp->w_wndp;
		}
		if (zotbuf(bp) != TRUE)
			return FALSE;
	}
	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	strcat(line, ">");
	strcat(line, filnam);
	system(line);
	TTopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;

	if (s != TRUE)
		return s;

	/* split the current window to make room for the command output */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and read the stuff in */
	if (getfile(filnam, FALSE) == FALSE)
		return FALSE;

	/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW;
	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}

	/* and get rid of the temporary file */
	unlink(filnam);
	return TRUE;
}


/*
 * filter a buffer through an external DOS program
 * Bound to ^X #
 */
int
filter(int f, int n)
{
	int s;			/* return status from CLI */
	BUFFER *bp;		/* pointer to buffer to zot */
	char line[NLINE];	/* command line send to shell */
	char tmpname[NFILEN];	/* place to store real file name */
	static char bname1[] = "fltinp";
	static char filnam1[] = "fltinp";
	static char filnam2[] = "fltout";
	unsigned int mtime_save;

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

	if (!checkmodify())
		return FALSE;

	/* get the filter name and its args */
	if ((s = mlreply("#", line, NLINE)) != TRUE) {
		return s;
	}

	/* setup the proper file names */
	bp = curbp;
	strcpy(tmpname, bp->b_fname);	/* save the original name */
	strcpy(bp->b_fname, bname1);	/* set it to our new one */

	mtime_save = bp->b_mtime;

	/* write it out, checking for errors */
	if (writeout(filnam1) != TRUE) {
		mlwrite(TEXT2);
		/* "[Cannot write filter file]" */
		strcpy(bp->b_fname, tmpname);
		return FALSE;
	}
//	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	strcat(line, " <fltinp >fltout");
	system(line);
	TTopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;

	/* on failure, escape gracefully */
	if (s != TRUE || (readin(filnam2, FALSE) == FALSE)) {
		mlwrite(TEXT3);
		/* "[Execution failed]" */
		strcpy(bp->b_fname, tmpname);
		unlink(filnam1);
		unlink(filnam2);
		return s;
	}
	/* reset file name */
	strcpy(bp->b_fname, tmpname);	/* restore name */
	bp->b_flag |= BFCHG;	/* flag it as changed */
	bp->b_mtime = mtime_save;

	/* and get rid of the temporary file */
	unlink(filnam1);
	unlink(filnam2);
	return TRUE;
}

/* return a system dependant string with the current time */

char *
timeset(void)
{
	char *sp;		/* temp string pointer */
	time_t buf;

	time(&buf);
	sp = ctime(&buf);
	sp[strlen(sp) - 1] = 0;
	return sp;
}

#if COMPLET
/* FILE Directory routines		 */

DIR *dirptr = NULL;		/* pointer to the current directory being
				 * searched */

char home[NFILEN];
char path[NFILEN];		/* path of file to find */
char rbuf[NFILEN];		/* return file buffer */
char *nameptr;			/* ptr past end of path in rbuf */

/* do a wild card directory search (for file name completion) */

char *
getffile(char *fspec)	/* pattern to match */
{
	int idx;		/* idx into various strings */
	int point;		/* idx into other strings */
//	int extflag;		/* does the file have an extention? */

	/* first parse the file path off the file spec */
	strcpy(path, fspec);
	idx = strlen(path) - 1;
	while (idx >= 0 && (path[idx] != '/'))
		--idx;

	path[idx + 1] = 0;

	/* check for an extension */
	point = strlen(fspec) - 1;
//	extflag = FALSE;
	while (point >= 0) {
		if (fspec[point] == '.') {
//			extflag = TRUE;
			break;
		}
		point--;
	}

	/* open the directory pointer */
	if (dirptr != NULL) {
		closedir(dirptr);
		dirptr = NULL;
	}
	if (path[0] == '\0') {
		dirptr = opendir(".");
	} else {
		if (path[0] == '~' && path[1] == '/') {
			char tmp[NFILEN];
			strcpy(tmp, home);
			strcat(tmp, path + 1);
			dirptr = opendir(tmp);
		} else {
			dirptr = opendir(path);
		}
	}

	if (dirptr == NULL)
		return NULL;

	strcpy(rbuf, path);

	nameptr = &rbuf[strlen(rbuf)];

	/* and call for the first file */
	return getnfile(NULL);
}


static
char *
tilde_expand(char *name)
{
	static char tmp[NFILEN];

	if (name[0] == '~' && name[1] == '/') {
		strcpy(tmp, home);
		strcat(tmp, name + 1);
		return tmp;
	} else {
		return name;
	}
}

int
isdir(char *file)
{
	struct stat statbuf;

	stat(tilde_expand(file), &statbuf);
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		return 1;
	else
		return 0;
}

int
is_writable(char *file)
{
	if (access(tilde_expand(file), W_OK) == 0)
		return 1;
	else
		return 0;
}



int
filecopy(char *src, char *dst)
{
	int sfd = -1;
	int dfd = -1;
	int len;
	char *p = 0;
	int anyerror = 0;
	struct stat statbuf;

#define READ_BLOCK_SIZE	1024 * 64
	do {
		if ((sfd = open(src, O_RDONLY)) < 0) {
			anyerror++;
			break;
		}

		stat(src, &statbuf);

		if ((dfd = open(dst, O_WRONLY | O_CREAT, statbuf.st_mode & 0777)) < 0) {
			anyerror++;
			break;
		}

		if (ftruncate(dfd, 0) < 0) {
			anyerror++;
			break;
		}

		if (!(p = (char*)MALLOC(READ_BLOCK_SIZE))) {
			anyerror++;
			break;
		}

		while (1) {
			if ((len = read(sfd, p, READ_BLOCK_SIZE)) < 0) {
				anyerror++;
				goto filecopy_exit;
			}

			if (write(dfd, p, len) < 0) {
				anyerror++;
				goto filecopy_exit;
			}
			if (len == 0)
				break;
		}
	}while(0);

filecopy_exit:
	if (p)
		FREE(p);

	if (sfd >= 0)
		if (close(sfd)<0)
			anyerror++;

	if (dfd >= 0)
		if (close(dfd)<0)
			anyerror++;

	if (anyerror)
		return -1;

	return 0;
}


int
write_inode_think(char *realname, char *tname)
{
	int rc;
	char backname[NSTRING];

	if (makbak) {
		strcpy(backname, realname);
		strcat(backname, "~");
		filecopy(realname, backname);
	}
	rc = filecopy(tname, realname);
	if (rc == 0)
		rc = unlink(tname);

	return rc;
}


char *
getnfile(char *dummy)
{
	struct dirent *dp;	/* directory entry pointer */
	struct stat statbuf;

	/* and call for the next file */
nxtdir:
	dp = readdir(dirptr);
	if (dp == NULL)
		return NULL;

	if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
		goto nxtdir;



	/* check to make sure we skip directory entries */
	strcpy(nameptr, dp->d_name);

	if (rbuf[0] == '~' && rbuf[1] == '/') {
		char tmp[NFILEN];
		strcpy(tmp, home);
		strcat(tmp, rbuf + 1);
		stat(tmp, &statbuf);
	} else {
		stat(rbuf, &statbuf);
	}

	if ((statbuf.st_mode & S_IFMT) != S_IFREG &&
	    (statbuf.st_mode & S_IFMT) != S_IFDIR) {
		goto nxtdir;
	}
	/* return the next file name! */
	return rbuf;
}
#endif



