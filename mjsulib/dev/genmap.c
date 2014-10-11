/* genmap: emit a random-mapping byte table to stdout.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
	{
	unsigned char h[256];
	unsigned int i, j, x;

	for (i = 0; i < 256; ++i)
		h[i] = i;

	srand(21863);
	for (i = 0; i < 256; ++i)
		{
		j = rand() & 0xFF;
		x = h[i];
		h[i] = h[j];
		h[j] = x;
		}

	printf("/* a random mapping of 8-bit values:\n */\n");
	printf("static unsigned char hashmap[256] =\n");
	printf("\t{\n");
	for (i = 0; i < 256; ++i)
		{
		if (h[i] == i)
			fprintf(stderr, "warning: cell %i not swapped\n", i);
		if (!(i % 16))
			printf("\t");
		printf("%3u", h[i]);
		if (i < 255)
			printf(",");
		if ((i % 16) == 15)
			printf("\n");
		}
	printf("\t};\n");
	return (0);
	}
