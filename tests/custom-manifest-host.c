/* Executes extracted production collection/sort/lookup with deterministic I/O. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t u32;
#define LAUNCHER_CUSTOM_THUMB_MANIFEST_MAX 256
#define LAUNCHER_THUMB_STYLE_BOX 1
#define LAUNCHER_THUMB_STYLE_TITLE 0
#define AM_DIR 16
#define FR_OK 0
static unsigned char launcher_custom_thumb_manifest_loaded[2];
static unsigned char launcher_custom_thumb_manifest_present[2];
static unsigned short launcher_custom_thumb_manifest_count[2];
static u32 launcher_custom_thumb_manifest_hash[2][256];
static unsigned launcher_thumbnail_style;
static int launcher_custom_thumb_scan_dir;
static struct { char fname[32]; unsigned fattrib; } launcher_custom_thumb_scan_info;
static char launcher_custom_thumb_scan_path[80], launcher_custom_thumb_scan_name[40];
static unsigned sort_comparisons, lookup_probes, opens, reads, closes;
static unsigned fixture_count, cursor, fail_at;
static int open_error;
static const char *Launcher_ThumbnailFolder(void) { return launcher_thumbnail_style == 1 ? "/IMGS2" : "/IMGS"; }
static u32 Launcher_CustomThumbHash(const char *name) { return (u32)strtoul(name, NULL, 16); }
static void Launcher_CustomThumbStripLine(char *name) { (void)name; }
static int f_opendir(int *dir, const char *path) { (void)dir; (void)path; opens++; cursor=0; return open_error; }
static int f_readdir(int *dir, void *info)
{
    (void)dir; (void)info; reads++;
    if(cursor == fail_at) return 1;
    launcher_custom_thumb_scan_info.fattrib = 0;
    if(cursor == fixture_count) launcher_custom_thumb_scan_info.fname[0] = 0;
    else snprintf(launcher_custom_thumb_scan_info.fname, 32, "%08x", 1000 - cursor++);
    return FR_OK;
}
static int f_closedir(int *dir) { (void)dir; closes++; return FR_OK; }
#include MANIFEST_FILE
static unsigned linear(const u32 *data, unsigned n, u32 key, unsigned *probes)
{
    unsigned i;
    for(i=0;i<n;i++) { (*probes)++; if(data[i] == key) return 1; }
    return 0;
}
static u32 random_state = 73;
static u32 random_value(void) { random_state = random_state * 1664525U + 1013904223U; return random_state; }
static void differential(void)
{
    u32 original[256], sorted[258];
    unsigned n, mode, i, k, expected, probes, cases=0;
    for(n=0;n<=256;n++) for(mode=0;mode<5;mode++)
    {
        sorted[0] = 0x12345678; sorted[n+1] = 0x87654321;
        for(i=0;i<n;i++) original[i] = mode==0 ? i : mode==1 ? n-i : mode==2 ? 0xffffffffU : mode==3 ? random_value()%17 : random_value();
        memcpy(sorted+1, original, n*sizeof(u32));
        Launcher_SortCustomThumbManifest(sorted+1,n);
        assert(sorted[0] == 0x12345678 && sorted[n+1] == 0x87654321);
        for(i=1;i<n;i++) assert(sorted[i] <= sorted[i+1]);
        for(k=0;k<n+4;k++)
        {
            u32 key = k<n ? original[k] : k==n ? 0 : k==n+1 ? 0xffffffffU : random_value();
            probes=0; expected=linear(original,n,key,&probes); lookup_probes=0;
            assert(Launcher_CustomThumbManifestContains(sorted+1,n,key) == expected);
            assert(lookup_probes <= 9); cases++;
        }
    }
    printf("manifest differential: %u queries, counts 0..256, ascending/reverse/duplicates/small-domain/full-width; canaries PASS\n",cases);
    for(n=30;n<=256;n=(n==120?256:n==256?257:n+30))
    {
        unsigned old_total=0,new_total=0;
        for(i=0;i<n;i++) original[i]=n-i;
        memcpy(sorted+1,original,n*sizeof(u32)); sort_comparisons=0;
        Launcher_SortCustomThumbManifest(sorted+1,n);
        for(k=0;k<120;k++)
        {
            probes=0; linear(original,n,0xffffffffU,&probes); old_total+=probes;
            lookup_probes=0; assert(!Launcher_CustomThumbManifestContains(sorted+1,n,0xffffffffU)); new_total+=lookup_probes;
        }
        printf("manifest M=%u: 120 misses old=%u hash probes new=%u; cold reverse-sort comparisons=%u\n", n,old_total,new_total,sort_comparisons);
    }
}
static void reset(void)
{
    memset(launcher_custom_thumb_manifest_loaded,0,sizeof(launcher_custom_thumb_manifest_loaded));
    memset(launcher_custom_thumb_manifest_present,0,sizeof(launcher_custom_thumb_manifest_present));
    memset(launcher_custom_thumb_manifest_count,0,sizeof(launcher_custom_thumb_manifest_count));
    opens=reads=closes=0; fixture_count=300; fail_at=999; open_error=0; launcher_thumbnail_style=0;
}
static void collection(void)
{
    unsigned i;
    reset();
    assert(!Launcher_ShouldTryCustomThumbnail(NULL)); assert(!Launcher_ShouldTryCustomThumbnail("")); assert(!opens);
    assert(Launcher_ShouldTryCustomThumbnail("3e8")); /* 1000, first */
    assert(Launcher_ShouldTryCustomThumbnail("2e9")); /* 745, 256th */
    assert(!Launcher_ShouldTryCustomThumbnail("2e8")); /* 744, omitted */
    assert(opens==1 && reads==256 && closes==1);
    for(i=0;i<256;i++) assert(launcher_custom_thumb_manifest_hash[0][i]==745+i);
    launcher_thumbnail_style=1; fixture_count=1;
    assert(Launcher_ShouldTryCustomThumbnail("3e8")); assert(!Launcher_ShouldTryCustomThumbnail("2e9"));
    assert(opens==2 && launcher_custom_thumb_manifest_count[1]==1);
    launcher_thumbnail_style=99; assert(Launcher_ShouldTryCustomThumbnail("2e9")); assert(opens==2);
    reset(); open_error=1;
    assert(!Launcher_ShouldTryCustomThumbnail("3e8")); open_error=0;
    assert(!Launcher_ShouldTryCustomThumbnail("3e8")); assert(opens==1 && !reads && !closes);
    reset(); fail_at=3;
    assert(Launcher_ShouldTryCustomThumbnail("3e6")); assert(!Launcher_ShouldTryCustomThumbnail("3e5"));
    assert(opens==1 && reads==4 && closes==1 && launcher_custom_thumb_manifest_count[0]==3);
    reset(); fixture_count=0;
    assert(!Launcher_ShouldTryCustomThumbnail("3e8")); assert(reads==1 && closes==1);
    puts("production collection: cap/partial read/empty/open failure/style isolation/invalid-style fallback/warm reuse PASS");
}
int main(void) { differential(); collection(); return 0; }
