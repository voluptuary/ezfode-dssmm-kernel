#!/usr/bin/env python3
"""Losslessly pack the fork's splash with devkitPro gbalzss."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
tool = shutil.which('gbalzss') or str(Path(os.environ.get('DEVKITPRO', '/opt/devkitpro')) / 'tools/bin/gbalzss')
header = (root / 'images/SPLASH.h').read_text()
array = re.search(r'gImage_splash\s*\[76800\][^=]*=\s*\{(.*?)\}', header, re.S)
if not array:
    raise SystemExit('Expected 240x160 16-bit splash')
raw = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', array[1]))
assert len(raw) == 76800
with tempfile.TemporaryDirectory() as temp:
    directory = Path(temp)
    (directory / 'splash.raw').write_bytes(raw)
    subprocess.run([tool, '--vram', 'e', str(directory / 'splash.raw'), str(directory / 'splash.lz')], check=True)
    subprocess.run([tool, 'd', str(directory / 'splash.lz'), str(directory / 'check.raw')], check=True)
    assert (directory / 'check.raw').read_bytes() == raw
    packed = (directory / 'splash.lz').read_bytes()
lines = [','.join(map(str, packed[i:i+24])) for i in range(0, len(packed), 24)]
text = '''/* Generated losslessly from images/SPLASH.h by scripts/pack-splash.py. */
static const u8 launcher_splash_lz[] __attribute__((aligned(4))) = {
'''+',\n'.join(lines)+'''
};
static void Launcher_DrawPackedSplash(void)
{
    REG_DISPCNT |= 0x0080;
    LZ77UnCompVram(launcher_splash_lz, (void*)VRAM);
}
'''
(root / 'source/launcher_splash_lz.h').write_text(text)
print(f'Packed local splash: {len(packed)} bytes; exact 76800-byte round trip verified.')
