#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace lycan {

struct PackageEntry {
    std::string path;
    std::vector<unsigned char> data;
};

struct PackageManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string publisher;
    std::string entry;
    std::vector<std::string> permissions;
};

class PackageArchive {
public:
    static bool create(const std::filesystem::path& output,
                       const PackageManifest& manifest,
                       const std::vector<PackageEntry>& appFiles,
                       std::string& error);
    static bool inspect(const std::filesystem::path& package,
                        PackageManifest& manifest,
                        std::vector<PackageEntry>& entries,
                        std::string& error);
    static bool extract(const std::filesystem::path& package,
                        const std::filesystem::path& destination,
                        PackageManifest& manifest,
                        std::string& error);
    static std::string sha256(const std::vector<unsigned char>& data);
    static std::string sha256File(const std::filesystem::path& path);
};

} // namespace lycan
