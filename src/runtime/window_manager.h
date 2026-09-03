#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace lycan {
enum class WindowState { Normal, Minimized, Maximized, Closed };
struct Window { uint32_t id{}; uint32_t pid{}; std::string appId; std::string title; int x{}; int y{}; int width{900}; int height{600}; uint32_t workspace{0}; WindowState state{WindowState::Normal}; bool focused{false}; bool alwaysOnTop{false}; };
struct Notification { uint64_t id{}; std::string appId; std::string title; std::string body; bool read{false}; };
class WindowManager {
public:
 WindowManager(uint32_t width=1280,uint32_t height=720);
 uint32_t create(uint32_t pid,std::string appId,std::string title,uint32_t workspace=0); bool close(uint32_t); bool minimize(uint32_t); bool maximize(uint32_t); bool restore(uint32_t); bool focus(uint32_t); bool move(uint32_t,int,int); bool resize(uint32_t,int,int); bool switchWorkspace(uint32_t);
 uint32_t activeWindow() const noexcept; uint32_t workspace() const noexcept; const std::vector<Window>& windows() const noexcept; std::vector<Window> workspaceWindows(uint32_t) const;
 uint64_t notify(std::string appId,std::string title,std::string body); bool markRead(uint64_t); const std::vector<Notification>& notifications() const noexcept;
 void setDesktopSize(uint32_t,uint32_t); uint32_t width()const noexcept;uint32_t height()const noexcept;
private: uint32_t width_,height_,nextWindow_{1}; uint32_t active_{0},workspace_{0}; std::vector<Window>windows_; std::vector<Notification>notifications_; uint64_t nextNotification_{1}; void normalizeFocus();
};
} // namespace lycan
