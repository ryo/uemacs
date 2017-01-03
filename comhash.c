/*
----------------------------------------
	COMHASH.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"

/*
========================================
	RCS id ‚ÌÝ’è
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/comhash.c,v 1.1 1992/01/31 12:14:06 SALT Exp SALT $'\n""	even\n");

/*
========================================
	ŠÖ”ƒe[ƒuƒ‹
========================================
*/

#define NHASH	128

static NBIND *command_head[NHASH];

NBIND command_table[] = {
	{NULL,	"abort-command",				ctrlg,					0},
	{NULL,	"add-global-mode",				setgmode,				0},
	{NULL,	"add-mode",						setmod,					0},
	{NULL,	"append-next-kill",				apendnext,				0},
	{NULL,	"apropos",						apro,					0},
	{NULL,	"autoload",						autoload,				0},
	{NULL,	"backward-character",			backchar,				0},
	{NULL,	"begin-macro",					ctlxlp,					0},
	{NULL,	"beginning-of-file",			gotobob,				0},
	{NULL,	"beginning-of-line",			gotobol,				0},
	{NULL,	"bell",							bell,					0},
	{NULL,	"bind-to-key",					bindtokey,				0},
	{NULL,	"bind-to-xf1-key",				bindtoxf1key,			0},
	{NULL,	"bind-to-xf2-key",				bindtoxf2key,			0},
	{NULL,	"bind-to-xf3-key",				bindtoxf3key,			0},
	{NULL,	"bind-to-xf4-key",				bindtoxf4key,			0},
	{NULL,	"bind-to-xf5-key",				bindtoxf5key,			0},
	{NULL,	"buffer-position",				showcpos,				0},
	{NULL,	"c-complete",					c_complete,				QVIEW},
	{NULL,	"c-delete-next-character",		c_forwdel,				QVIEW},
	{NULL,	"c-delete-previous-character",	c_backdel,				QVIEW},
	{NULL,	"c-insert-brace",				c_insbrace,				QVIEW},
	{NULL,	"c-insert-bracket",				c_insbracket,			QVIEW},
	{NULL,	"c-insert-paren",				c_insparen,				QVIEW},
	{NULL,	"c-insert-pound",				c_inspound,				QVIEW},
	{NULL,	"c-insert-tab",					c_instab,				QVIEW},
	{NULL,	"c-newline",					c_newline,				QVIEW | QMINUS},
	{NULL,	"caps-key-reverse",				capsreverse,			0},
	{NULL,	"case-region-lower",			lowerregion,			QVIEW},
	{NULL,	"case-region-upper",			upperregion,			QVIEW},
	{NULL,	"case-word-capitalize",			capword,				QVIEW | QMINUS},
	{NULL,	"case-word-lower",				lowerword,				QVIEW | QMINUS},
	{NULL,	"case-word-upper",				upperword,				QVIEW | QMINUS},
	{NULL,	"change-file-name",				filename,				QREST},
	{NULL,	"change-screen-size",			newsize,				0},
	{NULL,	"change-screen-width",			newwidth,				0},
	{NULL,	"clear-and-redraw",				refresh,				0},
	{NULL,	"clear-message-line",			clrmes,					0},
	{NULL,	"compare-buffer",				compareb,				0},
	{NULL,	"compare-line",					comparel,				0},
	{NULL,	"copy-line",					copyline,				QKILL},
	{NULL,	"copy-region",					copyregion,				QKILL},
	{NULL,	"count-words",					wordcount,				0},
	{NULL,	"ctrl-prefix",					cex,					0},
	{NULL,	"delete-blank-lines",			deblank,				QVIEW},
	{NULL,	"delete-buffer",				killbuffer,				0},
	{NULL,	"delete-global-mode",			delgmode,				0},
	{NULL,	"delete-kill-buffer",			delkbuf,				0},
	{NULL,	"delete-mode",					delmode,				0},
	{NULL,	"delete-next-character",		forwdel,				QVIEW},
	{NULL,	"delete-next-word",				delfword,				QVIEW | QMINUS | QKILL},
	{NULL,	"delete-other-windows",			onlywind,				0},
	{NULL,	"delete-previous-character",	backdel,				QVIEW},
	{NULL,	"delete-previous-word",			delbword,				QVIEW | QZMIN | QKILL},
	{NULL,	"delete-window",				delwind,				0},
	{NULL,	"describe-bindings",			desbind,				0},
	{NULL,	"describe-functions",			desfunc,				0},
	{NULL,	"describe-key",					deskey,					0},
	{NULL,	"describe-variables",			desvars,				0},
	{NULL,	"detab-line",					detabl,					QVIEW},
	{NULL,	"detab-region",					detabr,					QVIEW},
	{NULL,	"dired",						dired,					QREST},
	{NULL,	"dired-cancel-all",				do_dired_cancel_all,	QDIRED},
	{NULL,	"dired-cd",						do_dired_cd,			0},
	{NULL,	"dired-copy-file",				do_dired_cp,			QDIRED},
	{NULL,	"dired-do-deletions",			do_dired_rm,			QDIRED},
	{NULL,	"dired-drop-mark",				do_dired_drop_mark,		QDIRED},
	{NULL,	"dired-empty-trash",			do_empty_trash,			QDIRED},
	{NULL,	"dired-find-file",				find_dired_curfile,		QDIRED},
	{NULL,	"dired-find-files",				find_dired_files,		QDIRED},
	{NULL,	"dired-list-marks",				do_dired_list,			QDIRED},
	{NULL,	"dired-mark",					do_dired_mark,			QDIRED | QZMIN},
	{NULL,	"dired-mark-all",				do_dired_mark_all,		QDIRED},
	{NULL,	"dired-mark-back",				do_dired_mark_back,		QDIRED | QZMIN},
	{NULL,	"dired-mark-ext",				do_dired_mark_ext,		QDIRED},
	{NULL,	"dired-move-file",				do_dired_mv,			QDIRED},
	{NULL,	"dired-next-line",				do_dired_forwline,		QDIRED},
	{NULL,	"dired-popd",					popd,					0},
	{NULL,	"dired-previous-line",			do_dired_backline,		QDIRED},
	{NULL,	"dired-pushd",					pushd,					0},
	{NULL,	"dired-revert-buffer",			do_dired_rb,			QDIRED},
	{NULL,	"dired-trash",					do_dired_trash,			QDIRED},
	{NULL,	"dired-view-file",				view_dired_curfile,		QDIRED},
	{NULL,	"dired-view-files",				view_dired_files,		QDIRED},
	{NULL,	"display",						dispvar,				0},
	{NULL,	"dup-line",						dupline,				QVIEW | QZMIN},
	{NULL,	"end-macro",					ctlxrp,					0},
	{NULL,	"end-of-file",					gotoeob,				0},
	{NULL,	"end-of-line",					gotoeol,				0},
	{NULL,	"end-of-word",					endword,				0},
	{NULL,	"entab-line",					entabl,					QVIEW},
	{NULL,	"entab-region",					entabr,					QVIEW},
	{NULL,	"exchange-ctrl-prefix",			exgctrlprefix,			0},
	{NULL,	"exchange-point-and-mark",		swapmark,				0},
	{NULL,	"execute-buffer",				execbuf,				0},
	{NULL,	"execute-command-line",			execcmd,				0},
	{NULL,	"execute-file",					execfile,				0},
	{NULL,	"execute-macro",				ctlxe,					0},
	{NULL,	"execute-named-command",		namedcmd,				0},
	{NULL,	"execute-procedure",			execproc,				0},
	{NULL,	"execute-program",				execprg,				QREST},
	{NULL,	"exit-emacs",					quit,					0},
	{NULL,	"file-complete",				file_complete,			QVIEW},
	{NULL,	"fill-paragraph",				fillpara,				QVIEW},
	{NULL,	"filter-buffer",				filter,					QREST | QVIEW},
	{NULL,	"find-file",					filefind,				QREST},
	{NULL,	"forward-character",			forwchar,				0},
	{NULL,	"goto-line",					gotoline,				0},
	{NULL,	"goto-mark",					gotomark,				0},
	{NULL,	"goto-matching-fence",			getfence,				0},
	{NULL,	"goto-multimark-forward",		gotomultimarkf,			0},
	{NULL,	"goto-multimark-reverse",		gotomultimarkr,			0},
	{NULL,	"grow-window",					enlargewind,			0},
	{NULL,	"handle-tab",					tab,					QVIEW | QMINUS},
	{NULL,	"hankaku-word",					zentohanword,			QVIEW | QMINUS},
	{NULL,	"help",							help,					0},
	{NULL,	"hunt-backward",				backhunt,				0},
	{NULL,	"hunt-forward",					forwhunt,				0},
	{NULL,	"i-shell",						spawncli,				0},
	{NULL,	"incremental-search",			fisearch,				0},
	{NULL,	"insert-file",					insfile,				QREST | QVIEW},
	{NULL,	"insert-space",					insspace,				QVIEW},
	{NULL,	"insert-string",				istring,				QVIEW},
	{NULL,	"just-one-space",				justonespc,				QVIEW},
	{NULL,	"keyflush",						kflush,					0},
	{NULL,	"kill-line",					killline,				QVIEW | QKILL},
	{NULL,	"kill-paragraph",				killpara,				QVIEW},
	{NULL,	"kill-region",					killregion,				QVIEW | QKILL},
	{NULL,	"kill-to-end-of-line",			killtext,				QVIEW | QKILL},
	{NULL,	"label-function-key",			fnclabel,				0},
	{NULL,	"latex-abstract",				latex_abstruct,			QVIEW | QMINUS},
	{NULL,	"latex-array",					latex_array,			QVIEW | QMINUS},
	{NULL,	"latex-center",					latex_center,			QVIEW | QMINUS},
	{NULL,	"latex-chapter",				latex_chapter,			QVIEW},
	{NULL,	"latex-cite",					latex_cite,				QVIEW},
	{NULL,	"latex-close-block",			latex_closeblock,		QVIEW},
	{NULL,	"latex-complete",				latex_complete,			QVIEW},
	{NULL,	"latex-description",			latex_description,		QVIEW | QMINUS},
	{NULL,	"latex-document",				latex_document,			QVIEW},
	{NULL,	"latex-enumerate",				latex_enumerate,		QVIEW | QMINUS},
	{NULL,	"latex-eqnarray",				latex_eqnarray,			QVIEW | QMINUS},
	{NULL,	"latex-equation",				latex_equation,			QVIEW | QMINUS},
	{NULL,	"latex-example",				latex_example,			QVIEW | QMINUS},
	{NULL,	"latex-figure",					latex_figure,			QVIEW | QMINUS},
	{NULL,	"latex-footnote",				latex_footnote,			QVIEW},
	{NULL,	"latex-insert-begin",			latex_insbegin,			QVIEW},
	{NULL,	"latex-item",					latex_item,				QVIEW},
	{NULL,	"latex-itemize",				latex_itemize,			QVIEW | QMINUS},
	{NULL,	"latex-label",					latex_label,			QVIEW},
	{NULL,	"latex-make-environment",		latex_makeenv,			QVIEW},
	{NULL,	"latex-mathmode",				latex_mathmode,			QVIEW},
	{NULL,	"latex-minipage",				latex_minipage,			QVIEW | QMINUS},
	{NULL,	"latex-picture",				latex_picture,			QVIEW | QMINUS},
	{NULL,	"latex-quotation",				latex_quotation,		QVIEW | QMINUS},
	{NULL,	"latex-quote",					latex_quote,			QVIEW | QMINUS},
	{NULL,	"latex-ref",					latex_ref,				QVIEW},
	{NULL,	"latex-section",				latex_section,			QVIEW},
	{NULL,	"latex-sloppypar",				latex_sloppypar,		QVIEW | QMINUS},
	{NULL,	"latex-table",					latex_table,			QVIEW | QMINUS},
	{NULL,	"latex-tabular",				latex_tabular,			QVIEW | QMINUS},
	{NULL,	"latex-titlepage",				latex_titlepage,		QVIEW | QMINUS},
	{NULL,	"latex-verb",					latex_verb,				QVIEW},
	{NULL,	"latex-verbatim",				latex_verbatim,			QVIEW | QMINUS},
	{NULL,	"latex-verse",					latex_verse,			QVIEW | QMINUS},
	{NULL,	"list-buffers",					listbuffers,			0},
	{NULL,	"load-cfont",					loadcfont,				0},
	{NULL,	"load-cfont-half",				loadcfont_h,			0},
	{NULL,	"load-font",					loadfont,				0},
	{NULL,	"load-font-half",				loadfont_h,				0},
	{NULL,	"macro-to-key",					macrotokey,				0},
	{NULL,	"make-buffer-list",				makebuflist,			0},
	{NULL,	"meta-prefix",					meta,					0},
	{NULL,	"mouse-move-down",				movemdown,				0},
	{NULL,	"mouse-move-up",				movemup,				0},
	{NULL,	"mouse-region-down",			mregdown,				0},
	{NULL,	"mouse-region-up",				mregup,					0},
	{NULL,	"move-window-down",				mvdnwind,				0},
	{NULL,	"move-window-up",				mvupwind,				0},
	{NULL,	"name-buffer",					namebuffer,				0},
	{NULL,	"narrow-to-region",				narrow,					0},
	{NULL,	"newline",						newline,				QVIEW | QMINUS},
	{NULL,	"newline-and-indent",			indent,					QVIEW | QMINUS},
	{NULL,	"next-buffer",					nextbuffer,				0},
	{NULL,	"next-line",					forwline,				0},
	{NULL,	"next-page",					forwpage,				0},
	{NULL,	"next-paragraph",				gotoeop,				0},
	{NULL,	"next-window",					nextwind,				0},
	{NULL,	"next-word",					forwword,				0},
	{NULL,	"nop",							nullproc,				0},
	{NULL,	"open-line",					openline,				QVIEW | QMINUS | QZERO},
	{NULL,	"open-menu",					openmenu,				0},
	{NULL,	"overwrite-string",				ovstring,				QVIEW},
	{NULL,	"pipe-command",					pipecmd,				QVIEW | QREST},
	{NULL,	"previous-line",				backline,				0},
	{NULL,	"previous-page",				backpage,				0},
	{NULL,	"previous-paragraph",			gotobop,				0},
	{NULL,	"previous-window",				prevwind,				0},
	{NULL,	"previous-word",				backword,				0},
	{NULL,	"print",						writemsg,				0},
	{NULL,	"query-replace-string",			qreplace,				QVIEW},
	{NULL,	"quick-exit",					quickexit,				0},
	{NULL,	"quote-character",				quote,					QVIEW | QMINUS | QZERO},
	{NULL,	"read-file",					fileread,				QREST},
	{NULL,	"recover-layout",				recoverlayout,			0},
	{NULL,	"redraw-display",				reposition,				0},
	{NULL,	"remove-mark",					remmark,				0},
	{NULL,	"replace-string",				sreplace,				QVIEW},
	{NULL,	"reset-font-data",				fontreset,				0},
	{NULL,	"resize-window",				resize,					0},
	{NULL,	"restore-window",				restwnd,				0},
	{NULL,	"reverse-incremental-search",	risearch,				0},
	{NULL,	"run",							execproc,				0},
	{NULL,	"save-file",					filesave,				QVIEW},
	{NULL,	"save-layout",					savelayout,				0},
	{NULL,	"save-window",					savewnd,				0},
	{NULL,	"scroll-next-down",				nextdown,				0},
	{NULL,	"scroll-next-up",				nextup,					0},
	{NULL,	"search-forward",				forwsearch,				0},
	{NULL,	"search-reverse",				backsearch,				0},
	{NULL,	"select-buffer",				usebuffer,				0},
	{NULL,	"set",							setvar,					0},
	{NULL,	"set-dos-environment",			dossetenv,				0},
	{NULL,	"set-fill-column",				setfillcol,				0},
	{NULL,	"set-mark",						setmark,				0},
	{NULL,	"set-multimark",				setmultimark,			0},
	{NULL,	"shell-command",				spawn,					QREST},
	{NULL,	"shrink-window",				shrinkwind,				0},
	{NULL,	"silent-execute-program",		silentexecprg,			QREST},
	{NULL,	"source",						execfile,				0},
	{NULL,	"split-current-window",			splitwind,				0},
	{NULL,	"store-procedure",				storeproc,				0},
	{NULL,	"tex-insert-brace",				tex_insbrace,			QVIEW},
	{NULL,	"tex-insert-braces",			tex_insbraces,			QVIEW},
	{NULL,	"tex-insert-bracket",			tex_insbracket,			QVIEW},
	{NULL,	"tex-insert-dollar",			tex_insdollar,			QVIEW},
	{NULL,	"tex-insert-paren",				tex_insparen,			QVIEW},
	{NULL,	"tex-insert-quote",				tex_insquote,			QVIEW},
	{NULL,	"tex-terminate-paragraph", 		tex_termpara,			QVIEW | QMINUS},
	{NULL,	"toggle-cr",					togglecr,				0},
	{NULL,	"toggle-linenum",				swlinenum,				0},
	{NULL,	"toggle-over",					toggleover,				0},
	{NULL,	"top-bottom-cursor",			gotobeol,				0},
	{NULL,	"transpose-characters",			twiddle,				QVIEW},
	{NULL,	"trim-line",					triml,					QVIEW},
	{NULL,	"trim-region",					trimr,					QVIEW},
	{NULL,	"turn-off-funckey",				turnofffunckey,			0},
	{NULL,	"turn-on-funckey",				turnonfunckey,			0},
	{NULL,	"unbind-key",					unbindkey,				0},
	{NULL,	"universal-argument",			unarg,					0},
	{NULL,	"unmark-buffer",				unmark,					0},
	{NULL,	"up-list",						latex_uplist,			QVIEW},
	{NULL,	"update-screen",				upscreen,				0},
	{NULL,	"version-emacs",				veremacs,				0},
	{NULL,	"view-file",					viewfile,				QREST},
	{NULL,	"wcom",							wcom,					QVIEW | QREST},
	{NULL,	"widen-from-region",			widen,					0},
	{NULL,	"wrap-word",					wrapword,				QVIEW},
	{NULL,	"write-file",					filewrite,				QREST},
	{NULL,	"write-message",				writemsg,				0},
	{NULL,	"wshell",						wshell,					QVIEW | QREST},
	{NULL,	"yank",							yank,					QVIEW | QMINUS},
	{NULL,	"zap-to-char",					zaptochar,				QVIEW | QKILL},
	{NULL,	"zenkaku-word",					hantozenword,			QVIEW | QMINUS},
	{NULL,	NULL,							NULL,					0}
};

int numfunc = sizeof(command_table) / sizeof(command_table[0]) - 1;

/*
========================================
	init_com_in_word_set
========================================
*/

void init_com_in_word_set(void)
{
	int i;
	short hashnum;
	const char *p;

	for(i = numfunc - 1; i >= 0; i--) {
		for(hashnum = 0, p = command_table[i].n_name; *p; p++)
			hashnum = (hashnum * 71 + *p) & NHASH;

		command_table[i].n_next = command_head[hashnum];
		command_head[hashnum] = &command_table[i];
	}
}

/*
========================================
	com_in_word_set
========================================
*/

NBIND *com_in_word_set(const char *name)
{
	NBIND *ebind;
	short hashnum;
	const char *p;

	for(hashnum = 0, p = name; *p; p++)
		hashnum = (hashnum * 71 + *p) & NHASH;

	for(ebind = command_head[hashnum]; ebind; ebind = ebind->n_next) {
		if (!strcmp(name, ebind->n_name))
			return ebind;
	}

	return NULL;
}
