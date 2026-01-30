#!/bin/sh
# Run in WSL or Linux to fix CRLF in shell scripts. Usage: sh fix_crlf.sh
for f in build.sh fix_crlf.sh sender/run.sh receiver/scripts/fetch_cube.sh; do
  [ -f "$f" ] && sed -i 's/\r$//' "$f" 2>/dev/null || true
done
echo "Done. Run: sh build.sh"
