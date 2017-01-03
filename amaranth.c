/****************************************************************/
/* These are my extensions to microemacs.  They were originally */
/* distributed throughout the code in 3.10, but the rapid       */
/* release of 3.10e convinced me that it is a better idea to    */
/* keep them separate.  The changes and mods necessary to       */
/* implement them are described below.                          */
/*      8/7/90 Paul Amaranth                                    */
/* Comments and suggestions on these mods should be directed to */
/* me at:                                                       */
/*     amaranth@vela.acs.oakland.edu  (internet)                */
/*     amaranth@oakland               (bitnet)                  */
/*     ...!uunet!umich!vela!amaranth  (uucp)                    */
/****************************************************************/
/*   BUG LIST:
      Doing a ^G in ^X^W does not abort, but writes the file anyway
      to the default filename.
        This is a result of the kludge fix on the improper handling
        of filename completion.  See gtfilename and complete in input.
*/

/*
 *      6-jun-90    Paul Amaranth
 *      - Added set-srch-term-to-nl to toggle back and forth between META
 *        as a search termator and NL. (-1) as arg sets  it back.
 *        (Can't use set $sterm to do this, since it uses <NL> as terminator
 *        Note, even if you could do this, the prompt would be wrong.)
 *      - Made old EXACT mode the default and removed from modes string.
 *        Added ICASE mode to ignore case by choice.
 *      - Added reg-expr-search (bound to META-/) and reverse-reg-exp-search
 *        to do reg expression searches even when MAGIC mode is not enabled.
 *        Only active when MAGIC defined.  Modified search routines so last
 *        pattern remembered depends on mode.  Default for reg-etc is the
 *        last magic patter, whether used by other search routine or itself.
 *        Default for search routines is last ordinary pattern used.
 *      - Improved inword() to be look for white space or punctuation delimited
 *        strings, rather than anything not a letter or number.  Now, $12.30
 *        is a word.
 *      - Added transpose-words bound to M-T (twidlword) (mostly works).
 *      - Fixed twiddle to always be consistent and twiddle 2 chars before dot.
 *      - Added kill ring which saves items previously killed.  Size is
 *        determined by KSIZE.  This is a circular list with up to KSIZE
 *        items that (possibly) grows with each kill.
 *        A number of commands go with this.  
 *      +  Yank-previous (^X-^Y) yanks the previously killed thing (or nth
 *         killed thing given an argument).  
 *      +  Index-kill-ring (M-^I) moves the kill ring back by its arg value 
 *         so the next yank will get something else
 *      +  Kill-this-yank-that (M-Y) is my favorite though.  If you've yanked 
 *         something and its not what you want, just M-Y and it gets killed
 *         and the previous item becomes the current kill buffer and its yanked
 *         You can index through the entire kill ring this way.  An argument
 *         will yank the nth previous kill in the ring.
 *      +  Display-kill-ring will show what is in the ring with an arrow at
 *         the current item.
 *      -  Added twiddle-lines at suggestion of T. Hacker.  Bound to M-^T.
 *  Well, we can remove set-srch-term-to-nl, what ya gotta do is put
 *    the line "set $sterm ~n" into your .rc file (this will work from
 *    a buffer or a file, but not the keyboard).  BUT THE DANG PROMPT
 *    IS STILL WRONG.  Add the change below to readpattern in search.c.
 *  Geez, I wish he'd stop tinkering with the search routines, had to
 *    modify the reg expression routines to work again  8/9/90
 *    Also took out normal/reg expr pattern memory.  It seemed too
 *    complex to really justify.  Just remember last pattern whatever
 *    it was.
 *  Made the case sensitive default search a conditional compile.  Now, if
 *    you define RATSRCH true in edef.h, searches are case sensitive by
 *    default and you can use ICASE mode to ignore case.  If it is defined
 *    as FALSE, you get the normal backwards microemacs behavior.
 *  Made filewrite (^X-^W) use the remembered filename if none given on
 *    the command.  Makes systems using xon/xoff a LOT easier to use.
 *    See changes to file.c below
 *  Fixed bug in gtfilename that prevented above change from working.
 *    See changes to input.c below
 *  Changed the name of the startup from the undocumented .emacs310rc back
 *    to .emacsrc for Unix type (in epath.h).
 *  Added complaint when nothing in kill ring and yank executed.
 *  Someone screwed up and make M-5-^V go forward 5 lines, not 5 pages, same
 *    for ^U-M-V.  Geez, you use forward/backward line for that.  A config 
 *    option got lost in the shuffle.  See changes to basic.c below.
 *  Rebound SPEC-N (which turns out to be CTRL-<space>) to space (which is
 *    in the code below).  I like to do ^U-^<space> a lot and it really
 *    bugs me that it moves down instead of spacing over. 
 *  Stupid key interactions, FNN is down arrow as well, re-unbound it, you
 *    can use bind-to-key space FNN in an .rc file to fix it on a personal
 *    level.
 * 
 *    
 * FUTURE THOUGHTS:
 *  Add local-display screen.  Multics emacs used to do this.  Stuff that
 *  you don't really want, like a list of buffers and which are changed,
 *  the output from display-kill-ring, etc
 *  shows up in a window that goes away when you hit ^J.  I suppose you
 *  could bind  ^J to delete-other-windows, but you'd have to be in another
 *  window.  have to think a while on this one.  Probably make it a macro
 *  anyway.
 *
 *  Make the display kill ring function active, so you could change the
 *  active kill buffer by using cursor keys.  This might be very hard
 *  to do.  Have to think on it a while.
 
*******     Changes to ebind.h
  Add the following key bindings

    {CTLX|CTRL|'Y',   BINDFNC,     yankprev},
    {META|CTRL|'I',   BINDFNC,     indxkr},
    {META|CTRL|'T',   BINDFNC,     twidlines},
    {META|'/',        BINDFNC,     fresearch},
    {META|'T',        BINDFNC,     twidlword},
    {META,'Y',        BINDFNC,     wipenyank},
    {SPEC|'N',        BINDFNC,     space},

  Comment out
    {SPEC|'N',		BINDFNC,	forwline},  

*******   Changes to edef.h
   
   - Change 
	"WRAP", "CMODE", "SPELL", "EXACT", "VIEW", "OVER",
     To
       #if RATSRCH
          "WRAP", "CMODE", "SPELL", "ICASE", "VIEW", "OVER",
       #else
          "WRAP", "CMODE", "SPELL", "EXACT", "VIEW", "OVER",
       #endif

   - Add
      NOSHARE KILLRING kill_ring[KRSIZE];     /* Kill ring 
      NOSHARE int cur_kr = -2;                /* Ptr into kill ring 
      NOSHARE int max_kr = -1;                /* Max entry in kill ring 
      NOSHARE char st_prompt[10] = "<META>";  /* Search prompt 
      NOSHARE int meta_char;                  /* Used for set-srch-term-to-nl
                                                 to remember what meta is 
      NOSHARE extern KILLRING kill_ring[];
      NOSHARE extern cur_kr;
      NOSHARE extern max_kr;

     
****** Changes to estruct.h
   Add
   #define  MDICASE 0x0008  /* Inexact matching for searches
   #define RATSRCH 1  /* 1=case sensitive srch by default with ICASE mode 
                      /* 0 = normal backwards microemacs mode          

   #define KRSIZE  8    /* #buffers in kill ring

     typedef struct {
             struct KILL *kill_bufhp;   /* Header ptr 
             struct KILL *kill_cbufp;   /* Ptr to current chunk 
             int kill_used_in_last;     /* Bytes used in last chunk 
      } KILLRING;

***** Changes to efunc.h  (note these must be in alpha order)
     Add
         {"display-kill-ring",           dispklist},
         {"index-kill-ring",             indxkr},
         {"kill-this-yank-that",         wipenyank},
  ** These two should be conditional on MAGIC
#ifdef MAGIC
         {"reg-expr-search",             fresearch},
         {"reverse-reg-expr-search",     rresearch},
#endif
         {"space",                       space},
         {"transpose-lines",             twidlines},
         {"transpose-words",             twidlword},
         {"twiddle",                     twiddle},
         {"twiddle-lines",               twidlines},
         {"twiddle-words",               twidlword},


******* Changes to eproto.h
    Add  (note, these go in two places, once without the parms)
      extern PASCAL NEAR dispklist(int f, int n);
      extern PASCAL NEAR fresearch(int f, int n);
      extern PASCAL NEAR rresearch(int f, int n);
      extern PASCAL NEAR twiddle(int f, int n);
      extern PASCAL NEAR twidlword(int f, int n);
      extern PASCAL NEAR twidlines(int f, int n);
      extern PASCAL NEAR yankprev(int f, int n);
      extern PASCAL NEAR wipenyank(int f, int n);
      extern PASCAL NEAR indxkr(int f, int n);
      extern PASCAL NEAR space(int f, int n);

Change the declaration of readpattern from
extern int PASCAL NEAR readpattern(char *prompt, char apat[], int srch);
to
extern int PASCAL NEAR readpattern(char *prompt, char apat[], int srch, int force);

******* Changes to english.h (or your particular error text file)
   Add
      #define TEXT300 "R/E Search"
      #define TEXT301 "Reverse R/E Search"
      #define TEXT302 "No previous chunk to yank"
      #define TEXT303 "Nothing to yank"
      #define TEXT304 "Nothing in the kill ring"
      #define TEXT305 "Region does not match kill ring"
      #define TEXT306 "Kill ring out of sync"
      #define TEXT307 "Item %d of %d: "
      #define TEXT308 "Killring"
      #define TEXT309 "Can not display killring"


***** Changes to line.c
    Remove or comment out kdelete.  The version in this file
      replaces it.

    In yank(), change
	if (kbufh == NULL)
		return(TRUE);		/* not an error, just nothing 

    To
	if (kbufh == NULL)
              {
                mlwrite(TEXT304);       /* complain anyway 
		return(TRUE);		/* not an error, just nothing 
                }


**** Changes to word.c
   Add the following defines to the beginning:
#define isspace(c) (c==' '||c=='\t'||c==12)
#define ispunct(c) (c=='.'||c==';'||c==':'||c==','||c=='?'||c==')'||c=='(')

   In inword(), replace this code:
	if (isletter(c))
		return(TRUE);
	if (c>='0' && c<='9')
		return(TRUE);
	if (c == '_')
		return(TRUE);
	return(FALSE);
 
    with the following code:

        if (isspace(c)) return(FALSE);
        if (ispunct(c) && c != '.') return(FALSE);
        if (c == '.') /* Look at next character *//*
          {
          c = lgetc(curwp->w_dotp, curwp->w_doto+1);
          if (isspace(c)) return(FALSE);
          }
	return(TRUE);

****** Changes to search.c
  In calls to readpattern, add a fourth parameter with the value FALSE
  Add a fourth parameter to readpattern called force_magic (int).

  in routine readpattern change
           strcat(tpat, "]<META>: ");
  to
        /* Make sure we use the right prompt PGA
        if (sterm == ctoec('\r'))
           strcat(tpat, "]<NL>: ");
        else
           strcat(tpat, "]<META>: ");

  In routine readpattern change
   if ((curwp->w_bufp->b_mode & MDMAGIC) == 0)
  to
   if ((!force_magic && curwp->w_bufp->b_mode & MDMAGIC) == 0)

  Change this where ever it occurs (3 times in 3.10e)
	if ((curwp->w_bufp->b_mode & MDEXACT) == 0)
  To
   #if RATSRCH
	if ((curwp->w_bufp->b_mode & MDICASE) != 0)
   #else
	if ((curwp->w_bufp->b_mode & MDEXACT) == 0)
   #endif


***** Changes to tags.c
   Change
	curbp->b_mode |= MDEXACT;
   To
     #if RATSRCH
	curbp->b_mode |= MDICASE;
     #else
	curbp->b_mode |= MDEXACT;
     #endif


***** Changes to file.c
   In filewrite
   Change
     if ((fname = gtfilename(TEXT144)) == NULL)  /* ~/ and $VAR expansion 
   To
     if ((fname = gtfilename(TEXT144)) == NULL &&  /* ~/ and $VAR expansion 
           curbp->b_fname[0] == 0)

   A couple lines later change
		strcpy(curbp->b_fname, fname);
   To
                if (fname != NULL)
                   strcpy(curbp->b_fname, fname);


****** Changes to input.c  - THIS IS A BUG
  In gtfilename Change
	if (sp == NULL)    /* This doesn't work anymore  PGA 
  To
        if (*sp == '\0')
		return(NULL);


**** Changes to basic.c
   In routines forwpage() and backpage() change the lines (XXXX is either
   back (in forwpage) or forw (in backpage).

        } else if (n < 0)
                return(XXXXpage(f, -n));
        lp = curwp->w_linep;

   To
        } else if (n < 0)
                return(page(f, -n));
        else                                    /* Convert from pages   
                n *= curwp->w_ntrows;           /* to lines.            
        lp = curwp->w_linep;

**************************************************************/

