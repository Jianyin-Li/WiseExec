#=============================================================================
# CMake 配置文件 - wxWidgets 版本
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
    LANGUAGES CXX RC
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

# wxWidgets (static) - manual configuration
set(WX_ROOT "D:/DevTool/wxWidgets")
set(WX_BUILD "${WX_ROOT}/build_mingw")
set(WX_LIB "${WX_BUILD}/lib/gcc_x64_lib")
set(WX_INC "${WX_ROOT}/include")
set(WX_SETUP "${WX_BUILD}/lib/gcc_x64_lib/mswu")

# Verify wxWidgets is built
if(NOT EXISTS "${WX_LIB}/wxmsw33u_core.a")
    message(FATAL_ERROR "wxWidgets not built! Run 'mingw32-make' in ${WX_BUILD}")
endif()

message(STATUS "Using wxWidgets from ${WX_ROOT}")

# yaml-cpp
set(yaml-cpp_DIR "D:/DevTool/yaml-cpp/lib/cmake/yaml-cpp")
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

set(RESOURCES
    resources/app_icon.rc
    resources/WiseExec.manifest.rc
)

#=============================================================================
# Target
#=============================================================================

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES}
    include/translations.h)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${WX_INC}
    ${WX_SETUP}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    __WXMSW__
    _UNICODE UNICODE
    wxUSE_UNICODE=1
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    $<$<CONFIG:Release>:NDEBUG>
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${WX_LIB}
)

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

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
)

# Windows-specific: hide console in Release
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

#=============================================================================
# Options
#=============================================================================

option(BUILD_TESTS "Build tests" OFF)

