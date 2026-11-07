#include <gba_interrupt.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <gba_base.h>
#include <gba_dma.h>
#include <string.h>
#include <gba_systemcalls.h>

#include "ez_define.h"
#include "NORflash_OP.h"
#include "ezkernel.h"
#include "draw.h"
#include "lang.h"
#include "Ezcard_OP.h"
#include "GBApatch.h"
#include "nor_flash_status.h"
#include "nor_catalog_transaction.h"
//#define DEBUG

extern FM_NOR_FS pNorFS[MAX_NOR]EWRAM_BSS;
extern u8 pReadCache [MAX_pReadCache_size]EWRAM_BSS;
extern u32 gl_currentpage;
extern u32 gl_norOffset;
extern FIL gfile;
extern u32 game_total_NOR;
extern u32 iTrimSize;

typedef struct
{
	u32 art_path_slot;
	const char *art_path;
	u8 clear_art_paths;
} NOR_CatalogPersistContext;

static int NOR_PersistCatalog(void *context)
{
	NOR_CatalogPersistContext *persist = (NOR_CatalogPersistContext*)context;

	if(!persist)
		return 0;
	return Save_NOR_info_with_art_path((u16*)pNorFS,
		sizeof(FM_NOR_FS) * MAX_NOR, persist->art_path_slot,
		persist->art_path, persist->clear_art_paths);
}

static u8 NOR_EraseAllPPB(void)
{
	vu16 *flash = (vu16*)FlashBase_S98;
	u16 first, second;
	NOR_StatusDecision decision;
	u8 succeeded = 0;

	/* Enter the PPB command set and launch its global erase algorithm. */
	flash[0] = 0xF0;
	flash[0x555] = 0xAA;
	flash[0x2AA] = 0x55;
	flash[0x555] = 0xC0;
	flash[0] = 0x80;
	flash[0] = 0x30;

	/* The WS512P PPB algorithm requires at least 500 us before status reads. */
	delay(3000);
	for(;;)
	{
		/* The command selected the bank mapped at FlashBase_S98; poll its SA0. */
		first = flash[0];
		second = flash[0];
		decision = NOR_ClassifyPPBStatusPair(first, second);
		if(decision == NOR_STATUS_BUSY)
			continue;
		if(decision == NOR_STATUS_CONFIRM_DQ5)
		{
			first = flash[0];
			second = flash[0];
			decision = NOR_ClassifyDQ5ConfirmPair(first, second);
			if(decision == NOR_STATUS_OPERATION_FAILED)
				break;
		}
		else if(decision != NOR_STATUS_VERIFY_DATA)
			break;

		/* A cleared PPB reports DQ0=1 (unprotected). */
		succeeded = NOR_PPBStatusIsErased(flash[0]);
		break;
	}

	/* Always leave PPB mode, including DQ5 and PPB-status failure paths. */
	flash[0] = 0x90;
	flash[0] = 0x00;
	return succeeded;
}

#define NOR_CAPACITY_BYTES 0x04000000UL
#define NOR_WINDOW_BYTES   0x00800000UL
#define NOR_BLOCK_SIZE     0x00020000UL
#define NOR_MENU_WIDTH     168
#define NOR_MENU_CROP_Y    78
#define NOR_PROGRESS_X     72
#define NOR_PROGRESS_Y     104
#define NOR_PROGRESS_W     96
#define NOR_PROGRESS_H     8
#define NOR_PROGRESS_STEPS 16
#define NOR_VERIFY_PROGRESS_INTERVAL 0x00100000UL

static u32 NOR_ProgressStepCount(u32 count)
{
	return count > 64 ? NOR_PROGRESS_STEPS :
		(count * NOR_PROGRESS_STEPS) / 64;
}

static void NOR_RestoreStatusWindowBackground(void)
{
	const u16 *menu = (const u16*)gImage_MENU;
	u32 row;

	for(row = 30; row < 90; row++)
		DrawPic((u16*)(menu + row * NOR_MENU_WIDTH), 36, 25 + row,
			NOR_MENU_WIDTH, 1, 0, 0, 1);
}

