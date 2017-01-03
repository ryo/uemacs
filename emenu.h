/*
----------------------------------------
	MENU.H: MicroEMACS 3.10es
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/emenu.h,v 1.1 1991/08/11 09:02:10 SALT Exp $
----------------------------------------
*/

typedef struct M_WINDOW
  {
    struct M_WINDOW *w_wndp;
    struct BUFFER *w_bufp;
    struct LINE *w_linep;
    struct LINE *w_dotp;
    int w_doto;
    char w_toprow;
    char w_ntrows;
  }
M_WINDOW;
