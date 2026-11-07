#!/bin/sh
# Differential test: the enum settings readers in the working tree must behave exactly like the readers in a
# reference version of source/ezkernelnew.c (default: v8.2.0). The reader code is extracted verbatim.
# Usage: scripts/test-launcher-enum-settings-host.sh [reference-git-ref [source-file-to-test]]
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ref=${1:-v8.2.0}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

git -C "$repo_root" show "$ref:source/ezkernelnew.c" > "$tmp/ref.c"
new_src=${2:-$repo_root/source/ezkernelnew.c}

# Extract the readers of either form: the table-driven block (from its "Enum settings" comment to the last
# wrapper) or, in older versions, the seven separate functions.
extract() {
	awk '{ if ($0 ~ / \* Enum settings:/) { printf "%s\n", prev; s=1 } if (s) print; prev=$0 }
	     s && /^static void Launcher_ReadHorizontalSideSetting/ { exit }' "$1" > "$2"
	[ -s "$2" ] || awk '/^static void Launcher_Read(ClockFormat|ViewMode|ListArtPosition|ArtBorder|RoundedCorners|VerticalSide|HorizontalSide)Setting\(/ {p=1}
	     p {print} p && /^}/ {p=0; print ""}' "$1" > "$2"
	[ -s "$2" ] || { echo "FAIL: could not extract readers from $1" >&2; exit 1; }
}
extract "$tmp/ref.c" "$tmp/ref.inc"
extract "$new_src" "$tmp/new.inc"

# The values of the constants the readers use, copied from the source so the test cannot drift from it.
grep -E '^#define (LAUNCHER_VIEW_(LIST|HORIZONTAL|VERTICAL|LIST_ART)|LAUNCHER_SIDE_ALIGN_(TOP|CENTER|BOTTOM|LEFT|RIGHT|CUSTOM))[[:space:]]' "$new_src" > "$tmp/constants.h"
[ "$(wc -l < "$tmp/constants.h")" -eq 10 ] || { echo "FAIL: expected 10 constants, found $(wc -l < "$tmp/constants.h")" >&2; exit 1; }

for v in ref new; do
	gcc -std=gnu99 -O2 -Wall -Wno-unused-function -Wno-unused-variable -DSOURCE_CONSTANTS="\"$tmp/constants.h\"" \
		-DREADERS_FILE="\"$tmp/$v.inc\"" "$repo_root/tests/launcher-enum-settings-host.c" -o "$tmp/$v"
	"$tmp/$v" > "$tmp/$v.out"
done

lines=$(wc -l < "$tmp/new.out")
if cmp -s "$tmp/ref.out" "$tmp/new.out"; then
	echo "Enum settings readers behave identically to $ref over $lines cases."
else
	echo "FAIL: readers differ from $ref:" >&2
	diff "$tmp/ref.out" "$tmp/new.out" | head -10 >&2
	exit 1
fi
