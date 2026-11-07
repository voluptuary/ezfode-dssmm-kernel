#!/bin/sh
# Differentially compare artwork drawing against the original per-pixel row scans.
# Usage: scripts/test-launcher-artwork-draw-host.sh [reference-git-ref [source-file-to-test]]
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ref=${1:-v8.2.0}
new_src=${2:-$repo_root/source/ezkernelnew.c}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

git -C "$repo_root" show "$ref:source/ezkernelnew.c" > "$tmp/ref.c"

func() { awk -v n="$2" '$0 ~ "^static [^;]* "n"\\(" && $0 !~ /;[ \t]*$/ {p=1} p {print} p && /^}/ {exit}' "$1"; }
build_inc() {
	src=$1; out=$2
	{
		grep -F 'typedef u32 __attribute__((may_alias)) Launcher_PixelPair;' "$src" || true
		func "$src" Launcher_SpanHasTransparent
		func "$src" Launcher_DrawPicClipStride
		func "$src" Launcher_DrawPreparedThumbMasked
	} | sed 's/0x06000000/HOST_VRAM_BASE/g' > "$out"
	grep -q 'Launcher_DrawPicClipStride' "$out" && grep -q 'Launcher_DrawPreparedThumbMasked' "$out" ||
		{ echo "FAIL: could not extract the draw routines from $src" >&2; exit 1; }
}
build_inc "$tmp/ref.c" "$tmp/ref.inc"
build_inc "$new_src" "$tmp/new.inc"

grep -E '^#define LAUNCHER_THUMB_TRANSPARENT_SENTINEL[[:space:]]' "$new_src" > "$tmp/constants.h"
[ "$(wc -l < "$tmp/constants.h")" -eq 1 ] || { echo "FAIL: sentinel constant not found" >&2; exit 1; }

for v in ref new; do
	extra=""; grep -q 'Launcher_SpanHasTransparent' "$tmp/$v.inc" && extra="-DHAVE_HELPER"
	# Detect unaligned 32-bit loads, which are rotated by the ARM7TDMI.
	gcc -std=gnu99 -O1 -Wall -Wno-unused-function -fsanitize=alignment -fno-sanitize-recover=alignment \
		-DSOURCE_CONSTANTS="\"$tmp/constants.h\"" -DFUNCS_FILE="\"$tmp/$v.inc\"" $extra \
		"$repo_root/tests/launcher-artwork-draw-host.c" -o "$tmp/$v"
	"$tmp/$v" > "$tmp/$v.out" 2> "$tmp/$v.err" || { echo "FAIL: the $v build aborted (misaligned access?):" >&2; head -3 "$tmp/$v.err" >&2; exit 1; }
done

if grep -q '^span|' "$tmp/new.out" &&
	! grep -q '^span|.*mismatches=0$' "$tmp/new.out"; then
	echo "FAIL: helper disagrees with the exact scan:" >&2
	grep '^span|' "$tmp/new.out" >&2
	exit 1
fi
grep '^draw|' "$tmp/ref.out" > "$tmp/ref.draw"
grep '^draw|' "$tmp/new.out" > "$tmp/new.draw"
if cmp -s "$tmp/ref.draw" "$tmp/new.draw"; then
	echo "Artwork drawing matches $ref: $(tail -1 "$tmp/new.draw" | cut -d'|' -f2-)"
	grep '^span|' "$tmp/new.out" || true
else
	echo "FAIL: artwork drawing differs from $ref:" >&2
	diff "$tmp/ref.draw" "$tmp/new.draw" | head -6 >&2
	exit 1
fi
