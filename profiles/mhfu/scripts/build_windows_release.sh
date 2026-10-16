#!/usr/bin/env bash
# Builds Pokke (Monster Hunter Freedom Unite PC) for Windows x86_64
# using MinGW-w64 cross-compilation and stages a self-contained release folder.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
profile_dir="$(cd "$here/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"

export PATH="/home/linuxbrew/.linuxbrew/bin:/home/linuxbrew/.linuxbrew/sbin:$PATH"

work="${POKKE_WINDOWS_WORK:-$repo_dir/out/release-windows-mhfu}"
build="$work/build"
deps="$work/deps"
downloads="$work/downloads"
dist="$work/dist"
release_folder="$repo_dir/Pokke-Windows-x64"
jobs="${POKKE_JOBS:-4}"

mkdir -p "$deps" "$downloads" "$dist" "$build"

step() { printf '\n=== %s ===\n' "$*"; }

# 1. Verify cross-compilation tools
step "Checking MinGW-w64 cross-compilation toolchain"
command -v x86_64-w64-mingw32-g++ >/dev/null || { echo "error: x86_64-w64-mingw32-g++ not found" >&2; exit 1; }
command -v cmake >/dev/null || { echo "error: cmake not found" >&2; exit 1; }
command -v ninja >/dev/null || { echo "error: ninja not found" >&2; exit 1; }
command -v glslangValidator >/dev/null || { echo "error: glslangValidator not found" >&2; exit 1; }

# 2. Setup SDL3 MinGW development package
sdl3_zip="$downloads/SDL3-devel-3.4.16-mingw.zip"
if [[ ! -d "$deps/sdl3" ]]; then
    step "Downloading SDL3 MinGW development package"
    curl -fsSL -o "$sdl3_zip" "https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-devel-3.4.16-mingw.zip"
    unzip -q -o "$sdl3_zip" -d "$deps/"
    mv "$deps/SDL3-3.4.16/x86_64-w64-mingw32" "$deps/sdl3"
    rm -rf "$deps/SDL3-3.4.16"
fi

# 3. Setup Vulkan headers and import library
if [[ ! -f "$deps/vulkan/lib/libvulkan-1.a" ]]; then
    step "Setting up Vulkan headers and generating libvulkan-1.a"
    mkdir -p "$deps/vulkan/lib" "$deps/vulkan/include"
    cp -rL /home/linuxbrew/.linuxbrew/include/vulkan "$deps/vulkan/include/"
    cp -rL /home/linuxbrew/.linuxbrew/include/vk_video "$deps/vulkan/include/"
    vulkan_def="$work/vulkan-1.def"
    echo "LIBRARY vulkan-1.dll" > "$vulkan_def"
    echo "EXPORTS" >> "$vulkan_def"
    grep -o 'VKAPI_CALL vk[A-Za-z0-9_]*' "$deps/vulkan/include/vulkan/vulkan_core.h" | awk '{print $2}' | sort -u >> "$vulkan_def"
    x86_64-w64-mingw32-dlltool -d "$vulkan_def" -l "$deps/vulkan/lib/libvulkan-1.a"
fi

# 4. Check for Noto Sans CJK font
cjk_font="$downloads/NotoSansCJKjp-Regular.otf"
if [[ ! -f "$cjk_font" ]]; then
    step "Downloading NotoSansCJKjp-Regular.otf"
    curl -fsSL -o "$cjk_font" "https://raw.githubusercontent.com/googlefonts/noto-cjk/main/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf"
fi

# 5. Configure CMake for Windows x86_64
step "Configuring Pokke for Windows x86_64 (MinGW)"
cmake -S "$repo_dir" -B "$build" -G Ninja \
    --toolchain "$profile_dir/packaging/windows/mingw-w64-x86_64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPSPRECOMP_PROFILE=mhfu \
    -DMHFU_RELEASE=ON \
    -DMHP3RD_RELEASE=ON \
    -DMHP3RD_FFMPEG=bundled \
    -DMHP3RD_FFMPEG_DOWNLOAD_DIR="$downloads" \
    -DSDL3_DIR="$deps/sdl3/lib/cmake/SDL3" \
    -DVulkan_INCLUDE_DIR="$deps/vulkan/include" \
    -DVulkan_LIBRARY="$deps/vulkan/lib/libvulkan-1.a" \
    -DMHFU_GLSLANG="$(which glslangValidator)" \
    -DPSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH=ON \
    -DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=ON \
    -DMHFU_AVX2=ON \
    -DPSPRECOMP_BUILD_TESTS=OFF \
    -DPSPRECOMP_GENERATED_JOBS="$jobs"

# 6. Build Pokke.exe and all overlay libraries
step "Compiling Pokke.exe and all overlay modules"
cmake --build "$build" -j "$jobs"

# 7. Stage standalone release directory
step "Staging standalone release folder: $release_folder"
rm -rf "$release_folder"
mkdir -p "$release_folder/overlays" \
         "$release_folder/fonts" \
         "$release_folder/licenses"

cp "$build/bin/Pokke.exe" "$release_folder/"
x86_64-w64-mingw32-strip --strip-unneeded "$release_folder/Pokke.exe"

