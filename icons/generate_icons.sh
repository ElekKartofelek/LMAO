#!/bin/bash

read -p "Path to source image: " SRC
read -p "App ID: " APPID

# launch in terminal if not already in one
if [ ! -t 0 ]; then
    exec konsole -e bash "$0" "$@"
fi

if [ ! -f "$SRC" ]; then
    echo "Error: File not found: $SRC"
    exit 1
fi

if ! command -v magick &> /dev/null; then
    echo "Error: ImageMagick not found. Install with: sudo dnf install ImageMagick"
    exit 1
fi

SIZES=(16 32 48 64 128 256 512)

for SIZE in "${SIZES[@]}"; do
    DIR="hicolor/${SIZE}x${SIZE}/apps"
    mkdir -p "$DIR"
    if [ "$SIZE" -le 64 ]; then
        magick "$SRC" -filter Lanczos -resize ${SIZE}x${SIZE} -unsharp 0x1+0.5+0 "$DIR/${APPID}.png"
    else
        magick "$SRC" -filter Lanczos -resize ${SIZE}x${SIZE} "$DIR/${APPID}.png"
    fi
    echo "Created ${DIR}/${APPID}.png"
done

echo ""
echo "Done! Icon structure:"
find hicolor -type f | sort
