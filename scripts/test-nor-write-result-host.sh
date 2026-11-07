#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

gcc -std=c99 -O2 -Wall -Wextra -Werror -I"$repo_root/source" \
	"$repo_root/tests/nor-write-result-host.c" \
	-o "$test_tmp/nor-write-result-host"
"$test_tmp/nor-write-result-host"

grep -q 'result = NOR_WRITE_RESULT_ERASE_ERROR;' "$repo_root/source/NORflash_OP.c"
grep -q 'result = NOR_WRITE_RESULT_PROGRAM_ERROR;' "$repo_root/source/NORflash_OP.c"
grep -q 'NOR_WRITE_RESULT_CATALOG_ERROR' "$repo_root/source/NORflash_OP.c"
grep -q 'NOR_WriteResultErrorNumber(res)' "$repo_root/source/ezkernelnew.c"
grep -q 'gl_nor_device_error' "$repo_root/source/ezkernelnew.c"
grep -q 'gl_nor_erase_error' "$repo_root/source/ezkernelnew.c"
grep -q 'gl_nor_program_error' "$repo_root/source/ezkernelnew.c"
grep -q 'gl_nor_catalog_error' "$repo_root/source/ezkernelnew.c"
printf 'NOR write failure stage wiring passed.\n'
