#pragma once

// Profile adapter bridging MHFU constants and slots into the reusable host engine.
#include "mhfu_profile.hpp"

namespace mhp3rd {

inline constexpr std::uint32_t kLoadBase = mhfu::kLoadBase;
inline constexpr std::uint32_t kGuestRamBytes = mhfu::kGuestRamBytes;

using ProfilePaths = mhfu::ProfilePaths;

inline std::filesystem::path memory_stick_directory(const std::filesystem::path &game_dir) {
    return mhfu::memory_stick_directory(game_dir);
}

inline constexpr std::uint32_t kOverlaySlots[] = {
    0x09A5F300u, // *_task mode overlays (12)
    0x09C19100u, // demo_sub, game_sub (2)
    0x09D1A280u, // em* monster overlays (17)
    0x09D63100u, // stage* stage/quest overlays (267)
    0x09D65580u, // end of the load image
};

inline void install_profile(psprecomp::Runtime &runtime, const psprecomp::Elf32Image &elf, const ProfilePaths &paths) {
    mhfu::install_profile(runtime, elf, paths);
}

} // namespace mhp3rd
