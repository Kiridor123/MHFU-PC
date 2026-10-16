#include "platform/crash_logger.hpp"

#include "app_paths.hpp"
#include "install/user_data.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
#include <unistd.h>
#if __has_include(<execinfo.h>) && !defined(__ANDROID__)
#include <execinfo.h>
#define HAS_EXECINFO 1
#endif
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(MHP3RD_ANDROID_APP)
#include <SDL3/SDL.h>
#endif

namespace mhp3rd {
namespace {

namespace fs = std::filesystem;

fs::path g_current_log_path;
int g_orig_stdout_fd = -1;
int g_orig_stderr_fd = -1;
FILE *g_log_file = nullptr;

std::string timestamp_string() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &local);
    return buf;
}

fs::path determine_log_directory() {
    std::error_code ec;

    // 1. If running as an AppImage on Linux
    if (const char *appimage = std::getenv("APPIMAGE"); appimage != nullptr && *appimage != '\0') {
        const fs::path appimage_dir = fs::path(appimage).parent_path();
        const fs::path log_dir = appimage_dir / "log";
        fs::create_directories(log_dir, ec);
        if (!ec) return log_dir;
    }

#if defined(MHP3RD_ANDROID_APP)
    // 2. Android: internal app storage
    const char *storage = SDL_GetAndroidInternalStoragePath();
    if (storage != nullptr && *storage != '\0') {
        const fs::path log_dir = fs::path(storage) / "log";
        fs::create_directories(log_dir, ec);
        if (!ec) return log_dir;
    }
#endif

    // 3. Executable directory: log folder next to .exe or binary
    const fs::path exe_dir = executable_directory();
    if (!exe_dir.empty()) {
        const fs::path log_dir = exe_dir / "log";
        fs::create_directories(log_dir, ec);
        if (!ec) return log_dir;
    }

    // 4. Fallback: user data directory
    const fs::path user_log_dir = install::user_data_directory() / "log";
    fs::create_directories(user_log_dir, ec);
    return user_log_dir;
}

void write_crash_message(const char *msg) {
    if (msg == nullptr) return;
    const std::size_t len = std::char_traits<char>::length(msg);
    if (g_log_file != nullptr) {
        std::fwrite(msg, 1, len, g_log_file);
        std::fflush(g_log_file);
    }
    std::fwrite(msg, 1, len, stderr);
    std::fflush(stderr);
    if (g_orig_stderr_fd >= 0) {
#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
        (void)write(g_orig_stderr_fd, msg, len);
#endif
    }
}

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
void posix_signal_handler(int sig, siginfo_t *info, void *) {
    char header[256];
    const char *sig_name = "UNKNOWN";
    switch (sig) {
    case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault)"; break;
    case SIGABRT: sig_name = "SIGABRT (Aborted)"; break;
    case SIGBUS:  sig_name = "SIGBUS (Bus Error)"; break;
    case SIGFPE:  sig_name = "SIGFPE (Floating Point Exception)"; break;
    case SIGILL:  sig_name = "SIGILL (Illegal Instruction)"; break;
    default: break;
    }

    std::snprintf(header, sizeof(header),
        "\n\n================================================================\n"
        "FATAL CRASH DETECTED: %s (signal %d)\n"
        "Faulting memory address: %p\n"
        "================================================================\n"
        "Backtrace:\n",
        sig_name, sig, info ? info->si_addr : nullptr);

    write_crash_message(header);

#if defined(HAS_EXECINFO)
    void *stack[64];
    const int depth = backtrace(stack, 64);
    if (g_log_file != nullptr) {
        backtrace_symbols_fd(stack, depth, fileno(g_log_file));
    }
    backtrace_symbols_fd(stack, depth, STDERR_FILENO);
    if (g_orig_stderr_fd >= 0) {
        backtrace_symbols_fd(stack, depth, g_orig_stderr_fd);
    }
#endif

    write_crash_message("================================================================\n"
                        "Crash log flushed to disk.\n\n");

    flush_crash_logger();

    // Re-raise default handler or exit
    std::signal(sig, SIG_DFL);
    std::raise(sig);
    _exit(128 + sig);
}
#endif

