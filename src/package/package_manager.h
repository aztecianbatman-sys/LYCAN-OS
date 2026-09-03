#pragma once
#include "core/lyfs.h"
#include "core/security.h"
#include <filesystem>
#include <string>
#include <vector>
namespace lycan {
struct Package { std::string id,name,version,publisher,description,sha256,entry; std::vector<std::string> permissions; std::filesystem::path source; };
class PackageManager {
public:
    PackageManager(Lyfs& fs, SecurityPolicy& security);
    bool install(const Package& pkg);
    bool installArchive(const std::filesystem::path& package, std::string* error = nullptr);
    bool inspectArchive(const std::filesystem::path& package, Package& pkg, std::string* error = nullptr) const;
    bool uninstall(const std::string& id);
    bool rollback(const std::string& id);
    std::vector<Package> installed() const;
    static bool validId(const std::string& id);
private:
    Lyfs& fs_; SecurityPolicy& security_; std::filesystem::path db_;
};
}
