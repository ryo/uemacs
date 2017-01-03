/*
 * $Id: epath.h,v 1.5 2005/04/08 22:15:29 ryo Exp $
 *
 * EPATH:	This file contains certain info needed to locate the
 * MicroEMACS files on a system dependant basis.
 *
 */

/* possible names and paths of help files under different OSs	 */

char *pathname[] = {
	".emacsrc",
	"emacs.hlp",
	"/usr/local/lib/uemacs",
	""
};

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))
