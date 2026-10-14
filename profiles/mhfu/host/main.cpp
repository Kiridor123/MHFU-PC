#include "mhfu_profile.hpp"

#include "app_paths.hpp"
#include "platform/utf8_path.hpp"

#include "install/game_identity.hpp"
#include "kernel/kernel.hpp"
#include "settings/settings.hpp"

#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"
#include "psprecomp/sha256.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

struct EmbeddedNid {
    const char *library;
    std::uint32_t nid;
    const char *name;
};
constexpr EmbeddedNid kEmbeddedNids[] = {
#include "nid_table.inc"
};

std::uint64_t configured_max_dispatches() {
    constexpr std::uint64_t default_limit = 4'000'000'000ull;
    const char *text = std::getenv("PSPRECOMP_MAX_DISPATCHES");
    if (text == nullptr || *text == '\0') return default_limit;
    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 0);
    if (errno == ERANGE || end == text || *end != '\0' || parsed == 0u)
        throw psprecomp::Error(std::string("Invalid PSPRECOMP_MAX_DISPATCHES value: ") + text);
    return static_cast<std::uint64_t>(parsed);
}

constexpr const char *kUsage =
    "Monster Hunter Freedom Unite (PC Native Port)\n\n"
    "Usage:\n"
    "  MHFU [game_dir]\n"
    "  MHFU --install <image.iso>\n"
    "  MHFU --data-dir <dir>\n"
    "  MHFU --help\n\n"
    "Options:\n"
    "  game_dir        Directory containing BOOT.ELF, disc.iso, and ms0/\n"
    "  --install image Set up game data from your legally acquired ISO image\n"
    "  --data-dir dir  Specify custom user data directory for saves and settings\n"
    "  --portable      Store data locally next to the executable\n";

struct Options {
    std::optional<std::filesystem::path> game_dir;
    bool install = false;
    std::optional<std::filesystem::path> install_image;
    std::optional<std::filesystem::path> data_dir;
    bool portable = false;
};

class UsageError final : public std::runtime_error {
public:
    explicit UsageError(const std::string &message) : std::runtime_error(message) {}
};

Options parse_options(const std::vector<std::string> &arguments) {
    Options options;
    for (std::size_t i = 1; i < arguments.size(); ++i) {
        const std::string &arg = arguments[i];
        if (arg == "--help" || arg == "-h") throw UsageError("");
        if (arg == "--install") {
            options.install = true;
            if (i + 1 < arguments.size() && arguments[i + 1][0] != '-')
                options.install_image = mhp3rd::path_from_utf8(arguments[++i]);
        } else if (arg == "--portable") {
            options.portable = true;
        } else if (arg == "--data-dir") {
            if (i + 1 >= arguments.size()) throw UsageError("--data-dir requires a directory path");
            options.data_dir = mhp3rd::path_from_utf8(arguments[++i]);
        } else if (!arg.empty() && arg[0] == '-') {
            throw UsageError("Unknown option: " + arg);
        } else if (!options.game_dir) {
            options.game_dir = mhp3rd::path_from_utf8(arg);
        } else {
            throw UsageError("Unexpected argument: " + arg);
        }
    }
    return options;
}

struct GameFiles {
    std::filesystem::path executable;
    std::filesystem::path disc_image;
    std::filesystem::path memory_stick;
};

GameFiles files_in_game_directory(const std::filesystem::path &game_dir) {
    GameFiles files;
    files.executable = game_dir / "BOOT.ELF";
    if (!std::filesystem::exists(files.executable)) files.executable = game_dir / "EBOOT.ELF";
    files.disc_image = game_dir / "disc.iso";
    files.memory_stick = mhfu::memory_stick_directory(game_dir);
    if (!std::filesystem::exists(files.disc_image)) {
        std::cerr << "Warning: " << mhp3rd::path_to_utf8(files.disc_image) << " not found; disc0: is unavailable\n";
        files.disc_image.clear();
    }
    return files;
}

