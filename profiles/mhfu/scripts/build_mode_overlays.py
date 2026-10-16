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
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", nargs="?", default=BUILD_DIR, help="CMake build directory (default: out/mhfu)")
    parser.add_argument("-j", "--jobs", type=int, default=2, help="Parallel build jobs (default: 2)")
    parser.add_argument("--no-build", action="store_true", help="Only extract and recompile overlays to C++; do not run CMake build")
    args = parser.parse_args()

    build_dir = os.path.abspath(args.build_dir)
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
    targets = []
    for idx, filename, base in OVERLAYS:
        dump_path = os.path.join(EXTRACT_DIR, filename)
        print(f" -> Recompiling {filename} at {base}...")
        res = subprocess.run([
            sys.executable,
            os.path.join(TOOLS_DIR, "add_overlay.py"),
            "--no-build",
            build_dir,
            dump_path,
            base
        ], check=True, capture_output=True, text=True)
        for line in res.stdout.splitlines():
            if line.startswith("target: "):
                targets.append(line.split("target: ", 1)[1].strip())

    if args.no_build:
        print(f"[+] Recompiled {len(targets)} overlays into profiles/mhfu/overlays/")
        return 0

    print(f"[*] Re-running CMake and building {len(targets)} overlay modules in {build_dir}...")
    subprocess.run(["cmake", "-B", build_dir, "-S", "."], cwd=PROJECT_DIR, check=True)
    build_cmd = ["cmake", "--build", build_dir, "-j", str(args.jobs)]
    if targets:
        build_cmd.extend(["--target", *targets])
    subprocess.run(build_cmd, cwd=PROJECT_DIR, check=True)
    print(f"[+] All mode overlays successfully built and deployed to {os.path.join(build_dir, 'bin', 'overlays')}!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
