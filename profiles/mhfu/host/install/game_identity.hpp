#pragma once

// Identity of the Monster Hunter Freedom Unite release supported by this profile.
// The hashes match config/mhfu_ulus10391.toml and the profile README.
namespace mhfu::install {

// DISC_ID in PSP_GAME/PARAM.SFO.
inline constexpr const char *kDiscId = "ULUS10391";
inline constexpr const char *kDiscIdDisplay = "ULUS-10391";
inline constexpr const char *kGameTitle = "Monster Hunter Freedom Unite";

inline constexpr const char *kExecutablePathOnDisc = "PSP_GAME/SYSDIR/BOOT.BIN";
inline constexpr const char *kParamSfoPathOnDisc = "PSP_GAME/PARAM.SFO";

// SHA-256 of PSP_GAME/SYSDIR/EBOOT.BIN as it is on the original disc.
inline constexpr const char *kEncryptedExecutableSha256 =
    "3354adb3ee40821f230c4055b9bf7cc5d4c864e42abb74ce8ac022933f9697a7";

// SHA-256 of the decrypted executable (BOOT.BIN / BOOT.ELF) the recompiled code was generated from.
inline constexpr const char *kExecutableSha256 =
    "3070954e2dc88fd0b0836c0c1dc526531aca4a213b0f18806e324963efb514f4";

} // namespace mhfu::install

namespace mhp3rd::install {
using mhfu::install::kDiscId;
using mhfu::install::kDiscIdDisplay;
using mhfu::install::kGameTitle;
using mhfu::install::kExecutablePathOnDisc;
using mhfu::install::kParamSfoPathOnDisc;
using mhfu::install::kEncryptedExecutableSha256;
using mhfu::install::kExecutableSha256;
}
