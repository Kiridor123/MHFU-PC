#include "mhfu_profile.hpp"

#include "app_paths.hpp"
#include "platform/utf8_path.hpp"

#include "adhoc/session.hpp"
#include "install/game_identity.hpp"
#include "install/installer.hpp"
#include "install/user_data.hpp"
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

#if defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace {

struct EmbeddedNid {
    const char *library;
    std::uint32_t nid;
    const char *name;
};
constexpr EmbeddedNid kEmbeddedNids[] = {
#include "nid_table.inc"
};

#if defined(__linux__)
// Chained AOT calls nest native frames deeply and need a 64 MiB stack.
// Raise RLIMIT_STACK and re-exec once if the launcher did not already set it.
void ensure_main_stack(char **argv) {
    constexpr rlim_t kWanted = 64ull * 1024u * 1024u;
    constexpr const char *kMarker = "MHP3RD_STACK_RAISED";
    rlimit limit{};
    if (getrlimit(RLIMIT_STACK, &limit) != 0) return;
    if (limit.rlim_cur == RLIM_INFINITY || limit.rlim_cur >= kWanted) return;
    if (std::getenv(kMarker) != nullptr) return;
    const rlim_t target = limit.rlim_max == RLIM_INFINITY || limit.rlim_max >= kWanted ? kWanted : limit.rlim_max;
    if (target <= limit.rlim_cur) {
        std::cerr << "Warning: stack is limited to " << (limit.rlim_cur >> 20)
                  << " MiB and cannot be raised to 64 MiB\n";
        return;
    }
    limit.rlim_cur = target;
    if (setrlimit(RLIMIT_STACK, &limit) != 0) return;
    setenv(kMarker, "1", 1);
    const std::filesystem::path self = mhp3rd::executable_path();
    execv(self.empty() ? "/proc/self/exe" : self.c_str(), argv);
    std::cerr << "Warning: could not restart with a larger stack; deep call chains may overflow\n";
}
#endif

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
    "Pokke - Monster Hunter Freedom Unite (PC Native Port)\n\n"
    "Usage:\n"
    "  Pokke [game_dir]\n"
    "  Pokke --launcher\n"
    "  Pokke --install [image.iso [--in-place]]\n"
    "  Pokke --data-dir <dir>\n"
    "  Pokke --portable\n"
    "  Pokke --help\n\n"
    "Options:\n"
    "  game_dir        Directory containing BOOT.ELF, disc.iso, and ms0/\n"
    "  --launcher      Open graphical setup & settings wizard\n"
    "  --install       Open graphical setup screens to select ISO\n"
    "  --install image Set up game data directly from image.iso without GUI\n"
    "  --in-place      Use image where it is instead of copying\n"
    "  --data-dir dir  Specify custom user data directory for saves and settings\n"
    "  --portable      Store data locally in data/ next to Pokke\n";

