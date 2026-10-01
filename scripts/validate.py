"""Source validation only; a Linux ISO build and guest boot are separate checks."""
from pathlib import Path
import json
import struct
import xml.etree.ElementTree as ET
import configparser

repo = Path(__file__).resolve().parents[1]
root = repo / 'profile/airootfs'
for path in root.rglob('*.json'):
    json.loads(path.read_text(encoding='utf-8'))
svg_count = 0
for path in root.rglob('*.svg'):
    tree = ET.parse(path)
    ids = [node.attrib['id'] for node in tree.iter() if 'id' in node.attrib]
    assert len(ids) == len(set(ids)), f'Duplicate SVG IDs: {path}'
    if path.name in ('panel-background.svg', 'background.svg', 'tooltip.svg'):
        for part in ('center', 'top', 'bottom', 'left', 'right', 'topleft', 'topright', 'bottomleft', 'bottomright'):
            assert part in ids and f'mask-{part}' in ids, f'Missing frame/blur mask: {path}'
    if path.name == 'decoration.svg':
        assert 'decoration-top' in ids and 'mask-top' in ids
    if path.parent.name == 'SynterraGlass' and path.stem in ('close', 'maximize', 'minimize', 'restore'):
        assert 'active-center' in ids and 'hover-center' in ids
    svg_count += 1
for path in (repo / 'assets/wallpapers').glob('*.png'):
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II', data[16:24]) == (3840, 2160), f'Wrong wallpaper dimensions: {path}'
for path in list((repo / 'scripts').glob('*.sh')) + list((root / 'usr/local/bin').iterdir()):
    assert b'\r' not in path.read_bytes(), f'CRLF in Linux script: {path}'
    assert path.read_text().startswith('#!/usr/bin/env bash\n')
scheme = configparser.ConfigParser()
scheme.read(root / 'usr/share/color-schemes/SynterraGlass.colors')
assert all(f'Colors:{group}' in scheme for group in ('Window', 'View', 'Button', 'Selection', 'Tooltip'))
packages = (repo / 'profile/packages.txt').read_text().splitlines()
assert len(packages) == len(set(packages))
assert {'plasma-desktop', 'sddm', 'open-vm-tools', 'aurorae'} <= set(packages)
print(f'PASS: JSON metadata, {svg_count} SVGs and blur masks, 4K wallpapers, LF scripts, palette and package manifest.')
