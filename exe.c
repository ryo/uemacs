#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

void main(int argc, char *argv[])
{
	int i;
	int offset = 0;

	if (argc == 1) {
		printf("usage: exe [-n] command ...\n");
		exit(1);
	}

	if (argv[1][0] == '-') {
		if (argc == 2) {
			printf("usage: exe [-n] command ...\n");
			exit(1);
		}
		offset = atoi(&argv[1][1]);
		argc--;
		for(i = 1; i < argc; i++)
			argv[i] = argv[i + 1];
		argv[i] = NULL;
	}
	{
		char *begin, *end;
		int size;

		size = sizmem() * 4;
		begin = sbrk(0);
		end = sbrk(size);
		if (end == (char *)-1) {
			printf("??? sbrk error\n");
			exit(1);
		}
		end = sbrk(0);

		memset(begin, 0, end - begin);

		sbrk(-size);
	}
	{
		int p;

		p = (int)sbrk(0);
		if (p != (p & ~(4096 - 1)))
			sbrk(4096 - p & (4096 - 1));
	}
	sbrk(offset);

	execv(argv[1], &argv[1]);
}
