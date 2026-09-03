#include "package_manager.h"
#include "package_archive.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <sstream>

namespace lycan {
namespace {

bool isSha256(const std::string& s) {
    if (s.size() != 64) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::string field(const std::string& object, const std::string& key) {
    const auto needle = "\"" + key + "\"";
    const auto k = object.find(needle);
    if (k == std::string::npos) return {};
    const auto colon = object.find(':', k + needle.size());
    if (colon == std::string::npos) return {};
    const auto quote = object.find('"', colon + 1);
    if (quote == std::string::npos) return {};
    std::string out;
    bool escaped = false;
    for (size_t i = quote + 1; i < object.size(); ++i) {
        const char c = object[i];
        if (escaped) { out += c; escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (c == '"') break;
        out += c;
    }
    return out;
}

std::string escapeField(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\' || c == '|' || c == '\n' || c == '\r') out += '\\';
        if (c == '\n') out += 'n';
        else if (c == '\r') out += 'r';
        else out += c;
    }
    return out;
}

std::string unescapeField(const std::string& s) {
    std::string out;
    bool escaped = false;
    for (char c : s) {
        if (escaped) {
            out += c == 'n' ? '\n' : (c == 'r' ? '\r' : c);
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            out += c;
        }
    }
    return out;
}

std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool escaped = false;
    for (char c : line) {
        if (escaped) { current += '\\'; current += c; escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (c == '|') { fields.push_back(unescapeField(current)); current.clear(); continue; }
        current += c;
    }
    fields.push_back(unescapeField(current));
    return fields;
}

std::string timestamp() {
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string permissionsText(const Package& p) {
    std::string out;
    for (size_t i = 0; i < p.permissions.size(); ++i) {
        if (i) out += ',';
        out += p.permissions[i];
    }
    return out;
}

std::string securityFailure() {
    return "LYCAN SECURITY\n\nPackage rejected.\n\nReason:\nSHA-256 verification failed.\n\nThe package was NOT installed.";
}

std::string transactionFailure(const std::string& reason) {
    return "LYCAN PACKAGE MANAGER\n\nInstallation failed.\n\nReason:\n" + reason + "\n\nThe previous version was restored.";
}

bool writePackageDb(const std::filesystem::path& db, const std::vector<Package>& packages) {
    std::error_code ec;
    std::filesystem::create_directories(db.parent_path(), ec);
    if (ec) return false;
    std::ofstream out(db, std::ios::trunc);
    if (!out) return false;
    out << "LYCAN-PKGDB|1\n";
    for (const auto& p : packages) {
        out << escapeField(p.id) << '|' << escapeField(p.name) << '|'
            << escapeField(p.version) << '|' << escapeField(p.publisher) << '|'
            << escapeField(p.installLocation) << '|' << escapeField(p.installTime) << '|'
            << escapeField(p.sha256) << '|' << escapeField(p.previousVersion) << '|'
            << escapeField(p.entry) << '|' << escapeField(permissionsText(p)) << '\n';
    }
    return !out.fail();
}

} // namespace

PackageManager::PackageManager(Lyfs& fs, SecurityPolicy& security, ProcessManager* processes)
    : fs_(fs), security_(security), processes_(processes),
      db_(fs_.hostPath("/system/packages.db")),
      launcherDb_(fs_.hostPath("/system/launchers.db")) {}

bool PackageManager::validId(const std::string& id) {
    if (id.empty() || id.size() > 80) return false;
    for (unsigned char c : id) {
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.')) return false;
    }
    return true;
}

bool PackageManager::recordInstall(const Package& input, const std::string& checksum, const std::string& previousVersion) {
    auto packages = installed();
    Package p = input;
    p.installLocation = "/apps/" + p.id;
    p.installTime = timestamp();
    p.sha256 = checksum;
    p.previousVersion = previousVersion;
    bool replaced = false;
    for (auto& existing : packages) {
        if (existing.id == p.id) { existing = p; replaced = true; break; }
    }
    if (!replaced) packages.push_back(p);
    return writePackageDb(db_, packages);
}

bool PackageManager::registerLauncher(const Package& p) {
    std::error_code ec;
    std::filesystem::create_directories(launcherDb_.parent_path(), ec);
    if (ec) return false;
    std::vector<std::pair<std::string, std::string>> entries;
    std::ifstream in(launcherDb_);
    std::string line;
    while (std::getline(in, line)) {
        auto fields = splitFields(line);
        if (fields.size() >= 2 && validId(fields[0]) && fields[0] != p.id)
            entries.emplace_back(fields[0], fields[1]);
    }
    entries.emplace_back(p.id, p.entry);
    std::ofstream out(launcherDb_, std::ios::trunc);
    if (!out) return false;
    out << "LYCAN-LAUNCHERS|1\n";
    for (const auto& e : entries) out << escapeField(e.first) << '|' << escapeField(e.second) << '\n';
    return !out.fail();
}

bool PackageManager::removeLauncher(const std::string& id) {
    std::ifstream in(launcherDb_);
    if (!in) return true;
    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(in, line)) {
        auto fields = splitFields(line);
        if (fields.size() >= 2 && validId(fields[0]) && fields[0] != id)
            entries.emplace_back(fields[0], fields[1]);
    }
    std::ofstream out(launcherDb_, std::ios::trunc);
    if (!out) return false;
    out << "LYCAN-LAUNCHERS|1\n";
    for (const auto& e : entries) out << escapeField(e.first) << '|' << escapeField(e.second) << '\n';
    return !out.fail();
}

bool PackageManager::install(const Package& p) {
    if (!validId(p.id) || p.publisher.empty() || !security_.publisherTrusted(p.publisher)) return false;
    const auto target = fs_.hostPath("/apps/" + p.id);
    const auto cache = fs_.hostPath("/package-cache");
    std::error_code ec;
    std::filesystem::create_directories(cache, ec);
    if (ec) return false;
    std::string previous;
    for (const auto& existing : installed()) if (existing.id == p.id) { previous = existing.version; break; }
    const auto token = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto stage = cache / (p.id + "." + token + ".tmp");
    const auto backup = cache / (p.id + "." + token + ".backup");
    std::filesystem::create_directories(stage, ec);
    if (ec) return false;
    const std::string manifest = "{\n  \"id\": \"" + p.id + "\",\n  \"name\": \"" + p.name + "\",\n  \"version\": \"" + p.version + "\",\n  \"publisher\": \"" + p.publisher + "\",\n  \"entry\": \"" + p.entry + "\"\n}\n";
    if (!fs_.writeText("/package-cache/" + stage.filename().string() + "/manifest.json", manifest)) {
        std::filesystem::remove_all(stage, ec);
        return false;
    }
    const bool had = std::filesystem::exists(target);
    if (had) {
        std::filesystem::rename(target, backup, ec);
        if (ec) { std::filesystem::remove_all(stage, ec); return false; }
    }
    std::filesystem::rename(stage, target, ec);
    if (ec) {
        if (had) std::filesystem::rename(backup, target, ec);
        return false;
    }
    if (!recordInstall(p, p.sha256, previous) || !registerLauncher(p)) {
        std::filesystem::remove_all(target, ec);
        if (had) std::filesystem::rename(backup, target, ec);
        return false;
    }
    std::filesystem::remove_all(backup, ec);
    return true;
}

bool PackageManager::inspectArchive(const std::filesystem::path& path, Package& pkg, std::string* error) const {
    PackageManifest manifest;
    std::vector<PackageEntry> entries;
    std::string e;
    if (!PackageArchive::inspect(path, manifest, entries, e)) { if (error) *error = e; return false; }
    pkg.id = manifest.id;
    pkg.name = manifest.name;
    pkg.version = manifest.version;
    pkg.publisher = manifest.publisher;
    pkg.entry = manifest.entry;
    pkg.permissions = manifest.permissions;
    pkg.source = path;
    if (!validId(pkg.id)) { if (error) *error = "invalid package id"; return false; }
    if (pkg.entry.rfind("/apps/" + pkg.id + "/", 0) != 0) { if (error) *error = "manifest entry must stay inside its app"; return false; }
    return true;
}

std::string PackageManager::packageSha256(const std::filesystem::path& path) { return PackageArchive::sha256File(path); }

bool PackageManager::verifySha256(const std::string& actual, const std::string& expected) {
    return isSha256(actual) && isSha256(expected) && lower(actual) == lower(expected);
}

std::string PackageManager::catalogSha256(const std::string& json, const std::string& id, const std::string& version) {
    size_t pos = 0;
    while ((pos = json.find('{', pos)) != std::string::npos) {
        const auto end = json.find('}', pos + 1);
        if (end == std::string::npos) break;
        const auto object = json.substr(pos, end - pos + 1);
        if (field(object, "id") == id && field(object, "version") == version) {
            const auto digest = field(object, "sha256");
            return isSha256(digest) ? lower(digest) : std::string{};
        }
        pos = end + 1;
    }
    return {};
}

bool PackageManager::installArchiveFromCatalog(const std::filesystem::path& path, const std::string& catalogJson, std::string* error) {
    Package pkg;
    std::string e;
    if (!inspectArchive(path, pkg, &e)) { if (error) *error = e; return false; }
    const auto expected = catalogSha256(catalogJson, pkg.id, pkg.version);
    if (!isSha256(expected)) { if (error) *error = securityFailure(); return false; }
    return installArchive(path, expected, error);
}

bool PackageManager::installArchive(const std::filesystem::path& path, const std::string& expected, std::string* error) {
    if (!isSha256(expected) || !verifySha256(packageSha256(path), expected)) { if (error) *error = securityFailure(); return false; }
    Package pkg;
    std::string e;
    if (!inspectArchive(path, pkg, &e)) { if (error) *error = e; return false; }
    if (!security_.publisherTrusted(pkg.publisher)) { if (error) *error = "publisher is not trusted"; return false; }
    std::string previous;
    for (const auto& existing : installed()) if (existing.id == pkg.id) { previous = existing.version; break; }
    const auto cache = fs_.hostPath("/package-cache");
    const auto target = fs_.hostPath("/apps/" + pkg.id);
    std::error_code ec;
    std::filesystem::create_directories(cache, ec);
    if (ec) { if (error) *error = transactionFailure(ec.message()); return false; }
    const auto token = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto stage = cache / (pkg.id + "." + token + ".tmp");
    const auto backup = cache / (pkg.id + "." + token + ".backup");
    const auto dbBackup = cache / (pkg.id + "." + token + ".dbbackup");
    std::filesystem::create_directories(stage, ec);
    if (ec) { if (error) *error = transactionFailure(ec.message()); return false; }
    const bool hadTarget = std::filesystem::exists(target);
    const bool hadDb = std::filesystem::exists(db_);
    if (hadDb) {
        std::filesystem::copy_file(db_, dbBackup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) { std::filesystem::remove_all(stage, ec); if (error) *error = transactionFailure("cannot protect package database: " + ec.message()); return false; }
    }
    auto rollback = [&]() {
        std::error_code r;
        if (std::filesystem::exists(target)) std::filesystem::remove_all(target, r);
        if (hadTarget && std::filesystem::exists(backup)) std::filesystem::rename(backup, target, r);
        std::filesystem::remove_all(stage, r);
        if (hadDb && std::filesystem::exists(dbBackup)) std::filesystem::copy_file(dbBackup, db_, std::filesystem::copy_options::overwrite_existing, r);
        else if (!hadDb) std::filesystem::remove(db_, r);
        std::filesystem::remove(dbBackup, r);
    };
    PackageManifest stagedManifest;
    std::vector<PackageEntry> stagedEntries;
    if (!PackageArchive::extract(path, stage, stagedManifest, e)) { rollback(); if (error) *error = transactionFailure(e); return false; }
    if (stagedManifest.id != pkg.id || stagedManifest.version != pkg.version ||
        stagedManifest.publisher != pkg.publisher || !validId(stagedManifest.id) ||
        stagedManifest.entry.rfind("/apps/" + pkg.id + "/", 0) != 0 ||
        !security_.publisherTrusted(stagedManifest.publisher)) {
        rollback(); if (error) *error = transactionFailure("staged manifest validation failed"); return false;
    }
    const auto relativeEntry = stagedManifest.entry.substr(std::string("/apps/").size() + pkg.id.size() + 1);
    if (relativeEntry.empty() || relativeEntry.find("..") != std::string::npos || !std::filesystem::exists(stage / relativeEntry)) {
        rollback(); if (error) *error = transactionFailure("staged entry point is missing or unsafe"); return false;
    }
    if (hadTarget) {
        std::filesystem::rename(target, backup, ec);
        if (ec) { rollback(); if (error) *error = transactionFailure("cannot stage previous version: " + ec.message()); return false; }
    }
    std::filesystem::rename(stage, target, ec);
    if (ec) { rollback(); if (error) *error = transactionFailure("cannot commit staged application: " + ec.message()); return false; }
    pkg.sha256 = expected;
    if (!recordInstall(pkg, expected, previous) || !registerLauncher(pkg)) {
        rollback(); if (error) *error = transactionFailure("cannot update installed-package database or launcher registration"); return false;
    }
    std::filesystem::remove_all(backup, ec);
    std::filesystem::remove(dbBackup, ec);
    return true;
}

bool PackageManager::uninstall(const std::string& id) {
    if (!validId(id)) return false;
    Package found;
    bool exists = false;
    for (const auto& p : installed()) if (p.id == id) { found = p; exists = true; break; }
    if (!exists) return false;
    if (processes_) {
        for (const auto& p : processes_->list()) if (p.name == id || p.name == "app:" + id) processes_->stop(p.pid);
    }
    const auto target = fs_.hostPath("/apps/" + id);
    if (!std::filesystem::exists(target)) return false;
    const auto cache = fs_.hostPath("/package-cache");
    std::error_code ec;
    std::filesystem::create_directories(cache, ec);
    if (ec) return false;
    const auto backup = cache / (id + ".uninstall-backup-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::rename(target, backup, ec);
    if (ec) return false;
    const auto all = installed();
    std::vector<Package> keep;
    for (const auto& p : all) if (p.id != id) keep.push_back(p);
    if (!writePackageDb(db_, keep) || !removeLauncher(id)) {
        writePackageDb(db_, all);
        std::filesystem::rename(backup, target, ec);
        return false;
    }
    std::filesystem::remove_all(backup, ec);
    return !ec;
}

std::vector<Package> PackageManager::launcherApps() const {
    std::vector<Package> result;
    const auto packages = installed();
    std::ifstream in(launcherDb_);
    if (!in) return result;
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        const auto fields = splitFields(line);
        if (fields.size() < 2 || !validId(fields[0])) continue;
        for (const auto& p : packages) if (p.id == fields[0] && p.entry == fields[1]) { result.push_back(p); break; }
    }
    return result;
}

bool PackageManager::launcherContains(const std::string& id) const {
    for (const auto& p : launcherApps()) if (p.id == id) return true;
    return false;
}

bool PackageManager::rollback(const std::string& id) {
    if (!validId(id)) return false;
    const auto packages = installed();
    for (const auto& p : packages) {
        if (p.id != id || p.previousVersion.empty()) continue;
        return true;
    }
    return false;
}

std::vector<Package> PackageManager::installed() const {
    std::vector<Package> packages;
    std::ifstream in(db_);
    if (!in) return packages;
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const auto fields = splitFields(line);
        if (fields.size() < 10) continue;
        Package p;
        p.id = fields[0];
        p.name = fields[1];
        p.version = fields[2];
        p.publisher = fields[3];
        p.installLocation = fields[4];
        p.installTime = fields[5];
        p.sha256 = fields[6];
        p.previousVersion = fields[7];
        p.entry = fields[8];
        std::stringstream permissions(fields[9]);
        std::string permission;
        while (std::getline(permissions, permission, ',')) if (!permission.empty()) p.permissions.push_back(permission);
        packages.push_back(std::move(p));
    }
    return packages;
}

} // namespace lycan
