#include "window_manager.h"
#include <algorithm>
namespace lycan {
WindowManager::WindowManager(uint32_t w,uint32_t h):width_(w),height_(h){}
void WindowManager::normalizeFocus(){for(auto&w:windows_)w.focused=false;if(active_){for(auto&w:windows_)if(w.id==active_&&w.state!=WindowState::Closed&&w.workspace==workspace_){w.focused=true;return;}}for(auto&w:windows_)if(w.state!=WindowState::Closed&&w.workspace==workspace_){w.focused=true;active_=w.id;return;}active_=0;}
uint32_t WindowManager::create(uint32_t pid,std::string app,std::string title,uint32_t ws){Window w{nextWindow_++,pid,std::move(app),std::move(title),50+int((nextWindow_%5)*20),50+int((nextWindow_%5)*20),900,600,ws,WindowState::Normal,false,false};windows_.push_back(std::move(w));workspace_=ws;active_=windows_.back().id;normalizeFocus();return active_;}
bool WindowManager::close(uint32_t id){for(auto&w:windows_)if(w.id==id){w.state=WindowState::Closed;w.focused=false;if(active_==id)active_=0;normalizeFocus();return true;}return false;}
bool WindowManager::minimize(uint32_t id){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed){w.state=WindowState::Minimized;w.focused=false;if(active_==id)active_=0;normalizeFocus();return true;}return false;}
bool WindowManager::maximize(uint32_t id){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed){w.state=WindowState::Maximized;w.x=0;w.y=0;w.width=int(width_);w.height=int(height_);return focus(id);}return false;}
bool WindowManager::restore(uint32_t id){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed){w.state=WindowState::Normal;if(w.width>int(width_))w.width=int(width_);if(w.height>int(height_))w.height=int(height_);return focus(id);}return false;}
bool WindowManager::focus(uint32_t id){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed&&w.workspace==workspace_){if(w.state==WindowState::Minimized)w.state=WindowState::Normal;active_=id;normalizeFocus();return true;}return false;}
bool WindowManager::move(uint32_t id,int x,int y){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed){w.x=std::max(0,std::min(x,int(width_)-40));w.y=std::max(0,std::min(y,int(height_)-40));return true;}return false;}
bool WindowManager::resize(uint32_t id,int ww,int hh){for(auto&w:windows_)if(w.id==id&&w.state!=WindowState::Closed){w.width=std::max(320,std::min(ww,int(width_)));w.height=std::max(200,std::min(hh,int(height_)));return true;}return false;}
bool WindowManager::switchWorkspace(uint32_t ws){workspace_=ws;normalizeFocus();return true;}
uint32_t WindowManager::activeWindow()const noexcept{return active_;}uint32_t WindowManager::workspace()const noexcept{return workspace_;}const std::vector<Window>&WindowManager::windows()const noexcept{return windows_;}
std::vector<Window>WindowManager::workspaceWindows(uint32_t ws)const{std::vector<Window>v;for(const auto&w:windows_)if(w.workspace==ws&&w.state!=WindowState::Closed)v.push_back(w);return v;}
uint64_t WindowManager::notify(std::string app,std::string title,std::string body){auto id=nextNotification_++;notifications_.push_back({id,std::move(app),std::move(title),std::move(body),false});return id;}
bool WindowManager::markRead(uint64_t id){for(auto&n:notifications_)if(n.id==id){n.read=true;return true;}return false;}
const std::vector<Notification>&WindowManager::notifications()const noexcept{return notifications_;}
void WindowManager::setDesktopSize(uint32_t w,uint32_t h){width_=std::max(640u,w);height_=std::max(480u,h);}
uint32_t WindowManager::width()const noexcept{return width_;}uint32_t WindowManager::height()const noexcept{return height_;}
} // namespace lycan