std::optional<GameFiles> locate_game(const Options &options) {
    if (options.game_dir) return files_in_game_directory(*options.game_dir);
    if (const std::filesystem::path dir = mhp3rd::environment_path("MHFU_GAME_DIR"); !dir.empty())
        return files_in_game_directory(dir);

    // Check default profile directory in local checkout
    const std::filesystem::path checkout_game_dir = "profiles/mhfu/game";
    if (std::filesystem::exists(checkout_game_dir / "BOOT.ELF") || std::filesystem::exists(checkout_game_dir / "disc.iso"))
        return files_in_game_directory(checkout_game_dir);

    std::cerr << "Error: No game data found.\n"
              << "Please supply your legally acquired copy of " << mhfu::install::kGameTitle << " ("
              << mhfu::install::kDiscIdDisplay << "):\n"
              << "  MHFU profiles/mhfu/game\n"
              << "or run:\n"
              << "  python profiles/mhfu/scripts/prepare_game.py <path_to_mhfu.iso>\n";
    return std::nullopt;
}

} // namespace

int main(int argc, char **argv) {
    mhp3rd::use_utf8_console();
    try {
        Options options;
        try {
            options = parse_options(mhp3rd::utf8_arguments(argc, argv));
        } catch (const UsageError &e) {
            if (*e.what() == '\0') {
                std::cout << kUsage;
                return 0;
            }
            std::cerr << "MHFU: " << e.what() << "\n\n" << kUsage;
            return 2;
        }

        const auto files = locate_game(options);
        if (!files) return 1;

        const std::filesystem::path &executable = files->executable;
        mhfu::ProfilePaths paths;
        paths.disc_image = files->disc_image;
        paths.memory_stick = files->memory_stick;

        if (!std::filesystem::is_regular_file(executable))
            throw psprecomp::Error("Missing executable at " + mhp3rd::path_to_utf8(executable));

        const std::string sha256 = psprecomp::sha256_file(executable);
        if (sha256 != mhfu::install::kExecutableSha256)
            std::cerr << "Warning: Executable SHA-256 (" << sha256 << ") does not match supported release ("
                      << mhfu::install::kExecutableSha256 << ")\n";

        const psprecomp::Elf32Image elf = psprecomp::Elf32Image::from_file(executable);
        psprecomp::Runtime runtime(mhfu::kGuestRamBytes);

        for (const EmbeddedNid &entry : kEmbeddedNids)
            runtime.nids().add(entry.library, entry.nid, entry.name);

        (void)elf.load_and_relocate(runtime.memory(), mhfu::kLoadBase);
        psprecomp::register_generated_functions(runtime);
        mhfu::install_profile(runtime, elf, paths);

        std::cout << "========================================================\n"
                  << "  Monster Hunter Freedom Unite (PC Native Port)\n"
                  << "========================================================\n"
                  << "Executable: " << mhp3rd::path_to_utf8(executable) << "\n"
                  << "SHA-256:    " << sha256 << "\n"
                  << "Disc image: " << (paths.disc_image.empty() ? "<none>" : mhp3rd::path_to_utf8(paths.disc_image)) << "\n"
                  << "RAM:        64 MiB\n"
                  << "Entry:      " << psprecomp::hex32(elf.runtime_entry(mhfu::kLoadBase)) << "\n"
                  << "Functions:  " << runtime.function_count() << "\n"
                  << "--------------------------------------------------------\n"
                  << "Starting native execution...\n\n";

        if (runtime.function_count() == 0u) {
            std::cout << "Error: No generated functions linked. Run profiles/mhfu/scripts/generate.py and rebuild.\n";
            return 3;
        }

        runtime.run(elf.runtime_entry(mhfu::kLoadBase), configured_max_dispatches());
        std::cout << "\nRuntime stopped: " << runtime.stop_reason() << "\n";
        std::cout << mhp3rd::kernel().describe_threads() << "\n";
        runtime.report_hle_histogram();
        return runtime.stop_reason().empty() ? 0 : 4;
    } catch (const std::exception &e) {
        std::cerr << "MHFU error: " << e.what() << "\n";
        return 1;
    }
}