#include <stdio.h>
#include	"estruct.h"
#include	"eproto.h"
#include	"edef.h"
#include	"elang.h"



#if	MAGIC
/*
 * reg-expr-search - Search forward using a reg expression string, even
 *      if the user has not set MAGIC mode.  Kludged from forwsearch.
 */
PASCAL NEAR fresearch(f, n)
int f, n;	/* default flag / numeric argument */
{
	register int status = TRUE;

	/* If n is negative, search backwards.
	 * Otherwise proceed by asking for the search string.
	 */
	if (n < 0)
		return(rresearch(f, -n));

	/* Ask the user for the text of a pattern.  If the
	 * response is TRUE (responses other than FALSE are
	 * possible), search for the pattern for as long as
	 * n is positive (n == 0 will go through once, which
	 * is just fine).
	 */


	if ((status = readpattern(TEXT300, &pat[0], TRUE, TRUE)) == TRUE) {
/*                                "R/E Search" */ 
		do {
                   if (!mcstr())
                      return(FALSE);
                   if (magical)
                        {
			status = mcscanner(FORWARD, PTEND, n);
                        }
                   else
                        {
                        status = scanner(FORWARD, PTEND, n);
                        }
		} while ((--n > 0) && status);

		/* Save away the match, or complain
		 * if not there.
		 */
		if (status != TRUE)
			mlwrite(TEXT79);
/*                              "Not found" */
	}
        thisflag |= CFSRCH;
	return(status);
}


