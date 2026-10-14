#pragma once

#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <filesystem>

namespace mhfu {

inline constexpr std::uint32_t kLoadBase = psprecomp::kDefaultPspUserLoadBase;
inline constexpr std::uint32_t kGuestRamBytes = 64u * 1024u * 1024u;

struct ProfilePaths {
    std::filesystem::path disc_image;   // UMD ISO; empty disables disc0:
    std::filesystem::path memory_stick; // host directory backing ms0:
};

// Installs the kernel and HLE modules, binds logging stubs for the remaining
// imports, and prepares the loader thread that runs module_start.
void install_profile(psprecomp::Runtime &runtime, const psprecomp::Elf32Image &elf, const ProfilePaths &paths);

// The host directory that backs ms0:, where the game's saves live under
// PSP/SAVEDATA.
[[nodiscard]] std::filesystem::path memory_stick_directory(const std::filesystem::path &game_dir);

// Overlay slots from MHFU's executable section table.
// Code overlays are loaded dynamically from USRDIR/DATA.BIN into these fixed slots.
inline constexpr std::uint32_t kOverlaySlots[] = {
    0x09A5F300u, // *_task mode overlays: game, lobby, edit, demo, option, etc. (12)
    0x09C19100u, // demo_sub, game_sub task sub-routines (2)
    0x09D1A280u, // em* monster AI and combat overlays (17)
    0x09D63100u, // stage* stage and quest overlays: stage000..stage266 (267)
    0x09D65500u, // end of the load image / debug.elf
};

} // namespace mhfu
