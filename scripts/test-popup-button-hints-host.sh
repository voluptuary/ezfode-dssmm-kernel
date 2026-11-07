#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

gcc -std=c99 -O2 -Wall -Wextra \
	"$repo_root/tests/popup-button-hints-host.c" \
	-o "$test_tmp/popup-button-hints-host"

"$test_tmp/popup-button-hints-host"

awk '
	/^u8 NOR_list_MENU\(/ { in_nor_menu = 1 }
	in_nor_menu && /Launcher_DrawPopupButtonHints\(\);/ { found = 1 }
	in_nor_menu && /^static u32 Launcher_LoadTrimSize\(/ { in_nor_menu = 0 }
	END { exit !found }
' "$repo_root/source/ezkernelnew.c"
printf 'NOR game-management menu draws A/B hints.\n'

grep -q 'DrawHZText12(gl_nor_delete_info,0,text_x,LAUNCHER_NOR_DELETE_NOTICE_Y' \
	"$repo_root/source/ezkernelnew.c"
grep -q '#define LAUNCHER_NOR_DELETE_NOTICE_Y 34' "$repo_root/source/ezkernelnew.c"
printf 'NOR delete notice stays above all menu rows.\n'

awk '
	/^u8 NOR_list_MENU\(/ { in_nor_menu = 1 }
	in_nor_menu && /else if\(action_line==2\)/ { in_format = 1 }
	in_format && /Launcher_DrawPopupPanel\(36, 25\);/ { panel = 1 }
	in_format && /Launcher_DrawPopupButtonHints\(\);/ { hints = 1 }
	in_format && /FormatNor\(\)/ { found = panel && hints; exit }
	END { if(!found) exit 1 }
' "$repo_root/source/ezkernelnew.c"
printf 'NOR format confirmation clears the menu and retains A/B hints.\n'
