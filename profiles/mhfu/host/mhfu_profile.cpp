#include "mhfu_profile.hpp"

#include "hle/hle_common.hpp"
#include "kernel/kernel.hpp"
#include "overlays.hpp"

#include "psprecomp/common.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mhfu {
namespace {

constexpr std::string_view kBootPath = "disc0:/PSP_GAME/SYSDIR/BOOT.BIN";
constexpr std::uint32_t kBootArgumentAddress = 0x08800000u;

struct StubState {
    std::string label;
    std::uint64_t calls{};
};

void register_logging_stub(psprecomp::Runtime &runtime, const psprecomp::PspImport &import) {
    const std::string name = runtime.nids().resolve(import.library, import.nid).value_or(psprecomp::hex32(import.nid));
    auto state = std::make_shared<StubState>();
    state->label = import.library + "::" + name;
    runtime.register_hle(import.library, import.nid, [state](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
        if (state->calls++ == 0u) {
            std::cerr << "[hle-stub] " << state->label
                      << " a0=" << psprecomp::hex32(ctx.gpr[4])
                      << " a1=" << psprecomp::hex32(ctx.gpr[5])
                      << " a2=" << psprecomp::hex32(ctx.gpr[6])
                      << " a3=" << psprecomp::hex32(ctx.gpr[7])
                      << " ra=" << psprecomp::hex32(ctx.gpr[31]) << "\n";
        }
        mhp3rd::kernel().finish(ctx, 0u);
    });
}

std::uint32_t load_image_end(const psprecomp::Elf32Image &elf) {
    std::uint64_t end = 0u;
    for (std::size_t i = 0; i < elf.segments().size(); ++i) {
        const auto &segment = elf.segments()[i];
        if (segment.type != 1u) continue;
        end = std::max<std::uint64_t>(end, elf.segment_runtime_address(i, kLoadBase) + segment.memory_size);
    }
    return static_cast<std::uint32_t>(end);
}

} // namespace

std::filesystem::path memory_stick_directory(const std::filesystem::path &game_dir) {
    return game_dir / "ms0";
}

void install_profile(psprecomp::Runtime &runtime, const psprecomp::Elf32Image &elf, const ProfilePaths &paths) {
    const auto module = elf.find_module_info(runtime.memory(), kLoadBase);
    if (!module) throw psprecomp::Error("PSP module info not found in executable");

    mhp3rd::kernel().install(runtime, module->gp, load_image_end(elf));
    mhp3rd::install_overlay_support(runtime);

    mhp3rd::HleRegistrar hle(runtime);
    mhp3rd::register_threadman(hle);
    mhp3rd::register_sysmem(hle);
    mhp3rd::register_io(hle, paths.disc_image, paths.memory_stick);
    mhp3rd::register_system(hle);
    mhp3rd::register_media(hle);
    mhp3rd::register_atrac(hle);
    mhp3rd::register_mpeg(hle);
    mhp3rd::register_font(hle);
    mhp3rd::register_utility(hle, paths.memory_stick);
    mhp3rd::register_adhoc(hle);

    const bool strict = std::getenv("MHFU_STRICT_HLE") != nullptr;
    std::size_t stubbed = 0u;
    const auto imports = elf.scan_imports(runtime.memory(), *module);
    for (const auto &import : imports) {
        if (hle.bound(import.library, import.nid) || strict) continue;
        register_logging_stub(runtime, import);
        ++stubbed;
    }
    std::cout << "HLE imports: " << imports.size() << " total, " << hle.count() << " implemented, " << stubbed
              << " logging stubs" << (strict ? " (strict mode)" : "") << "\n";

    mhp3rd::install_overlay_support(runtime);

    auto &ctx = runtime.cpu();
    auto &memory = runtime.memory();
    for (std::size_t i = 0; i < kBootPath.size(); ++i)
        memory.store8(kBootArgumentAddress + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>(kBootPath[i]));
    memory.store8(kBootArgumentAddress + static_cast<std::uint32_t>(kBootPath.size()), 0u);
    ctx.set_gpr(4, static_cast<std::uint32_t>(kBootPath.size() + 1u));
    ctx.set_gpr(5, kBootArgumentAddress);
    mhp3rd::kernel().start_loader_thread(ctx, elf.runtime_entry(kLoadBase), 0u);
}

} // namespace mhfu
