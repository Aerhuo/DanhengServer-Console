# DanhengServer-Console 项目

一个基于 C++ 构建的DanhengServer控制台，更多功能正在编写中。

## 📖 项目简介

该控制台程序作为 DanhengServer 的交互入口，当前已实现基本的会话管理模块、控制台输出美化、服务器数据自动保存功能，并正在逐步集成一键指令等更多功能。

### 📜 功能实现

- **HTTP请求**: 向DanhengServer发送命令、接收DanhengServer的各种信息。(完成)
- **美化终端**: 采用打印机效果和彩色文本使终端变得更加美观且易读。(完成)
- **一键化功能实现**: 实现一键处理服务端信息和发送指令。（制作中）

## 🛠️技术栈与依赖

| 技术组件       | 用途                                 |
|----------------|-------------------------------------|
| C++17          | 核心开发语言                         |
| OpenSSL        | 提供加密与安全通信支持                |
| libcurl        | 用于发送 HTTP 请求与接口集成          |
| nlohmann/json  | 用于处理 JSON 配置、数据序列化与解析  |

## 🚀 快速构建

### 推荐环境

- [vcpkg](https://github.com/microsoft/vcpkg)
- [CMake](https://cmake.org/download/) ≥3.21
- Windows 10及以上

### Step 1: 克隆仓库

```bash
git clone https://github.com/Aerhuo/DanhengServer-Console.git
cd DanhengServer-Console
```

### Step 2: 安装依赖项

```bash
vcpkg install
```

如果你没有安装vcpkg，直接进行第三步，将会自动安装vcpkg并安装依赖项。

### Step 3: 构建项目

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release 或 cmake --build .
```

可执行文件将会在 ./bin 中生成
