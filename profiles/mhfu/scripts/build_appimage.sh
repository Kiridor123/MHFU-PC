#!/usr/bin/env bash
# Build Pokke-x86_64.AppImage for Linux (Bazzite / SteamOS / Fedora / Ubuntu / Arch).
#
# Usage:
#   ./profiles/mhfu/scripts/build_appimage.sh [--iso /path/to/MHFU.iso] [--jobs N] [--host]
#
# By default, runs the build inside the official Steam Runtime 3 "sniper" SDK
# container via Podman or Docker so you do not need to install compiler/dev
# packages on immutable distributions like Bazzite or SteamOS, and the resulting
# AppImage runs on any Linux distribution with glibc 2.31+.
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
packaging="$profile_dir/packaging/linux"
# shellcheck source=../../mhp3rd/packaging/linux/sources.sh
source "$repo_dir/profiles/mhp3rd/packaging/linux/sources.sh"

iso_path=""
jobs=""
use_host=0
bootstrap=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --iso) iso_path="${2:?--iso requires a path}"; shift 2 ;;
        -j|--jobs) jobs="${2:?--jobs requires a number}"; shift 2 ;;
        --host) use_host=1; shift ;;
        --bootstrap) bootstrap=1; shift ;;
        -h|--help)
            sed -n '2,10s/^# \{0,1\}//p' "$0"
            exit 0
            ;;
        *)
            if [[ -z "$iso_path" && -f "$1" ]]; then
                iso_path="$1"
                shift
            else
                echo "Unknown option: $1" >&2
                exit 2
            fi
            ;;
    esac
done

if [[ -z "$jobs" ]]; then
    mem_gib="$(awk '/MemTotal/ { int_gib = int($2 / 1024 / 1024); print (int_gib > 0 ? int_gib : 4) }' /proc/meminfo 2>/dev/null || echo 8)"
    jobs=$(( mem_gib / 4 ))
    (( jobs < 1 )) && jobs=1
    (( jobs > 6 )) && jobs=6
fi

# 1. Prepare game data if not already present in profiles/mhfu/game
game_dir="$profile_dir/game"
if [[ -n "$iso_path" ]]; then
    iso_path="$(readlink -f "$iso_path")"
    echo "[*] Preparing MHFU game data from: $iso_path"
    python3 "$profile_dir/scripts/prepare_game.py" "$iso_path"
elif [[ ! -f "$game_dir/BOOT.ELF" ]]; then
    # Look for an ISO in the repository root
    candidate_iso=""
    if [[ -f "$repo_dir/Monster Hunter Freedom Unite.iso" ]]; then
        candidate_iso="$repo_dir/Monster Hunter Freedom Unite.iso"
    else
        for f in "$repo_dir"/*.iso "$repo_dir"/*.ISO; do
            if [[ -f "$f" ]]; then
                candidate_iso="$f"
                break
            fi
        done
    fi
    if [[ -n "$candidate_iso" ]]; then
        echo "[*] Found ISO at: $candidate_iso"
        python3 "$profile_dir/scripts/prepare_game.py" "$candidate_iso"
    elif [[ $bootstrap -eq 0 ]] && ! compgen -G "$profile_dir/generated/*.cpp" > /dev/null; then
        echo "Error: Game data not prepared yet in $game_dir." >&2
        echo "Please pass your Monster Hunter Freedom Unite (ULUS-10391) ISO:" >&2
        echo "  $0 --iso \"/path/to/Monster Hunter Freedom Unite.iso\"" >&2
        exit 1
    fi
fi

work="${POKKE_WORK:-$repo_dir/out/release-linux-mhfu}"
output_appimage="${POKKE_OUTPUT:-$repo_dir/Pokke-x86_64.AppImage}"
version="$(git -C "$repo_dir" describe --tags --always --dirty 2>/dev/null || echo "0.1.0")"
version="${version#v}"

if [[ $use_host -eq 1 ]]; then
    echo "[*] Building directly on host with $jobs parallel jobs..."
    POKKE_WORK="$work" POKKE_JOBS="$jobs" POKKE_OUTPUT="$output_appimage" POKKE_VERSION="$version" \
        bash "$packaging/build_in_sdk.sh"
    exit 0
fi

if command -v podman >/dev/null 2>&1; then
    container=podman
elif command -v docker >/dev/null 2>&1; then
    container=docker
else
    echo "Error: Neither podman nor docker found. Install podman, or pass --host if dependencies are installed." >&2
    exit 1
fi

mounts=(-v "$repo_dir:$repo_dir")
[[ "$work" == "$repo_dir"/* ]] || mounts+=(-v "$work:$work")
overlay_count="$(find "$profile_dir/overlays" -name 'meta.txt' 2>/dev/null | wc -l)"
if ! compgen -G "$profile_dir/generated/*.cpp" > /dev/null || [[ "$overlay_count" -lt 14 ]]; then
    for file in BOOT.ELF disc.iso; do
        link="$game_dir/$file"
        if [[ -L "$link" ]]; then
            target="$(readlink -f "$link")"
            if [[ -e "$target" && "$target" != "$repo_dir"/* ]]; then
                mounts+=(-v "$target:$target:ro")
            fi
        fi
    done
fi

user_args=()
if [[ "$container" == podman ]]; then
    user_args=(--userns=keep-id)
else
    user_args=(--user "$(id -u):$(id -g)")
fi

echo "[*] Launching Steam Runtime 3 SDK container ($container) with $jobs parallel jobs..."
mkdir -p "$work/home"
"$container" run --rm "${user_args[@]}" --security-opt label=disable \
    "${mounts[@]}" -w "$repo_dir" \
    -e HOME="$work/home" \
    -e POKKE_WORK="$work" \
    -e POKKE_JOBS="$jobs" \
    -e POKKE_OUTPUT="$output_appimage" \
    -e POKKE_VERSION="$version" \
    "$SDK_IMAGE" bash "$packaging/build_in_sdk.sh"
