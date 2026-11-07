#!/bin/sh
# Differential test: the on/off settings readers in the working tree must behave exactly like the
# readers in a reference version of source/ezkernelnew.c (default: v8.2.0).
# Usage: scripts/test-launcher-settings-readers-host.sh [reference-git-ref]
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ref=${1:-v8.2.0}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

# Extract the verbatim readers (and the shared reader, if the version has one) from a source file.
extract() {
	awk '
		/^static void Launcher_Read(Sounds|HideSystemFiles|ListFolders|CleanList|WrapAround|OnOff)Setting\(/ {p=1}
		p {print}
		p && /^}/ {p=0; print ""}
	' "$1"
}

git -C "$repo_root" show "$ref:source/ezkernelnew.c" > "$tmp/ref.c"
extract "$tmp/ref.c" > "$tmp/ref_readers.inc"
extract "$repo_root/source/ezkernelnew.c" > "$tmp/new_readers.inc"
[ -s "$tmp/ref_readers.inc" ] && [ -s "$tmp/new_readers.inc" ] || { echo "FAIL: could not extract readers" >&2; exit 1; }

for v in ref new; do
	gcc -std=gnu99 -O2 -Wall -Wno-unused-function -I"$repo_root/source" \
		-DREADERS_FILE="\"$tmp/${v}_readers.inc\"" \
		"$repo_root/tests/launcher-settings-readers-host.c" -o "$tmp/$v"
	"$tmp/$v" > "$tmp/$v.out"
done

# WrapAround intentionally defaults On when absent. Its third value is new, so
# compare legacy inputs with the reference and assert both new cases separately.
awk -F'|' '$1 != "WrapAround" || ($3 != "absent" && $3 != "On, Not SYSTEM")' "$tmp/ref.out" > "$tmp/ref.comparable"
awk -F'|' '$1 != "WrapAround" || ($3 != "absent" && $3 != "On, Not SYSTEM")' "$tmp/new.out" > "$tmp/new.comparable"
lines=$(wc -l < "$tmp/new.comparable")
if cmp -s "$tmp/ref.comparable" "$tmp/new.comparable"; then
	echo "Settings readers match $ref over $lines comparable cases."
else
	echo "FAIL: readers differ from $ref:" >&2
	diff "$tmp/ref.comparable" "$tmp/new.comparable" | head -10 >&2
	exit 1
fi
awk -F'|' '$1 == "WrapAround" && $3 == "absent" { if($4 != 1 || $5 != 0) exit 1; count++ }
	END { if(count != 10) exit 1 }' "$tmp/new.out" ||
	{ echo "FAIL: WrapAround must default On for every locale and initial value" >&2; exit 1; }
awk -F'|' '$1 == "WrapAround" && $3 == "On, Not SYSTEM" { if($4 != 2 || $5 != 0) exit 1; count++ }
	END { if(count != 5) exit 1 }' "$tmp/new.out" ||
	{ echo "FAIL: WrapAround must read On, Not SYSTEM for every locale and initial value" >&2; exit 1; }
echo "WrapAround defaults On; legacy values match $ref and On, Not SYSTEM is preserved."
