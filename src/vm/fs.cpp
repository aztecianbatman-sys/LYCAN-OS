#include "vm.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace lycan {

std::string VirtualMachine::ls(const std::string& path) const {
    const auto p = guestPath(path);
    if (!validGuestPath(p) || std::filesystem::is_symlink(p)) return "ACCESS DENIED: OUTSIDE LYFS";
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) return "PATH NOT FOUND";
    if (ec) return "PATH ERROR";
    if (!std::filesystem::is_directory(p, ec)) return "NOT A DIRECTORY";

    std::ostringstream out;
    out << "PATH " << (path.empty() ? "/home" : path) << "\n";
    out << "TYPE  NAME\n";
    out << "--------------\n";
    bool any = false;
    for (const auto& entry : std::filesystem::directory_iterator(p, ec)) {
        if (ec) return "DIRECTORY READ FAILED";
        any = true;
        if (entry.is_symlink(ec)) {
            if (ec) return "DIRECTORY READ FAILED";
            out << "LINK  " << entry.path().filename().string() << "\n";
        } else if (entry.is_directory(ec)) {
            if (ec) return "DIRECTORY READ FAILED";
            out << "DIR   " << entry.path().filename().string() << "\n";
        } else if (entry.is_regular_file(ec)) {
            if (ec) return "DIRECTORY READ FAILED";
            out << "FILE  " << entry.path().filename().string() << "\n";
        } else {
            out << "NODE  " << entry.path().filename().string() << "\n";
        }
    }
    if (!any) out << "(empty)\n";
    return out.str();
}

std::string VirtualMachine::tree(const std::string& path) const {
    const auto root = guestPath(path);
    if (!validGuestPath(root) || std::filesystem::is_symlink(root)) return "ACCESS DENIED: OUTSIDE LYFS";
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return "PATH NOT FOUND";
    if (ec) return "PATH ERROR";

    std::ostringstream out;
    out << (root.filename().empty() ? root.string() : root.filename().string()) << "\n";

    std::function<void(const std::filesystem::path&, const std::string&, int)> walk;
    walk = [&](const std::filesystem::path& dir, const std::string& prefix, int depth) {
        if (depth >= 32) {
            out << prefix << "`-- [MAX DEPTH]\n";
            return;
        }
        if (!std::filesystem::is_directory(dir, ec)) return;

        std::vector<std::filesystem::path> entries;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) return;
            if (!entry.is_symlink(ec) && validGuestPath(entry.path())) entries.push_back(entry.path());
        }
        std::sort(entries.begin(), entries.end());

        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = entries[i];
            const bool last = i + 1 == entries.size();
            const bool directory = std::filesystem::is_directory(entry, ec);
            const auto connector = last ? "`-- " : "|-- ";
            out << prefix << connector << entry.filename().string() << (directory ? "/" : "") << "\n";
            if (directory) walk(entry, prefix + (last ? "    " : "|   "), depth + 1);
        }
    };

    if (std::filesystem::is_directory(root, ec)) walk(root, "", 0);
    return out.str();
}

} // namespace lycan
