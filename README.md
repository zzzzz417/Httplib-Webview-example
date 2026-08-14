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

## InnoCAD 客户端网络结构

Windows 客户端打开 `http://127.0.0.1:17321`，由内置 `cpp-httplib` 服务提供
`Web/` 中的 Vite 构建产物。所有 `/api/*` 请求由本地服务通过 Windows WinHTTP
转发到 `https://cad2.innosoc.com`。代理保留请求路径、查询参数、请求体、认证头、
CAD AI 关联头、上游状态码和响应头，并使用分块响应传递 AI 事件流。

构建后，CMake 会把源目录中的 `Web/` 复制到可执行文件旁边。最终发布目录至少需要：

```text
Main.exe
WebView2Loader.dll
Web/
```

MinGW 动态运行库仍需按实际编译方式一同发布。应用只监听 `127.0.0.1`；固定端口被
占用时会显示错误并退出。

登录回调地址固定为：

```text
http://127.0.0.1:17321/sso/callback
```

该地址需要同时加入 CAD backend 和上游 SSO 客户端的允许回调列表。
