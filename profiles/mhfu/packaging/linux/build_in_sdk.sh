#!/usr/bin/env bash
# Builds Pokke (MHFU-PC) and packages it into a self-contained AppImage.
# Runs either inside the Steam Runtime 3 "sniper" SDK container (started by
# profiles/mhfu/scripts/build_appimage.sh) or directly on a Linux host with
# the required build tools.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
profile_dir="$(cd "$here/../.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
work="${POKKE_WORK:-$repo_dir/out/release-linux-mhfu}"
jobs="${POKKE_JOBS:-4}"
output_appimage="${POKKE_OUTPUT:-$repo_dir/Pokke-x86_64.AppImage}"

# shellcheck source=../../../mhp3rd/packaging/linux/sources.sh
source "$repo_dir/profiles/mhp3rd/packaging/linux/sources.sh"

sources="$work/sources"
deps="$work/deps"
deps_build="$work/deps-build"
build="$work/build"
appdir="$work/Pokke.AppDir"
dist="$work/dist"

if command -v gcc-14 >/dev/null 2>&1 && command -v g++-14 >/dev/null 2>&1; then
    export CC=gcc-14 CXX=g++-14
fi
export CCACHE_DIR="$work/ccache" CCACHE_MAXSIZE=20G
export PKG_CONFIG_PATH="$deps/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
mkdir -p "$sources" "$deps" "$deps_build" "$dist"

step() { printf '\n=== %s ===\n' "$*"; }

fetch() {
    local target="$sources/$1"
    if [[ -f "$target" ]] && echo "$3  $target" | sha256sum -c --status; then return; fi
    curl -fsSL -o "$target.part" "$2"
    if ! echo "$3  $target.part" | sha256sum -c --status; then
        echo "error: $2 does not match its pinned SHA-256" >&2
        exit 1
    fi
    mv "$target.part" "$target"
}

stamp_matches() { [[ -f "$deps/.$1.stamp" && "$(cat "$deps/.$1.stamp")" == "$2" ]]; }

step "Fetching pinned dependencies (SDL3, fonts, appimagetool)"
fetch "SDL3-$SDL3_VERSION.tar.gz" "$SDL3_URL" "$SDL3_SHA256"
fetch NotoSansCJKjp-Regular.otf "$NOTO_CJK_URL" "$NOTO_CJK_SHA256"
fetch NotoSansCJK-LICENSE.txt "$NOTO_CJK_LICENSE_URL" "$NOTO_CJK_LICENSE_SHA256"

appimagetool_bin="$sources/appimagetool-x86_64.AppImage"
if [[ ! -x "$appimagetool_bin" ]]; then
    curl -fsSL -o "$appimagetool_bin.part" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$appimagetool_bin.part"
    mv "$appimagetool_bin.part" "$appimagetool_bin"
fi

sdl_stamp="$SDL3_VERSION $SDL3_SHA256"
if ! stamp_matches sdl3 "$sdl_stamp"; then
    step "Building SDL3 $SDL3_VERSION"
    rm -rf "$deps_build/SDL3-$SDL3_VERSION"
    tar -xzf "$sources/SDL3-$SDL3_VERSION.tar.gz" -C "$deps_build"
    cmake -S "$deps_build/SDL3-$SDL3_VERSION" -B "$deps_build/sdl3-build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps" -DCMAKE_INSTALL_LIBDIR=lib \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
    cmake --build "$deps_build/sdl3-build" -j "$jobs"
    cmake --install "$deps_build/sdl3-build"
    echo "$sdl_stamp" > "$deps/.sdl3.stamp"
fi

step "Configuring Pokke (MHFU Release)"
cmake -S "$repo_dir" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPSPRECOMP_PROFILE=mhfu \
    -DMHFU_RELEASE=ON \
    -DMHP3RD_FFMPEG=bundled \
    -DMHP3RD_FFMPEG_DOWNLOAD_DIR="$sources" \
    -DCMAKE_PREFIX_PATH="$deps" \
    -DPSPRECOMP_GENERATED_JOBS="$jobs"

step "Building static recompiler tools (psp_analyze, psp_recomp)"
cmake --build "$build" -j "$jobs" --target psp_analyze psp_recomp

need_reconfigure=0
if [[ -f "$profile_dir/game/BOOT.ELF" ]]; then
    if ! compgen -G "$profile_dir/generated/*.cpp" > /dev/null; then
        step "Generating AOT C++ corpus from MHFU BOOT.ELF"
        python3 "$profile_dir/scripts/generate.py" "$build"
        need_reconfigure=1
    else
        step "Reusing existing AOT C++ corpus in profiles/mhfu/generated/"
    fi
fi

mkdir -p "$profile_dir/overlays"
overlay_meta_count="$(find "$profile_dir/overlays" -name 'meta.txt' 2>/dev/null | wc -l)"
if [[ -e "$profile_dir/game/disc.iso" && "$overlay_meta_count" -lt 14 ]]; then
    step "Extracting and recompiling 14 MHFU mode overlays"
    python3 "$profile_dir/scripts/build_mode_overlays.py" --no-build "$build"
    need_reconfigure=1
elif [[ "$overlay_meta_count" -gt 0 ]]; then
    step "Reusing existing recompiled overlay sources ($overlay_meta_count overlays)"
fi

step "Building Pokke executable and all overlay modules"
if [[ "$need_reconfigure" -eq 1 ]]; then
    cmake -S "$repo_dir" -B "$build"
