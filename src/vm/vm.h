#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace lycan {

struct Process {
    uint32_t pid{};
    std::string name;
    std::string state;
    std::string error;
    uint64_t memoryBytes{};
    uint64_t pageStart{};
};

struct AppRecord {
    std::string id;
    std::string version;
    std::string permissions;
    uint64_t quotaBytes{};
};

class VirtualMachine {
public:
    explicit VirtualMachine(std::filesystem::path root);
    void boot();
    std::string execute(const std::string& command);
    const std::filesystem::path& root() const noexcept { return root_; }
private:
    std::string diagnostics() const;
    std::string ls(const std::string& path) const;
    std::string tree(const std::string& path) const;
    std::string apps() const;
    std::string ps() const;
    std::string snapshots() const;
    std::string snapshotCreate(const std::string& name);
    std::string snapshotInfo(const std::string& name) const;
    std::string snapshotRestore(const std::string& name);
    std::string snapshotDelete(const std::string& name);
    std::string registerApp(const std::string& spec);
    std::string unregisterApp(const std::string& id);
    std::string appState(const std::string& id) const;
    std::string storageList(const std::string& id, const std::string& bucket, const std::string& path) const;
    std::string storageRead(const std::string& id, const std::string& bucket, const std::string& path) const;
    std::string storageWrite(const std::string& id, const std::string& bucket, const std::string& path, const std::string& text);
    std::string storageDelete(const std::string& id, const std::string& bucket, const std::string& path);
    std::string storageUsage(const std::string& id) const;
    std::string storageQuota(const std::string& id) const;
    std::string networkStatus() const;
    std::string networkInterfaces() const;
    std::string memoryStatus() const;
    std::string setRam(const std::string& mb);
    void persistState() const;
    void loadState();
    void ensureAppStorage(const std::string& id) const;
    bool hasPermission(const std::string& id, const std::string& permission) const;
    uint64_t appStorageUsage(const std::string& id) const;
    bool validAppId(const std::string& id) const;
    bool validBucket(const std::string& bucket) const;
    bool validStoragePath(const std::filesystem::path& path) const;
    std::filesystem::path appDataRoot(const std::string& id) const;
    std::filesystem::path storagePath(const std::string& id, const std::string& bucket, const std::string& path) const;
    bool allocateProcessMemory(Process& process);
    void releaseProcessMemory(const Process& process);
    void rebuildMemoryMap();
    std::filesystem::path guestPath(const std::string& path) const;
    bool validGuestPath(const std::filesystem::path& path) const;
    bool isGuestRoot(const std::filesystem::path& path) const;
    bool validSnapshotName(const std::string& name) const;
    std::filesystem::path snapshotPath(const std::string& name) const;
    std::filesystem::path snapshotFsPath(const std::string& name) const;

    std::filesystem::path root_;
    uint64_t cycles_{0};
    uint64_t ramBytes_{536870912ULL};
    uint64_t usedMemoryBytes_{0};
    bool network_{true};
    uint32_t nextPid_{3};
    uint64_t generation_{1};
    std::vector<uint8_t> pageMap_;
    std::vector<Process> processes_;
    std::map<std::string, AppRecord> installed_;
};

}
