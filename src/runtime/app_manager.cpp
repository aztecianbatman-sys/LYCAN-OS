#include "app_manager.h"
#include <algorithm>

namespace lycan {
namespace {
AppSurface fromId(const std::string& id) noexcept {
    if (id == "lycan-terminal") return AppSurface::Terminal;
    if (id == "lycan-files") return AppSurface::Files;
    if (id == "lycan-web") return AppSurface::Web;
    if (id == "lycan-snapshots") return AppSurface::Snapshots;
    if (id == "lycan-diagnostics") return AppSurface::Diagnostics;
    if (id == "crawford") return AppSurface::Crawford;
    return AppSurface::Unknown;
}
}

const char* appSurfaceName(AppSurface s) noexcept {
    switch (s) {
        case AppSurface::Terminal: return "terminal";
        case AppSurface::Files: return "files";
        case AppSurface::Web: return "web";
        case AppSurface::Snapshots: return "snapshots";
        case AppSurface::Diagnostics: return "diagnostics";
        case AppSurface::Crawford: return "crawford";
        default: return "unknown";
    }
}

ApplicationManager::ApplicationManager(ProcessManager& processes, PackageManager& packages)
    : processes_(processes), packages_(packages) {}

AppSurface ApplicationManager::inferSurface(const Package& package) noexcept {
    const auto byId = fromId(package.id);
    if (byId != AppSurface::Unknown) return byId;
    if (package.entry.rfind("builtin://", 0) == 0) {
        const auto name = package.entry.substr(10);
        if (name == "terminal") return AppSurface::Terminal;
        if (name == "files") return AppSurface::Files;
        if (name == "web") return AppSurface::Web;
        if (name == "snapshots") return AppSurface::Snapshots;
        if (name == "diagnostics") return AppSurface::Diagnostics;
        if (name == "crawford") return AppSurface::Crawford;
    }
    return AppSurface::Unknown;
}

bool ApplicationManager::registerBuiltInSurface(const std::string& appId, AppSurface surface) {
    if (appId.empty() || surface == AppSurface::Unknown) return false;
    for (auto& item : registry_) {
        if (item.first == appId) { item.second = surface; return true; }
    }
    registry_.push_back({appId, surface});
    return true;
}

bool ApplicationManager::registerPackageSurface(const Package& package) {
    return registerBuiltInSurface(package.id, inferSurface(package));
}

uint32_t ApplicationManager::open(const std::string& appId) {
    if (appId.empty()) return 0;
    if (const auto* existing = find(appId)) {
        processes_.resume(existing->pid);
        return existing->pid;
    }
    const auto installed = packages_.installed();
    auto it = std::find_if(installed.begin(), installed.end(), [&](const Package& p) { return p.id == appId; });
    if (it == installed.end()) return 0;
    if (surfaceFor(appId) == AppSurface::Unknown) registerPackageSurface(*it);
    const uint32_t pid = processes_.launchApp(it->id, it->entry, 8192);
    if (!pid) return 0;
    sessions_.push_back({pid, it->id, it->name, it->entry, surfaceFor(it->id), true});
    return pid;
}

bool ApplicationManager::close(const std::string& appId) {
    const auto* session = find(appId);
    if (!session) return false;
    const bool stopped = processes_.closeApp(appId);
    if (!stopped) return false;
    sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(), [&](const AppSession& s) { return s.appId == appId; }), sessions_.end());
    return true;
}

bool ApplicationManager::suspend(const std::string& appId) {
    const auto* session = find(appId);
    return session && processes_.suspend(session->pid);
}

bool ApplicationManager::resume(const std::string& appId) {
    const auto* session = find(appId);
    return session && processes_.resume(session->pid);
}

bool ApplicationManager::isOpen(const std::string& appId) const {
    return find(appId) != nullptr;
}

const AppSession* ApplicationManager::find(const std::string& appId) const noexcept {
    for (const auto& s : sessions_) if (s.appId == appId) return &s;
    return nullptr;
}

const AppSession* ApplicationManager::find(uint32_t pid) const noexcept {
    for (const auto& s : sessions_) if (s.pid == pid) return &s;
    return nullptr;
}

std::vector<AppSession> ApplicationManager::sessions() const { return sessions_; }

AppSurface ApplicationManager::surfaceFor(const std::string& appId) const noexcept {
    for (const auto& item : registry_) if (item.first == appId) return item.second;
    return AppSurface::Unknown;
}

} // namespace lycan
