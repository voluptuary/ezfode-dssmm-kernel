#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

awk '
	/^static u32 Launcher_SaveArtworkFormatValid\(/ && $0 !~ /;[ \t]*$/ { p=1 }
	p { print }
	p && /^}/ { print ""; p=0; found++ }
	END { if(found != 1) exit 1 }
' "$repo_root/source/save_popup.h" > "$test_tmp/save-artwork.inc"
[ -s "$test_tmp/save-artwork.inc" ] || {
	echo "FAIL: could not extract save artwork validator" >&2
	exit 1
}
grep -Fq 'width = Launcher_ThumbnailSourceWidth();' "$repo_root/source/save_popup.h" || {
	echo "FAIL: save popup does not use the configured thumbnail width" >&2
	exit 1
}
grep -Fq 'height = Launcher_ThumbnailSourceHeight();' "$repo_root/source/save_popup.h" || {
	echo "FAIL: save popup does not use the configured thumbnail height" >&2
	exit 1
}
grep -Fq 'if(launcher_thumbnail_style == LAUNCHER_THUMB_STYLE_BOX)' "$repo_root/source/save_popup.h" || {
	echo "FAIL: save popup does not select drawing geometry by thumbnail style" >&2
	exit 1
}
grep -Fq '((const u16*)(pReadCache + 0x18000)) + 8' "$repo_root/source/save_popup.h" || {
	echo "FAIL: box artwork crop is not offset by eight pixels" >&2
	exit 1
}
if grep -Fq 'launcher_thumbnail_style = LAUNCHER_THUMB_STYLE_BOX;' "$repo_root/source/save_popup.h"; then
	echo "FAIL: save popup overrides the configured thumbnail style" >&2
	exit 1
fi
grep -Fq 'char display_name[MAX_path_len];' "$repo_root/source/ezkernelnew.c" || {
	echo "FAIL: save popup truncates long ROM basenames before artwork lookup" >&2
	exit 1
}
grep -Fq 'Launcher_SaveArtworkFormatValid(raw, width, height)' "$repo_root/source/save_popup.h" || {
	echo "FAIL: save popup does not validate loaded artwork dimensions" >&2
	exit 1
}

gcc -std=gnu99 -O2 -Wall -Wextra -Werror \
	-DSAVE_ARTWORK_HELPERS_FILE="\"$test_tmp/save-artwork.inc\"" \
	"$repo_root/tests/save-popup-artwork-host.c" \
	-o "$test_tmp/save-popup-artwork-host"

"$test_tmp/save-popup-artwork-host"
