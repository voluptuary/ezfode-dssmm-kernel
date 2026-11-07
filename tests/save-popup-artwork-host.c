#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned int u32;

#include SAVE_ARTWORK_HELPERS_FILE

static void put32(u8 *p, u32 value)
{
	memcpy(p, &value, sizeof(value));
}

static void make_header(u8 *bmp, u32 width, u32 height)
{
	memset(bmp, 0, 54);
	bmp[0] = 'B';
	bmp[1] = 'M';
	put32(bmp + 10, 54);
	put32(bmp + 14, 40);
	put32(bmp + 18, width);
	put32(bmp + 22, height);
	bmp[26] = 1;
	bmp[28] = 16;
}

static void expect(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

int main(void)
{
	u8 bmp[54];

	make_header(bmp, 80, 80);
	expect(Launcher_SaveArtworkFormatValid(bmp, 80, 80),
		"box artwork BMP is accepted");
	put32(bmp + 22, 0u - 80u);
	expect(Launcher_SaveArtworkFormatValid(bmp, 80, 80),
		"top-down box artwork BMP is accepted");

	make_header(bmp, 120, 80);
	expect(!Launcher_SaveArtworkFormatValid(bmp, 80, 80),
		"title artwork is rejected for the box-art popup");
	make_header(bmp, 80, 79);
	expect(!Launcher_SaveArtworkFormatValid(bmp, 80, 80),
		"incorrectly sized artwork is rejected");
	make_header(bmp, 80, 80);
	bmp[28] = 24;
	expect(!Launcher_SaveArtworkFormatValid(bmp, 80, 80),
		"non-16-bit artwork is rejected");
	expect(!Launcher_SaveArtworkFormatValid(NULL, 80, 80),
		"null artwork is rejected");

	puts("Save popup box-art format scenarios passed.");
	return 0;
}
