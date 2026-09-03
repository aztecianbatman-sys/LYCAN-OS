#include "lycan_platform.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
LRESULT CALLBACK LycanWnd(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);} 
#endif
int main(){lycan::Platform os; if(!os.boot()) return 1; std::cout<<"LYCAN OS 0.4.1\n"<<os.status()<<"\n"; auto pid=os.kernel().spawn("crawford",10); std::cout<<"PID="<<pid<<"\n";
#ifdef _WIN32
 HINSTANCE hi=GetModuleHandleW(nullptr); WNDCLASSW wc{};wc.lpfnWndProc=LycanWnd;wc.hInstance=hi;wc.lpszClassName=L"LYCAN_OS";RegisterClassW(&wc);HWND w=CreateWindowExW(0,L"LYCAN_OS",L"LYCAN OS",WS_OVERLAPPEDWINDOW,120,120,1100,700,nullptr,nullptr,hi,nullptr);ShowWindow(w,SW_SHOW);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);} 
#endif
 return 0;}
