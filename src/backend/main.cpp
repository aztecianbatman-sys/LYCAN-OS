#include "runtime/app_host.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>

static std::filesystem::path dataRoot() {
#ifdef _WIN32
    if (const char* local = std::getenv("LOCALAPPDATA")) return std::filesystem::path(local) / "LYCAN";
#endif
    return std::filesystem::current_path() / "lycan-data";
}

int main() {
    const auto root = dataRoot();
    std::filesystem::create_directories(root);
    lycan::AppHost host(root);
    host.boot();

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "__LYCAN_EXIT__") break;
        std::cout << host.execute(line) << "\n<<<LYCAN_END>>>\n" << std::flush;
    }
    return 0;
}
