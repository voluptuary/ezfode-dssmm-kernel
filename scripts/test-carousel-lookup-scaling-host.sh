#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT
cp -R "$repo_root/source/ff15" "$test_tmp/ff15"
sed -i 's/#define FF_USE_MKFS[[:space:]]*0/#define FF_USE_MKFS 1/' "$test_tmp/ff15/ffconf.h"
python3 - "$test_tmp/ff15/ff.c" <<'PY'
from pathlib import Path
import re, sys
p=Path(sys.argv[1]); text=p.read_text()
for name in ['dir_find','dir_next','f_open','f_open_by_dirent','f_read']:
    pattern=r'((?:static )?FRESULT '+name+r'\s*\([^;]*?\)\s*\{)'
    text,n=re.subn(pattern,lambda m:m[1]+'\n extern unsigned long metric_'+name+'; metric_'+name+'++;',text,count=1)
    assert n==1, name
p.write_text(text)
PY
gcc -std=c99 -O2 -Wall -Wextra -I"$test_tmp/ff15" -I"$repo_root/source" \
    "$test_tmp/ff15/ff.c" "$test_tmp/ff15/ffunicode.c" \
    "$repo_root/tests/carousel-lookup-scaling-host.c" -o "$test_tmp/scaling"
"$test_tmp/scaling"