struct Options {
    std::optional<std::filesystem::path> game_dir;
    bool install = false;
    std::optional<std::filesystem::path> install_image;
    bool in_place = false;
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
        if (arg == "--install" || arg == "--launcher" || arg == "--setup") {
            options.install = true;
            if (i + 1 < arguments.size() && arguments[i + 1][0] != '-')
                options.install_image = mhp3rd::path_from_utf8(arguments[++i]);
        } else if (arg == "--in-place") {
            options.in_place = true;
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

std::filesystem::path checkout_game_directory() {
#if defined(MHFU_DEFAULT_GAME_DIR)
    return MHFU_DEFAULT_GAME_DIR;
#else
    return {};
#endif
}

int prepare_data_directory(const Options &options) {
    namespace install = mhp3rd::install;
    if (options.data_dir) install::set_data_directory_override(*options.data_dir);
    if (options.portable) install::set_portable_requested();
    const install::DataDirectory &data = install::data_directory();
    if (const std::string problem = install::check_writable(data.path); !problem.empty()) {
        (void)install::report_problem("Can't write to the data folder", problem, false);
        return 1;
    }
    return -1;
}

std::optional<GameFiles> locate_game(const Options &options) {
    namespace install = mhp3rd::install;
    if (options.game_dir) return files_in_game_directory(*options.game_dir);
    if (const std::filesystem::path dir = mhp3rd::environment_path("MHFU_GAME_DIR"); !dir.empty())
        return files_in_game_directory(dir);

    const std::filesystem::path data_dir = install::user_data_directory();
    const std::filesystem::path checkout_game_dir = checkout_game_directory();
    bool run_setup = options.install;

    for (;;) {
        if (!run_setup) {
            if (const auto installed = install::find_installation(data_dir)) {
                std::error_code ec;
                if (std::filesystem::is_regular_file(installed->disc_image, ec)) {
                    GameFiles files;
                    files.executable = installed->executable;
                    files.disc_image = installed->disc_image;
                    files.memory_stick = data_dir / "ms0";
                    if (!std::filesystem::exists(files.memory_stick))
                        std::filesystem::create_directories(files.memory_stick / "PSP" / "SAVEDATA");
                    return files;
                }
                const std::string where = install::path_to_utf8(installed->disc_image);
                const std::string message = installed->image_copied
                    ? "The copy of the disc image Pokke made is missing:\n" + where +
                        "\n\nSet up again to restore it (Pokke --launcher)."
                    : "The disc image Pokke was set up with is no longer at:\n" + where +
                        "\n\nPut it back there, or set up again to choose where it is now "
                        "(Pokke --launcher).";
                if (!install::report_problem("Disc image not found", message, true)) return std::nullopt;
                run_setup = true;
                continue;
            }

            // Developer checkout in profiles/mhfu/game
            if (!checkout_game_dir.empty() &&
                (std::filesystem::exists(checkout_game_dir / "BOOT.ELF") ||
                 std::filesystem::exists(checkout_game_dir / "disc.iso")))
                return files_in_game_directory(checkout_game_dir);
        }

        // Direct command-line install if image was given
        if (options.install_image) {
            try {
                install::install(*options.install_image,
                    options.in_place ? install::ImageStorage::InPlace : install::ImageStorage::Copy,
                    data_dir, install::print_progress);
                std::cout << "Game data is ready in " << install::path_to_utf8(data_dir) << ".\n";
                run_setup = false;
                continue;
            } catch (const install::InstallError &e) {
                std::cerr << "Setup failed: " << e.what() << "\n";
                return std::nullopt;
            }
        }

        // Open graphical launcher / setup wizard in Vulkan window
        auto ui = install::make_installer_ui();
        if (!ui) {
            std::cerr << "No game data found in " << install::path_to_utf8(data_dir) << ".\n"
                      << "Please supply your legally acquired copy of " << mhfu::install::kGameTitle << " ("
                      << mhfu::install::kDiscIdDisplay << "):\n"
                      << "  Pokke --install /path/to/image.iso\n";
            return std::nullopt;
        }
        if (!install::run_installer(*ui, data_dir)) return std::nullopt;
        run_setup = false;
    }
}

} // namespace

int main(int argc, char **argv) {
#if defined(__linux__)
    ensure_main_stack(argv);
#endif
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
            std::cerr << "Pokke: " << e.what() << "\n\n" << kUsage;
            return 2;
        }

        if (const int exit_code = prepare_data_directory(options); exit_code >= 0)
            return exit_code;

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
                  << "  Pokke - Monster Hunter Freedom Unite (PC Native Port)\n"
                  << "========================================================\n"
                  << "Executable:   " << mhp3rd::path_to_utf8(executable) << "\n"
                  << "SHA-256:      " << sha256 << "\n"
                  << "Disc image:   " << (paths.disc_image.empty() ? "<none>" : mhp3rd::path_to_utf8(paths.disc_image)) << "\n"
                  << "Save data:    " << mhp3rd::path_to_utf8(paths.memory_stick) << "\n"
                  << "RAM:          64 MiB\n"
                  << "Entry:        " << psprecomp::hex32(elf.runtime_entry(mhfu::kLoadBase)) << "\n"
                  << "Functions:    " << runtime.function_count() << "\n"
                  << "In-game menu: Press [F1] or Esc\n"
                  << "--------------------------------------------------------\n"
                  << "Starting native execution...\n\n";

        if (runtime.function_count() == 0u) {
            std::cout << "Error: No generated functions linked. Run profiles/mhfu/scripts/generate.py and rebuild.\n";
            return 3;
        }

        runtime.run(elf.runtime_entry(mhfu::kLoadBase), configured_max_dispatches());
        std::cout << "\nRuntime stopped: " << runtime.stop_reason() << "\n";
        mhp3rd::adhoc_shutdown();
        if (mhp3rd::install::setup_requested_on_exit()) return mhp3rd::install::restart_for_setup(argv[0]);
        if (mhp3rd::install::restart_requested_on_exit()) return mhp3rd::install::restart(argv);
        std::cout << mhp3rd::kernel().describe_threads() << "\n";
        runtime.report_hle_histogram();
        return runtime.stop_reason().empty() ? 0 : 4;
    } catch (const std::exception &e) {
        mhp3rd::adhoc_shutdown();
        std::cerr << "Pokke error: " << e.what() << "\n";
        return 1;
    }
}
