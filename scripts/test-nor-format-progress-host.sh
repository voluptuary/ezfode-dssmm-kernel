#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

extract_function()
{
	source_file=$1
	function_name=$2
	awk -v name="$function_name" \
		'$0 ~ ("^static [^;]*[ *]" name "\\(") && $0 !~ /;[ \t]*$/ { printing = 1 }
		printing { print }
		printing && /^}/ { exit }' "$source_file"
}

{
	grep '^#define NOR_PROGRESS_STEPS ' "$repo_root/source/NORflash_OP.c"
	grep '^#define NOR_VERIFY_PROGRESS_INTERVAL ' "$repo_root/source/NORflash_OP.c"
	extract_function "$repo_root/source/NORflash_OP.c" NOR_ProgressStepCount
	extract_function "$repo_root/source/NORflash_OP.c" NOR_ShouldReportEraseProgress
} > "$test_tmp/nor-format-progress.impl.h"

gcc -std=c99 -O2 -Wall -Wextra -I"$test_tmp" \
	-I"$repo_root/source" \
	"$repo_root/tests/nor-format-progress-host.c" \
	-o "$test_tmp/nor-format-progress-host"
"$test_tmp/nor-format-progress-host"

awk '
	/^u8 NOR_list_MENU\(/ { in_menu = 1 }
	in_menu && /u8 format_result = FormatNor\(\)/ { in_format = 1 }
	in_format && /if\(format_result == 0\)/ { in_cancel = 1 }
	in_cancel && /re_menu = 1;/ { redraw_menu = 1 }
	in_cancel && /continue;/ { return_to_menu = 1; exit }
	END { if(!(redraw_menu && return_to_menu)) exit 1 }
' "$repo_root/source/ezkernelnew.c"

grep -q 'DrawHZText12(gl_formatnor_info2, message_length, message_x, 82' \
	"$repo_root/source/NORflash_OP.c"
grep -q 'while(keysHeld() & KEY_B)' "$repo_root/source/NORflash_OP.c"
grep -q 'Deleting, please wait...' "$repo_root/source/ezkernelnew.c"
grep -q 'NOR_ShouldReportEraseProgress(byte_offset)' "$repo_root/source/NORflash_OP.c"
grep -q 'Loadfile2NOR(pfilename, currentpath,' "$repo_root/source/ezkernelnew.c"
grep -q 'Save_NOR_info_with_art_path' "$repo_root/source/NORflash_OP.c"
grep -q 'Read_NOR_art_path(absolute_index)' "$repo_root/source/ezkernelnew.c"
grep -q 'sizeof(pNorFS) + slot \* MAX_path_len' "$repo_root/source/Ezcard_OP.c"
grep -q 'NOR_StoreArtPathRecord' "$repo_root/source/Ezcard_OP.c"
printf 'NOR format prompt, progress cadence and cancel-to-menu wiring passed.\n'