static void NOR_DrawProgress(u32 count)
{
	u32 step;
	u32 filled = NOR_ProgressStepCount(count);
	u32 active_step = count % NOR_PROGRESS_STEPS;

	Clear(NOR_PROGRESS_X, NOR_PROGRESS_Y, NOR_PROGRESS_W, NOR_PROGRESS_H,
		gl_color_selectBG_sd, 1);
	for(step = 0; step < NOR_PROGRESS_STEPS; step++)
		if(step < filled && !(filled == NOR_PROGRESS_STEPS && step == active_step))
			Clear(NOR_PROGRESS_X + step * 6, NOR_PROGRESS_Y, 5,
				NOR_PROGRESS_H, gl_color_text, 1);
}

static u32 NOR_ShouldReportEraseProgress(u32 byte_offset)
{
	return (byte_offset & (NOR_VERIFY_PROGRESS_INTERVAL - 1)) ==
		(NOR_VERIFY_PROGRESS_INTERVAL - 2);
}

static void NOR_RestoreFormatProgressBackground(void)
{
	const u16 *menu = (const u16*)gImage_MENU;
	u32 row;

	// The menu art is 168 pixels wide; DrawPic uses its width as the source stride.
	for(row = 0; row < NOR_PROGRESS_H; row++)
	{
		DrawPic((u16*)(menu + (NOR_MENU_CROP_Y + row) * NOR_MENU_WIDTH),
			NOR_PROGRESS_X, NOR_PROGRESS_Y + row, NOR_PROGRESS_W, 1, 0, 0, 1);
	}
}

//---------------------------------------------------------------
void Chip_Reset()
{
	*((vu16 *)(FlashBase_S98)) = 0xF0 ;
}
//---------------------------------------------------------------
u8 Block_Erase(u32 blockAdd) //0x20000 BYTE pre block
{
	vu16 page;
	u32 Address;
	u32 loop;

	page=gl_currentpage;
	Address=blockAdd;
	while(Address>=0x800000)
	{
		Address-=0x800000;
		page+=0x1000;
	}
	//u8 ramdata = *((vu8 *)(0xE000000+1)) ;
	SetRompage(page);
	Chip_Reset();
	if((blockAdd==0) || (blockAdd==0x3FE0000))
	{
		//Address=blockAdd;
		for(loop=0;loop<0x20000;loop+=0x8000)
		{
			*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase_S98+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase_S98+Address+loop)) = 0x30 ;
			if(!Wait_Flash_Operation((vu16*)FlashBase_S98,
				(vu16*)(FlashBase_S98+Address+loop), 0xFFFF, 0, 0, 0))
			{
				SetRompage(gl_currentpage);
				return 0;
			}
			if(!Verify_Flash_Erased((vu16*)(FlashBase_S98+Address+loop),0x8000/2))
			{
				*((vu16*)FlashBase_S98) = 0xF0;
				SetRompage(gl_currentpage);
				return 0;
			}
		}
	}
	else
	{
		loop=Address;

		*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase_S98+0x555*2)) = 0x80 ;
		*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase_S98+loop)) = 0x30 ;
		if(!Wait_Flash_Operation((vu16*)FlashBase_S98,
			(vu16*)(FlashBase_S98+loop), 0xFFFF, 0, 0, 0))
		{
			SetRompage(gl_currentpage);
			return 0;
		}
		if(!Verify_Flash_Erased((vu16*)(FlashBase_S98+loop),NOR_BLOCK_SIZE/2))
		{
			*((vu16*)FlashBase_S98) = 0xF0;
			SetRompage(gl_currentpage);
			return 0;
		}
	}
	SetRompage(gl_currentpage);
	return 1;
}
//-----------------------------------------------------------
static void NOR_ChipEraseProgress(void *context)
{
	u32 *count = (u32*)context;
	VBlankIntrWait();
	VBlankIntrWait();
	ShowTime(NOR_list,0);
	NOR_RestoreFormatProgressBackground();
	NOR_DrawProgress(*count);
	(*count)++;
    VBlankIntrWait();
}

