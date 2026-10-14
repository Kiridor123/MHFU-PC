#!/usr/bin/env bash
# Populate profiles/mhfu/game from the user's legally acquired MHFU ISO.
#
#   prepare_game.sh <image.iso>
#
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <Monster_Hunter_Freedom_Unite.iso>" >&2
    exit 2
fi

script_dir="$(cd "$(dirname "$0")" && pwd)"
python3 "$script_dir/prepare_game.py" "$1"
