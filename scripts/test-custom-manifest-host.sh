#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT
python3 - "$repo_root/source/ezkernelnew.c" "$test_tmp/manifest.inc" <<'PY'
import re, sys
from pathlib import Path
source = Path(sys.argv[1]).read_text()
names = ['Launcher_SortCustomThumbManifest', 'Launcher_CustomThumbManifestContains',
         'Launcher_LoadCustomThumbManifest', 'Launcher_ShouldTryCustomThumbnail']
parts = []
for name in names:
    match = re.search(r'^static [^\n]*\b' + name + r'\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
    if not match:
        raise SystemExit('missing production function: ' + name)
    part = match.group()
    if name == 'Launcher_SortCustomThumbManifest':
        part = part.replace('hashes[j - gap] > value', '(sort_comparisons++, hashes[j - gap] > value)')
    if name == 'Launcher_CustomThumbManifestContains':
        part = part.replace('u32 mid = low + (high - low) / 2;', 'u32 mid = low + (high - low) / 2; lookup_probes++;')
    parts.append(part)
Path(sys.argv[2]).write_text('\n'.join(parts))
PY
gcc -std=c99 -O2 -Wall -Wextra -Werror -DMANIFEST_FILE="\"$test_tmp/manifest.inc\"" \
    "$repo_root/tests/custom-manifest-host.c" -o "$test_tmp/manifest"
"$test_tmp/manifest"
