#include <filesystem>
#include <string>
#include <thread>

#include "httplib.h"

#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#include "Proxy.h"

#ifdef _WIN32
#define PLATFORM_NAME "Windows"
#ifndef WEBVIEW_WINAPI
#define WEBVIEW_WINAPI
#endif
#elif __APPLE__
#define PLATFORM_NAME "macOS"
#elif __linux__
#define PLATFORM_NAME "Linux"
#endif
#include "webview.h"
#include "resource.h"

namespace {

constexpr int kLocalPort = 17321;

std::filesystem::path executableDirectory() {
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) return std::filesystem::current_path();
    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

} // namespace

#ifdef _WIN32
HMODULE LoadDllFromResource(int resourceId)
{
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    DWORD dllSize = SizeofResource(NULL, hRes);
    void* dllData = LockResource(hGlobal);
    HMODULE hMod = ::LoadLibraryExA(
        (LPCSTR)dllData,
        NULL,
        LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE
    );
    FreeResource(hGlobal);
    return hMod;
}
#endif

int main(){
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    LoadDllFromResource(IDR_DLL_WEBVIEW2LOADER);
    LoadDllFromResource(IDR_DLL_LIBGCC01);
    LoadDllFromResource(IDR_DLL_LIBSTD01);
    LoadDllFromResource(IDR_DLL_LIBSTD02);
#endif

    const auto webRoot = executableDirectory() / L"Web";
    const auto indexPath = webRoot / L"index.html";
    if (!std::filesystem::exists(indexPath)) {
        MessageBoxW(
            nullptr,
            (L"Web resources were not found:\n" + indexPath.wstring()).c_str(),
            L"Inno Studio Cad",
            MB_OK | MB_ICONERROR
        );
        return -1;
    }

    httplib::Server localServer;
    localServer.set_default_headers({
        {"Cross-Origin-Embedder-Policy", "credentialless"},
        {"Cross-Origin-Opener-Policy", "same-origin"},
        {"X-Content-Type-Options", "nosniff"},
    });

    registerApiProxy(localServer, {L"cad2.innosoc.com", 443, true});

    if (!localServer.set_mount_point("/", webRoot.string())) {
        MessageBoxW(
            nullptr,
            L"Unable to mount the Web directory.",
            L"Inno Studio Cad",
            MB_OK | MB_ICONERROR
        );
        return -1;
    }

    localServer.set_error_handler([indexPath](
        const httplib::Request& request,
        httplib::Response& response
    ) {
        const auto accept = request.get_header_value("Accept");
        const bool isPageNavigation = request.method == "GET" &&
            accept.find("text/html") != std::string::npos &&
            request.path.rfind("/api", 0) != 0;
        if (response.status == 404 && isPageNavigation) {
            response.status = 200;
            response.set_file_content(indexPath.string(), "text/html; charset=utf-8");
        }
    });

    if (!localServer.bind_to_port("127.0.0.1", kLocalPort)) {
        MessageBoxW(
            nullptr,
            L"Local port 17321 is already in use. Close the other instance and try again.",
            L"Inno Studio Cad",
            MB_OK | MB_ICONERROR
        );
        return -1;
    }

    std::thread serverThread([&localServer]() {
        localServer.listen_after_bind();
    });
    localServer.wait_until_ready();

    const std::string localUrl = "http://127.0.0.1:" + std::to_string(kLocalPort);

    webview::webview w(false, nullptr);
    w.set_title("Inno Studio Cad");
    // w.set_size(2050, 1153, WEBVIEW_HINT_MIN);
    w.set_size(1600, 900, WEBVIEW_HINT_FIXED);
    // w.set_size(1600, 900, WEBVIEW_HINT_NONE);
    w.navigate(localUrl);

#ifdef _WIN32
    void* hwnd_ptr =w.window().value();
    HWND hwnd=(HWND)hwnd_ptr;
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    //------------------------
#endif

    w.run();
    localServer.stop();
    if (serverThread.joinable()) serverThread.join();
    return 0;    
}
