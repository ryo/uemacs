#include <stdio.h>
#include <stdlib.h>

#define DIFF_SIZE 0x18000

volatile void read_fail(char *name)
{
	printf("Read error : %s\n", name);
	exit(1);
}

volatile void write_fail(char *name)
{
	printf("Write error : %s\n", name);
	exit(1);
}

volatile void no_memory(void)
{
	printf("Out of memory.");
	exit(1);
}

volatile void illegal_difference(void)
{
	printf("ロードアドレスの差が %06x ではありません\n", DIFF_SIZE);
	exit(1);
}

void main(int argc, char *argv[])
{
	void *src1, *src2;
	unsigned int offset_table_sz;
	unsigned int file_length;
	unsigned int exec_adr1, exec_adr2;
	unsigned int base_adr1, base_adr2;
	unsigned char *offset_table;

	if (argc != 4) {
		printf("usage: makex <dump-file-1> <dump-file-2> <out-file>\n");
		exit(1);
	}

	/*
	 *	read <dump-file-1> to src1,
	 *	read <dump-file-2> to src2,
	 *	and read exec_adr1, base_adr1 form <dump-file-1>
	 */
	{
		FILE *fp1, *fp2;

		fp1 = fopen(argv[1], "rb");
		if (fp1 == NULL) {
			printf("Can't open : %s\n", argv[1]);
			exit(1);
		}
		fp2 = fopen(argv[2], "rb");
		if (fp2 == NULL) {
			printf("Can't open : %s\n", argv[2]);
			exit(1);
		}

		fseek(fp1, 0, SEEK_END);
		fseek(fp2, 0, SEEK_END);
		if (ftell(fp1) != ftell(fp2)) {
			printf("Unmatch file length.\n");
			exit(1);
		}
		file_length = ftell(fp1) - 8;

		fseek(fp1, 0, SEEK_SET);
		if (fread(&base_adr1, 4, 1, fp1) != 1)
			read_fail(argv[1]);
		if (fread(&exec_adr1, 4, 1, fp1) != 1)
			read_fail(argv[1]);

		fseek(fp2, 0, SEEK_SET);
		if (fread(&base_adr2, 4, 1, fp2) != 1)
			read_fail(argv[2]);
		if (fread(&exec_adr2, 4, 1, fp2) != 1)
			read_fail(argv[2]);

		printf("%s : base adr = %6x\n", argv[1], base_adr1);
		printf("%s : base adr = %6x\n", argv[2], base_adr2);

		if (base_adr2 - base_adr1 != DIFF_SIZE)
			illegal_difference();

		src1 = malloc(file_length);
		src2 = malloc(file_length);
		if (src1 == NULL || src2 == NULL)
			no_memory();

		if (fread(src1, file_length, 1, fp1) != 1)
			read_fail(argv[1]);
		if (fread(src2, file_length, 1, fp2) != 1)
			read_fail(argv[1]);

		fclose(fp1);
		fclose(fp2);
	}

	offset_table = malloc(4);
	offset_table_sz = 0;

	{
		unsigned int prev_diff_point = 0;
		unsigned int offset;
		unsigned int i;
		int diff_count = 0;
		int check = 1;
		unsigned short *s1, *s2;

		s1 = src1;
		s2 = src2;
		s1++;
		s2++;
		for(i = 2; i < file_length; i += 2, s1++, s2++) {
			if (*s1 != *s2) {
				unsigned long *ss1, *ss2;

				ss1 = (unsigned long *)&s1[-1];
				ss2 = (unsigned long *)&s2[-1];
				if (*ss2 - *ss1 == DIFF_SIZE && (*ss1 >= base_adr1 && *ss1 <= base_adr1 + file_length)) {
					offset = (i - 2) - prev_diff_point;
					if (offset < 0x10000) {
						offset_table = realloc(offset_table, offset_table_sz + 4 + 2);
						if (offset_table == NULL)
							no_memory();
						{
							unsigned short *p;

							p = (unsigned short *)&offset_table[offset_table_sz];
							*p = offset;
						}
						offset_table_sz += 2;
					} else {
						offset_table = realloc(offset_table, offset_table_sz + 4 + 6);
						if (offset_table == NULL)
							no_memory();
						{
							unsigned short *p;

							p = (unsigned short *)&offset_table[offset_table_sz];
							*p = 1;
						}
						{
							unsigned int *p;

							p = (unsigned int *)&offset_table[offset_table_sz + 2];
							*p = offset;
						}
						offset_table_sz += 6;
					}

					diff_count++;
					*ss1 -= base_adr1;
					prev_diff_point = i - 2;
					check = 0;
				} else {
					check++;
					if (check == 2) {
#if 0
						printf("Ummm? illegal data at %8x.\n", i);
#endif
						check = 0;
					}
				}
			} else
				check = 0;
		}
	}

	{
		FILE *fp;
		unsigned int header[16];
		int i;

		for(i = 0; i < 16; i++)
			header[i] = 0;

		header[0] = 0x48550000;		/* 'HU' */
		header[2] = exec_adr1;
		header[3] = file_length;
		header[6] = offset_table_sz;

		fp = fopen(argv[3], "wb");
		if (fp == NULL) {
			printf("Can't create : %s\n", argv[3]);
			exit(1);
		}

		if (fwrite(header, 64, 1, fp) != 1)
			write_fail(argv[3]);
		if (fwrite(src1, file_length, 1, fp) != 1)
			write_fail(argv[3]);
		if (fwrite(offset_table, offset_table_sz, 1, fp) != 1)
			write_fail(argv[3]);

		fclose(fp);
	}

		printf("リロケートテーブルのサイズは %d byte です\n", offset_table_sz);
}
