#!/bin/bash
# Collect firmware files into firmware/ directory

set -e

echo "Creating firmware directories..."
mkdir -p firmware/receiver
mkdir -p firmware/sender

echo "Copying receiver firmware..."
if [ -f "receiver/build/rots_receiver.bin" ]; then
    cp receiver/build/rots_receiver.bin firmware/receiver/
    cp receiver/build/rots_receiver.hex firmware/receiver/
    cp receiver/build/rots_receiver.elf firmware/receiver/
    echo "✓ Receiver firmware copied"
else
    echo "⚠ Receiver not built yet. Run: cd receiver && make"
fi

echo "Copying sender firmware..."
if [ -f "sender/.pio/build/esp32dev/firmware.bin" ]; then
    cp sender/.pio/build/esp32dev/firmware.bin firmware/sender/
    cp sender/.pio/build/esp32dev/firmware.elf firmware/sender/
    echo "✓ Sender firmware copied"
else
    echo "⚠ Sender not built yet. Run: cd sender && ~/.local/bin/pio run"
fi

echo ""
echo "Firmware files:"
ls -lh firmware/receiver/ 2>/dev/null || echo "  (no receiver firmware)"
ls -lh firmware/sender/ 2>/dev/null || echo "  (no sender firmware)"

echo ""
echo "Done! Firmware files are in the firmware/ directory"
