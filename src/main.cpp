#include "runtime/app_host.h"
#ifdef _WIN32
#include "platform/win32_desktop.h"
#endif
#include <cstdlib>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    std::filesystem::path root;
#ifdef _WIN32
    if (const char* local = std::getenv("LOCALAPPDATA")) root = std::filesystem::path(local) / "LYCAN";
    else root = std::filesystem::current_path() / "lycan-data";
#else
    if (const char* home = std::getenv("HOME")) root = std::filesystem::path(home) / ".lycan";
    else root = std::filesystem::current_path() / "lycan-data";
#endif
    std::filesystem::create_directories(root);
    lycan::AppHost host(root);
    host.boot();
#ifdef _WIN32
    return lycan::runDesktop(host);
#else
    if (argc > 1) {
        std::string command;
        for (int i = 1; i < argc; ++i) { if (!command.empty()) command += ' '; command += argv[i]; }
        std::cout << host.execute(command) << '\n';
        return 0;
    }
    std::cout << "LYCAN VM booted at " << root << "\n";
    std::cout << host.execute("diagnostics") << '\n';
    return 0;
#endif
}