# Overlays
shopt -s nullglob
overlay_dlls=("$build/bin/overlays/"*.dll)
shopt -u nullglob
if [[ ${#overlay_dlls[@]} -gt 0 ]]; then
    cp "${overlay_dlls[@]}" "$release_folder/overlays/"
    x86_64-w64-mingw32-strip --strip-unneeded "$release_folder/overlays/"*.dll
fi

# SDL3
cp "$deps/sdl3/bin/SDL3.dll" "$release_folder/"

# FFmpeg DLLs
for dll in "$build/bin/"*.dll; do
    cp "$dll" "$release_folder/"
done

# 8. Locate and copy MinGW runtime DLLs
copy_mingw_dll() {
    local name="$1"
    local found=""
    for dir in \
        /home/linuxbrew/.linuxbrew/Cellar/mingw-w64/*/toolchain-x86_64/x86_64-w64-mingw32 \
        /usr/x86_64-w64-mingw32 \
        /usr/lib/gcc/x86_64-w64-mingw32/*; do
        if [[ -f "$dir/bin/$name" ]]; then found="$dir/bin/$name"; break; fi
        if [[ -f "$dir/lib/$name" ]]; then found="$dir/lib/$name"; break; fi
    done
    if [[ -z "$found" ]]; then
        found="$(find /home/linuxbrew/.linuxbrew/Cellar/mingw-w64/ -name "$name" 2>/dev/null | grep x86_64 | head -n 1 || true)"
    fi
    if [[ -n "$found" && -f "$found" ]]; then
        cp "$found" "$release_folder/"
    else
        echo "warning: $name could not be located" >&2
    fi
}

copy_mingw_dll "libwinpthread-1.dll"
copy_mingw_dll "libgcc_s_seh-1.dll"
copy_mingw_dll "libstdc++-6.dll"

# Fonts
cp "$cjk_font" "$release_folder/fonts/"

# Licenses
cp "$repo_dir/LICENSE" "$release_folder/licenses/Pokke-LICENSE.txt"
cp "$profile_dir/../mhp3rd/packaging/THIRD_PARTY_NOTICES.md" "$release_folder/licenses/THIRD_PARTY_NOTICES.md"
cp "$deps/sdl3/share/licenses/SDL3/LICENSE.txt" "$release_folder/licenses/SDL3-LICENSE.txt" 2>/dev/null || true
if [[ -f "$build/bin/FFmpeg-LICENSE.txt" ]]; then
    cp "$build/bin/FFmpeg-LICENSE.txt" "$build/bin/FFmpeg-SOURCE.txt" "$release_folder/licenses/"
fi

# Portable marker so data/ms0 stays right next to Pokke.exe
printf "%s\r\n" "This file makes Pokke portable: saves and settings are stored in the data folder next to Pokke.exe." > "$release_folder/portable.txt"

# Default optimized settings.ini (1x internal scale for integrated GPUs, 30 fps, vsync, etc.)
mkdir -p "$release_folder/data"
cat << 'EOF' | sed 's/$/\r/' > "$release_folder/data/settings.ini"
[video]
internal_scale = 1
frame_rate = 30
frame_rate_auto = true
ui_textures = off
texture_pack = false
gpu_compat = auto
present_mode = fifo

[text]
crisp = false
EOF

# README
cat << 'EOF' | sed 's/$/\r/' > "$release_folder/README.txt"
========================================================
  Pokke - Monster Hunter Freedom Unite (PC Native Port)
  Windows x86-64 Release
========================================================

Instrucciones de uso:
1. Extrae esta carpeta en cualquier lugar de tu PC (por ejemplo, en el Escritorio o Disco C:).
2. Coloca tu archivo .iso de Monster Hunter Freedom Unite (versión USA ULUS-10391) dentro de esta carpeta o seleccionalo cuando te lo pida.
3. Ejecuta Pokke.exe.
4. Presiona [F1] o Esc durante el juego para abrir el menu de opciones.
5. Si juegas en una laptop con grafica integrada (como Intel HD Graphics), en F1 > Video puedes pulsar "Low-Spec / iGPU Preset (Max FPS)" para rendimiento maximo a 30 FPS estables.

Para jugar con mando:
- Conecta cualquier mando (Xbox, PlayStation, etc.) y se detectara automaticamente.

Tus partidas guardadas se guardan de forma portatil en:
  data\ms0\PSP\SAVEDATA\
========================================================
EOF

step "Packaging ZIP archive: $dist/Pokke-Windows-x64.zip"
rm -f "$dist/Pokke-Windows-x64.zip" "$repo_dir/Pokke-Windows-x64.zip"
(cd "$repo_dir" && zip -q -r "$dist/Pokke-Windows-x64.zip" "Pokke-Windows-x64")
cp "$dist/Pokke-Windows-x64.zip" "$repo_dir/Pokke-Windows-x64.zip"

echo ""
echo "========================================================"
echo "  Windows Standalone Release successfully created!"
echo "  Release Folder: $release_folder"
echo "  Release ZIP:    $repo_dir/Pokke-Windows-x64.zip"
echo "  Executable:     $release_folder/Pokke.exe"
echo "  Overlay DLLs:   $(find "$release_folder/overlays" -name '*.dll' | wc -l)"
echo "========================================================"
