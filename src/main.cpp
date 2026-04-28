#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<vector>
#include<fstream>
#include<string>
#include <thread>
#include <variant>
#include <dwmapi.h>
#include <windowsx.h>

#include"httplib.h"
#include"nlohmann/json.hpp"
#include"Server.h"

#ifdef _WIN32
#include <windows.h>
#define PLATFORM_NAME "Windows"
#define WEBVIEW_WINAPI
#elif __APPLE__
#define PLATFORM_NAME "macOS"
#elif __linux__
#define PLATFORM_NAME "Linux"
#endif
#include"webview.h"

std::string url="http://127.0.0.1:";
int main(){
    #ifdef _WIN32
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    #endif

    httplib::Server basic_server;

    std::string AUTH_TOKEN="aaaa";

    basic_server.set_pre_request_handler([&](const httplib::Request& request, httplib::Response& response)->httplib::Server::HandlerResponse{
        if(request.get_header_value("Auth-Token")!=AUTH_TOKEN){
            response.status=403;
            return httplib::Server::HandlerResponse::Handled;
        }else return httplib::Server::HandlerResponse::Unhandled;
    });

    basic_server.Get("/api/getData",ApiHandlers::getData);
    basic_server.Post("/api/upData",ApiHandlers::upData);   
    basic_server.Post("/api/addData",ApiHandlers::addData);
    basic_server.Post("/api/delData",ApiHandlers::delData);

    basic_server.set_mount_point("/","../Web");
    int port=basic_server.bind_to_any_port("127.0.0.1");
    if(port<0)return -1;


    url+=std::to_string(port);
    std::thread server_thread([&]() {
        basic_server.listen_after_bind();
    });

    basic_server.wait_until_ready();

    webview::webview w(false, nullptr);
    w.set_title("Schedule");
    // w.set_size(2050, 1153, WEBVIEW_HINT_MIN);
    w.set_size(1600, 900, WEBVIEW_HINT_FIXED);
    // w.set_size(1600, 900, WEBVIEW_HINT_NONE);
    w.navigate(url);
    w.bind("getToken",[&](const std::string str)->std::string{
        return "\""+AUTH_TOKEN+"\"";
    });

#ifdef _WIN32
    void* hwnd_ptr =w.window().value();
    HWND hwnd=(HWND)hwnd_ptr;
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    //------------------------
#endif

    w.run();
    basic_server.stop();
    if(server_thread.joinable()) {
        server_thread.join();
    }
    return 0;    
}