/*
 * reverse-reg-expr-search.  Like reg-expr-search above, but in reverse.
 */
PASCAL NEAR rresearch(f, n)
int f, n;	/* default flag / numeric argument */
{
	register int status = TRUE;

	/* If n is negative, search forwards.
	 * Otherwise proceed by asking for the search string.
	 */
	if (n < 0)
		return(fresearch(f, -n));

	/* Ask the user for the text of a pattern.  If the
	 * response is TRUE (responses other than FALSE are
	 * possible), search for the pattern for as long as
	 * n is positive (n == 0 will go through once, which
	 * is just fine).
	 */

	if ((status = readpattern(TEXT301, &pat[0], TRUE, TRUE)) == TRUE) {
/*                                "Reverse r/e search" */
		do {
                   if (!mcstr())
                       return(FALSE);
                    if (magical)
                        {
			status = mcscanner(REVERSE, PTBEG, n);
                        }
                    else
                        {
                        status = scanner(REVERSE, PTBEG, n);
                        }
		} while ((--n > 0) && status);

		/* Save away the match, or complain
		 * if not there.
		 */
		if (status != TRUE)
			mlwrite(TEXT79);
/*                              "Not found" */
	}
        thisflag |= CFSRCH;
	return(status);
}
#endif


/****************************************************************/
/* Notes on kill ring                                           */
/*  The kill ring is an array with KSIZE entries.  This forms   */
/*  a two tier structure with the varaibles kbufh, kbufp and    */
/*  kused at the lower level and the kill_ring array of KSIZE   */
/*  KILL structures at the top.  This is a bit of a hack and,   */
/*  when time allows, I should make into a single kill_ring     */
/*  reference.  There are probably some execution advantages in */
/*  leaving it the way it is, though.                           */
/*                                                              */
/* Everytime you kill something new, we put the contents of     */
/* kbufh into the ring and increment cur_kr mod KSIZE.  Its     */
/* possible it is already in the ring, in which case we just    */
/* reset cur_kr to point to it.  If cur_kr does not point to    */
/* the last item in the list, the next kill will overwrite the  */
/* next item.  This can be a little confusing until you get     */
/* used to it, but it really does have a basis in logic.  You   */
/* will get this behavior when you reach KSIZE items in the     */
/* ring anyway and it would be more confusing to be on item 2   */
/* and then, after a kill, be on item 5.  This way, you're on   */
/* item 3, period.                                              */
/*                                                              */
/* The kill ring commands are:  index-kill-ring (indxkr),       */
/* yank-previous (yankprev), kill-this-yank-that (wipenyank)    */
/* and display-kill-ring (dispklist).                           */
/*                                                              */
/* Anyway, the kill ring stuff was written by Paul Amaranth     */
/* June, 1990.  I've been waiting for this stuff a looong time  */
/* and I finally got tired of waiting.                          */
/*                                                              */
/* General kudos to Lawrence and Conroy, though.  They did a    */
/* fine job of making and keeping this thing structured.  This  */
/* was a pretty easy add (even if it is a hack :-)              */
/****************************************************************/


