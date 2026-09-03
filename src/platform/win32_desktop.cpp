#include "win32_desktop.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <string>
#include <vector>

namespace lycan {
namespace {
AppHost* g_host = nullptr;
HWND g_output = nullptr;
HWND g_input = nullptr;
HWND g_status = nullptr;
HWND g_window = nullptr;

std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

void paintText(HDC dc, const wchar_t* value, int x, int y, int size, bool bold = false, COLORREF color = RGB(235,239,247)) {
    HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    TextOutW(dc, x, y, value, static_cast<int>(wcslen(value)));
    SelectObject(dc, old);
    DeleteObject(font);
}

void fill(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void appendOutput(const std::string& text) {
    if (!g_output) return;
    SetWindowTextW(g_output, wide(text).c_str());
}

void runCommand() {
    if (!g_host || !g_input) return;
    const int n = GetWindowTextLengthW(g_input);
    std::wstring ws(static_cast<size_t>(n), L'\0');
    if (n) GetWindowTextW(g_input, ws.data(), n + 1);
    if (ws.empty()) return;
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, ws.data(), n, nullptr, 0, nullptr, nullptr);
    std::string command(static_cast<size_t>(bytes), '\0');
    if (bytes) WideCharToMultiByte(CP_UTF8, 0, ws.data(), n, command.data(), bytes, nullptr, nullptr);
    appendOutput(g_host->execute(command));
    SetWindowTextW(g_input, L"");
    SetFocus(g_input);
}

void openFirstApp() {
    if (!g_host) return;
    const auto apps = g_host->packages().launcherApps();
    if (apps.empty()) return;
    appendOutput(g_host->execute("open " + apps.front().id));
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_window = hwnd;
        g_status = CreateWindowExW(0, L"STATIC", L"LYCAN OS 1.0  |  Windows-hosted virtual workspace",
                                   WS_CHILD | WS_VISIBLE, 28, 22, 700, 28, hwnd, nullptr,
                                   GetModuleHandleW(nullptr), nullptr);
        g_output = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Booting LYCAN...",
                                   WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                                   28, 70, 744, 330, hwnd, reinterpret_cast<HMENU>(101),
                                   GetModuleHandleW(nullptr), nullptr);
        g_input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                  28, 420, 610, 30, hwnd, reinterpret_cast<HMENU>(102),
                                  GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"BUTTON", L"RUN",
                        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                        650, 420, 122, 30, hwnd, reinterpret_cast<HMENU>(103),
                        GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"BUTTON", L"OPEN FIRST APP",
                        WS_CHILD | WS_VISIBLE,
                        28, 468, 180, 32, hwnd, reinterpret_cast<HMENU>(104),
                        GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"BUTTON", L"DIAGNOSTICS",
                        WS_CHILD | WS_VISIBLE,
                        220, 468, 150, 32, hwnd, reinterpret_cast<HMENU>(105),
                        GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"BUTTON", L"SESSIONS",
                        WS_CHILD | WS_VISIBLE,
                        382, 468, 140, 32, hwnd, reinterpret_cast<HMENU>(106),
                        GetModuleHandleW(nullptr), nullptr);
        appendOutput(g_host ? g_host->execute("diagnostics") : "LYCAN host unavailable");
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case 102:
            if (HIWORD(wp) == EN_UPDATE) InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case 103:
            runCommand();
            return 0;
        case 104:
            openFirstApp();
            return 0;
        case 105:
            if (g_host) appendOutput(g_host->execute("diagnostics"));
            return 0;
        case 106:
            if (g_host) appendOutput(g_host->execute("sessions"));
            return 0;
        default:
            break;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_RETURN && GetFocus() == g_input) {
            runCommand();
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_window = nullptr;
        g_input = nullptr;
        g_output = nullptr;
        g_status = nullptr;
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);
        fill(dc, client, RGB(10,14,20));
        paintText(dc, L"LYCAN", 28, 20, 20, true, RGB(130,148,255));
        paintText(dc, L"VIRTUAL WORKSPACE", 104, 24, 9, false, RGB(130,140,155));
        paintText(dc, L"ARES command console  ·  LYFS  ·  applications  ·  recovery", 28, 398, 9, false, RGB(120,132,150));
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}

int runDesktop(AppHost& host) {
    g_host = &host;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"LYCANDesktopWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, className, L"LYCAN OS — Virtual Workspace",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, 820, 560,
                               nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    g_host = nullptr;
    return static_cast<int>(msg.wParam);
}

} // namespace lycan
