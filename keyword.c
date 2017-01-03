/*
 * $Id: keyword.c,v 1.19 2017/01/02 15:17:50 ryo Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "keyword.h"

static void hash_store_keyword(struct hashroot *,  char *);
static void hash_destroy_leaves(struct hashleaf *);
static struct hashleaf *hash_create_leaf(void);
static struct hashroot *hash_create_root(void);
static unsigned int makehash(char *, unsigned int);
static struct hashroot *load_keywords(char *);
static char *l_strdup(const char *);

static unsigned int
makehash(char *str, unsigned int len)
{
	unsigned int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0;
	unsigned int value;


	switch (len) {
	default:
		c5 = str[len - 1]-0x20;
	case 5:
		c4 = str[4]-0x20;
	case 4:
		c3 = str[3]-0x20;
	case 3:
		c2 = str[2]-0x20;
	case 2:
	case 1:
		c1 = str[0]-0x20;
	case 0:
		break;
	}

	value = c1 * 11 + c2 * 13 + c3 * 17 + c4 * 19 + c5 * 23 + len * 29;

	return value % HASHSIZE;
}


int
iskeysym(struct hashroot *hashroot, int ch)
{
	if (ch < 0)
		ch = 256 - ch;

	if (ch < 0x20 || ch >= 0x7f)
		return 0;

	if (hashroot)
		return hashroot->usedch[ch];
	else
		return 0;
}


static struct hashroot *
hash_create_root(void)
{
	struct hashroot *root;
	root = (void*)malloc(sizeof(struct hashroot));
	if (root)
		memset(root, 0, sizeof(struct hashroot));

	return root;
}


struct hashleaf *
hash_create_leaf(void)
{
	struct hashleaf *leaf;
	leaf = (void*)malloc(sizeof(struct hashleaf));
	if (leaf)
		memset(leaf, 0, sizeof(struct hashleaf));
	return leaf;
}

static void
hash_destroy_leaves(struct hashleaf *leaf)
{
	if (!leaf)
		return;

	if (leaf->next) {
		hash_destroy_leaves(leaf->next);
		leaf->next = NULL;
	}

	if (leaf->str)
		free(leaf->str);

	free(leaf);
}

void
hash_destroy_root(struct hashroot *root)
{
	int i;

	if (!root)
		return;

	for (i = 0; i < HASHSIZE; i++) {
		hash_destroy_leaves(root->hashs[i]);
	}
	free(root);
}


static char *
l_strdup(const char *str)
{
	char *p;
	unsigned int len;

	len = strlen(str) + 1;
	if ((p = malloc(len))) {
		memcpy(p, str, len);
		return p;
	}
	return NULL;
}



static void
hash_store_keyword(struct hashroot *root, char *str)
{
	struct hashleaf *leaf;
	unsigned char *ustr;
	unsigned int hashvalue;
	int i;

	leaf = hash_create_leaf();
	if (!leaf)
		return;

	leaf->str = (void *)l_strdup(str);
	if (!leaf->str) {
		free(leaf);
		return;
	}
	leaf->len = strlen(str);
	hashvalue = makehash(leaf->str, leaf->len);

	leaf->next = root->hashs[hashvalue];
	root->hashs[hashvalue] = leaf;

	ustr = (unsigned char *)str;
	for (i = 0; i < leaf->len; i++) {
		root->usedch[ustr[i]] = 1;
	}

	return;	
}


int
storekeyword(int f, int n)
{
	int status;
	char buf[NPAT];

	if ((status = mlreply(TEXT220, buf, NPAT - 1)) != TRUE)
		/* "What keyword to append: " */
		return status;

	if (!curbp->b_hashroot) {
		struct hashroot *myroot = hash_create_root();
		if (!myroot) {
			return FALSE;
		}
		curbp->b_hashroot = myroot;
	}

	if (iskeyword(curbp->b_hashroot, buf, strlen(buf)))
		return FALSE;

	hash_store_keyword(curbp->b_hashroot, buf);
	updall(curwp);
	return TRUE;
}







static struct hashroot *
load_keywords(char *fname)
{
#define	TMPBUFSIZE	1024
	char tmp[TMPBUFSIZE];
	int s;
	struct hashroot *myroot = 0;

	if ((s = ffropen(fname)) == FIOERR) {
		/* file open error */

	} else if (s == FIOFNF) {	/* File not found.	 */
		mlwrite(TEXT152);
		/* "[No such file]" */

	} else {
		myroot = hash_create_root();
		if (!myroot) {
			ffclose();
			return 0;
		}

		while ((s = ffgetline()) >= 0) {
			strncpy(tmp, fline, sizeof(tmp));
			chomp(tmp);
			if (tmp[0])
				hash_store_keyword(myroot, tmp);
		}
		ffclose();
	}

	return myroot;
}


int
iskeyword16(struct hashroot *hashroot, unsigned short *key16, unsigned int len)
{
	char *key;
	char *p;
	unsigned int i;
	int rc;

	if (len == 0)
		return 0;
	key = malloc(len + 1);
	if (key == NULL)
		return 0;

	p = key;
	for (i = 0; i < len; i++) {
		*p++ = *key16++;
	}

	rc = iskeyword(hashroot, key, len);
	free(key);

	return rc;
}

int
iskeyword(struct hashroot *hashroot, char *key, unsigned int len)
{
	unsigned int hashvalue;
	struct hashleaf *leaf;

	if (hashroot) {
		hashvalue = makehash(key, len);
		leaf = hashroot->hashs[hashvalue];

		while (leaf) {
			if (len == leaf->len && strncmp(leaf->str, key, len) == 0)
				return 1;

			leaf = leaf->next;
		}
	}
	return 0;
}


int
loademp(int f, int n)
{
	char *fname;

	if (restflag)		/* don't allow this command if restricted */
		return resterr();

	if ((fname = gtfilename("Emphasis text")) == NULL)
		return FALSE;

	if (curbp->b_hashroot)
		hash_destroy_root(curbp->b_hashroot);

	if ((curbp->b_hashroot = load_keywords(fname))) {
		updall(curwp);
		return TRUE;
	} else {
		return FALSE;
	}
}