/*
 * Get a new slot in the kill ring saving the current kill context.  
 * If we are reusing an old one, release all the old space.  
 */
PASCAL NEAR kdelete()
{
	KILL *kp;	/* ptr to scan kill buffer chunk list */
	KILL *kp2;
        int indx;

        if (cur_kr < -1)  /* Init structure */
	   {
           for (indx=0; indx<KRSIZE; ++indx)
              {
              kill_ring[indx].kill_bufhp = NULL;
              kill_ring[indx].kill_cbufp = NULL;
              kill_ring[indx].kill_used_in_last = KBLOCK;
              }
           ++cur_kr;  /* Ready for next time */
           return;
           }


      /* Some possible scenarios: 
	 a) this may be a new chunk, just put it in somewhere
	 b) this may already be in the kill ring somewhere, reset the kill
	    variables
      */
        if (kbufh != NULL)
        for (indx=0; indx <= max_kr; ++indx)
           {
	   if (kill_ring[indx].kill_bufhp == kbufh &&
	       kill_ring[indx].kill_cbufp == kbufp &&
	       kill_ring[indx].kill_used_in_last == kused)
             {
             cur_kr = indx;
             kbufh = kbufp = NULL;
             kused = KBLOCK;
             return;
             }
          }

        /* Not currently in the ring */
	++cur_kr;       /* See if there's room to put this one */
        cur_kr = cur_kr%KRSIZE;
        if (cur_kr > max_kr) max_kr = cur_kr;  /* KR is expandable upto KRSIZE */
    
        if (kill_ring[cur_kr].kill_bufhp != NULL)  /* Gotta reuse one */
          {   /* Remove old item from buffer */
		kp2 = kill_ring[cur_kr].kill_bufhp;
		while (kp2 != NULL) {
			kp = kp2->d_next;
			free(kp2);
			kp2 = kp;
		}
          }
        
        /* Save current kill context into kill ring */
        kill_ring[cur_kr].kill_bufhp = kbufh;
        kill_ring[cur_kr].kill_cbufp = kbufp;
        kill_ring[cur_kr].kill_used_in_last = kused;

        /* Reset kill variables */

	kbufh = kbufp = NULL;
	kused = KBLOCK; 	        
}

