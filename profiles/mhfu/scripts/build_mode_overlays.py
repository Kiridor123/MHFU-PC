#!/usr/bin/env python3
"""Batch compile all 298 Monster Hunter Freedom Unite overlays (14 mode overlays,
17 monster AI overlays, and 267 stage/quest overlays).
"""
import glob
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

PROFILE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_DIR = os.path.dirname(os.path.dirname(PROFILE_DIR))
TOOLS_DIR = os.path.join(PROFILE_DIR, "tools")
BUILD_DIR = os.path.join(PROJECT_DIR, "out", "mhfu")
EXTRACT_DIR = os.path.join(PROFILE_DIR, "extracted_overlays")
OVERLAY_DIR = os.path.join(PROFILE_DIR, "overlays")
HEADER_BYTES = 64

MODE_OVERLAYS = [
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

MONSTER_OVERLAYS = [
    (6095, "06095_em01.ovl", "0x09d1a280"),
    (6096, "06096_em02.ovl", "0x09d1a280"),
    (6097, "06097_em07.ovl", "0x09d1a280"),
    (6098, "06098_em14.ovl", "0x09d1a280"),
    (6099, "06099_em15.ovl", "0x09d1a280"),
    (6100, "06100_em17.ovl", "0x09d1a280"),
    (6101, "06101_em20.ovl", "0x09d1a280"),
    (6102, "06102_em21.ovl", "0x09d1a280"),
    (6103, "06103_em33.ovl", "0x09d1a280"),
    (6104, "06104_em40.ovl", "0x09d1a280"),
    (6105, "06105_em54.ovl", "0x09d1a280"),
    (6106, "06106_em55.ovl", "0x09d1a280"),
    (6107, "06107_em58.ovl", "0x09d1a280"),
    (6108, "06108_em59.ovl", "0x09d1a280"),
    (6109, "06109_em75.ovl", "0x09d1a280"),
    (6110, "06110_em82.ovl", "0x09d1a280"),
    (6111, "06111_em83.ovl", "0x09d1a280"),
]

STAGE_OVERLAYS = [
    (5542 + i, f"{5542 + i:05d}_stage{i:03d}.ovl", "0x09d63100")
    for i in range(267)
]


def fnv1a64(data):
    value = 0xCBF29CE484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def parse_header(data, base):
    if data[:4] != b"MWo3":
        raise SystemExit("not an overlay image: expected the MWo3 magic")
    _id, load, code_size, data_size = struct.unpack("<4I", data[4:20])
    if load != base:
        raise SystemExit(f"image loads at {load:#010x}, not at {base:#010x}")
    name = data[32:HEADER_BYTES].split(b"\0")[0].decode("ascii", "replace")
    name = re.sub(r"\W", "_", os.path.splitext(name)[0])
    return name, HEADER_BYTES + code_size + data_size, code_size


def generate_bundle(recompiler, bundle_name, overlays, unity_chunks=0):
    bundle_dir = os.path.join(OVERLAY_DIR, bundle_name)
    bundle_marker = os.path.join(bundle_dir, "bundle.txt")
    if os.path.exists(bundle_marker) and os.path.exists(os.path.join(bundle_dir, "bundle_module.cpp")):
        print(f" -> Bundle {bundle_name} already generated ({len(overlays)} overlays)")
        return f"overlay_{bundle_name}"

    if os.path.exists(bundle_dir):
        shutil.rmtree(bundle_dir)
    os.makedirs(bundle_dir, exist_ok=True)

    entries = []
    unity_buffers = [[] for _ in range(unity_chunks)] if unity_chunks > 0 else None

    with tempfile.TemporaryDirectory() as tmp_root:
        for i, (_idx, filename, base_str) in enumerate(overlays):
            dump_path = os.path.join(EXTRACT_DIR, filename)
            base = int(base_str, 0)
            base_text = f"0x{base:08X}"
            with open(dump_path, "rb") as handle:
                data = handle.read()
            name, image_size, code_size = parse_header(data, base)
            digest = fnv1a64(data[:HEADER_BYTES + code_size])
            prefix = f"ovl{base:08X}_{name}_{digest:016X}"

            sub_tmp = os.path.join(tmp_root, prefix)
            os.makedirs(sub_tmp, exist_ok=True)
            elf_path = os.path.join(sub_tmp, "overlay.elf")
            subprocess.run(
                [sys.executable, os.path.join(TOOLS_DIR, "wrap_overlay.py"), dump_path, base_text, elf_path],
                check=True,
                stdout=subprocess.DEVNULL,
            )
            subprocess.run(
                [recompiler, elf_path, "--auto", sub_tmp, base_text, "--prefix", prefix],
                check=True,
                stdout=subprocess.DEVNULL,
            )

            units_hpp = os.path.join(sub_tmp, f"{prefix}_units.hpp")
            shutil.copy2(units_hpp, os.path.join(bundle_dir, f"{prefix}_units.hpp"))

            unit_cpps = sorted(glob.glob(os.path.join(sub_tmp, "generated_unit_*.cpp")))
            reg_cpp = os.path.join(sub_tmp, f"{prefix}_registry.cpp")

            if unity_buffers is not None:
                chunk_idx = i % unity_chunks
                for u_cpp in unit_cpps:
                    with open(u_cpp, "r") as f:
                        unity_buffers[chunk_idx].append(f.read())
                with open(reg_cpp, "r") as f:
                    unity_buffers[chunk_idx].append(f.read())
            else:
                for u_cpp in unit_cpps:
                    u_name = os.path.basename(u_cpp).replace("generated_", f"{prefix}_")
                    shutil.copy2(u_cpp, os.path.join(bundle_dir, u_name))
                shutil.copy2(reg_cpp, os.path.join(bundle_dir, f"{prefix}_registry.cpp"))

            entries.append((prefix, name, base, image_size, code_size, digest))

    if unity_buffers is not None:
        for chunk_idx, parts in enumerate(unity_buffers):
            if not parts:
                continue
            chunk_path = os.path.join(bundle_dir, f"bundle_chunk_{chunk_idx:02d}.cpp")
            with open(chunk_path, "w") as out:
                out.write("\n".join(parts))

    module_cpp = os.path.join(bundle_dir, "bundle_module.cpp")
    with open(module_cpp, "w") as out:
        out.write("#define MHP3RD_OVERLAY_MODULE 1\n")
        out.write('#include "overlay_module.hpp"\n')
        out.write('#include "psprecomp/runtime.hpp"\n\n')
        out.write("namespace psprecomp {\n")
        for prefix, *_ in entries:
            out.write(f"void register_{prefix}_functions(Runtime &runtime);\n")
        out.write("} // namespace psprecomp\n\n")
        out.write("namespace {\n")
        for prefix, *_ in entries:
            out.write(f"void install_{prefix}(psprecomp::Runtime &runtime) {{\n")
            out.write(f"    psprecomp::register_{prefix}_functions(runtime);\n")
            out.write("}\n")
        out.write("\nconstexpr mhp3rd::OverlayBundleEntry kEntries[] = {\n")
        for prefix, name, base, image_size, code_size, digest in entries:
            out.write(
                f'    {{{{mhp3rd::kOverlayAbiVersion, 0x{base:08X}u, {image_size}u, {code_size}u, '
                f'0x{digest:016X}ull, "{name}"}}, &install_{prefix}}},\n'
            )
        out.write("};\n")
        out.write("} // namespace\n\n")
        out.write("const mhp3rd::OverlayBundleEntry *mhp3rd_overlay_bundle(std::size_t *count) {\n")
        out.write("    if (count != nullptr) *count = sizeof(kEntries) / sizeof(kEntries[0]);\n")
        out.write("    return kEntries;\n")
        out.write("}\n")

    with open(bundle_marker, "w") as out:
        out.write(f"bundle={bundle_name}\ncount={len(entries)}\n")

    print(f" -> Generated bundle {bundle_name} with {len(entries)} overlays")
    return f"overlay_{bundle_name}"


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

    all_overlays = MODE_OVERLAYS + MONSTER_OVERLAYS + STAGE_OVERLAYS
    os.makedirs(EXTRACT_DIR, exist_ok=True)
    missing = [str(idx) for idx, fname, _ in all_overlays if not os.path.exists(os.path.join(EXTRACT_DIR, fname))]
    if missing:
        print(f"[*] Extracting {len(missing)} overlays from {iso_path}...")
        subprocess.run([
            sys.executable,
            os.path.join(TOOLS_DIR, "databin.py"),
            iso_path,
            "extract",
            EXTRACT_DIR,
            *missing
        ], check=True)

    recompiler = os.path.join(build_dir, "psp_recomp.exe" if os.name == "nt" else "psp_recomp")
    if not os.path.exists(recompiler):
        for fallback in (os.path.join(PROJECT_DIR, "out", "mhfu", "psp_recomp"),
                         os.path.join(PROJECT_DIR, "out", "release-linux-mhfu", "build", "psp_recomp")):
            if os.path.exists(fallback):
                recompiler = fallback
                break

    print("[*] Ensuring 14 core mode overlays are recompiled...")
    targets = []
    existing_meta = glob.glob(os.path.join(OVERLAY_DIR, "ovl*", "meta.txt"))
    if len(existing_meta) < len(MODE_OVERLAYS):
        for _idx, filename, base in MODE_OVERLAYS:
            dump_path = os.path.join(EXTRACT_DIR, filename)
            res = subprocess.run([
                sys.executable,
                os.path.join(TOOLS_DIR, "add_overlay.py"),
                "--no-build",
                os.path.dirname(recompiler),
                dump_path,
                base
            ], check=True, capture_output=True, text=True)
            for line in res.stdout.splitlines():
                if line.startswith("target: "):
                    targets.append(line.split("target: ", 1)[1].strip())
    else:
        for meta in sorted(existing_meta):
            prefix = os.path.basename(os.path.dirname(meta))
            targets.append(f"overlay_{prefix}")

    print("[*] Recompiling 17 monster AI overlays (em01..em83) into ovl09D1A280_monsters_bundle...")
    targets.append(generate_bundle(recompiler, "ovl09D1A280_monsters_bundle", MONSTER_OVERLAYS, unity_chunks=0))

    print("[*] Recompiling 267 stage/quest overlays (stage000..stage266) into ovl09D63100_stages_bundle...")
    targets.append(generate_bundle(recompiler, "ovl09D63100_stages_bundle", STAGE_OVERLAYS, unity_chunks=4))

    if args.no_build:
        print(f"[+] Recompiled all 298 overlays into {len(targets)} module targets in profiles/mhfu/overlays/")
        return 0

    print(f"[*] Re-running CMake and building {len(targets)} overlay module targets in {build_dir}...")
    subprocess.run(["cmake", "-B", build_dir, "-S", "."], cwd=PROJECT_DIR, check=True)
    build_cmd = ["cmake", "--build", build_dir, "-j", str(args.jobs)]
    if targets:
        build_cmd.extend(["--target", *targets])
    subprocess.run(build_cmd, cwd=PROJECT_DIR, check=True)
    print(f"[+] All 298 overlays successfully built and deployed to {os.path.join(build_dir, 'bin', 'overlays')}!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
