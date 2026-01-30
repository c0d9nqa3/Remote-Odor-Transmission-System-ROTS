#!/bin/sh
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

check_cmd() { command -v "$1" >/dev/null 2>&1; }
if ! check_cmd arm-none-eabi-gcc; then
    echo "Missing arm-none-eabi-gcc. Install: sudo apt-get install -y gcc-arm-none-eabi make git"
    exit 1
fi
if ! check_cmd make; then
    echo "Missing make. Install: sudo apt-get install -y make"
    exit 1
fi

echo "== Building receiver (STM32) =="
(cd receiver && make)
echo "Receiver done: receiver/build/rots_receiver.elf .hex .bin"

echo "== Building sender (ESP32) =="
run_pio() {
    (cd "$ROOT/sender" && (pio run 2>/dev/null || platformio run 2>/dev/null || sh run.sh 2>/dev/null || python3 -m platformio run))
}
if run_pio 2>/dev/null; then
    echo "Sender done. Upload: cd sender && sh run.sh -t upload"
else
    echo "Skip sender. Install: pip install -U platformio. Then: cd sender && sh run.sh"
fi
