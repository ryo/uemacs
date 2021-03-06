/*
 * $Id: ebind.h,v 1.9 2009/11/18 07:21:21 ryo Exp $
 *
 * EBIND:		Initial default key to function bindings for
 * MicroEMACS 3.10
 */

/*
 * Command table.
 * This table  is *roughly* in ASCII order, left to right across the
 * characters of the command. This explains the funny location of the
 * control-X commands.
 */
KEYTAB  keytab[NBINDS] = {
	{CTRLBIT | '@',		BINDFNC,	{ setmark }	},
	{CTRLBIT | 'A',		BINDFNC,	{ gotobol }	},
	{CTRLBIT | 'B',		BINDFNC,	{ backchar }	},
	{CTRLBIT | 'C',		BINDFNC,	{ cec }		},
	{CTRLBIT | 'D',		BINDFNC,	{ forwdel }	},
	{CTRLBIT | 'E',		BINDFNC,	{ gotoeol }	},
	{CTRLBIT | 'F',		BINDFNC,	{ forwchar }	},
	{CTRLBIT | 'G',		BINDFNC,	{ ctrlg }	},
	{CTRLBIT | 'H',		BINDFNC,	{ backdel }	},
	{CTRLBIT | 'I',		BINDFNC,	{ tab }		},
	{CTRLBIT | 'J',		BINDFNC,	{ indent }	},
	{CTRLBIT | 'K',		BINDFNC,	{ killtext }	},
	{CTRLBIT | 'L',		BINDFNC,	{ refresh }	},
	{CTRLBIT | 'M',		BINDFNC,	{ newline }	},
	{CTRLBIT | 'N',		BINDFNC,	{ forwline }	},
	{CTRLBIT | 'O',		BINDFNC,	{ openline }	},
	{CTRLBIT | 'P',		BINDFNC,	{ backline }	},
	{CTRLBIT | 'Q',		BINDFNC,	{ quote }	},
	{CTRLBIT | 'R',		BINDFNC,	{ backsearch }	},
	{CTRLBIT | 'S',		BINDFNC,	{ forwsearch }	},
	{CTRLBIT | 'T',		BINDFNC,	{ twiddle }	},
	{CTRLBIT | 'U',		BINDFNC,	{ unarg }	},
	{CTRLBIT | 'V',		BINDFNC,	{ forwpage }	},
	{CTRLBIT | 'W',		BINDFNC,	{ killregion }	},
	{CTRLBIT | 'X',		BINDFNC,	{ cex }		},
	{CTRLBIT | 'Y',		BINDFNC,	{ yank }	},
	{CTRLBIT | 'Z',		BINDFNC,	{ bktoshell }	},
	{CTRLBIT | '[',		BINDFNC,	{ meta }	},

	{CTLX | CTRLBIT | 'B',	BINDFNC,	{ listbuffers }	},
	{CTLX | CTRLBIT | 'C',	BINDFNC,	{ quit }	},
	{CTLX | CTRLBIT | 'D',	BINDFNC,	{ detab }	},
	{CTLX | CTRLBIT | 'E',	BINDFNC,	{ entab }	},
	{CTLX | CTRLBIT | 'F',	BINDFNC,	{ filefind }	},
	{CTLX | CTRLBIT | 'I',	BINDFNC,	{ insfile }	},
	{CTLX | CTRLBIT | 'K',	BINDFNC,	{ macrotokey }	},
	{CTLX | CTRLBIT | 'L',	BINDFNC,	{ lowerregion }	},
	{CTLX | CTRLBIT | 'M',	BINDFNC,	{ delmode }	},
	{CTLX | CTRLBIT | 'N',	BINDFNC,	{ mvdnwind }	},
	{CTLX | CTRLBIT | 'O',	BINDFNC,	{ deblank }	},
	{CTLX | CTRLBIT | 'P',	BINDFNC,	{ mvupwind }	},
	{CTLX | CTRLBIT | 'R',	BINDFNC,	{ fileread }	},
	{CTLX | CTRLBIT | 'S',	BINDFNC,	{ filesave }	},
	{CTLX | CTRLBIT | 'T',	BINDFNC,	{ trim }	},
	{CTLX | CTRLBIT | 'U',	BINDFNC,	{ upperregion }	},
	{CTLX | CTRLBIT | 'V',	BINDFNC,	{ viewfile }	},
	{CTLX | CTRLBIT | 'W',	BINDFNC,	{ filewrite }	},
	{CTLX | CTRLBIT | 'X',	BINDFNC,	{ swapmark }	},
	{CTLX | CTRLBIT | 'Z',	BINDFNC,	{ shrinkwind }	},

	{CTLX | '?',		BINDFNC,	{ deskey }	},
	{CTLX | '!',		BINDFNC,	{ spawn }	},
	{CTLX | '@',		BINDFNC,	{ pipecmd }	},
	{CTLX | '#',		BINDFNC,	{ filter }	},
	{CTLX | '$',		BINDFNC,	{ execprg }	},
	{CTLX | '=',		BINDFNC,	{ showcpos }	},
	{CTLX | '(',		BINDFNC,	{ ctlxlp }	},
	{CTLX | ')',		BINDFNC,	{ ctlxrp }	},
	{CTLX | '<',		BINDFNC,	{ narrow }	},
	{CTLX | '>',		BINDFNC,	{ widen }	},
	{CTLX | '^',		BINDFNC,	{ enlargewind }	},
	{CTLX | ' ',		BINDFNC,	{ remmark }	},
	{CTLX | '0',		BINDFNC,	{ delwind }	},
	{CTLX | '1',		BINDFNC,	{ onlywind }	},
	{CTLX | '2',		BINDFNC,	{ splitwind }	},
	{CTLX | 'A',		BINDFNC,	{ setvar }	},
	{CTLX | 'B',		BINDFNC,	{ usebuffer }	},
	{CTLX | 'C',		BINDFNC,	{ bktoshell }	},
	{CTLX | 'D',		BINDFNC,	{ dispvar }	},
	{CTLX | 'E',		BINDFNC,	{ ctlxe }	},
	{CTLX | 'F',		BINDFNC,	{ setfillcol }	},
	{CTLX | 'K',		BINDFNC,	{ killbuffer }	},
	{CTLX | 'M',		BINDFNC,	{ setmod }	},
	{CTLX | 'N',		BINDFNC,	{ filename }	},
	{CTLX | 'O',		BINDFNC,	{ nextwind }	},
	{CTLX | 'P',		BINDFNC,	{ prevwind }	},
	{CTLX | 'R',		BINDFNC,	{ risearch }	},
	{CTLX | 'S',		BINDFNC,	{ fisearch }	},
	{CTLX | 'W',		BINDFNC,	{ resize }	},
	{CTLX | 'X',		BINDFNC,	{ nextbuffer }	},
	{CTLX | 'Z',		BINDFNC,	{ enlargewind }	},
	{META | CTRLBIT | 'C',	BINDFNC,	{ wordcount }	},
	{META | CTRLBIT | 'E',	BINDFNC,	{ execproc }	},
	{META | CTRLBIT | 'F',	BINDFNC,	{ getfence }	},
	{META | CTRLBIT | 'G',	BINDFNC,	{ gotomark }	},
	{META | CTRLBIT | 'H',	BINDFNC,	{ delbword }	},
	{META | CTRLBIT | 'K',	BINDFNC,	{ unbindkey }	},
	{META | CTRLBIT | 'L',	BINDFNC,	{ reposition }	},
	{META | CTRLBIT | 'M',	BINDFNC,	{ delgmode }	},
	{META | CTRLBIT | 'N',	BINDFNC,	{ namebuffer }	},
	{META | CTRLBIT | 'R',	BINDFNC,	{ qreplace }	},
	{META | CTRLBIT | 'S',	BINDFNC,	{ execfile }	},
	{META | CTRLBIT | 'V',	BINDFNC,	{ nextdown }	},
	{META | CTRLBIT | 'W',	BINDFNC,	{ killpara }	},
	{META | CTRLBIT | 'X',	BINDFNC,	{ execcmd }	},
	{META | CTRLBIT | 'Z',	BINDFNC,	{ nextup }	},
	{META | ' ',		BINDFNC,	{ setmark }	},
	{META | '?',		BINDFNC,	{ help }	},
	{META | '!',		BINDFNC,	{ reposition }	},
	{META | '.',		BINDFNC,	{ setmark }	},
	{META | '>',		BINDFNC,	{ gotoeob }	},
	{META | '<',		BINDFNC,	{ gotobob }	},
	{META | '~',		BINDFNC,	{ unmark }	},
	{META | 'A',		BINDFNC,	{ apro }	},
	{META | 'B',		BINDFNC,	{ backword }	},
	{META | 'C',		BINDFNC,	{ capword }	},
	{META | 'D',		BINDFNC,	{ delfword }	},
	{META | 'E',		BINDFNC,	{ setekey }	},
	{META | 'F',		BINDFNC,	{ forwword }	},
	{META | 'G',		BINDFNC,	{ gotoline }	},
	{META | 'K',		BINDFNC,	{ bindtokey }	},
	{META | 'L',		BINDFNC,	{ lowerword }	},
	{META | 'M',		BINDFNC,	{ setgmode }	},
	{META | 'N',		BINDFNC,	{ gotoeop }	},
	{META | 'P',		BINDFNC,	{ gotobop }	},
	{META | 'Q',		BINDFNC,	{ fillpara }	},
	{META | 'R',		BINDFNC,	{ sreplace }	},
	{META | 'S',		BINDFNC,	{ bktoshell }	},
	{META | 'U',		BINDFNC,	{ upperword }	},
	{META | 'V',		BINDFNC,	{ backpage }	},
	{META | 'W',		BINDFNC,	{ copyregion }	},
	{META | 'X',		BINDFNC,	{ namedcmd }	},
	{META | 'Z',		BINDFNC,	{ quickexit }	},
	{META | 0x7F,		BINDFNC,	{ delbword }	},

	{ALTD | 'S',		BINDFNC,	{ forwhunt }	},
	{ALTD | 'R',		BINDFNC,	{ backhunt }	},
	{SPEC | '<',		BINDFNC,	{ gotobob }	},
	{SPEC | 'P',		BINDFNC,	{ backline }	},
	{SPEC | 'Z',		BINDFNC,	{ backpage }	},
	{SPEC | 'B',		BINDFNC,	{ backchar }	},
	{SPEC | 'F',		BINDFNC,	{ forwchar }	},
	{SPEC | '>',		BINDFNC,	{ gotoeob }	},
	{SPEC | 'N',		BINDFNC,	{ forwline }	},
	{SPEC | 'V',		BINDFNC,	{ forwpage }	},
	{SPEC | 'C',		BINDFNC,	{ insspace }	},
	{SPEC | 'D',		BINDFNC,	{ forwdel }	},
	{SPEC | CTRLBIT | 'B',	BINDFNC,	{ backword }	},
	{SPEC | CTRLBIT | 'F',	BINDFNC,	{ forwword }	},
	{SPEC | CTRLBIT | 'Z',	BINDFNC,	{ gotobop }	},
	{SPEC | CTRLBIT | 'V',	BINDFNC,	{ gotoeop }	},

	{0x7F,			BINDFNC,	{ backdel }	},

	{0,			BINDNUL,	{ NULL }	}
};
