#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

awk '
	/^static void Launcher_InvalidateListRowCache\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	/^static void Launcher_SetCleanListEnabled\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	/^static void Launcher_GetListDisplayName\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	/^static void Launcher_GetBrowserDisplayTitle\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	p { print }
	p && /^}/ { print ""; p=0; found++ }
	END { if(found != 4) exit 1 }
' "$repo_root/source/ezkernelnew.c" > "$test_tmp/clean-list.inc"
[ -s "$test_tmp/clean-list.inc" ] || { echo "FAIL: could not extract Clean list cache helpers" >&2; exit 1; }
grep -Fq 'Launcher_SetCleanListEnabled(!launcher_clean_list);' "$repo_root/source/ezkernelnew.c" || {
	echo "FAIL: Clean list setting toggle does not use cache-aware setter" >&2
	exit 1
}
grep -Fc 'Launcher_GetBrowserDisplayTitle(selected.name, cleaned, sizeof(cleaned));' "$repo_root/source/ezkernelnew.c" |
	grep -qx '3' || {
	echo "FAIL: all browser title renderers must honor Clean list" >&2
	exit 1
}

gcc -std=gnu99 -O2 -Wall -Wextra -Werror \
	-DCLEAN_LIST_FILE="\"$test_tmp/clean-list.inc\"" \
	"$repo_root/tests/launcher-clean-list-cache-host.c" \
	-o "$test_tmp/launcher-clean-list-cache-host"

"$test_tmp/launcher-clean-list-cache-host"
