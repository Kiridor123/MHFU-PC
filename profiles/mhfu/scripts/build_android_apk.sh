#!/usr/bin/env bash
# Builds Pokke (Monster Hunter Freedom Unite PC / Android Port)
# for Android arm64-v8a and packages a signed APK with touch controls.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
profile_dir="$(cd "$here/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"

export PATH="/home/linuxbrew/.linuxbrew/bin:/home/linuxbrew/.linuxbrew/sbin:$PATH"

work="${POKKE_ANDROID_WORK:-$repo_dir/out/release-android-mhfu}"
build="$work/build"
deps="$work/deps"
sources="$work/sources"
dist="$work/dist"
jobs="${POKKE_JOBS:-4}"
output_apk="${POKKE_OUTPUT_APK:-$repo_dir/Pokke-v1.2.0-android-arm64.apk}"

mkdir -p "$work" "$deps" "$sources" "$dist" "$build"

step() { printf '\n=== %s ===\n' "$*"; }

# 1. Check Android SDK and NDK
export ANDROID_HOME="${ANDROID_HOME:-$repo_dir/out/android-sdk}"
export ANDROID_NDK="${ANDROID_NDK:-$ANDROID_HOME/ndk/28.2.13676358}"
toolchain="$ANDROID_NDK/build/cmake/android.toolchain.cmake"

[[ -f "$toolchain" ]] || { echo "error: Android toolchain not found at $toolchain" >&2; exit 1; }

# 2. Check SDL3 for Android
sdl_install="$deps/sdl3-install"
sdl_source="$deps/SDL3-3.4.16"
if [[ ! -f "$sdl_install/lib/libSDL3.so" ]]; then
    step "Building SDL3 for Android arm64-v8a"
    if [[ ! -f "$sources/SDL3-3.4.16.tar.gz" ]]; then
        if [[ -f "$repo_dir/out/release-linux-mhfu/sources/SDL3-3.4.16.tar.gz" ]]; then
            cp "$repo_dir/out/release-linux-mhfu/sources/SDL3-3.4.16.tar.gz" "$sources/"
        else
            curl -fsSL -o "$sources/SDL3-3.4.16.tar.gz" "https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz"
        fi
    fi
    tar -xzf "$sources/SDL3-3.4.16.tar.gz" -C "$deps"
    cmake -S "$sdl_source" -B "$deps/sdl3-build" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-29 \
        -DCMAKE_BUILD_TYPE=Release \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX="$sdl_install"
    cmake --build "$deps/sdl3-build" -j "$jobs"
    cmake --install "$deps/sdl3-build"
fi

# 3. Font and FFmpeg sources
if [[ ! -f "$sources/NotoSansCJKjp-Regular.otf" && -f "$repo_dir/out/release-linux-mhfu/sources/NotoSansCJKjp-Regular.otf" ]]; then
    cp "$repo_dir/out/release-linux-mhfu/sources/NotoSansCJKjp-Regular.otf" "$sources/"
    cp "$repo_dir/out/release-linux-mhfu/sources/NotoSansCJK-LICENSE.txt" "$sources/" 2>/dev/null || true
fi
if [[ ! -f "$sources/ffmpeg-7.1.5.tar.xz" && -f "$repo_dir/out/release-linux-mhfu/sources/ffmpeg-7.1.5.tar.xz" ]]; then
    cp "$repo_dir/out/release-linux-mhfu/sources/ffmpeg-7.1.5.tar.xz" "$sources/"
fi

# 4. Configure Pokke for Android
step "Configuring Pokke for Android arm64-v8a"
cmake -S "$repo_dir" -B "$build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-29 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPSPRECOMP_PROFILE=mhfu \
    -DMHFU_RELEASE=ON \
    -DMHP3RD_RELEASE=ON \
    -DMHP3RD_ANDROID_APP=ON \
    -DMHP3RD_FFMPEG=bundled \
    -DMHP3RD_FFMPEG_DOWNLOAD_DIR="$sources" \
    -DSDL3_DIR="$sdl_install/lib/cmake/SDL3" \
    -DCMAKE_FIND_ROOT_PATH="$sdl_install" \
    -DMHFU_GLSLANG="$(which glslangValidator)" \
    -DPSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH=ON \
    -DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=ON \
    -DPSPRECOMP_BUILD_TESTS=OFF \
    -DPSPRECOMP_GENERATED_JOBS="$jobs"

# 5. Build libmain.so and all 16 overlay libraries
step "Compiling libmain.so and all overlay modules for arm64-v8a"
cmake --build "$build" -j "$jobs"

# 6. Package APK
step "Packaging Android APK: $output_apk"
VERSION_NAME="1.2.0" \
VERSION_CODE=4 \
FONT_DIR="$sources" \
ANDROID_NDK="$ANDROID_NDK" \
ANDROID_HOME="$ANDROID_HOME" \
"$profile_dir/packaging/android/build_apk.sh" \
    "$build" \
    "$sdl_source" \
    "$sdl_install/lib/libSDL3.so" \
    "$output_apk"

echo ""
echo "========================================================"
echo "  Android APK successfully created!"
echo "  File:    $output_apk"
echo "  Size:    $(du -h "$output_apk" | cut -f1)"
echo "  SHA-256: $(sha256sum "$output_apk" | cut -d' ' -f1)"
echo "========================================================"