/*
	Instead of yanking the current item in the kill buffer, yank the 
        previous chunk.
	Uses indxkr to index n positions through the kill ring, then yanks
	it.  Succeeds unless the indicated buffer is NULL.  Bound to ^X-^Y.
*/
PASCAL NEAR yankprev(f, n)
{

    if (n < 0) return (FALSE);

    if (cur_kr < 0) 
      {
	mlwrite (TEXT302);  /* No previous chunk to yank */
        return(FALSE);
      }

    do_indxkr(n);

    return(yank(0,1));
}


/* Index kill ring pointer.  Moves backward through kill ring n positions */
/* Bound to Meta-^I.  Actual work done by do_indxkr                       */

PASCAL NEAR indxkr(f, n)
{
    char indx_msg[80];
    int chrs_to_copy;
    int indx;
    char *strend;
    char *ch;
    int more_flag;

    if (cur_kr < 0) 
      {
	mlwrite (TEXT304);  /* Nothing in the kill ring */
        return(FALSE);
      }

    do_indxkr(n);
    sprintf(indx_msg, TEXT307, cur_kr+1, max_kr+1);
    strend = &indx_msg[strlen(indx_msg)];
    more_flag = TRUE;
    if (kbufh != kbufp) chrs_to_copy = 20;
    else
    if (kused > 20) chrs_to_copy = 20;
    else
      {
      more_flag = FALSE;
      chrs_to_copy = kused;
      }

    ch = kbufh -> d_chunk;      
    for (indx=0; indx<chrs_to_copy; ++indx)
     {
     if (*ch != '\r') *strend = *ch;
     else
      { *strend = '\\'; ++strend; *strend = 'n';}
     ++ch; ++strend;
     }
    *strend = '\0';
    if (more_flag) strcat (indx_msg, " <MORE>");
    mlwrite(indx_msg);
    return(TRUE);
}

