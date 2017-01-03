/*
 * $Id: keyword.h,v 1.6 2009/11/18 07:57:28 ryo Exp $
 */

#define	HASHSIZE	127

struct hashroot {
	unsigned char usedch[256];
	struct hashleaf *hashs[HASHSIZE];
};

struct hashleaf {
	struct hashleaf *next;
	char *str;
	unsigned int len;
};


int iskeysym(struct hashroot *, int);
int iskeyword(struct hashroot *, char *, unsigned int);
int iskeyword16(struct hashroot *, unsigned short *, unsigned int);
void hash_destroy_root(struct hashroot *);

