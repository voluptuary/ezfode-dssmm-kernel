#ifndef NOR_WRITE_RESULT_H
#define NOR_WRITE_RESULT_H

enum
{
	NOR_WRITE_RESULT_OK = 0,
	NOR_WRITE_RESULT_FILE_ERROR = 1,
	NOR_WRITE_RESULT_FULL = 2,
	NOR_WRITE_RESULT_DEVICE_ERROR = 3,
	NOR_WRITE_RESULT_ERASE_ERROR = 4,
	NOR_WRITE_RESULT_PROGRAM_ERROR = 5,
	NOR_WRITE_RESULT_CATALOG_ERROR = 6
};

static inline unsigned char NOR_WriteResultErrorNumber(unsigned int result)
{
	switch(result)
	{
		case NOR_WRITE_RESULT_FILE_ERROR: return 1;
		case NOR_WRITE_RESULT_DEVICE_ERROR: return 7;
		case NOR_WRITE_RESULT_ERASE_ERROR: return 8;
		case NOR_WRITE_RESULT_PROGRAM_ERROR: return 9;
		case NOR_WRITE_RESULT_CATALOG_ERROR: return 10;
		default: return 0;
	}
}

#endif
