# Monster Hunter Freedom Unite (PC Native Port Profile)

This profile enables the static recompilation and native execution of **Monster Hunter Freedom Unite** (`ULUS-10391`, v1.01) on PC (Windows, Linux, macOS) using the **PSPRecomp** framework.

Rather than running through traditional emulator overhead, the game's original MIPS Allegrex machine code is translated ahead-of-time (AOT) into native C++ instructions and paired with a modern host runtime powered by **Vulkan** and **SDL3**.

---

## Legal & "Bring Your Own Copy" Policy

This repository contains **NO copyrighted game code, assets, audio, textures, or binaries**.
- Users must supply their own legally acquired ISO disc image of *Monster Hunter Freedom Unite*.
- The executable files and translation corpora are generated locally on the user's machine and are ignored by version control.
- At runtime, the native application streams music, cinematics, models, and game data directly from the user's verified disc image or local user data directory.

---

## Profile Architecture

- **Platform Target:** Windows (x86_64), Linux, macOS
- **Graphics API:** Vulkan (PSP GE hardware instruction translator)
- **Input & Windowing:** SDL3 (supports keyboard & mouse, modern gamepads with analog dual-stick camera controls)
- **Audio Subsystem:** SDL3 Audio + ATRAC3 / RIFF decoder
- **Guest Architecture:** Sony PSP MIPS Allegrex + VFPU (Vector Floating Point Unit)
- **Base Memory Address:** `0x08804000` (64 MiB user RAM layout)
- **Entry Point:** `0x08906A54`
- **Global Pointer (GP):** `0x089D3BA0`

### Overlay Memory Layout

Monster Hunter Freedom Unite organizes dynamic content into **298 code overlays** (`MWo3`) loaded into 4 fixed runtime slots from `DATA.BIN`:

| Slot Address | Overlays Count | Description |
| :--- | :--- | :--- |
| `0x09A5F300` | 12 | Mode and task overlays (`game_task`, `lobby_task`, `edit_task`, etc.) |
| `0x09C19100` | 2 | Sub-task routines (`demo_sub`, `game_sub`) |
| `0x09D1A280` | 17 | Large monster AI and combat logic (`em01` to `em17`) |
| `0x09D63100` | 267 | Stage, map, and quest definitions (`stage000` to `stage266`) |

---

## Building from Source

### Prerequisites
- **CMake** (v3.20 or newer)
- **C++20 Compiler** (GCC 12+, Clang 15+, or MSVC 2022+)
- **Python** (v3.10+)
- **Vulkan SDK**
- **SDL3**

### Step-by-Step Instructions

1. **Prepare Game Data from your ISO:**
   ```bash
   python profiles/mhfu/scripts/prepare_game.py "/path/to/Monster Hunter Freedom Unite.iso"
   ```
   This extracts the decrypted `BOOT.ELF` and sets up the disc reference in `profiles/mhfu/game/`.

2. **Generate Native Recompiled Corpus:**
   ```bash
   python profiles/mhfu/scripts/generate.py
   ```
   Runs `psp_analyze` and `psp_recomp` to produce the local C++ translation units in `profiles/mhfu/generated/`.

3. **Configure with CMake:**
   ```bash
   cmake -S . -B out/mhfu -DPSPRECOMP_PROFILE=mhfu
   ```

4. **Build the Native Executable:**
   ```bash
   cmake --build out/mhfu -j 4
   ```

5. **Run the Game:**
   ```bash
   ./out/mhfu/bin/MHFU
   ```
