#!/usr/bin/env bash
set -Eeuo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
dest=${1:?Usage: build-grub-theme.sh NEW_OUTPUT_DIRECTORY}
[[ ! -e $dest ]] || { echo 'Theme destination must be new.' >&2; exit 1; }
for tool in rsvg-convert grub-mkfont; do
    command -v "$tool" >/dev/null || { echo "Missing theme build tool: $tool" >&2; exit 1; }
done
font=/usr/share/fonts/TTF/DejaVuSans.ttf
[[ -f $font ]] || { echo 'Install ttf-dejavu on the build host.' >&2; exit 1; }
mkdir -p "$dest"
install -m644 "$repo/assets/grub/theme.txt" "$dest/theme.txt"
# Keep the supplied Aurora wallpaper unchanged; GRUB scales it to the display.
install -m644 "$repo/assets/wallpapers/Prism-aurora-4K.png" "$dest/background.png"
rsvg-convert -w 96 -h 96 "$repo/profile/airootfs/usr/share/icons/hicolor/scalable/apps/synterra.svg" -o "$dest/logo.png"
for size in 16 18 24 32; do
    # The theme uses Latin text; exclude oversized glyphs from other scripts.
    grub-mkfont -n Synterra -r 0x20-0x7e,0xa0-0xff -s "$size" -o "$dest/synterra-$size.pf2" "$font"
done
temp=$(mktemp -d)
trap 'rm -rf -- "$temp"' EXIT
for style in panel selected; do
    if [[ $style == panel ]]; then
        top='#243c62'; bottom='#172943'; opacity='0.88'; border='#adcfe8'
        corner=14; center=36; far=50; radius=13
    else
        top='#8bc9f1'; bottom='#3377bc'; opacity='0.70'; border='#def3ff'
        corner=8; center=48; far=56; radius=7
    fi
    while read -r part x y width height; do
        # Nine slices from one vector frame keep the glass edges continuous.
        cat > "$temp/$part.svg" <<SVG
<svg xmlns="http://www.w3.org/2000/svg" width="$width" height="$height" viewBox="$x $y $width $height">
<defs><linearGradient id="glass" x1="0" y1="0" x2="0" y2="64" gradientUnits="userSpaceOnUse"><stop stop-color="$top"/><stop offset="1" stop-color="$bottom"/></linearGradient></defs>
<rect x="1" y="1" width="62" height="62" rx="$radius" fill="url(#glass)" fill-opacity="$opacity" stroke="$border" stroke-opacity="0.65" stroke-width="2"/>
</svg>
SVG
        rsvg-convert "$temp/$part.svg" -o "$dest/${style}_$part.png"
    done <<SLICES
nw 0 0 $corner $corner
n $corner 0 $center $corner
ne $far 0 $corner $corner
w 0 $corner $corner $center
c $corner $corner $center $center
e $far $corner $corner $center
sw 0 $far $corner $corner
s $corner $far $center $corner
se $far $far $corner $corner
SLICES
done
echo "Built Synterra GRUB theme in $dest"