fi
cmake --build "$build" -j "$jobs"

step "Staging Pokke.AppDir"
rm -rf "$appdir"
mkdir -p "$appdir/usr/bin/lib" \
         "$appdir/usr/bin/overlays" \
         "$appdir/usr/bin/fonts" \
         "$appdir/usr/bin/licenses" \
         "$appdir/usr/share/applications" \
         "$appdir/usr/share/icons/hicolor/256x256/apps" \
         "$appdir/usr/share/icons/hicolor/scalable/apps"

install -m 755 "$here/AppRun" "$appdir/AppRun"
install -m 644 "$here/Pokke.desktop" "$appdir/Pokke.desktop"
install -m 644 "$here/Pokke.desktop" "$appdir/usr/share/applications/Pokke.desktop"
install -m 644 "$profile_dir/packaging/mhfu_icon.png" "$appdir/Pokke.png"
install -m 644 "$profile_dir/packaging/mhfu_icon.png" "$appdir/usr/share/icons/hicolor/256x256/apps/Pokke.png"
install -m 644 "$profile_dir/packaging/mhfu_icon.svg" "$appdir/Pokke.svg"
install -m 644 "$profile_dir/packaging/mhfu_icon.svg" "$appdir/usr/share/icons/hicolor/scalable/apps/Pokke.svg"
ln -sf Pokke.png "$appdir/.DirIcon"

install -m 755 "$build/bin/Pokke" "$appdir/usr/bin/Pokke"
strip --strip-unneeded "$appdir/usr/bin/Pokke"
if compgen -G "$build/bin/overlays/*.so" > /dev/null; then
    cp "$build/bin/overlays/"*.so "$appdir/usr/bin/overlays/"
    strip --strip-unneeded "$appdir/usr/bin/overlays/"*.so
fi

# Copy bundled shared libraries (SDL3, FFmpeg libavcodec/libavutil) into usr/bin/lib/
needed() { objdump -p "$1" | awk '$1 == "NEEDED" { print $2 }'; }
pending=("$appdir/usr/bin/Pokke")
while [[ ${#pending[@]} -gt 0 ]]; do
    current="${pending[0]}"
    pending=("${pending[@]:1}")
    for soname in $(needed "$current"); do
        [[ -e "$appdir/usr/bin/lib/$soname" ]] && continue
        for dir in "$build/bin/lib" "$deps/lib"; do
            [[ -e "$dir/$soname" ]] || continue
            cp -L "$dir/$soname" "$appdir/usr/bin/lib/$soname"
            chmod 644 "$appdir/usr/bin/lib/$soname"
            strip --strip-unneeded "$appdir/usr/bin/lib/$soname"
            pending+=("$appdir/usr/bin/lib/$soname")
            break
        done
    done
done

cp "$sources/NotoSansCJKjp-Regular.otf" "$appdir/usr/bin/fonts/"
cp "$repo_dir/LICENSE" "$appdir/usr/bin/licenses/Pokke-LICENSE.txt"
cp "$repo_dir/profiles/mhp3rd/packaging/THIRD_PARTY_NOTICES.md" "$appdir/usr/bin/licenses/THIRD_PARTY_NOTICES.md"
if [[ -f "$deps_build/SDL3-$SDL3_VERSION/LICENSE.txt" ]]; then
    cp "$deps_build/SDL3-$SDL3_VERSION/LICENSE.txt" "$appdir/usr/bin/licenses/SDL3-LICENSE.txt"
fi
if [[ -f "$build/bin/lib/FFmpeg-COPYING.LGPLv2.1.txt" ]]; then
    cp "$build/bin/lib/FFmpeg-COPYING.LGPLv2.1.txt" "$build/bin/lib/FFmpeg-SOURCE.txt" "$appdir/usr/bin/licenses/"
fi
cp "$sources/NotoSansCJK-LICENSE.txt" "$appdir/usr/bin/licenses/NotoSansCJK-OFL.txt"

step "Verifying staged binaries and shared libraries"
missing="$(LD_LIBRARY_PATH='' ldd "$appdir/usr/bin/Pokke" | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
    echo "error: unresolved libraries in Pokke:" >&2
    echo "$missing" >&2
    exit 1
fi
ldd "$appdir/usr/bin/Pokke" | grep "$appdir/usr/bin/lib" || {
    echo "error: bundled libraries in usr/bin/lib are not used by Pokke" >&2
    exit 1
}
shopt -s nullglob
overlay_sos=("$appdir/usr/bin/overlays/"*.so)
shopt -u nullglob
if objdump -p "$appdir/usr/bin/Pokke" "${overlay_sos[@]}" | grep -q 'NEEDED.*libstdc++'; then
    echo "error: a staged binary dynamically links libstdc++.so" >&2
    exit 1
fi

step "Creating AppImage: $output_appimage"
rm -f "$output_appimage"
ARCH=x86_64 VERSION="${POKKE_VERSION:-0.1.0}" \
    "$appimagetool_bin" --appimage-extract-and-run --no-appstream "$appdir" "$output_appimage"
chmod +x "$output_appimage"
cp -f "$output_appimage" "$dist/Pokke-x86_64.AppImage"

echo ""
echo "========================================================"
echo "  AppImage successfully created!"
echo "  File:     $output_appimage"
echo "  SHA-256:  $(sha256sum "$output_appimage" | cut -d' ' -f1)"
echo "  Overlays: $(find "$appdir/usr/bin/overlays" -name '*.so' | wc -l)"
echo "========================================================"
