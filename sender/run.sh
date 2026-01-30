#!/bin/sh
# Use python3 -m platformio to avoid broken system pio (e.g. resultcallback on Python 3.12).
# Usage: sh run.sh   or   sh run.sh -t upload
exec python3 -m platformio run "$@"
