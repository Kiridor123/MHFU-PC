#pragma once

#include <filesystem>
#include <string>

namespace mhp3rd {

// Initializes automatic session logging to a `log/` folder next to the executable / AppImage.
// Redirects stdout & stderr, installs fatal signal handlers, and flushes on exit or crash.
void init_crash_logger(int argc, char **argv);

// Returns the active log file path.
[[nodiscard]] std::filesystem::path current_log_path();

// Flushes all log buffers immediately.
void flush_crash_logger();

} // namespace mhp3rd
