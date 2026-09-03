#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace lycan {
struct SnapshotInfo { std::string name; std::string stage; uint64_t cycles; };
class SnapshotManager {
public:
    explicit SnapshotManager(std::filesystem::path dir);
    bool save(const std::string& name,const std::string& stage,uint64_t cycles);
    bool load(const std::string& name, SnapshotInfo& out) const;
    std::vector<SnapshotInfo> list() const;
private:
    std::filesystem::path dir_;
};
}
