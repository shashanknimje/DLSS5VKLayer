#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <unistd.h>

// Keep the Wine/Proton command-line identity while accepting Linux programs
// whose argv[0] is rewritten (notably Chromium's GPU subprocess).
namespace dlssnr::process_filter {

enum class MatchSource { None, CommandLine, Executable };

inline std::string Basename(std::string path) {
    const size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) path.erase(0, pos + 1);
    return path;
}

inline bool EqualIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) return false;
    }
    return true;
}

inline std::string ReadExecutablePath() {
    char buf[4096];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf));
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(buf)) return {};
    return std::string(buf, static_cast<size_t>(n));
}

inline MatchSource Match(const std::string& filter,
                         const std::string& command0,
                         const std::string& executablePath) {
    const std::string wanted = Basename(filter);
    if (wanted.empty()) return MatchSource::None;

    const std::string commandName = Basename(command0);
    if (!commandName.empty() && EqualIgnoreCase(commandName, wanted))
        return MatchSource::CommandLine;

    const std::string executableName = Basename(executablePath);
    if (!executableName.empty() && EqualIgnoreCase(executableName, wanted))
        return MatchSource::Executable;

    return MatchSource::None;
}

}  // namespace dlssnr::process_filter
