#=============================================================================
# CMake 配置文件 - 标准化版本
#=============================================================================

# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.16)

# 设置项目变量（如果未在命令行中定义）
if(NOT DEFINED PROJECT_NAME)
    set(PROJECT_NAME "QtAppLauncher" CACHE STRING "Project name")
endif()

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION "2.0.0" CACHE STRING "Project version")
endif()

# 项目基本信息
project(
    ${PROJECT_NAME} 
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Qt Application Launcher"
    LANGUAGES CXX
)

# 设置 CMake 策略
cmake_policy(SET CMP0071 NEW)  # 使用绝对路径在 target_sources() 中
cmake_policy(SET CMP0077 NEW)  # 选项() 尊重正常变量

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用 Qt 自动处理
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 设置 UI 文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

# 设置构建输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#=============================================================================
# 配置选项
#=============================================================================

# 安装路径配置
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix")
set(CMAKE_INSTALL_BINDIR "bin" CACHE STRING "Binary installation directory" FORCE)

# 项目选项
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(ENABLE_DEBUG "Enable debug features" OFF)


# 平台特定选项
if(WIN32)
    option(USE_STATIC_RUNTIME "Use static runtime libraries" ON)
    option(GENERATE_PDB "Generate PDB debug files" ON)
endif()

#=============================================================================
# 源文件自动发现
#=============================================================================

# 自动发现源文件
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

# 自动发现头文件
file(GLOB_RECURSE HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# 自动发现 UI 文件
file(GLOB_RECURSE UI_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)

# 自动发现资源文件
file(GLOB_RECURSE RESOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc"
)

# 过滤掉不需要的文件（如果有）
list(FILTER SOURCES EXCLUDE REGEX ".*_test\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*_mock\\.cpp$")
list(FILTER HEADERS EXCLUDE REGEX ".*_test\\.h$")
list(FILTER HEADERS EXCLUDE REGEX ".*_mock\\.h$")

# 计算文件数量
list(LENGTH SOURCES SOURCES_COUNT)
list(LENGTH HEADERS HEADERS_COUNT)
list(LENGTH UI_FILES UI_FILES_COUNT)
list(LENGTH RESOURCE_FILES RESOURCE_FILES_COUNT)

# 打印文件统计信息
message(STATUS "Found ${SOURCES_COUNT} source files")
message(STATUS "Found ${HEADERS_COUNT} header files")
message(STATUS "Found ${UI_FILES_COUNT} UI files")
message(STATUS "Found ${RESOURCE_FILES_COUNT} resource files")

#=============================================================================
# 目标创建
#=============================================================================

# 创建可执行文件目标
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

#=============================================================================
# 目标属性配置
#=============================================================================

# 包含目录
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Qt${QT_VERSION_MAJOR}::Widgets
)

# 编译定义
if(ENABLE_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_DEBUG=1)
endif()

# 平台特定设置
if(WIN32)
    # Windows 特定设置
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        SUFFIX ".exe"
    )
    
    # 设置图标
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RC_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico"
        )
    endif()
    
    # 运行时库设置
    if(USE_STATIC_RUNTIME)
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE /MT$<$<CONFIG:Debug>:d>)
        endif()
    endif()
    
    # 调试信息生成
    if(GENERATE_PDB AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(MSVC)
            # MSVC 编译器选项
            target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
            target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
        else()
            # GCC/MinGW 编译器选项
            target_compile_options(${PROJECT_NAME} PRIVATE -g)
            target_link_options(${PROJECT_NAME} PRIVATE -g)
        endif()
    endif()
    
elseif(APPLE)
    # macOS 特定设置
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
else()
    # Linux/Unix 特定设置
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

# 通用目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

#=============================================================================
# 安装配置
#=============================================================================

# 安装可执行文件
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)



#=============================================================================
# 测试配置（如果启用）
#=============================================================================

if(BUILD_TESTS)
    enable_testing()
    
    # 自动发现测试文件
    file(GLOB_RECURSE TEST_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    )
    
    if(TEST_SOURCES)
        add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
        target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME})
        add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
    endif()
endif()

#=============================================================================
# 生成配置文件
#=============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

#=============================================================================
# 总结信息
#=============================================================================

message(STATUS "==========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION_MAJOR}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Source files: ${SOURCES_COUNT}")
message(STATUS "Header files: ${HEADERS_COUNT}")
message(STATUS "UI files: ${UI_FILES_COUNT}")
message(STATUS "Resource files: ${RESOURCE_FILES_COUNT}")
message(STATUS "==========================================")

#=============================================================================
# CPack 配置
#=============================================================================

# 包含 CPack 模块
include(InstallRequiredSystemLibraries)
include(CPack)

# 设置 CPack 变量
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Example Company")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt Application Launcher")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# 设置包文件名
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")

# Windows 特定配置
if(WIN32)
    # NSIS 配置
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico")
    set(CPACK_NSIS_HELP_LINK "https://example.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://example.com")
    set(CPACK_NSIS_CONTACT "support@example.com")
    
    # 创建桌面快捷方式
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$DESKTOP\\\\${PROJECT_NAME}.lnk' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'
    ")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "
        Delete '$DESKTOP\\\\${PROJECT_NAME}.lnk'
    ")
    
    # 添加到 PATH
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

# 设置生成器（根据平台）
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "ZIP;TGZ")
endif()

# 设置源包生成器
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\\\.git/"
    "/\\\\.github/"
    "/build/"
    "/\\\\.vscode/"
    "\\\\.swp$"
    "\\\\.bak$"
    "\\\\.tmp$"
)

# 启用 CPack
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

