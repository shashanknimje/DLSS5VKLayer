#include "../layer_linux/src/process_filter.h"

#include <cstdio>
#include <string>

using dlssnr::process_filter::Basename;
using dlssnr::process_filter::Match;
using dlssnr::process_filter::MatchSource;
using dlssnr::process_filter::ReadExecutablePath;

static int failures = 0;

static void Expect(const char* name, MatchSource actual, MatchSource expected) {
    if (actual != expected) {
        std::fprintf(stderr, "FAIL: %s (actual=%d expected=%d)\n",
                     name, static_cast<int>(actual), static_cast<int>(expected));
        ++failures;
    }
}

int main() {
    // Chrome writes an entire command line into argv[0] after GPU-process startup.
    const std::string chromeGpu =
        "/opt/google/chrome/chrome --type=gpu-process "
        "--user-data-dir=/home/test/.config/xbox-dlssnr-chrome";
    Expect("Chrome rewritten argv[0] falls back to executable",
           Match("chrome", chromeGpu, "/opt/google/chrome/chrome"),
           MatchSource::Executable);
    Expect("Chrome direct argv[0] retains priority",
           Match("chrome", "/opt/google/chrome/chrome", "/opt/google/chrome/chrome"),
           MatchSource::CommandLine);
    Expect("Chrome case-insensitive executable match",
           Match("CHROME", chromeGpu, "/opt/google/chrome/chrome"),
           MatchSource::Executable);
    Expect("Chrome profile path alone must not match",
           Match("chrome", chromeGpu, "/usr/bin/other-browser"),
           MatchSource::None);
    Expect("Chromium must not match Chrome",
           Match("chrome", "/usr/bin/chromium", "/usr/bin/chromium"),
           MatchSource::None);

    // The Linux executable under Wine does not identify the Windows game.
    Expect("Proton 007 game remains matched by command line",
           Match("007FirstLight.exe", "Z:\\Games\\007FirstLight.exe",
                 "/usr/bin/wine64-preloader"), MatchSource::CommandLine);
    Expect("Proton RDR2 game remains matched by command line",
           Match("rdr2.EXE", "RDR2.exe", "/usr/bin/wine64"),
           MatchSource::CommandLine);
    Expect("Windows filter path basenames preserved",
           Match("C:\\Games\\RDR2.exe", "rdr2.EXE", "/usr/bin/wine64"),
           MatchSource::CommandLine);
    Expect("No accidental Wine executable match",
           Match("RDR2.exe", "other.exe", "/usr/bin/wine64"),
           MatchSource::None);

    Expect("Native Linux matching via executable path",
           Match("chrome", "rewritten title", "/opt/google/chrome/chrome"),
           MatchSource::Executable);
    Expect("Unmatched filter remains inert",
           Match("not-real", "unrelated", "/usr/bin/unrelated"),
           MatchSource::None);
    Expect("Empty identity stays unmatched",
           Match("chrome", "", ""), MatchSource::None);
    Expect("Empty filter stays unmatched (unfiltered handled by caller)",
           Match("", "chrome", "/opt/google/chrome/chrome"),
           MatchSource::None);

    const std::string ownExe = ReadExecutablePath();
    if (ownExe.empty()) {
        std::fprintf(stderr, "FAIL: /proc/self/exe could not be resolved\n");
        ++failures;
    } else {
        Expect("/proc/self/exe resolves this native test executable",
               Match(Basename(ownExe), "modified argv[0]", ownExe),
               MatchSource::Executable);
    }

    if (failures) return 1;
    std::puts("PASS: process filter command-line and executable fallbacks");
    return 0;
}
