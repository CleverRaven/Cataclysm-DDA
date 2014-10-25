#include <stdio.h>

int djb2_hash(unsigned char *str)
{
	unsigned long hash = 5381;
	unsigned char c = *str++;
	while (c != '\0') {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		c = *str++;
	}
	return hash;
}

int main()
{
		char djb2_char[1000];
		fread( djb2_char, 1, 1000, stdin );
		printf("%u",djb2_hash((unsigned char *)djb2_char));
}