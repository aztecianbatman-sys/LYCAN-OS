#include "lyfs.h"
#include <fstream>

namespace lycan {
Lyfs::Lyfs(std::filesystem::path root) : root_(std::move(root)) {}
bool Lyfs::format() {
    std::error_code ec; std::filesystem::create_directories(root_ / "system", ec); if(ec) return false;
    std::filesystem::create_directories(root_ / "home", ec); if(ec) return false;
    std::filesystem::create_directories(root_ / "apps", ec); if(ec) return false;
    std::ofstream(root_ / "system" / "LYFS.MARKER") << "LYFS 1.0\n"; return true;
}
bool Lyfs::ready() const noexcept { return std::filesystem::exists(root_ / "system" / "LYFS.MARKER"); }
bool Lyfs::safeGuestPath(const std::string& path, std::filesystem::path& out) const {
    if(path.empty() || path[0] != '/') return false;
    std::filesystem::path rel(path.substr(1));
    rel = rel.lexically_normal();
    if(rel.empty() || rel == ".") { out = root_; return true; }
    for(const auto& part : rel) if(part == "..") return false;
    out = root_ / rel; return true;
}
std::filesystem::path Lyfs::hostPath(const std::string& guestPath) const { std::filesystem::path out; return safeGuestPath(guestPath,out) ? out : std::filesystem::path{}; }
std::vector<FsEntry> Lyfs::list(const std::string& path) const {
    std::vector<FsEntry> result; std::filesystem::path dir;
    if(!safeGuestPath(path,dir) || !std::filesystem::is_directory(dir)) return result;
    for(const auto& e : std::filesystem::directory_iterator(dir)) {
        result.push_back({std::string("/") + std::filesystem::relative(e.path(),root_).generic_string(), e.is_directory(), e.is_regular_file() ? uint64_t(e.file_size()) : 0});
    }
    return result;
}
bool Lyfs::writeText(const std::string& path,const std::string& text){ std::filesystem::path p; if(!safeGuestPath(path,p)) return false; std::error_code ec; std::filesystem::create_directories(p.parent_path(),ec); if(ec) return false; std::ofstream f(p,std::ios::binary); if(!f) return false; f<<text; return true; }
bool Lyfs::readText(const std::string& path,std::string& out) const{ std::filesystem::path p; if(!safeGuestPath(path,p)) return false; std::ifstream f(p,std::ios::binary); if(!f) return false; out.assign(std::istreambuf_iterator<char>(f),{}); return true; }
bool Lyfs::createDirectory(const std::string& path){ std::filesystem::path p; if(!safeGuestPath(path,p)) return false; std::error_code ec; return std::filesystem::create_directories(p,ec) || !ec; }
bool Lyfs::remove(const std::string& path){ std::filesystem::path p; if(!safeGuestPath(path,p)) return false; if(p==root_) return false; std::error_code ec; std::filesystem::remove_all(p,ec); return !ec; }
uint64_t Lyfs::usedBytes() const { uint64_t n=0; if(!std::filesystem::exists(root_)) return 0; for(auto& e:std::filesystem::recursive_directory_iterator(root_)) if(e.is_regular_file()) n+=e.file_size(); return n; }
uint64_t Lyfs::capacityBytes() const { return 4ull*1024ull*1024ull*1024ull; }
} // namespace lycan
