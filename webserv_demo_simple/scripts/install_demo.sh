#!/usr/bin/env bash
set -e

HERE="$(cd "$(dirname "$0")/.." && pwd)"
DEST="/tmp/webserv-demo-simple"

rm -rf "$DEST"
mkdir -p "$DEST/www" "$DEST/cgi" "$DEST/uploads" "$DEST/test" "$DEST/forbidden"

cp -R "$HERE/www/." "$DEST/www/"
cp -R "$HERE/cgi/." "$DEST/cgi/"
cp -R "$HERE/test/." "$DEST/test/"

printf 'This directory has no index file.\n' > "$DEST/forbidden/readme.txt"
chmod +x "$DEST"/cgi/*.py

echo "Installed demo files in $DEST"
echo
echo "Run:"
echo "  ./webserv $HERE/config/demo.conf"
echo
echo "Open:"
echo "  http://127.0.0.1:8080/"
