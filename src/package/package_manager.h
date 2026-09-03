#pragma once
#include "core/lyfs.h"
#include "core/security.h"
#include <filesystem>
#include <string>
#include <vector>

namespace lycan {
struct Package {
    std::string id,name,version,publisher,description,sha256,entry;
    std::vector<std::string> permissions;
    std::filesystem::path source;
    std::string installLocation;
    std::string installTime;
    std::string previousVersion;
};

class PackageManager {
public:
    PackageManager(Lyfs& fs, SecurityPolicy& security);
    bool install(const Package& pkg);
    bool installArchive(const std::filesystem::path& package, const std::string& expectedSha256, std::string* error = nullptr);
    bool installArchiveFromCatalog(const std::filesystem::path& package, const std::string& catalogJson, std::string* error = nullptr);
    static std::string packageSha256(const std::filesystem::path& package);
    static bool verifySha256(const std::string& calculated, const std::string& expected);
    static std::string catalogSha256(const std::string& catalogJson, const std::string& id, const std::string& version);
    bool inspectArchive(const std::filesystem::path& package, Package& pkg, std::string* error = nullptr) const;
    bool uninstall(const std::string& id);
    bool rollback(const std::string& id);
    std::vector<Package> installed() const;
    static bool validId(const std::string& id);
private:
    Lyfs& fs_; SecurityPolicy& security_; std::filesystem::path db_;
    bool recordInstall(const Package& pkg, const std::string& checksum, const std::string& previousVersion);
};
}