u8 Chip_Erase()
{
	u32 count=0;
	u32 page_index;
	u32 byte_offset;
	u16 old_ime = REG_IME;
	REG_IME = 0;
	*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
	*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase_S98+0x555*2)) = 0x80 ;
	*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
	*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase_S98+0x555*2)) = 0x10 ;
	if(!Wait_Flash_Operation((vu16*)FlashBase_S98, (vu16*)FlashBase_S98, 0xFFFF, 0,
		NOR_ChipEraseProgress, &count))
	{
		REG_IME = old_ime;
		return 0;
	}
	for(page_index=0;page_index<(NOR_CAPACITY_BYTES/NOR_WINDOW_BYTES);page_index++)
	{
		SetRompage((u16)(gl_currentpage + page_index*0x1000));
		for(byte_offset=0;byte_offset<NOR_WINDOW_BYTES;byte_offset+=2)
		{
			if(*((vu16*)(FlashBase_S98+byte_offset)) != 0xFFFF)
			{
				SetRompage(gl_currentpage);
				REG_IME = old_ime;
				return 0;
			}
			if(NOR_ShouldReportEraseProgress(byte_offset))
				NOR_ChipEraseProgress(&count);
		}
	}
	SetRompage(gl_currentpage);
	REG_IME = old_ime;
	return 1;
}

