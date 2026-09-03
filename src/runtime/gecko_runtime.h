#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace lycan {
struct WebPermission { std::string origin; std::string permission; bool granted{false}; };
struct GeckoTab { uint32_t id{}; std::string url{"about:blank"}; bool active{false}; };
class GeckoRuntime {
public:
 explicit GeckoRuntime(std::filesystem::path root);
 bool configure(std::filesystem::path executable);
 bool start(std::string* error=nullptr); bool stop(); bool running() const noexcept;
 uint32_t newTab(const std::string& url="about:blank"); bool closeTab(uint32_t id); bool navigate(uint32_t id,const std::string& url);
 bool setPermission(const std::string& origin,const std::string& permission,bool granted); bool permission(const std::string& origin,const std::string& permission) const;
 std::vector<GeckoTab> tabs() const; std::vector<WebPermission> permissions() const; std::filesystem::path profilePath() const noexcept; std::filesystem::path downloadPath() const noexcept;
private:
 std::filesystem::path root_,executable_,profile_,downloads_; bool running_{false}; uint32_t nextTab_{1}; std::vector<GeckoTab> tabs_; std::vector<WebPermission> permissions_;
 static bool httpsOrLocal(const std::string&); bool launchProcess(std::string* error);
};
} // namespace lycan
