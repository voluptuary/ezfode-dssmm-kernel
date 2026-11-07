#include <stdio.h>
#include <stdlib.h>

#include "nor_write_result.h"

static void check(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

int main(void)
{
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_OK) == 0,
		"successful writes do not display an error");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_FULL) == 0,
		"capacity results continue through the dedicated NOR-full message");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_FILE_ERROR) == 1,
		"source file failures use the file error message");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_DEVICE_ERROR) == 7,
		"unsupported NOR devices use a device-specific message");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_ERASE_ERROR) == 8,
		"NOR erase failures use the NOR erase message");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_PROGRAM_ERROR) == 9,
		"NOR program failures use the NOR program message");
	check(NOR_WriteResultErrorNumber(NOR_WRITE_RESULT_CATALOG_ERROR) == 10,
		"catalog persistence failures use the catalog message");
	puts("NOR write result scenarios passed.");
	return 0;
}
