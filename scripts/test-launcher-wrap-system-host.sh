#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

awk '
	/^static u32 Launcher_(IsSystemBrowser|BrowserWrapAroundEnabled)\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	p { print }
	p && /^}/ { print ""; p=0; found++ }
	END { if(found != 2) exit 1 }
' "$repo_root/source/ezkernelnew.c" > "$test_tmp/wrap-helpers.inc"
[ -s "$test_tmp/wrap-helpers.inc" ] || { echo "FAIL: could not extract wrap scope helpers" >&2; exit 1; }

gcc -std=gnu99 -O2 -Wall -Wextra -Werror \
	-DWRAP_HELPERS_FILE="\"$test_tmp/wrap-helpers.inc\"" \
	"$repo_root/tests/launcher-wrap-system-host.c" \
	-o "$test_tmp/launcher-wrap-system-host"

"$test_tmp/launcher-wrap-system-host"