//-----------------------------------------------------------
u8 FormatNor()
{
	char msg[128];
	NOR_CatalogPersistContext persist_context = {0, 0, 1};
	sprintf(msg,"%s",gl_formatnor_info1);

	DrawHZText12(msg,0,60,90+13,gl_color_text,1);

	while(1)
	{
		delay(500);
		scanKeys();
		u16 keys = keysDown();
		if (keys & KEY_A) {
			u16 message_length = DrawText12VisibleLength(gl_formatnor_info2);
			u16 message_x = (240 - message_length * 6) / 2;
			NOR_RestoreStatusWindowBackground();
			// The status panel is only 168 pixels wide. Split the English message
			// into three centered lines so the full text stays inside its window.
			if(strcmp(gl_formatnor_info2,
				"Formatting, this may take some time. Please wait...") == 0)
			{
				DrawHZText12("Formatting, this may",0,57,46,gl_color_text,1);
				DrawHZText12("take some time.",0,75,60,gl_color_text,1);
				DrawHZText12("Please wait...",0,78,74,gl_color_text,1);
			}
			else
				DrawHZText12(gl_formatnor_info2, message_length, message_x, 82,
					gl_color_text, 1);
			NOR_DrawProgress(0);
			if(!Chip_Erase())
				return 2;
			if(!NOR_ClearCatalogAndPersist(pNorFS,
				sizeof(FM_NOR_FS)*MAX_NOR, NOR_PersistCatalog, &persist_context))
				return 3;
			return 1;
		}
		else if(keys & KEY_B) {
			// Consume the cancel press and its release here so NOR_list_MENU
			// cannot interpret the same B input as its own back action.
			while(keysHeld() & KEY_B)
			{
				VBlankIntrWait();
				scanKeys();
			}
			scanKeys();
			(void)keysDown();
			(void)keysUp();
			(void)keysDownRepeat();
			return 0;
		}
	}
}
//---------------------------------------------------------------
u8 WriteFlash(u32 address,u8 *buffer,u32 size)
{
	vu16 page;
	register u32 loopwrite ;
	vu16* buf = (vu16*)buffer ;

	page=gl_currentpage;
	while(address>=0x800000)
	{
		address-=0x800000;
		page+=0x1000;
	}
	SetRompage(page);

	Chip_Reset();
	for(loopwrite=0;loopwrite<(size/2);loopwrite++)
	{
		*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase_S98+0x555*2)) = 0xA0 ;
		*((vu16 *)(FlashBase_S98+address+loopwrite*2)) = buf[loopwrite];
		if(!Wait_Flash_Operation((vu16*)FlashBase_S98,
			(vu16*)(FlashBase_S98+address+loopwrite*2), buf[loopwrite], 0, 0, 0))
		{
			SetRompage(gl_currentpage);
			return 0;
		}
	}
	SetRompage(gl_currentpage);
	return 1;
}
//---------------------------------------------------------------
u8 WriteFlash_with32word(u32 address,u8 *buffer,u32 size)
{
	vu16 page;
	register u32 loopwrite ;
	vu16* buf = (vu16*)buffer ;
	u32 i;

	page=gl_currentpage;
	while(address>=0x800000)
	{
		address-=0x800000;
		page+=0x1000;
	}
	SetRompage(page);

	Chip_Reset();
	for(loopwrite=0;loopwrite<(size/32);loopwrite++)
	{
		*((vu16 *)(FlashBase_S98+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase_S98+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase_S98+address+loopwrite*32)) = 0x25;
		*((vu16 *)(FlashBase_S98+address+loopwrite*32)) = 15;
		delay(10);
		for(i=0;i<=15;i++)
		{
			*((vu16 *)(FlashBase_S98+address+loopwrite*32 +2*i )) = buf[loopwrite*16+i];
		}
		*((vu16 *)(FlashBase_S98+address+loopwrite*32)) = 0x29;

		delay(40);
		if(!Wait_Flash_Operation((vu16*)FlashBase_S98,
			(vu16*)(FlashBase_S98+address+loopwrite*32+0xF*2),
			buf[loopwrite*16+15], 1, 0, 0))
		{
			SetRompage(gl_currentpage);
			return 0;
		}
		if(!Verify_Flash_Data((vu16*)(FlashBase_S98+address+loopwrite*32),
			&buf[loopwrite*16],16))
		{
			*((vu16*)FlashBase_S98) = 0xF0;
			SetRompage(gl_currentpage);
			return 0;
		}
	}
	SetRompage(gl_currentpage);
	return 1;
}
//-----------------------------------------------------------
u32 Loadfile2NOR(TCHAR *filename, const TCHAR *rom_dir, u32 NORaddress,
	u16 have_patch,u8 SAVEMODE)
{
	u32 res;
	u32 result;
	UINT ret;
	u32 filesize;
	u32 fileneedsize;
	u32 blocknum;
	u32 read_size;
	u32 file_open;
	char msg[128];
	char source_path[MAX_path_len * 2 + 2];
	FM_NOR_FS tmpNorFS;
	FM_NOR_FS previousNorFS;
	NOR_CatalogPersistContext persist_context;
	char flag[4];
	u16 flag_64MBrom;
	u16 norid;
	u32 add_patch = 0;

	if(filename == NULL || (NORaddress % NOR_BLOCK_SIZE) != 0)
		return NOR_WRITE_RESULT_FILE_ERROR;
	if(game_total_NOR >= MAX_NOR || NORaddress >= NOR_CAPACITY_BYTES)
		return NOR_WRITE_RESULT_FULL;
	if(filename[0] == '/')
	{
		if(strlen(filename) >= sizeof(source_path))
			return NOR_WRITE_RESULT_FILE_ERROR;
		strcpy(source_path, filename);
	}
	else if(rom_dir && rom_dir[0])
	{
		size_t dir_length = strlen(rom_dir);
		int written = snprintf(source_path, sizeof(source_path), "%s%s%s",
			rom_dir, rom_dir[dir_length - 1] == '/' ? "" : "/", filename);
		if(written <= 0 || (size_t)written >= sizeof(source_path))
			return NOR_WRITE_RESULT_FILE_ERROR;
	}
	else
	{
		if(strlen(filename) >= sizeof(source_path))
			return NOR_WRITE_RESULT_FILE_ERROR;
		strcpy(source_path, filename);
	}

	norid = Read_S98NOR_ID();
	if(norid != 0x223D)//S98
		return NOR_WRITE_RESULT_DEVICE_ERROR;

	res = f_open(&gfile, source_path, FA_READ);
	if(res != FR_OK)
		return NOR_WRITE_RESULT_FILE_ERROR;
	file_open = 1;
	result = NOR_WRITE_RESULT_FILE_ERROR;

	filesize = f_size(&gfile);
	if(filesize < 0xB4)
		goto close_file;
	if(filesize > NOR_CAPACITY_BYTES)
	{
		result = NOR_WRITE_RESULT_FULL;
		goto close_file;
	}

	memset(&tmpNorFS, 0, sizeof(tmpNorFS));
	if(f_lseek(&gfile, 0xA0) != FR_OK ||
	   f_read(&gfile, tmpNorFS.gamename, sizeof(tmpNorFS.gamename), &ret) != FR_OK ||
	   ret != sizeof(tmpNorFS.gamename))
		goto close_file;
	if(f_lseek(&gfile, 0xB0) != FR_OK ||
	   f_read(&gfile, flag, sizeof(flag), &ret) != FR_OK ||
	   ret != sizeof(flag))
		goto close_file;

	flag_64MBrom = (memcmp(flag, "5G", 2) == 0) ? 1 : 0;
	tmpNorFS.rompage = NORaddress / NOR_BLOCK_SIZE;

	fileneedsize = ((filesize + NOR_BLOCK_SIZE - 1) / NOR_BLOCK_SIZE) * NOR_BLOCK_SIZE;
	if(have_patch && iTrimSize >= fileneedsize)
	{
		fileneedsize += NOR_BLOCK_SIZE;
		add_patch = 1;
	}
	if(fileneedsize > (NOR_CAPACITY_BYTES - NORaddress))
	{
		result = NOR_WRITE_RESULT_FULL; //Not enough NOR space or game slots
		goto close_file;
	}

	if(strlen(filename) >= sizeof(tmpNorFS.filename))
		goto close_file;
	strcpy(tmpNorFS.filename, filename);
	tmpNorFS.filesize = fileneedsize;
	tmpNorFS.have_patch = have_patch;
	tmpNorFS.have_RTS = gl_rts_on;
	tmpNorFS.is_64MBrom = flag_64MBrom;
	tmpNorFS.savemode = SAVEMODE;

	if(!NOR_EraseAllPPB())
	{
		result = NOR_WRITE_RESULT_ERASE_ERROR;
		goto close_file;
	}

	Clear(60,160-15,120,15,gl_color_cheat_black,1);
	DrawHZText12(gl_writing,0,70,160-15,0x7fff,1);
	for(blocknum=0;blocknum<filesize;blocknum+=NOR_BLOCK_SIZE)
	{
		sprintf(msg,"%luMb/%luMb",(blocknum)/NOR_BLOCK_SIZE,filesize/NOR_BLOCK_SIZE);
		Clear(70+54,160-15,100,15,gl_color_cheat_black,1);
		DrawHZText12(msg,0,70+54,160-15,0x7fff,1);

		read_size = filesize - blocknum;
		if(read_size > NOR_BLOCK_SIZE)
			read_size = NOR_BLOCK_SIZE;
		if(f_lseek(&gfile, blocknum) != FR_OK)
			goto close_file;
		memset(pReadCache, 0xFF, NOR_BLOCK_SIZE);
		if(f_read(&gfile, pReadCache, (UINT)read_size, &ret) != FR_OK || ret != read_size)
			goto close_file;

		if(have_patch){
			if((gl_reset_on==1) || (gl_rts_on==1) || (gl_sleep_on==1) || (gl_cheat_on==1))
			{
				PatchInternal((u32*)pReadCache,NOR_BLOCK_SIZE,blocknum);
				GBApatch_NOR((u32*)pReadCache,NOR_BLOCK_SIZE,blocknum);//some nes need check
			}
		}
		else{
			GBApatch_Cleanrom_NOR((u32*)pReadCache,blocknum);
		}

		if(!Block_Erase(blocknum+NORaddress))
		{
			result = NOR_WRITE_RESULT_ERASE_ERROR;
			goto close_file;
		}
		if(!WriteFlash_with32word(blocknum+NORaddress,pReadCache,NOR_BLOCK_SIZE))
		{
			result = NOR_WRITE_RESULT_PROGRAM_ERROR;
			goto close_file;
		}
	}

	if(f_close(&gfile) != FR_OK)
	{
		file_open = 0;
		return NOR_WRITE_RESULT_FILE_ERROR;
	}
	file_open = 0;

	if(have_patch && add_patch)
	{
		if(!Block_Erase(blocknum+NORaddress))
			return NOR_WRITE_RESULT_ERASE_ERROR;
		GBApatch_NOR((u32*)pReadCache,NOR_BLOCK_SIZE,blocknum);
		if(!WriteFlash_with32word(blocknum+NORaddress,pReadCache,NOR_BLOCK_SIZE))
			return NOR_WRITE_RESULT_PROGRAM_ERROR;
	}

	// Publish the new slot only while persistence sees it; restore RAM on failure.
	persist_context.art_path_slot = game_total_NOR;
	persist_context.art_path = rom_dir;
	persist_context.clear_art_paths = 0;
	if(!NOR_CommitCatalogSlot(&pNorFS[game_total_NOR], &tmpNorFS, &previousNorFS,
		sizeof(FM_NOR_FS), NOR_PersistCatalog, &persist_context))
		return NOR_WRITE_RESULT_CATALOG_ERROR;
	return NOR_WRITE_RESULT_OK;

close_file:
	if(file_open)
		(void)f_close(&gfile);
	return result;
}
//-----------------------------------------------------------
u32 GetFileListFromNor(void)
{
	u16 old_ime;
	u32 page=0 ,count=0;
	u32 StartAddress = FlashBase_S98;
	//FM_NOR_FS tmpNorFS ;
	char temp[50];
	vu16 Value, x24;

	old_ime = REG_IME;
	REG_IME = 0;

	Value = *(vu16 *)(StartAddress + 0xbe);
	x24 = *(vu16 *)(StartAddress + 0x6);

	//DEBUG_printf(" %x %x %x",StartAddress,Value,x24);

	while( ((Value&0xFF)==0xCE) || ((Value&0xFF)==0xCF)|| ((Value&0xFF)==0x00)|| (x24==0x51ae))
	{
		if(count >= MAX_NOR)
			break;

		//DEBUG_printf(" %x %x %x",StartAddress,Value,x24);
		if(*(vu8 *)(StartAddress+0xb2) == 0x96)
		{
			if(gl_norOffset > NOR_CAPACITY_BYTES ||
			   pNorFS[count].filesize < NOR_BLOCK_SIZE ||
			   (pNorFS[count].filesize % NOR_BLOCK_SIZE) != 0 ||
			   pNorFS[count].filesize > NOR_CAPACITY_BYTES - gl_norOffset)
				break;

			memcpy(temp,(char*)(StartAddress+0xa0),0x10);
			//temp[12] = 0 ;

			//DEBUG_printf("  %s VS %s",temp ,pNorFS[count].gamename);

			if(memcmp(temp,pNorFS[count].gamename,0x10) ==0) //if(!strcasecmp(temp, pNorFS[count].gamename))
			{
				gl_norOffset += pNorFS[count].filesize;
				StartAddress += pNorFS[count].filesize;
				count ++ ;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}

		while(StartAddress >= FlashBase_S98_end)
		{
			page += 0x1000 ;
			if(page>0x7000)
				goto scan_done;
			SetRompage(gl_currentpage+page);
			StartAddress -= 0x800000 ;
		}

		Value = *(vu16 *)(StartAddress + 0xbe);
		x24 = *(vu16 *)(StartAddress + 0x6);
	}
scan_done:
	SetRompage(gl_currentpage);
	REG_IME = old_ime;
	return count;
}
//-----------------------------------------------------------