/* Do actual work of indexing kill ring */
do_indxkr (n)
{
/* If anything recently killed, save it away */
        if (kbufh != NULL) 
           {
           kdelete();  /* This resets kill pointers */
           }

        while (n--) {   /* Index through kill ring for item  */
	  --cur_kr;
	  if (cur_kr < 0) cur_kr = max_kr;
          }

  	kbufh = kill_ring[cur_kr].kill_bufhp;
  	kbufp = kill_ring[cur_kr].kill_cbufp;
  	kused = kill_ring[cur_kr].kill_used_in_last;
      return;
}


/* 	Wipe the current area and yank the previous item in the kill ring.
	An error condition arises if a) the current region does not match
	the kill ring, or b) there is no previous buffer to yank
        If a numeric arg is given, go back through the kill ring mod(KRSIZE)
	to find the proper buffer
*/
PASCAL NEAR wipenyank(f, n)
{
    char *ch;
    KILL *kp;
    register int regions_ident;
    register int indx;
    register int chars_to_dl;

    register LINE *start_dot;
    register int   start_off;
    register LINE *end_dot;
    register int   end_off;

if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
	return(rdonly());	/* we are in read only mode	*/
if (n < 0)			/* No negative args allowed     */
	return(FALSE);

/* Illegal unless something in the kill buffer/ring */
if (kbufh == NULL)
  {
  mlwrite(TEXT304);  /* Nothing in the kill ring */
  return(FALSE);
  }

/* Now find out if the cursor points to the bottom of the region */
/* Do this by 1) backing up the proper number of characters from */
/* where we are and then, char by char, going through both the   */
/* kill buffer and the region checking for equality.  If they    */
/* don't match, complain.  Else, delete with no save from the    */
/* remembered beginning, index back one in the kill and yank.    */

/* Remember where we start */
    end_dot = curwp -> w_dotp;
    end_off = curwp -> w_doto;

/* Now back up the proper number of chars */
   kp = kbufh;
   chars_to_dl = 0;
   while (kp != kbufp)
     {
     backchar(0, KBLOCK);
     kp = kp -> d_next;
     chars_to_dl += KBLOCK;
     }
   backchar (0, kused);
   chars_to_dl += kused;
   start_dot = curwp -> w_dotp;  /* Remember this too */
   start_off = curwp -> w_doto;

/* Now see if the two regions are identical */
   kp = kbufh;
   regions_ident = TRUE;  /* Assume to start */
   while (kp != kbufp && regions_ident)
     {
     ch = kp->d_chunk;
     for (indx=0; indx<KBLOCK && regions_ident; ++indx)
      {
      if (*ch == '\r' || *ch == lgetc(curwp->w_dotp, curwp->w_doto))
       {
       ++ch;
       forwchar(FALSE, 1);
       }
      else
        regions_ident = FALSE;
      }  /* End for loop */
    kp = kp -> d_next;
    } /* End while loop */

  /* Do same thing for last chunk */
  ch = kp->d_chunk;
  for (indx=0; indx<kused && regions_ident; ++indx)
    {
      if (*ch == '\r' || *ch == lgetc(curwp->w_dotp, curwp->w_doto))
       {
       ++ch;
       forwchar(FALSE, 1);
       }
      else
        regions_ident = FALSE;
    }  /* End for loop */

  if (!regions_ident)
    {
    mlwrite(TEXT305);          /* Region does not match kill ring */
    curwp -> w_dotp = end_dot;  /* Cursor back to where we started */
    curwp -> w_doto = end_off;
    return(FALSE);     
    }

  /* Set the mark at the current "." position */
  curwp->w_markp[0] = curwp->w_dotp;
  curwp->w_marko[0] = curwp->w_doto;
 
 /* Go to beginning of region */
  curwp->w_dotp = start_dot;
  curwp->w_doto = start_off;

 /* Delete region w/o saving it (its already in the kill ring) */
 ldelete(chars_to_dl, FALSE);

 /* Index back n args in the kill ring */
 do_indxkr(n);

 /* Yank the current stuff out */
 yank (FALSE, 1);

return(TRUE);
}



PASCAL NEAR dispklist(f,n)  /* Display the kill ring in a window */

