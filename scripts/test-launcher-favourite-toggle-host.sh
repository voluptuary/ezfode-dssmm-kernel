#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

awk '
	/^static u32 Launcher_ToggleFavouriteFullPath\(/ { p=1 }
	p { print }
	p && /^}/ { exit }
' "$repo_root/source/ezkernelnew.c" > "$test_tmp/toggle.inc"
[ -s "$test_tmp/toggle.inc" ] || { echo "FAIL: could not extract favourite toggle" >&2; exit 1; }

gcc -std=gnu99 -O2 -Wall -Wextra -Werror \
	-DFUNCTION_FILE="\"$test_tmp/toggle.inc\"" \
	"$repo_root/tests/launcher-favourite-toggle-host.c" \
	-o "$test_tmp/launcher-favourite-toggle-host"
"$test_tmp/launcher-favourite-toggle-host"
echo "Favourite toggle add/remove, capacity and preservation scenarios passed."
