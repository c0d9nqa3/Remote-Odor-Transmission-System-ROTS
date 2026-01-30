#!/bin/sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENDOR="$ROOT/vendor"
CUBE="$VENDOR/STM32CubeF4"
URL="https://github.com/STMicroelectronics/STM32CubeF4"

if [ -d "$CUBE/Drivers" ]; then
    echo "STM32CubeF4 already at $CUBE"
    exit 0
fi
command -v git >/dev/null 2>&1 || { echo "Need git."; exit 1; }
mkdir -p "$VENDOR"
echo "Cloning STM32CubeF4 into $CUBE ..."
git clone --recursive --depth 1 "$URL" "$CUBE"
echo "Done."
