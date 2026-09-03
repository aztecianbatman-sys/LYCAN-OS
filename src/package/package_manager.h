#pragma once
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include <filesystem>
#include <string>
namespace lycan {
struct Package { std::string id,name,version,publisher,description,sha256; std::filesystem::path source; };
class PackageManager {
public:
    PackageManager(Lyfs& fs, SecurityPolicy& security);
    bool install(const Package& pkg);
    bool uninstall(const std::string& id);
    bool rollback(const std::string& id);
    std::vector<Package> installed() const;
    static bool validId(const std::string& id);
private:
    Lyfs& fs_; SecurityPolicy& security_; std::filesystem::path db_;
};
}
