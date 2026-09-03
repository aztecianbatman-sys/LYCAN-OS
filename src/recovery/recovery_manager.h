#pragma once
#include "core/lyfs.h"
#include "core/ares_vm.h"
#include <filesystem>
#include <string>
namespace lycan {
enum class RecoveryAction { VerifyFilesystem, ResetUserData, ResetSystem, CreateBackup, RestoreBackup };
struct RecoveryResult { bool ok{}; std::string message; };
class RecoveryManager {
public:
 RecoveryManager(Lyfs&,AresVm&,std::filesystem::path root);
 RecoveryResult verifyFilesystem(); RecoveryResult resetUserData(); RecoveryResult resetSystem(); RecoveryResult backup(const std::filesystem::path& out); RecoveryResult restore(const std::filesystem::path& archive);
 bool safeMode() const noexcept; void enterSafeMode(); void leaveSafeMode();
private: Lyfs& fs_; AresVm& vm_; std::filesystem::path root_; bool safeMode_{false};
};
} // namespace lycan