#if defined(_WIN32)
LONG WINAPI windows_exception_filter(EXCEPTION_POINTERS *pointers) {
    char header[256];
    DWORD code = pointers ? pointers->ExceptionRecord->ExceptionCode : 0;
    void *addr = pointers ? pointers->ExceptionRecord->ExceptionAddress : nullptr;
    std::snprintf(header, sizeof(header),
        "\n\n================================================================\n"
        "FATAL CRASH DETECTED: Windows Exception Code 0x%08lX\n"
        "Faulting memory address: %p\n"
        "================================================================\n",
        code, addr);
    write_crash_message(header);
    flush_crash_logger();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void terminate_handler() {
    write_crash_message("\n\n================================================================\n"
                        "FATAL: std::terminate() called!\n");
    if (const auto exc = std::current_exception()) {
        try {
            std::rethrow_exception(exc);
        } catch (const std::exception &e) {
            write_crash_message("Exception message: ");
            write_crash_message(e.what());
            write_crash_message("\n");
        } catch (...) {
            write_crash_message("Unknown non-std exception caught.\n");
        }
    }
    write_crash_message("================================================================\n");
    flush_crash_logger();
    _exit(1);
}

} // namespace

void init_crash_logger(int argc, char **argv) {
    const fs::path log_dir = determine_log_directory();
    const std::string ts = timestamp_string();
    g_current_log_path = log_dir / ("pokke_" + ts + ".log");
    const fs::path latest_log_path = log_dir / "latest.log";

    // Inform user in terminal before redirecting
    std::cout << "[log] Recording session to: " << g_current_log_path.string() << std::endl;

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    g_orig_stdout_fd = dup(STDOUT_FILENO);
    g_orig_stderr_fd = dup(STDERR_FILENO);
#endif

    g_log_file = std::fopen(g_current_log_path.string().c_str(), "w");
    if (g_log_file != nullptr) {
        std::setvbuf(g_log_file, nullptr, _IONBF, 0);

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
        const int log_fd = fileno(g_log_file);
        (void)dup2(log_fd, STDOUT_FILENO);
        (void)dup2(log_fd, STDERR_FILENO);
        std::setvbuf(stdout, nullptr, _IONBF, 0);
        std::setvbuf(stderr, nullptr, _IONBF, 0);
#elif defined(_WIN32)
        std::freopen(g_current_log_path.string().c_str(), "w", stdout);
        std::freopen(g_current_log_path.string().c_str(), "a", stderr);
        std::setvbuf(stdout, nullptr, _IONBF, 0);
        std::setvbuf(stderr, nullptr, _IONBF, 0);
#endif

        // Duplicate/link latest.log
        std::error_code ec;
        fs::copy_file(g_current_log_path, latest_log_path, fs::copy_options::overwrite_existing, ec);

        std::cout << "================================================================\n"
                  << "  Pokke / MHFU Log Session\n"
                  << "  Started: " << ts << "\n"
                  << "  Log path: " << g_current_log_path.string() << "\n"
                  << "  Arguments:";
        for (int i = 0; i < argc; ++i) {
            std::cout << " " << argv[i];
        }
        std::cout << "\n================================================================\n" << std::flush;
    }

    // Signal handlers
#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    struct sigaction sa{};
    sa.sa_sigaction = &posix_signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
#endif

#if defined(_WIN32)
    SetUnhandledExceptionFilter(&windows_exception_filter);
#endif

    std::set_terminate(&terminate_handler);

    std::atexit([]() {
        std::cout << "\n[log] Session ended normally.\n" << std::flush;
        flush_crash_logger();
    });
}

fs::path current_log_path() {
    return g_current_log_path;
}

void flush_crash_logger() {
    std::fflush(stdout);
    std::fflush(stderr);
    if (g_log_file != nullptr) {
        std::fflush(g_log_file);
#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
        (void)fsync(fileno(g_log_file));
#endif
    }
}

} // namespace mhp3rd