{
	register WINDOW *wp;	/* scanning pointer to windows */
	register KEYTAB *ktp;	/* pointer into the command table */
	register NBIND *nptr;	/* pointer into the name binding table */
	register BUFFER *bp;	/* buffer to put binding list into */
	int cmark;		/* current mark */
	char outseq[80];	/* output buffer for keystroke sequence */
	char *ch, *strend;
	char bflag[10];
	int kindx, indx;
	int chrs_to_copy;
        int more_flag;

	/* split the current window to make room for the binding list */
	if (splitwind(FALSE, 1) == FALSE)
			return(FALSE);

	/* and get a buffer for it */
	bp = bfind(TEXT308, TRUE, 0);
/*                 "Killring" */
	if (bp == NULL || bclear(bp) == FALSE) {
		mlwrite(TEXT309);
/*                      "Can not display killring "*/
		return(FALSE);
	}

        /* Make sure the current kill buffer is in the ring */
        if (kbufh != NULL)
           {
           kdelete();
           kbufh = kill_ring[cur_kr].kill_bufhp;
           kbufp = kill_ring[cur_kr].kill_cbufp;
           kused = kill_ring[cur_kr].kill_used_in_last;
           }

	/* disconect the current buffer */
        if (--curbp->b_nwnd == 0) {             /* Last use.            */
                curbp->b_dotp  = curwp->w_dotp;
                curbp->b_doto  = curwp->w_doto;
		for (cmark = 0; cmark < NMARKS; cmark++) {
	                curbp->b_markp[cmark] = curwp->w_markp[cmark];
        	        curbp->b_marko[cmark] = curwp->w_marko[cmark];
        	}
		curbp->b_fcol  = curwp->w_fcol;
        }

	/* connect the current window to this buffer */
	curbp = bp;	/* make this buffer current in current window */
	bp->b_mode = 0;		/* no modes active in binding list */
	bp->b_nwnd++;		/* mark us as more in use */
	wp = curwp;
	wp->w_bufp = bp;
	wp->w_linep = bp->b_linep;
	wp->w_flag = WFHARD|WFFORCE;
	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		wp->w_markp[cmark] = NULL;
		wp->w_marko[cmark] = 0;
	}

	/* build the contents of this window, inserting it line by line */

	for (kindx=0; kindx <= max_kr; ++kindx) {
		if (kill_ring[kindx].kill_bufhp == kbufh) strcpy(bflag, "-> ");
 		else strcpy (bflag, "   ");
		sprintf(outseq, "%s %d: ", bflag, kindx+1);

	    strend = &outseq[strlen(outseq)];  more_flag = TRUE;
	    if (kill_ring[kindx].kill_bufhp != kill_ring[kindx].kill_cbufp) 
		chrs_to_copy = 50;
	    else
	    if (kill_ring[kindx].kill_used_in_last > 50) chrs_to_copy = 50;
	    else
              {
	      chrs_to_copy = kill_ring[kindx].kill_used_in_last;
              more_flag = FALSE;
              }

	    ch = kill_ring[kindx].kill_bufhp -> d_chunk;      
	    for (indx=0; indx<chrs_to_copy; ++indx)
	     {
	     if (*ch != '\r') *strend = *ch;
	     else
	      { *strend = '\\'; ++strend; *strend = 'n';}
	     ++ch; ++strend;
	     }
	    *strend = '\0';
            if (more_flag) strcat(outseq, " <MORE>");
   	    if (linstr(outseq) != TRUE) return(FALSE);
            if (lnewline() != TRUE) return(FALSE);
            }

	curwp->w_bufp->b_mode |= MDVIEW;/* put this buffer view mode */
	 curbp->b_flag &= ~BFCHG;	/* don't flag this as a change */
	wp->w_dotp = lforw(curbp->b_linep);/* back to the beginning */
	wp->w_doto = 0;
	upmode();
	mlwrite("");	/* clear the mode line */
	return(TRUE);
}

/*	Twiddle-lines.  This function is in at the suggestion of Tom
	Hacker.  It does what the name suggests.  If there is only
	1 line in the buffer, it is essentially a NOP.
*/

PASCAL NEAR twidlines(f, n)
{
   register int start_offset;

  if (curbp->b_mode&MDVIEW)  /* Not in R/O mode */
     return(rdonly());

  if (n < 0)   /* Ignore if neg args */
     return(FALSE);

  /* Remember where the cursor is in the line */
  start_offset = curwp -> w_doto;

  /* Goto beginning of the line */
  gotobol (0,0);

  /* Kill the line */
  if (curwp->w_dotp->l_used > 0) killtext (1,1);  /* Get text, if any */
  else  killtext (0,1);   /* Get return */

  /* Reset flag so if next command is a kill, a merge doesn't happen */
  thisflag &= !CFKILL;

  /* Go up one line */
  backline (0,1);

  /* Yank it back */
  yank (0,1);

  /* Since we took the return, we are on the next line, move cursor */
  if (start_offset > curwp ->w_dotp->l_used) 
        start_offset = curwp ->w_dotp->l_used;
  forwchar (0, start_offset);
  return;
}

