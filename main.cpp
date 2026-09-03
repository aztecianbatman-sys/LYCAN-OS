#include "lycan_platform.h"
#include "lycan_store.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
LRESULT CALLBACK LycanWnd(HWND h, UINT m, WPARAM w, LPARAM l){
    if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(h,m,w,l);
}
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int nCmdShow){
    lycan::Platform os;
    if(!os.boot()) return 1;

    lycan::GeckoRuntime gecko;
    gecko.discover();

    HINSTANCE instance = hi ? hi : GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = LycanWnd;
    wc.hInstance = instance;
    wc.lpszClassName = L"LYCAN_OS";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    HWND w = CreateWindowExW(0, L"LYCAN_OS", L"LYCAN OS 0.5.0",
        WS_OVERLAPPEDWINDOW, 120, 120, 1100, 700, nullptr, nullptr, instance, nullptr);
    if(!w) return 2;
    ShowWindow(w, nCmdShow ? nCmdShow : SW_SHOW);
    UpdateWindow(w);

    std::wstring status = L"LYCAN OS 0.5.0\n\nVirtual environment: READY\nLycan Store: READY\nLycan Web / Gecko: ";
    status += gecko.available() ? L"DETECTED" : L"NOT INSTALLED";
    status += L"\n\nThe Windows host remains untouched.\nStore packages are downloaded into the Lycan package boundary.";
    SetWindowTextW(w, L"LYCAN OS 0.5.0 — Store + Web Runtime");
    MessageBoxW(w, status.c_str(), L"LYCAN OS", MB_OK | MB_ICONINFORMATION);

    MSG msg{};
    while(GetMessageW(&msg, nullptr, 0, 0) > 0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return static_cast<int>(msg.wParam);
}
#else
int main(){
    lycan::Platform os;
    if(!os.boot()) return 1;
    lycan::LycanStore store;
    std::cout << "LYCAN OS 0.5.0\n" << os.status() << "\n";
    std::cout << "Store catalog subsystem: ready\n";
    auto pid = os.kernel().spawn("crawford", 10);
    std::cout << "PID=" << pid << "\n";
    return 0;
}
#endif
