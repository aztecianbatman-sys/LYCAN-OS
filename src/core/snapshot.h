#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace lycan {
struct SnapshotInfo { std::string name; std::string stage; uint64_t cycles{}; };
class SnapshotManager {
public:
    explicit SnapshotManager(std::filesystem::path dir);
    bool save(const std::string&,const std::string&,uint64_t);
    bool load(const std::string&,SnapshotInfo&) const;
    bool remove(const std::string&);
    std::vector<SnapshotInfo> list() const;
private:
    std::filesystem::path dir_;
};
}
