#!/usr/bin/env python3
"""Prepare Monster Hunter Freedom Unite (ULUS-10391) game data from a PSP ISO image.

Usage:
    python prepare_game.py <image.iso> [output_dir]

Validates the user's legally acquired copy of Monster Hunter Freedom Unite:
- Reads and checks PARAM.SFO (DISC_ID: ULUS10391)
- Extracts decrypted BOOT.BIN as BOOT.ELF and validates its SHA-256
- Sets up disc.iso link/reference and simulated Memory Stick (ms0/) directory
"""

import argparse
import hashlib
import os
import shutil
import struct
import sys

EXPECTED_DISC_ID = "ULUS10391"
EXPECTED_ELF_SHA256 = "3070954e2dc88fd0b0836c0c1dc526531aca4a213b0f18806e324963efb514f4"
BLOCK_SIZE = 2048


def find_in_iso(iso_file, target_path):
    """Locate a file in ISO 9660 filesystem, return (lba, size)."""
    iso_file.seek(16 * BLOCK_SIZE)
    pvd = iso_file.read(BLOCK_SIZE)
    if pvd[1:6] != b"CD001":
        raise ValueError("Invalid ISO 9660 disc image.")

    target_path = target_path.strip("/").upper()

    def walk_dir(lba, size, parent=""):
        iso_file.seek(lba * BLOCK_SIZE)
        data = iso_file.read(size)
        offset = 0
        while offset < len(data):
            rec_len = data[offset]
            if rec_len == 0:
                offset = ((offset // BLOCK_SIZE) + 1) * BLOCK_SIZE
                continue
            rec = data[offset:offset + rec_len]
            offset += rec_len
            name = rec[33:33 + rec[32]].decode("ascii", errors="replace").split(";")[0]
            if name in ("\x00", "\x01"):
                continue
            cur_path = f"{parent}/{name}".lstrip("/")
            rec_lba = struct.unpack_from("<I", rec, 2)[0]
            rec_size = struct.unpack_from("<I", rec, 10)[0]
            flags = rec[25]
            if flags & 2:  # Directory
                res = walk_dir(rec_lba, rec_size, cur_path)
                if res:
                    return res
            elif cur_path.upper() == target_path:
                return rec_lba, rec_size
        return None

    root_rec = pvd[156:190]
    root_lba = struct.unpack_from("<I", root_rec, 2)[0]
    root_size = struct.unpack_from("<I", root_rec, 10)[0]
    return walk_dir(root_lba, root_size)


def extract_iso_file(iso_file, target_path):
    """Extract full content of a file from ISO."""
    loc = find_in_iso(iso_file, target_path)
    if not loc:
        return None
    lba, size = loc
    iso_file.seek(lba * BLOCK_SIZE)
    return iso_file.read(size)


def parse_param_sfo(sfo_bytes):
    """Parse key-value pairs from PARAM.SFO."""
    if sfo_bytes[:4] != b"\x00PSF":
        return {}
    magic, version, key_off, val_off, count = struct.unpack("<4sIIII", sfo_bytes[:20])
    entries = {}
    for i in range(count):
        koff, fmt, dlen, mlen, voff = struct.unpack("<HHIII", sfo_bytes[20 + i*16:36 + i*16])
        key = sfo_bytes[key_off + koff:].split(b"\x00")[0].decode("ascii", errors="ignore")
        val = sfo_bytes[val_off + voff:val_off + voff + dlen]
        if fmt in (0x0004, 0x0204):
            entries[key] = val.decode("utf-8", errors="ignore").rstrip("\x00")
        elif fmt == 0x0404 and len(val) >= 4:
            entries[key] = struct.unpack("<I", val[:4])[0]
        else:
            entries[key] = val
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("iso", help="Path to Monster Hunter Freedom Unite ISO")
    parser.add_argument("output", nargs="?", default=None, help="Profile game directory (default: profiles/mhfu/game)")
    args = parser.parse_args()

    iso_path = os.path.abspath(args.iso)
    if not os.path.isfile(iso_path):
        print(f"Error: ISO file not found: {iso_path}", file=sys.stderr)
        return 1

    profile_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    game_dir = os.path.abspath(args.output) if args.output else os.path.join(profile_dir, "game")
    os.makedirs(game_dir, exist_ok=True)
    os.makedirs(os.path.join(game_dir, "ms0"), exist_ok=True)

    print(f"Opening disc image: {iso_path}")
    with open(iso_path, "rb") as f:
        sfo_data = extract_iso_file(f, "PSP_GAME/PARAM.SFO")
        if not sfo_data:
            print("Error: /PSP_GAME/PARAM.SFO not found in ISO.", file=sys.stderr)
            return 1
        sfo = parse_param_sfo(sfo_data)
        disc_id = sfo.get("DISC_ID", "")
        title = sfo.get("TITLE", "")
        print(f"Title: {title}")
        print(f"Disc ID: {disc_id}")

        if disc_id != EXPECTED_DISC_ID:
            print(f"Warning: Expected Disc ID '{EXPECTED_DISC_ID}', got '{disc_id}'.", file=sys.stderr)

        print("Extracting /PSP_GAME/SYSDIR/BOOT.BIN...")
        boot_data = extract_iso_file(f, "PSP_GAME/SYSDIR/BOOT.BIN")
        if not boot_data:
            print("Error: /PSP_GAME/SYSDIR/BOOT.BIN not found in ISO.", file=sys.stderr)
            return 1

        actual_sha256 = hashlib.sha256(boot_data).hexdigest()
        print(f"BOOT.ELF SHA256: {actual_sha256}")
        if actual_sha256 != EXPECTED_ELF_SHA256:
            print(f"Warning: Executable SHA256 does not match expected ({EXPECTED_ELF_SHA256}).", file=sys.stderr)

        out_elf = os.path.join(game_dir, "BOOT.ELF")
        with open(out_elf, "wb") as elf_f:
            elf_f.write(boot_data)
        print(f"Wrote executable: {out_elf} ({len(boot_data)} bytes)")

    # Create link or copy for disc.iso
    out_iso = os.path.join(game_dir, "disc.iso")
    if os.path.exists(out_iso):
        os.remove(out_iso)
    try:
        os.link(iso_path, out_iso)
        print(f"Hard-linked disc.iso -> {iso_path}")
    except (OSError, AttributeError):
        try:
            os.symlink(iso_path, out_iso)
            print(f"Symlinked disc.iso -> {iso_path}")
        except OSError:
            print("Note: Copying disc.iso (hard link / symlink not supported across volumes)...")
            shutil.copyfile(iso_path, out_iso)
            print(f"Copied disc.iso -> {out_iso}")

    print(f"\nGame data successfully prepared in: {game_dir}")
    print("Ready for static recompilation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
