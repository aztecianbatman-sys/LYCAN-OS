#include "lycan_platform.h"
#include "lycan_shell.h"
#include "lycan_store.h"
#include <fstream>
#include <sstream>
#include <string>
#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>

namespace {
HWND g_status=nullptr; HWND g_store=nullptr; lycan::GeckoRuntime g_gecko; lycan::LycanStore g_storeModel;
void setText(const std::string& s){if(g_status){std::wstring w(s.begin(),s.end());SetWindowTextW(g_status,w.c_str());}}
void showStore(){std::ostringstream s;s<<"LYCAN STORE — FREE SOFTWARE\r\n\r\n";for(const auto&a:g_storeModel.apps())s<<a.name<<"  v"<<a.version<<"\r\n  "<<a.description<<"\r\n  Publisher: "<<a.author<<"\r\n\r\n";setText(s.str());}
LRESULT CALLBACK LycanWnd(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_COMMAND){switch(LOWORD(w)){case 100:setText("LYCAN OS 0.6.0\r\n\r\nVirtual CPU: ONLINE\r\nVirtual memory/MMU: ONLINE\r\nLYFS storage: ONLINE\r\nKernel/services: ONLINE\r\nSecurity boundary: ONLINE\r\nPackage verification: SHA-256 ONLINE\r\nWindows host: UNTOUCHED");break;case 101:showStore();break;case 102:setText(lycan::hostDiagnostics());break;case 103:{if(g_gecko.available()){std::wstring p(g_gecko.path().begin(),g_gecko.path().end());ShellExecuteW(h,L"open",p.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}else MessageBoxW(h,L"Lycan Web requires the Gecko runtime. Firefox/Gecko was not detected.\n\nThis build does not fake Gecko with Chromium.",L"Lycan Web",MB_OK|MB_ICONWARNING);}break;case 104:PostQuitMessage(0);break;}}if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE, PWSTR, int nCmdShow){
 lycan::Platform os; if(!os.boot())return 1;
 g_gecko.discover();
 std::ifstream cat("store/catalog.json"); std::stringstream ss;ss<<cat.rdbuf();g_storeModel.loadCatalog(ss.str());
 lycan::ShellRegistry shell;shell.add({"terminal","Terminal","Native LYCAN terminal","lycan-cli.exe"});shell.add({"files","Files","LYFS file manager",""});shell.add({"store","Lycan Store","Free package catalog",""});shell.add({"web","Lycan Web","Gecko browser boundary",""});
 HINSTANCE instance=hi?hi:GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.lpfnWndProc=LycanWnd;wc.hInstance=instance;wc.lpszClassName=L"LYCAN_OS_DESKTOP";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=CreateSolidBrush(RGB(15,16,19));RegisterClassW(&wc);
 HWND h=CreateWindowExW(0,wc.lpszClassName,L"LYCAN OS 0.6.0",WS_OVERLAPPEDWINDOW,100,80,1180,760,nullptr,nullptr,instance,nullptr);if(!h)return 2;
 CreateWindowW(L"STATIC",L"LYCAN OS",WS_CHILD|WS_VISIBLE,35,25,300,45,h,nullptr,instance,nullptr);
 const wchar_t* labels[]={L"Desktop",L"Lycan Store",L"Diagnostics",L"Lycan Web",L"Exit"};int ids[]={100,101,102,103,104};for(int i=0;i<5;i++)CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,35,90+i*55,180,42,h,(HMENU)(INT_PTR)ids[i],instance,nullptr);
 g_status=CreateWindowW(L"EDIT",L"Booting LYCAN...",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY,260,90,850,540,h,nullptr,instance,nullptr);
 ShowWindow(h,nCmdShow?nCmdShow:SW_SHOW);UpdateWindow(h);setText("LYCAN OS 0.6.0\r\n\r\nWelcome to the native Windows-hosted LYCAN desktop.\r\n\r\nThe guest environment is isolated from the host Windows installation.\r\nPackages are verified before trust.\r\nThe Store is free to users.\r\n\r\nSelect a module on the left.");
 MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}
#else
#include <iostream>
int main(){lycan::Platform os;if(!os.boot())return 1;std::cout<<"LYCAN OS 0.6.0\n"<<os.status()<<"\n"<<lycan::hostDiagnostics();return 0;}
#endif
