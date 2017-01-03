/*	EPATH:	This file contains certain info needed to locate the
		MicroEMACS files on a system dependant basis.

									*/

/*	possible names and paths of help files under different OSs	*/

CONST char *pathname[] =

#if	AMIGA
{
	".emacsrc",
	"emacs.hlp",
	"",
	"sys:c/",
	"sys:t/",
	"sys:s/",
	"c:/",
	":t/",
	":s/"
};
#endif

#if	ST520
{
	"emacs.rc",
	"emacs.hlp",
	"\\",
	"\\bin\\",
	"\\util\\",
	""
};
#endif
 
#if	FINDER
{
	"emacs.rc",
	"emacs.hlp",
	"/bin",
	"/sys/public",
	""
};
#endif

#if	MSDOS
{
	"emacs.rc",
	"emacs.hlp",
	"\\sys\\public\\",
	"\\usr\\bin\\",
	"\\bin\\",
	"\\",
	""
};
#endif

#if	V7 | BSD | USG | SMOS | HPUX | XENIX
{
	".uemacsrc",
	"uemacs.hlp",
	"/usr3/lib/",
	"/usr/local/lib/",
	""
};
#endif

#if	VMS
{
	"emacs.rc",
	"emacs.hlp",
	"MICROEMACS$LIB:",
	"SYS$LOGIN:",
	"",
	"sys$sysdevice:[vmstools]"
};
#endif

#if WMCS
{
	"emacs.rc",
	"emacs.hlp",
	"",
	"sys$disk/syslib.users/"
};
#endif

#if AOSVS
/*
    NOTE: you must use the Unix style pathnames here!
*/
{
    "emacs.rc",
    "emacs.hlp",
    "",
    "/macros/",
    "/help/"
};
#endif

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))