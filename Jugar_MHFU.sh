#!/usr/bin/env bash
# Lanzador de un clic para Pokke - Monster Hunter Freedom Unite (Linux / Bazzite / Steam Deck)
set -euo pipefail

HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
ulimit -s 65536 2>/dev/null || true

if [[ -x "$HERE/Pokke-x86_64.AppImage" ]]; then
    exec "$HERE/Pokke-x86_64.AppImage" "$@"
elif [[ -x "$HERE/out/mhfu/bin/Pokke" ]]; then
    exec "$HERE/out/mhfu/bin/Pokke" "$@"
elif [[ -x "$HERE/out/release-linux-mhfu/Pokke.AppDir/AppRun" ]]; then
    exec "$HERE/out/release-linux-mhfu/Pokke.AppDir/AppRun" "$@"
else
    echo "[ERROR] No se encontro Pokke-x86_64.AppImage ni out/mhfu/bin/Pokke." >&2
    echo "Genera el AppImage primero ejecutando:" >&2
    echo "  ./profiles/mhfu/scripts/build_appimage.sh --iso \"/ruta/a/Monster Hunter Freedom Unite.iso\"" >&2
    exit 1
fi
