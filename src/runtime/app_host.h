#pragma once
#include "core/ares_vm.h"
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include "package/package_manager.h"
#include "core/snapshot.h"
#include <filesystem>
#include <string>
namespace lycan {
class AppHost {
public:
    AppHost(std::filesystem::path dataRoot);
    void boot();
    std::string execute(const std::string& command);
    AresVm& vm(); Lyfs& fs(); ProcessManager& processes(); SecurityPolicy& security(); PackageManager& packages(); SnapshotManager& snapshots();
private:
    std::filesystem::path root_;
    AresVm vm_; Lyfs fs_; ProcessManager processes_; SecurityPolicy security_; PackageManager packages_; SnapshotManager snapshots_;
};
}
