# Webview-Httplib 桌面混合应用框架
基于 **cpp-httplib + Webview** 实现的轻量桌面(cpp-web)混合开发框架
> 同类型框架:Tauri(Rust-Webview)
- 在Windows使用Webview2网页引擎
- 支持macOS(WKWebView)
- 支持Linux(WebKitGTK)
 
C++ 后端 HTTP 服务 + 前端 HTML/CSS/JS 界面，前后端双向通信，原生窗口嵌入 Web 引擎，无臃肿依赖。

## 特性
-  轻量纯头文件库，开箱即用
-  cpp-httplib 高性能本地 HTTP 服务
-  原生 WebView2 内核渲染前端页面
-  C++ 与 JS 双向函数调用
-  本地静态资源托管、JSON 数据交互
-  CMake 跨平台构建
