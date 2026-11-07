/* Reuse the real in-memory FatFs fixture; counters are injected only in its
   temporary host copy of ff.c, never in firmware or on the target. */
#define main existing_long_name_tests
#include "fatfs-long-name-host.c"
#undef main
unsigned long metric_dir_find, metric_dir_next, metric_f_open, metric_f_open_by_dirent, metric_f_read;
static void reset_metrics(void)
{
    metric_dir_find=metric_dir_next=metric_f_open=metric_f_open_by_dirent=metric_f_read=0;
}
static unsigned long open_case(const char *directory, unsigned n, int missing)
{
    char path[180]; FIL file; FRESULT result; UINT got; char bytes[3];
    snprintf(path,sizeof(path),"%s/Game Collection Item %03u.gba",directory,missing ? n+9 : n-1);
    reset_metrics();
    result=f_open(&file,path,FA_READ);
    if(missing) check(result==FR_NO_FILE,"missing file stays absent");
    else
    {
        check(result==FR_OK,"late-position existing file opens");
        check(f_read(&file,bytes,sizeof(bytes),&got)==FR_OK && got==3 && !memcmp(bytes,"rom",3),"opened identity has expected payload");
        check(f_close(&file)==FR_OK,"close measured file");
    }
    check(metric_f_open==1,"one measured open");
    printf("FatFs N=%u %s: opens=%lu reads=%lu dir_find=%lu dir_next=%lu\n",n,missing?"missing":"last ordinary LFN",metric_f_open,metric_f_read,metric_dir_find,metric_dir_next);
    return metric_dir_next;
}
static unsigned long indexed_open_case(const char *directory, unsigned n)
{
    DIR dir; FILINFO info; FIL file; DWORD offset=0xFFFFFFFF; DWORD size=0;
    char alias[FF_SFN_BUF+1]={0}; char path[180]; char bytes[3]; UINT got=0;
    unsigned target=n-1; int found=0; FRESULT result; unsigned long fast_dir_next;
    snprintf(path,sizeof(path),"%s/Game Collection Item %03u.gba",directory,target);
    check(f_opendir(&dir,directory)==FR_OK,"open directory to capture indexed file offset");
    while(f_readdir_with_offset(&dir,&info,&offset)==FR_OK && info.fname[0])
    {
        if(strcmp(info.fname,strrchr(path,'/')+1)==0)
        {
            size=info.fsize;
            strncpy(alias,info.altname[0]?info.altname:info.fname,sizeof(alias)-1);
            alias[sizeof(alias)-1]=0;
            found=1;
            break;
        }
    }
    check(f_closedir(&dir)==FR_OK && found,"capture target file offset and SFN");
    check(offset!=0xFFFFFFFF,"FAT readdir returns the current SFN entry offset");
    reset_metrics();
    result=f_open_by_dirent(&file,directory,offset,alias,size);
    check(result==FR_OK,"open indexed FAT entry");
    check(f_read(&file,bytes,sizeof(bytes),&got)==FR_OK && got==3 && !memcmp(bytes,"rom",3),
        "indexed open reads the same ROM payload");
    check(f_close(&file)==FR_OK,"close indexed ROM file");
    check(metric_f_open_by_dirent==1,"one indexed open call");
    check(metric_dir_next<20,"indexed open avoids scanning the large title directory");
    fast_dir_next=metric_dir_next;
    printf("FatFs N=%u validated indexed open: opens=%lu dir_find=%lu dir_next=%lu\n",
        n,metric_f_open_by_dirent,metric_dir_find,fast_dir_next);

    /* Stale or mismatched hints fail closed; the normal path still opens the
       requested object and remains the production fallback. */
    reset_metrics();
    check(f_open_by_dirent(&file,directory,offset,"WRONG.GBA",size)!=FR_OK,
        "mismatched SFN hint is rejected");
    check(f_open(&file,path,FA_READ)==FR_OK,"ordinary path fallback opens after a stale hint");
    check(f_close(&file)==FR_OK,"close path-fallback file");
    check(f_open_by_dirent(&file,directory,offset,alias,size+1)!=FR_OK,
        "changed-size hint is rejected");
    return fast_dir_next;
}

int main(void)
{
    const unsigned counts[]={30,60,90,120,240};
    unsigned i, entries; unsigned long prev_hit=0,prev_miss=0,hit,miss,indexed;
    char directory[24];
    memset(disk,0,sizeof(disk)); format_disk();
    for(i=0;i<sizeof(counts)/sizeof(counts[0]);i++)
    {
        snprintf(directory,sizeof(directory),"0:/SCALE%u",counts[i]);
        entries=run_indexed_name_performance_case(counts[i],directory);
        printf("indexed N=%u: complete names 0 resolver scans; clipped scan=%u returned entries\n",counts[i],entries);
        hit=open_case(directory,counts[i],0); miss=open_case(directory,counts[i],1);
        indexed=indexed_open_case(directory,counts[i]);
        check(hit>prev_hit && miss>prev_miss,"ordinary directory traversal grows with fixture size");
        check(indexed<20,"indexed open remains bounded as the title directory grows");
        prev_hit=hit; prev_miss=miss;
    }
    puts("Scaling evidence only: dir_next counts include LFN slots/parents, not SD latency or frame time. No per-move simulation.");
    return 0;
}
