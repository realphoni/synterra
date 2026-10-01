"""Generate original KDE SVG assets; does not modify the supplied wallpapers."""
from pathlib import Path
import configparser

ROOT = Path(__file__).resolve().parents[1] / 'profile/airootfs/usr/share'
STYLE = ROOT / 'plasma/desktoptheme/SynterraGlass'
AURORAE = ROOT / 'aurorae/themes/SynterraGlass'
PARTS = {'topleft': (0, 0, 7, 7), 'top': (7, 0, 50, 7),
         'topright': (57, 0, 7, 7), 'left': (0, 7, 7, 50),
         'center': (7, 7, 50, 50), 'right': (57, 7, 7, 50),
         'bottomleft': (0, 57, 7, 7), 'bottom': (7, 57, 50, 7),
         'bottomright': (57, 57, 7, 7)}


def frame(prefix='', offset=0, opacity=1, mask=False, title_height=7):
    result = []
    for part, (x, y, w, h) in PARTS.items():
        if y == 0:
            h = title_height
        else:
            y += title_height - 7
        name = f'{prefix}-{part}' if prefix else part
        fill = '#000000' if mask else 'url(#glass)'
        result.append(f'<rect id="{name}" x="{x+offset}" y="{y}" width="{w}" height="{h}" fill="{fill}" opacity="{opacity}"/>')
    return '\n'.join(result)


def svg(body, width=300, height=90):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<defs><linearGradient id="glass" x1="0" y1="0" x2="0" y2="1" gradientUnits="objectBoundingBox">
<stop offset="0" stop-color="#eff8ff" stop-opacity=".70"/>
<stop offset=".08" stop-color="#a7cbe9" stop-opacity=".65"/>
<stop offset=".48" stop-color="#537eaf" stop-opacity=".58"/>
<stop offset=".51" stop-color="#32537e" stop-opacity=".68"/>
<stop offset="1" stop-color="#87afd3" stop-opacity=".72"/>
</linearGradient></defs>{body}</svg>'''


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text + '\n', encoding='utf-8', newline='\n')


body = frame() + frame('mask', 80, mask=True)
body += '<rect id="hint-stretch-borders" x="0" y="75" width="1" height="1"/>'
for side in ('top', 'bottom', 'left', 'right'):
    body += f'<rect id="hint-{side}-margin" x="10" y="75" width="7" height="7"/>'
for name in ('widgets/panel-background.svg', 'dialogs/background.svg',
             'widgets/background.svg', 'widgets/tooltip.svg'):
    write(STYLE / name, svg(body))
write(AURORAE / 'decoration.svg', svg(frame('decoration', title_height=34) + frame('decoration-inactive', 80, .65, title_height=34) + frame('mask', 160, mask=True, title_height=34), height=100))

glyphs = {'close': '<path d="M17 8l9 9m0-9l-9 9"/>',
          'minimize': '<path d="M10 17h10"/>',
          'maximize': '<rect x="10" y="7" width="10" height="9" fill="none"/>',
          'restore': '<path d="M10 10h9v8h-9zM13 10V7h9v8h-3" fill="none"/>'}
for name, glyph in glyphs.items():
    groups = []
    width = 43 if name == 'close' else 29
    for index, state in enumerate(('active', 'inactive', 'hover', 'pressed', 'hover-inactive', 'pressed-inactive')):
        red = name == 'close'
        color = ('#fb8f79' if 'hover' in state else '#b83b35') if red else ('#6dccff' if 'hover' in state else '#7194b8')
        opacity = '.55' if state == 'inactive' else '.95'
        groups.append(f'''<g id="{state}-center" transform="translate({index*50},0)" opacity="{opacity}">
<rect x="0" y="0" width="{width}" height="23" rx="3" fill="{color}" stroke="#e3f3ff" stroke-opacity=".65"/>
<rect x="1" y="1" width="{width-2}" height="10" rx="2" fill="#ffffff" opacity=".25"/>
<g stroke="#f8fcff" stroke-width="1.8" stroke-linecap="square">{glyph}</g></g>''')
    write(AURORAE / f'{name}.svg', svg(''.join(groups), 300, 25))

orb = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
<defs><radialGradient id="o" cx=".4" cy=".25" r=".8"><stop stop-color="#b4eaff"/><stop offset=".5" stop-color="#3c9bea"/><stop offset="1" stop-color="#183f83"/></radialGradient></defs>
<circle cx="32" cy="32" r="29" fill="url(#o)" stroke="#e4f6ff" stroke-width="2"/>
<ellipse cx="32" cy="19" rx="22" ry="11" fill="#fff" opacity=".22"/>
<path d="M43 20C25 9 13 28 30 32c22 5 8 23-10 11" fill="none" stroke="#fff" stroke-width="5" stroke-linecap="round"/>
</svg>'''
write(ROOT / 'icons/hicolor/scalable/apps/synterra.svg', orb)

scheme = configparser.ConfigParser()
scheme.optionxform = str
scheme['General'] = {'Name': 'Synterra Glass', 'ColorScheme': 'SynterraGlass', 'shadeSortColumn': 'true'}
palettes = {
    'Window': ('239,245,251', '23,37,53'), 'View': ('255,255,255', '23,37,53'),
    'Button': ('230,240,249', '23,37,53'), 'Selection': ('58,126,194', '255,255,255'),
    'Tooltip': ('242,248,255', '23,37,53'), 'Complementary': ('37,58,84', '247,251,255'),
    'Header': ('229,240,250', '23,37,53')}
for name, (bg, fg) in palettes.items():
    scheme[f'Colors:{name}'] = {'BackgroundNormal': bg, 'BackgroundAlternate': '231,239,247',
        'ForegroundNormal': fg, 'ForegroundInactive': '108,123,140',
        'ForegroundActive': '42,118,191', 'ForegroundLink': '31,102,175',
        'ForegroundVisited': '112,78,168', 'ForegroundNegative': '189,57,57',
        'ForegroundNeutral': '165,108,28', 'ForegroundPositive': '34,128,91',
        'DecorationFocus': '76,157,231', 'DecorationHover': '116,187,243'}
scheme['WM'] = {'activeBackground': '80,132,182', 'activeForeground': '248,252,255',
                'inactiveBackground': '156,177,199', 'inactiveForeground': '222,233,247'}
path = ROOT / 'color-schemes/SynterraGlass.colors'
path.parent.mkdir(parents=True, exist_ok=True)
with path.open('w', encoding='utf-8', newline='\n') as stream:
    scheme.write(stream, space_around_delimiters=False)
write(STYLE / 'colors', path.read_text(encoding='utf-8'))
# KColorScheme reads palette groups from kdeglobals, not only the scheme name.
defaults = ROOT.parents[1] / 'etc/skel/.config/kdeglobals'
settings = configparser.ConfigParser()
settings.optionxform = str
settings.read(defaults, encoding='utf-8')
for section in scheme.sections():
    if section.startswith('Colors:') or section == 'WM':
        if section not in settings:
            settings[section] = {}
        settings[section].update(scheme[section])
with defaults.open('w', encoding='utf-8', newline='\n') as stream:
    settings.write(stream, space_around_delimiters=False)
print('Generated Synterra glass style, Aurorae frames, color scheme and launcher orb.')
