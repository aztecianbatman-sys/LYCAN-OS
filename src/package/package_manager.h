#pragma once
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include <filesystem>
#include <string>
#include <vector>
namespace lycan {
struct Package { std::string id,name,version,publisher,description,sha256,entry; std::vector<std::string> permissions; std::filesystem::path source; std::string installLocation,installTime,previousVersion; };
class PackageManager {
public:
 PackageManager(Lyfs& fs, SecurityPolicy& security, ProcessManager* processes = nullptr);
 bool install(const Package& pkg); bool installArchive(const std::filesystem::path&,const std::string&,std::string* error=nullptr); bool installArchiveFromCatalog(const std::filesystem::path&,const std::string&,std::string* error=nullptr);
 static std::string packageSha256(const std::filesystem::path&); static bool verifySha256(const std::string&,const std::string&); static std::string catalogSha256(const std::string&,const std::string&,const std::string&);
 bool inspectArchive(const std::filesystem::path&,Package&,std::string* error=nullptr) const; bool uninstall(const std::string& id); bool rollback(const std::string& id); std::vector<Package> installed() const;
 std::vector<Package> launcherApps() const;
 bool launcherContains(const std::string& id) const;
 static bool validId(const std::string&);
private:
 Lyfs& fs_; SecurityPolicy& security_; ProcessManager* processes_; std::filesystem::path db_,launcherDb_;
 bool recordInstall(const Package&,const std::string&,const std::string&); bool registerLauncher(const Package&); bool removeLauncher(const std::string&);
};
}
