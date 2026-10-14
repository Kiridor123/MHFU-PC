#!/usr/bin/env bash
# Analyze the decrypted MHFU executable and regenerate the AOT C++ corpus.
#
#   generate.sh [build_dir]
#
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
python3 "$script_dir/generate.py" "${1:-}"
