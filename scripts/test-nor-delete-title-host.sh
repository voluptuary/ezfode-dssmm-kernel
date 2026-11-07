#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

awk '
	$0 ~ /^static u32 Launcher_EraseNorTitle\(/ { printing = 1 }
	printing { print }
	printing && /^}/ { exit }
' "$repo_root/source/ezkernelnew.c" > "$test_tmp/nor-delete-title.impl.h"

gcc -std=c99 -O2 -Wall -Wextra -I"$test_tmp" \
	"$repo_root/tests/nor-delete-title-host.c" \
	-o "$test_tmp/nor-delete-title-host"
"$test_tmp/nor-delete-title-host"

grep -q 'Launcher_EraseNorTitle(' "$repo_root/source/ezkernelnew.c"
grep -q 'memset(&pNorFS\[selected_index\], 0, sizeof(FM_NOR_FS))' \
	"$repo_root/source/ezkernelnew.c"
grep -q 'Save_NOR_info_with_art_path((u16\*)pNorFS' \
	"$repo_root/source/ezkernelnew.c"
printf 'NOR title deletion and metadata cleanup wiring passed.\n'