/*
 * Twiddle the words immediately preceding dot.
 * Bound to "M-T".  
 * This command fails if word 1 is the very first item in the buffer.
 * The cursor also screws up a bit if we twiddle when the dot is in the
 * middle of the word.  Not too serious, but M-t M-t is not exactly what
 * we started with.    -  Paul Amaranth
 */
PASCAL NEAR twidlword(f, n)
{
	register LINE	*w1_dotp;	/* word cursor line */
	register int	w1_doto;	/* and row  to move */
        register LINE  *w2_dotp;
        register int    w2_doto;
	register int    tmp_doto;
	register int c;		/* temp char */
        register char *tmp_chars;
        register int     same_line;
	long w1_size;		/* # of chars to delete */
        long w2_size;           /* # to move forward */
        int w1_indx;

	/* don't allow this command if we are in read only mode */
	if (curbp->b_mode&MDVIEW)
		return(rdonly());

	/* ignore the command if there is a negative argument */
	if (n < 0)
		return(FALSE);

	/* figure out how many characters to give the axe */
	w2_size = 0;

	/* get us into a word.... */
	while (inword() == FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return(FALSE);
	}

        /* Find beginning of word */
	while (inword() != FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return(FALSE);
		++w2_size;
	}

        forwchar(FALSE, 1);
        w2_dotp = curwp->w_dotp;   /* Remember where to delete from */
        w2_doto = curwp->w_doto;  /* we are only deleting the chars, not */
        backchar(FALSE, 1);       /* the white space */

        w1_size = 0;
        /* Now find where to put word */
	/* get us into a word.... */
	while (inword() == FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return(FALSE);
	}

        /* Find beginning of word */
	while (inword() != FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return(FALSE);
        ++w1_size;
	}

        /* There is a word to twiddle.  Remember where we are */
        forwchar(FALSE, 1);
        w1_dotp = curwp->w_dotp;
        w1_doto = curwp->w_doto;
        same_line = w1_dotp == w2_dotp;

        /* Swap to where the word 2 is  and kill it */
        curwp->w_dotp = w2_dotp;
        curwp->w_doto = w2_doto;

	kdelete();
	thisflag |= CFKILL;	/* this command is a kill */

        ldelete(w2_size,TRUE);     /* Kill word 2 */

        tmp_doto = w1_doto;    
        /* Copy word1 to current location of word 2 */
        tmp_chars = (char *)malloc(w1_size);
	for (w1_indx=0; w1_indx<w1_size; ++w1_indx)
          {
  	  tmp_chars[w1_indx] = lgetc(w1_dotp, tmp_doto);
          ++tmp_doto;
          }
	for (w1_indx=0; w1_indx<w1_size; ++w1_indx)
          linsert(1, tmp_chars[w1_indx]);
        free(tmp_chars);

	/* Set mark zero where we currently are. Always return here */
	  curwp->w_markp[0] = curwp->w_dotp;
	  curwp->w_marko[0] = curwp->w_doto;

        /* Delete word 1                                           */
	/* If on the same line, leave curwp->w_dotp alone, it may  */
	/* have been reallocated in linsert.                       */
	if (!same_line) curwp->w_dotp = w1_dotp;
        curwp->w_doto = w1_doto;
        ldelete (w1_size, FALSE);

        /* Get word 2 into position */
        yank(FALSE,1);

        /* Reset cursor to either end of orig word 1 or original pos */
        curwp->w_dotp = curwp->w_markp[0];
        curwp->w_doto = curwp->w_marko[0];
        lchange(WFHARD);
   
	return(TRUE);
}

/****************************************************************/
/* Insert (at least one) space, move cursor forward after space */
/* This routine exists soley to allow CTRL-<space> to be bound  */
/* to <space>.                                                  */
/****************************************************************/
PASCAL NEAR space(f, n)
int f, n;	/* default flag / numeric argument */
{
 return(linsert(n, ' '));
}
