#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

gcc -std=gnu99 -O2 -Wall -Wextra -Werror -I"$repo_root/source" \
	"$repo_root/tests/launcher-auto-theme-host.c" \
	-o "$test_tmp/launcher-auto-theme-host"
"$test_tmp/launcher-auto-theme-host"
echo "Auto theme clock boundaries and pending mode scenarios passed."
