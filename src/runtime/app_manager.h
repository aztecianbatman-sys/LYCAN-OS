#pragma once
#include "core/process.h"
#include "package/package_manager.h"
#include <cstdint>
#include <string>
#include <vector>
namespace lycan {
enum class AppSurface { Terminal, Files, Web, Snapshots, Diagnostics, Crawford, Native, Unknown };
const char* appSurfaceName(AppSurface surface) noexcept;
struct AppSession { uint32_t pid{}; std::string appId; std::string name; std::string launchTarget; AppSurface surface{AppSurface::Unknown}; bool windowOpen{true}; };
class ApplicationManager {
public:
 ApplicationManager(ProcessManager& processes, PackageManager& packages);
 bool registerBuiltInSurface(const std::string& appId, AppSurface surface); bool registerPackageSurface(const Package& package); uint32_t open(const std::string& appId); bool close(const std::string& appId); bool suspend(const std::string& appId); bool resume(const std::string& appId); bool suspend(uint32_t pid); bool resume(uint32_t pid); bool isOpen(const std::string& appId) const; const AppSession* find(const std::string& appId) const noexcept; const AppSession* find(uint32_t pid) const noexcept; std::vector<AppSession> sessions() const; AppSurface surfaceFor(const std::string& appId) const noexcept;
private: static AppSurface inferSurface(const Package& package) noexcept; ProcessManager& processes_; PackageManager& packages_; std::vector<std::pair<std::string,AppSurface>> registry_; std::vector<AppSession> sessions_;
};
} // namespace lycan
