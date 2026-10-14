#!/usr/bin/env python3
"""Run static analysis and generate the recompiled C++ corpus for Monster Hunter Freedom Unite.

Usage:
    python generate.py [build_dir]
"""

import argparse
import os
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", nargs="?", default=None, help="Framework build directory containing psp_analyze/psp_recomp")
    args = parser.parse_args()

    profile_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    repo_dir = os.path.dirname(os.path.dirname(profile_dir))

    # Look for built tools
    candidate_build_dirs = [
        args.build_dir if args.build_dir else None,
        os.path.join(repo_dir, "out", "mhfu"),
        os.path.join(repo_dir, "out", "framework"),
    ]
    build_dir = None
    for d in filter(None, candidate_build_dirs):
        if os.path.isdir(d):
            build_dir = d
            break

    if not build_dir:
        build_dir = os.path.join(repo_dir, "out", "framework")

    exe_suffix = ".exe" if sys.platform == "win32" else ""
    psp_analyze = os.path.join(build_dir, f"psp_analyze{exe_suffix}")
    psp_recomp = os.path.join(build_dir, f"psp_recomp{exe_suffix}")

    elf_path = os.path.join(profile_dir, "game", "BOOT.ELF")
    if not os.path.isfile(elf_path):
        print(f"Error: Missing executable at {elf_path}", file=sys.stderr)
        print("Please run scripts/prepare_game.py <image.iso> first.", file=sys.stderr)
        return 1

    if not os.path.isfile(psp_analyze) or not os.path.isfile(psp_recomp):
        print(f"Building psp_analyze and psp_recomp in {build_dir}...")
        subprocess.check_call(["cmake", "--build", build_dir, "--target", "psp_analyze", "psp_recomp"])

    analysis_dir = os.path.join(profile_dir, "analysis")
    generated_dir = os.path.join(profile_dir, "generated")
    os.makedirs(analysis_dir, exist_ok=True)
    os.makedirs(generated_dir, exist_ok=True)

    report_json = os.path.join(analysis_dir, "report.json")
    print(f"\n--- Running psp_analyze on {elf_path} ---")
    subprocess.check_call([psp_analyze, elf_path, report_json])

    print(f"\n--- Running psp_recomp AOT translation -> {generated_dir} ---")
    subprocess.check_call([psp_recomp, elf_path, "--auto", generated_dir])

    print("\nStatic recompilation corpus generation complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
