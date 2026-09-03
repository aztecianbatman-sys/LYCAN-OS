#pragma once
#include "package/package_manager.h"
#include <filesystem>
#include <string>
namespace lycan {
struct StoreResult { bool ok{false}; std::string message; std::filesystem::path archive; };
class StoreClient {
public:
    explicit StoreClient(PackageManager& packages);
    StoreResult installFromCatalog(const std::string& catalogJson,const std::string& packageId,const std::filesystem::path& cacheDir);
private:
    PackageManager& packages_;
    static bool httpsUrl(const std::string& url);
    static bool catalogField(const std::string& json,const std::string& id,const std::string& field,std::string& out);
    static bool downloadHttps(const std::string& url,const std::filesystem::path& destination,std::string& error);
};
}
