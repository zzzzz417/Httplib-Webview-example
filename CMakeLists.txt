cmake_minimum_required(VERSION 3.10)
project(Main VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-w)
# --- 1. 源码与资源收集 ---
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")

# --- 2. 创建可执行文件 (关键：先执行 add_executable) ---
if(WIN32)
    # 检查图标资源
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resource.rc")
        set(RES_FILE "resource.rc")
    endif()
    # WIN32 参数用于隐藏控制台黑框
    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${RES_FILE})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

# --- 3. 配置路径 (Target 存在后才能配置) ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- 4. 平台差异化链接逻辑 ---

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _WIN32_WINNT=0x0A00 
        WINVER=0x0A00
        NTDDI_VERSION=0x0A000000
    )
    # Windows 宏定义
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBVIEW_WINAPI)
    
    # 链接核心库
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ws2_32 ole32 uuid version shlwapi
    )
    
    # 自动拷贝 WebView2Loader.dll 到生成目录
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/WebView2Loader.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/WebView2Loader.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()

elseif(APPLE)
    # macOS 框架链接
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework WebKit" "-framework Cocoa")
    # 设置 Bundle 图标 (需准备 app.icns)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app.icns")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_ICON_FILE app.icns
        )
        set_source_files_properties(app.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(${PROJECT_NAME} PRIVATE app.icns)
    endif()

elseif(UNIX AND NOT APPLE)
    # Linux 依赖寻找
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GTK3_LIBRARIES} ${WEBKIT_LIBRARIES})
endif()