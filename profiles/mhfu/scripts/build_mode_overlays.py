#!/usr/bin/env python3
"""Batch compile Monster Hunter Freedom Unite mode overlays.
"""
import os
import subprocess
import sys

PROFILE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_DIR = os.path.dirname(os.path.dirname(PROFILE_DIR))
TOOLS_DIR = os.path.join(PROFILE_DIR, "tools")
BUILD_DIR = os.path.join(PROJECT_DIR, "out", "mhfu")
EXTRACT_DIR = os.path.join(PROFILE_DIR, "extracted_overlays")

OVERLAYS = [
    # (index, filename, load_base)
    (63, "00063_demo_task.ovl", "0x09a5f300"),
    (64, "00064_edit_task.ovl", "0x09a5f300"),
    (65, "00065_cont_task.ovl", "0x09a5f300"),
    (66, "00066_option_task.ovl", "0x09a5f300"),
    (67, "00067_gallery_task.ovl", "0x09a5f300"),
    (68, "00068_download_task.ovl", "0x09a5f300"),
    (69, "00069_install_task.ovl", "0x09a5f300"),
    (70, "00070_lobby_task.ovl", "0x09a5f300"),
    (71, "00071_game_task.ovl", "0x09a5f300"),
    (72, "00072_show_select_task.ovl", "0x09a5f300"),
    (73, "00073_arcade_task.ovl", "0x09a5f300"),
    (74, "00074_movie_task.ovl", "0x09a5f300"),
    (75, "00075_demo_sub.ovl", "0x09c19100"),
    (76, "00076_game_sub.ovl", "0x09c19100"),
]

def main():
    iso_path = os.path.join(PROFILE_DIR, "game", "disc.iso")
    if not os.path.exists(iso_path):
        iso_path = os.path.join(PROJECT_DIR, "Monster Hunter Freedom Unite.iso")
    
    print(f"[*] Extracting {len(OVERLAYS)} core mode overlays from {iso_path}...")
    os.makedirs(EXTRACT_DIR, exist_ok=True)
    indices = [str(item[0]) for item in OVERLAYS]
    subprocess.run([
        sys.executable,
        os.path.join(TOOLS_DIR, "databin.py"),
        iso_path,
        "extract",
        EXTRACT_DIR,
        *indices
    ], check=True)

    print("[*] Recompiling overlays to native C++ with psp_recomp...")
    for idx, filename, base in OVERLAYS:
        dump_path = os.path.join(EXTRACT_DIR, filename)
        print(f" -> Recompiling {filename} at {base}...")
        subprocess.run([
            sys.executable,
            os.path.join(TOOLS_DIR, "add_overlay.py"),
            "--no-build",
            BUILD_DIR,
            dump_path,
            base
        ], check=True)

    print("[*] Re-running CMake and building all overlay DLLs...")
    subprocess.run(["cmake", "-B", BUILD_DIR, "-S", "."], cwd=PROJECT_DIR, check=True)
    subprocess.run(["cmake", "--build", BUILD_DIR], cwd=PROJECT_DIR, check=True)
    print("[+] All mode overlays successfully built and deployed to out/mhfu/bin/overlays/!")

if __name__ == "__main__":
    main()
