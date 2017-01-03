#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estruct.h"
#include "edef.h"
#include "etype.h"
#include "elang.h"

#define HASH_TABLE_SIZE  1024
#define BUFFER_SIZE  2048

typedef struct keyword_list_t {
  struct keyword_list_t *next;
  char name[0];
} keyword_list_t;

typedef struct keyword_buf_t {
  struct keyword_buf_t *next;
  char buf[0];
} keyword_buf_t;

static keyword_list_t *keyhash[HASH_TABLE_SIZE];
static keyword_buf_t *keyword_buf;


static inline int make_hashval (char *name, int len)
{
  return (name[0] * 4 + name[len - 1] + len * 64) % HASH_TABLE_SIZE;
}

static void release_keyword (void)
{
  keyword_buf_t *next;

  while (keyword_buf) {
    next = keyword_buf->next;
    free (keyword_buf);
    keyword_buf = next;
  }
  memset (keyhash, 0, sizeof (keyhash));

  {
    int i;

    for (i = 0; i < 256; i++)
      _keyword[i] &= ~2;

    /* [0...9] */
    for (i = 0x30; i <= 0x39; i++)
      _keyword[i] |= 2;
    /* [A...Z] */
    for (i = 0x41; i <= 0x5a; i++)
      _keyword[i] |= 2;
    /* _ */
    _keyword[0x5f] |= 2;
    /* [a...z] */
    for (i = 0x61; i <= 0x7a; i++)
      _keyword[i] |= 2;
  }
}

/*
----------------------------------------
	‹­’²ƒf[ƒ^‚Ì“Ç‚Ýž‚Ý
----------------------------------------
*/

int loadempdata (int f, int n)
{
  char *fname;

  fname = singleexpwild (gtfilename (KTEX272));
  if (fname == 0) {
    mlwrite (KTEX8);
    return FALSE;
  }

  {
    int status;

    status = ffropen (fname);
    if (status != FIOSUC) {
      mlwrite (KTEX152);
      return FALSE;
    }

    release_keyword ();

    {
      int table_left;
      int length;
      char *bufp;

      table_left = 0;
      bufp = NULL;

      while (1) {
        status = ffgetline (&length);
        if (status == FIOEOF)
          break;
        if (status != FIOSUC) {
          release_keyword ();
          ffclose ();
          mlwrite (KTEX158);
          return FALSE;
        }

        if (!strchr (fline, ' ') && length) {
          int hashval;
          int size;

          size = ((sizeof (keyword_list_t) + length + 1) + (4 - 1)) & ~(4 - 1);
          table_left -= size;
          if (table_left < 0) {
            keyword_buf_t *newbuf;

            newbuf = malloc (sizeof (keyword_buf) + BUFFER_SIZE);
            if (newbuf == NULL) {
              release_keyword ();
              ffclose ();
              mlwrite (KTEX99);
              return FALSE;
            }
            newbuf->next = keyword_buf;
            keyword_buf = newbuf;

            table_left = BUFFER_SIZE;
            bufp = keyword_buf->buf;
          }

          {
            keyword_list_t *keyp;

            keyp = (keyword_list_t *)bufp;
            bufp += size;
            table_left -= size;

            hashval = make_hashval (fline, length);
            strcpy (keyp->name, fline);
            keyp->next = keyhash[hashval];
            keyhash[hashval] = keyp;

            {
              char ch, *p;

              p =fline;
              while (ch = *p++)
                _keyword[ch] |= 2;
            }
          }
        }
      }
    }

    ffclose ();
  }

  return TRUE;
}

/*
----------------------------------------
	keyword_in_word_set
----------------------------------------
*/

char *keyword_in_word_set (char *name, int len)
{
  if (keyhash == NULL)
    return NULL;

  {
    keyword_list_t *keyp;
    int hash_val;
    char ch;

    hash_val = make_hashval (name, len);
    keyp = keyhash[hash_val];
    ch = name[0];

    while (keyp) {
      char *p;

      p = keyp->name;
      if (*p == ch && !strcmp (p + 1, name + 1))
        return keyp->name;
      keyp = keyp->next;
    }

    return NULL;
  }
}
