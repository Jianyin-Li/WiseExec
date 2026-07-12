# 项目结构说明

## 目录结构

```
QuickStart/
├── .github/                    # GitHub Actions 工作流配置
│   └── workflows/
│       ├── ci.yml             # CI/CD 配置文件
│       └── README.md          # 工作流说明文档
├── .vscode/                   # VS Code 开发环境配置
│   ├── settings.json          # 编辑器设置
│   └── version-language-configuration.json
├── build/                     # 构建目录（已清理）
├── include/                   # 公共头文件
│   ├── appconfigdialog.h
│   ├── appitem.h
│   ├── config.h
│   ├── funcconfigdialog.h
│   ├── funcitem.h
│   ├── icongenerator.h
│   └── mainwindow.h
├── resources/                 # 资源文件
│   ├── app_icon.ico
│   ├── app_icon.rc
│   └── favicon.ico
├── src/                       # 源代码文件
│   ├── appconfigdialog.cpp
│   ├── appitem.cpp
│   ├── funcconfigdialog.cpp
│   ├── funcitem.cpp
│   ├── icongenerator.cpp
│   ├── main.cpp
│   └── mainwindow.cpp
├── tools/                     # 工具脚本
│   └── test-tools/
│       ├── README.md
│       └── test_icon_generator.cpp
├── ui/                        # Qt UI 设计文件
│   ├── appconfigdialog.ui
│   ├── funcconfigdialog.ui
│   └── mainwindow.ui
├── .clang-format              # 代码格式化配置
├── .clang-tidy                # 代码静态分析配置
├── .gitattributes             # Git 属性配置
├── .gitignore                 # Git 忽略文件配置
├── CMakeLists.txt             # CMake 构建配置
├── CMakePresets.json          # CMake 预设配置
├── config.json                # 应用程序配置文件
├── LICENSE                    # 许可证文件
├── QuickStart_zh_CN.ts        # 中文翻译文件
├── resources.qrc              # Qt 资源文件
└── VERSION                    # 版本文件
```

## 文件说明

### 核心配置文件
- **CMakeLists.txt**: 主要的 CMake 构建配置文件
- **CMakePresets.json**: CMake 预设配置，支持不同构建环境
- **config.json**: 应用程序运行时配置文件

### 开发工具配置
- **.clang-format**: Clang 代码格式化规则
- **.clang-tidy**: Clang 静态代码分析规则
- **.vscode/settings.json**: VS Code 编辑器设置

### 源代码组织
- **include/**: 公共头文件目录，包含所有类的声明
- **src/**: 源代码文件目录，包含所有类的实现
- **ui/**: Qt Designer 生成的 UI 文件
- **resources/**: 应用程序资源文件（图标等）

### 构建和部署
- **.github/workflows/ci.yml**: GitHub Actions CI/CD 流水线
- **resources.qrc**: Qt 资源系统配置文件
- **VERSION**: 项目版本信息文件

## 构建说明

1. **配置构建环境**:
   ```bash
   cmake -B build -S .
   ```

2. **编译项目**:
   ```bash
   cmake --build build --config Release
   ```

3. **运行应用程序**:
   ```bash
   ./build/bin/WiseExec
   ```

## 清理说明

项目已清理以下内容：
- 删除了 `build/` 目录（构建生成的文件）
- 检查并清理了临时文件
- 保持了源代码和配置文件的完整性

## 注意事项

1. 构建生成的文件会自动放入 `build/` 目录
2. 不要将 `build/` 目录提交到版本控制系统
3. 使用 `git status` 检查文件状态
4. 所有源代码文件已按功能模块组织