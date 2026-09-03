#pragma once
#include "core/ares_vm.h"
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include "core/snapshot.h"
#include "core/settings.h"
#include "package/package_manager.h"
#include "runtime/app_manager.h"
#include "store/store_client.h"
#include <filesystem>
#include <string>
namespace lycan {
class AppHost {
public:
    AppHost(std::filesystem::path dataRoot);
    void boot();
    std::string execute(const std::string& command);
    AresVm& vm(); Lyfs& fs(); ProcessManager& processes(); SecurityPolicy& security(); PackageManager& packages(); SnapshotManager& snapshots(); ApplicationManager& apps(); SettingsManager& settings(); StoreClient& store();
private:
    std::filesystem::path root_;
    AresVm vm_; Lyfs fs_; ProcessManager processes_; SecurityPolicy security_; PackageManager packages_; SnapshotManager snapshots_; SettingsManager settings_; ApplicationManager applications_; StoreClient store_;
};
}
