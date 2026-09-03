#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lycan {

struct StoreApp {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string downloadUrl;
    std::string sha256;
    uint64_t sizeBytes = 0;
};

class LycanStore {
public:
    bool loadCatalog(const std::string& json);
    const std::vector<StoreApp>& apps() const { return apps_; }
    std::optional<StoreApp> find(const std::string& id) const;
    bool verifyPackage(const std::string& path, const std::string& expectedSha256) const;
    bool download(const StoreApp& app, const std::string& destination, std::string& error) const;
private:
    std::vector<StoreApp> apps_;
};

class GeckoRuntime {
public:
    bool discover(const std::string& explicitPath = {});
    bool available() const { return !runtimePath_.empty(); }
    const std::string& path() const { return runtimePath_; }
private:
    std::string runtimePath_;
};

}
