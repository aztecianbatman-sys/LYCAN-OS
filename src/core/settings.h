#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
namespace lycan {
struct VmSettings {
    uint32_t virtualCpus{2};
    uint64_t ramBytes{512ull * 1024ull * 1024ull};
    uint32_t width{1280};
    uint32_t height{720};
    bool fullscreen{false};
    bool networkEnabled{true};
    bool strictPackages{true};
};
class SettingsManager {
public:
    explicit SettingsManager(std::filesystem::path file);
    bool load();
    bool save() const;
    const VmSettings& get() const noexcept { return settings_; }
    bool set(const std::string& key, const std::string& value);
    std::string describe() const;
private:
    std::filesystem::path file_;
    VmSettings settings_{};
};
}
