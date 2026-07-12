# Qt 应用程序启动器

一个基于 Qt 的应用程序启动器，提供美观的界面和便捷的应用管理功能。

## 功能特性

- 🎨 现代化的 Qt 界面设计
- 📱 应用程序快捷启动
- ⚙️ 可配置的应用设置
- 🎯 图标生成和管理
- 🌐 多语言支持（中文）
- 🏗️ 基于 CMake 的跨平台构建

## 项目结构

项目已整理为清晰的结构：

```
QuickStart/
├── include/     # 头文件
├── src/         # 源代码
├── ui/          # Qt UI 文件
├── resources/   # 资源文件
└── tools/       # 工具脚本
```

详细结构请参阅 [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)。

## 构建要求

- **CMake** 3.16 或更高版本
- **Qt** 5.15 或 Qt 6.x
- **C++17** 兼容的编译器

## 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd QuickStart
```

### 2. 配置构建
```bash
cmake -B build -S .
```

### 3. 编译项目
```bash
cmake --build build --config Release
```

### 4. 运行应用程序
```bash
# Windows
build\bin\WiseExec.exe

# Linux/macOS
./build/bin/WiseExec
```

## 开发

### 代码风格
项目使用 Clang-Format 进行代码格式化：
```bash
# 格式化所有代码
clang-format -i include/*.h src/*.cpp
```

### 代码质量
使用 Clang-Tidy 进行静态代码分析：
```bash
# 运行静态分析
clang-tidy include/*.h src/*.cpp -- -Iinclude
```

## 配置

应用程序配置存储在 `config.json` 文件中：
```json
{
    "name": "主界面",
    "iconPath": "",
    "subApps": [],
    "funcs": []
}
```

## 许可证

本项目基于 [LICENSE](LICENSE) 文件中的许可证条款发布。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进本项目。

## 支持

如有问题，请查看：
- [项目结构文档](PROJECT_STRUCTURE.md)
- GitHub Issues
- 项目 Wiki（如有）