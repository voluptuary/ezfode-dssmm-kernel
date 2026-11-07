#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

extract_function()
{
	awk -v name="$1" \
		'$0 ~ ("^static [^;]* " name "\\(") && $0 !~ /;[ \t]*$/ { printing = 1 }
		printing { print }
		printing && /^}/ { exit }' "$repo_root/source/ezkernelnew.c"
}

{
	extract_function Launcher_CustomThumbFileName
	extract_function Launcher_IsValidGameCodeBytes
	extract_function Launcher_LoadRootNamedThumbnail
	extract_function Launcher_LoadNorThumbnail
} > "$test_tmp/nor-thumbnail-fallback.impl.h"

if ! grep -q 'Launcher_LoadNorThumbnail' "$test_tmp/nor-thumbnail-fallback.impl.h"; then
	printf 'Could not extract NOR filename-art loader from production source.\n' >&2
	exit 1
fi

gcc -std=c99 -O2 -Wall -Wextra -I"$test_tmp" \
	"$repo_root/tests/nor-thumbnail-fallback-host.c" \
	-o "$test_tmp/nor-thumbnail-fallback-host"
"$test_tmp/nor-thumbnail-fallback-host"

awk '
	/^static u32 Launcher_LoadThumbDataForIndex\(/ { in_loader = 1 }
	in_loader && /if\(Launcher_IsNORPage\(\)\)/ { in_nor = 1 }
	in_nor && /Launcher_LoadNorThumbnail/ { helper_line = NR; exit }
	END { if(!helper_line) exit 1 }
' "$repo_root/source/ezkernelnew.c"
printf 'NOR titles use the tested custom/header/filename artwork fallback.\n'
