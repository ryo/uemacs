/*
 * $Id: efunc.h,v 1.10 2009/11/18 18:40:15 ryo Exp $
 *
 * EFUNC.H:	MicroEMACS function declarations and names
 *
 * This file list all the C code functions used by MicroEMACS and the names to
 * use to bind keys to them. To add functions, declare it here in both the
 * extern function list and the name binding table.
 *
 */

/*
 * Name to function binding table
 *
 * This table gives the names of all the bindable functions end their C function
 * address. These are used for the bind-to-key function.
 */

NBIND names[] = {
	{"abort-command", ctrlg},
	{"add-global-mode", setgmode},
	{"add-mode", setmod},
#if	APROP
	{"apropos", apro},
#endif
	{"backward-character", backchar},
	{"begin-macro", ctlxlp},
	{"beginning-of-file", gotobob},
	{"beginning-of-line", gotobol},
	{"bell", bell},
	{"bind-to-key", bindtokey},
	{"buffer-position", showcpos},
	{"case-region-lower", lowerregion},
	{"case-region-upper", upperregion},
	{"case-word-capitalize", capword},
	{"case-word-lower", lowerword},
	{"case-word-upper", upperword},
	{"change-file-name", filename},
	{"change-screen-size", newsize},
	{"change-screen-width", newwidth},
	{"clear-and-redraw", refresh},
	{"clear-message-line", clrmes},
	{"copy-region", copyregion},
#if	WORDPRO
	{"count-words", wordcount},
#endif
	{"ctlx-prefix", cex},
	{"ctlc-prefix", cec},
	{"delete-blank-lines", deblank},
	{"delete-buffer", killbuffer},
	{"delete-global-mode", delgmode},
	{"delete-mode", delmode},
	{"delete-next-character", forwdel},
	{"delete-next-word", delfword},
	{"delete-other-windows", onlywind},
	{"delete-previous-character", backdel},
	{"delete-previous-word", delbword},
	{"delete-window", delwind},
	{"describe-bindings", desbind},
#if	DEBUGM
	{"describe-functions", desfunc},
#endif
	{"describe-key", deskey},
#if	DEBUGM
	{"describe-variables", desvars},
#endif
#if	AEDIT
	{"detab-line", detab},
	{"detab-region", detab},
#endif
#if	DEBUGM
	{"display", dispvar},
#endif
	{"end-macro", ctlxrp},
	{"end-of-file", gotoeob},
	{"end-of-line", gotoeol},
	{"end-of-word", endword},
#if	AEDIT
	{"entab-line", entab},
	{"entab-region", entab},
#endif
	{"exchange-point-and-mark", swapmark},
	{"execute-buffer", execbuf},
	{"execute-command-line", execcmd},
	{"execute-file", execfile},
	{"execute-macro", ctlxe},
	{"execute-named-command", namedcmd},
#if	PROC
	{"execute-procedure", execproc},
#endif
	{"execute-program", execprg},
	{"exit-emacs", quit},
#if	WORDPRO
	{"fill-paragraph", fillpara},
#endif
	{"filter-buffer", filter},
	{"find-file", filefind},
	{"forward-character", forwchar},
	{"goto-line", gotoline},
	{"goto-mark", gotomark},
#if	CFENCE
	{"goto-matching-fence", getfence},
#endif
	{"grow-window", enlargewind},
	{"handle-tab", tab},
	{"hankaku-word", zentohanword},
	{"help", help},
	{"hunt-backward", backhunt},
	{"hunt-forward", forwhunt},
	{"i-shell", spawncli},
#if	ISRCH
	{"incremental-search", fisearch},
#endif
	{"insert-file", insfile},
	{"insert-space", insspace},
	{"insert-string", istring},
#if	WORDPRO
	{"kill-paragraph", killpara},
#endif
	{"kill-region", killregion},
	{"kill-to-end-of-line", killtext},
	{"list-buffers", listbuffers},

	{"load-emphasis", loademp},

	{"macro-to-key", macrotokey},

	{"meta-prefix", meta},
#if	MOUSE
	{"mouse-move-down", movemd},
	{"mouse-move-up", movemu},
	{"mouse-region-down", mregdown},
	{"mouse-region-up", mregup},
	{"mouse-resize-screen", resizm},
#endif
	{"move-window-down", mvdnwind},
	{"move-window-up", mvupwind},
	{"name-buffer", namebuffer},
	{"narrow-to-region", narrow},
	{"newline", newline},
	{"newline-and-indent", indent},
	{"next-buffer", nextbuffer},
	{"next-line", forwline},
	{"next-page", forwpage},
#if	WORDPRO
	{"next-paragraph", gotoeop},
#endif
	{"next-window", nextwind},
	{"next-word", forwword},
	{"nop", nullproc},
	{"open-line", openline},
	{"overwrite-string", ovstring},
	{"pipe-command", pipecmd},
	{"previous-line", backline},
	{"previous-page", backpage},
#if	WORDPRO
	{"previous-paragraph", gotobop},
#endif
	{"previous-window", prevwind},
	{"previous-word", backword},
	{"print", writemsg},
	{"query-replace-string", qreplace},
	{"quick-exit", quickexit},
	{"quote-character", quote},
	{"read-file", fileread},
	{"redraw-display", reposition},
	{"remove-mark", remmark},
	{"replace-string", sreplace},
	{"resize-window", resize},
	{"restore-window", restwnd},
#if	ISRCH
	{"reverse-incremental-search", risearch},
#endif
#if	PROC
	{"run", execproc},
#endif
	{"save-file", filesave},
	{"save-window", savewnd},
	{"scroll-next-down", nextdown},
	{"scroll-next-up", nextup},
	{"search-forward", forwsearch},
	{"search-reverse", backsearch},
	{"select-buffer", usebuffer},
	{"set", setvar},
#if	CRYPT
	{"set-encryption-key", setekey},
#endif
	{"set-fill-column", setfillcol},
	{"set-mark", setmark},
	{"shell-command", spawn},
	{"shrink-window", shrinkwind},
	{"silent-execute-program", nowaitexecprg},
	{"source", execfile},
	{"split-current-window", splitwind},
	{"store-keyword", storekeyword},
#if	PROC
	{"store-procedure", storeproc},
#endif
#if	BSD
	{"suspend-emacs", bktoshell},
#endif
	{"transpose-characters", twiddle},
#if	AEDIT
	{"trim-line", trim},
	{"trim-region", trim},
#endif
	{"unbind-key", unbindkey},
	{"universal-argument", unarg},
	{"unmark-buffer", unmark},
	{"update-screen", upscreen},
	{"view-file", viewfile},

	{"wcom", wcom},

	{"widen-from-region", widen},
	{"wrap-word", wrapword},
	{"write-file", filewrite},
	{"write-message", writemsg},
	{"yank", yank},

	{"zenkaku-word", hantozenword},

	{"", NULL}
};

#define	NBINDFUNCS	((sizeof(names)/sizeof(NBIND))-1)
