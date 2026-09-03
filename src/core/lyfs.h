#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lycan {

struct FsEntry { std::string path; bool directory{false}; uint64_t bytes{0}; };

class Lyfs {
public:
    explicit Lyfs(std::filesystem::path root);
    bool format();
    bool ready() const noexcept;
    std::vector<FsEntry> list(const std::string& path = "/") const;
    bool writeText(const std::string& path, const std::string& text);
    bool readText(const std::string& path, std::string& out) const;
    bool createDirectory(const std::string& path);
    bool remove(const std::string& path);
    std::filesystem::path hostPath(const std::string& guestPath) const;
    uint64_t usedBytes() const;
    uint64_t capacityBytes() const;
private:
    std::filesystem::path root_;
    bool safeGuestPath(const std::string& path, std::filesystem::path& out) const;
};

} // namespace lycan
