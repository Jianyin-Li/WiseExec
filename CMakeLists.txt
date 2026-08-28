#=============================================================================
# CMake 配置文件 - wxWidgets 版本（跨平台）
#=============================================================================

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED PROJECT_NAME)
    set(PROJECT_NAME "WiseExec" CACHE STRING "Project name")
endif()

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION "2.0.0" CACHE STRING "Project version")
endif()

project(
    ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "WiseExec"
    LANGUAGES CXX
)

cmake_policy(SET CMP0077 NEW)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#=============================================================================
# Dependencies
#=============================================================================

# --- wxWidgets ---
# Windows: 使用手动构建的 wxWidgets 静态库（MinGW）。
# 注意：Windows 上不要调用 find_package(wxWidgets)——当 wxWidgets 源码树位于
# 常见路径（如 C:/wxWidgets）时，FindwxWidgets.cmake 探测失败会把缓存变量
# WX_LIB_DIR 污染成 "wxWidgets_LIB_DIR-NOTFOUND"。
# Linux/macOS: 使用系统包提供的 find_package(wxWidgets)。
if(WIN32)
    message(STATUS "Using manual MinGW wxWidgets configuration")
    set(WX_ROOT "D:/DevTool/wxWidgets" CACHE PATH "wxWidgets root (MinGW static build)")
    # WX_LIB_DIR 可能被 FindwxWidgets.cmake 污染为 NOTFOUND，必须强制修正
    if(NOT WX_LIB_DIR OR WX_LIB_DIR MATCHES "-NOTFOUND$")
        set(WX_LIB_DIR "${WX_ROOT}/build_mingw/lib/gcc_x64_lib" CACHE PATH "wxWidgets static lib dir" FORCE)
    endif()
    set(WX_SETUP_DIR "${WX_LIB_DIR}/mswu" CACHE PATH "wxWidgets setup.h dir" FORCE)
    set(WX_INC "${WX_ROOT}/include" CACHE PATH "wxWidgets include dir" FORCE)

    if(NOT EXISTS "${WX_LIB_DIR}/wxmsw33u_core.a")
        message(FATAL_ERROR "wxWidgets static libs not found in ${WX_LIB_DIR}. "
            "Build wxWidgets 3.3.0 for MinGW or set -DWX_LIB_DIR=...")
    endif()
    message(STATUS "Using wxWidgets from ${WX_ROOT}")
else()
    find_package(wxWidgets QUIET COMPONENTS core adv net base xml html)
    if(NOT wxWidgets_FOUND)
        message(FATAL_ERROR "wxWidgets not found. Install it via your package manager "
            "(e.g. libwxgtk3.2-dev on Debian/Ubuntu, 'brew install wxwidgets' on macOS).")
    endif()
endif()

# yaml-cpp
if(EXISTS "D:/DevTool/yaml-cpp/lib/cmake/yaml-cpp")
    set(yaml-cpp_DIR "D:/DevTool/yaml-cpp/lib/cmake/yaml-cpp")
endif()
find_package(yaml-cpp CONFIG REQUIRED)

#=============================================================================
# Sources
#=============================================================================

set(HEADERS
    include/config.h
    include/appitem.h
    include/funcitem.h
    include/icongenerator.h
    include/icongridpanel.h
    include/appconfigdialog.h
    include/funcconfigdialog.h
    include/mainwindow.h
    include/translations.h
)

set(SOURCES
    src/main.cpp
    src/appitem.cpp
    src/funcitem.cpp
    src/icongenerator.cpp
    src/icongridpanel.cpp
    src/appconfigdialog.cpp
    src/funcconfigdialog.cpp
    src/mainwindow.cpp
)

# Windows 资源文件（.rc）仅在 Windows 上可用
set(RESOURCES)
if(WIN32)
    list(APPEND RESOURCES
        resources/app_icon.rc
        resources/WiseExec.manifest.rc
    )
endif()

#=============================================================================
# Target
#=============================================================================

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)

if(wxWidgets_FOUND)
    # 使用系统安装的 wxWidgets（Linux/macOS）
    include(${wxWidgets_USE_FILE})
    target_include_directories(${PROJECT_NAME} PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES} yaml-cpp::yaml-cpp)
else()
    # Windows: 手动 MinGW 静态库配置
    target_include_directories(${PROJECT_NAME} PRIVATE ${WX_INC} ${WX_SETUP_DIR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __WXMSW__
        _UNICODE UNICODE
        wxUSE_UNICODE=1
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
    )
    target_link_directories(${PROJECT_NAME} PRIVATE ${WX_LIB_DIR})

    # Use : prefix to specify exact filename (MinGW expects lib prefix, but wx uses plain names)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        :wxmsw33u_core.a
        :wxmsw33u_adv.a
        :wxbase33u.a
        :wxtiff.a
        :wxjpeg.a
        :wxpng.a
        :wxzlib.a
        :wxexpat.a
        :wxregexu.a
        :wxwebp.a
        :wxwebpdemux.a
        :wxsharpyuv.a
        yaml-cpp::yaml-cpp
        comctl32
        dwmapi
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        gdi32
        winspool
        winmm
        rpcrt4
        uxtheme
        gdiplus
        shlwapi
        version
        wininet
        msimg32
        oleacc
        imm32
        bcrypt
        propsys
        shlwapi
    )
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
)

# Windows-specific: hide console in Release
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # 静态链接 MinGW 运行时（libgcc/libstdc++/winpthread），
    # 生成免 DLL 的单文件可执行程序（无需随附 *.dll 即可分发）。
    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

#=============================================================================
# Options
#=============================================================================

option(BUILD_TESTS "Build tests" OFF)

#=============================================================================
# Packaging (CPack)
#=============================================================================

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WiseExec application launcher")
set(CPACK_PACKAGE_VENDOR "WiseExec")
set(CPACK_GENERATOR "ZIP")
include(CPack)

