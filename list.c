#include "dzip.h"
#include "dzipcon.h"

void dzList (char *src)
{
	int i;
	direntry_t *de;

	if (!dzOpen(src, 0))
		return;

	printf("contents of %s, created using version %u.%u:\n",
		src, maj_ver, min_ver);
	de = directory;
	for (i = 0; i < numfiles; i++, de++)
	{
		if (de->type == TYPE_DIR) 
		{
			printf(" %s\n", de->name);
			continue;
		}

		if (de->pak && de->type != TYPE_PAK)
		{
			printf("  %-17s size: %-8u packed: %-8u\n", 
				de->name, de->real, de->size);
			continue;
		}
		printf(" %-18s size: %-8u packed: %-8u", 
			de->name, de->real, de->size);
		if (maj_ver != 1)
			printf(" %02u.%02u.%4u %02u:%02u:%02u",
				(de->date >> 16) & 0x1f,
				((de->date >> 21) & 0x0f) + 1,
				((de->date >> 25) & 0x7f) + 1980,
				(de->date >> 11) & 0x1f,
				(de->date >> 5) & 0x3f, (de->date & 0x1f) << 1);
		printf("\n");
	}
	printf("\n");
	dzClose();
}
