#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <nlohmann/json.hpp>
#include "SessionManager.hpp"
#include "AutoSaver.hpp"
#include "ConsoleOutputManager.hpp"
#include "ConsoleInputManager.hpp"
#include "CommandManager.hpp"
#include <conio.h>

using json = nlohmann::json;
using namespace std;

constexpr MessageType Error = MessageType::Error;
constexpr MessageType Info = MessageType::Info;
constexpr MessageType Success = MessageType::Success;
constexpr MessageType Warn = MessageType::Warning;
constexpr MessageType Command = MessageType::Command;
constexpr MessageType Newline = MessageType::Newline;

static void GetItemMenu()
{
}

int main()
{
    buffer("欢迎使用DanhengServer-Console项目", Info);
    buffer("本项目仅供学习交流使用，请勿用于商业用途", Info);

    ConsoleOutputManager::start(); // 启动输出线程
    ConsoleInputManager::start();  // 启动输入线程

    if (!CommandManager::loadConfig())
    {
        buffer("按回车键退出……", Newline);
        ConsoleInputManager::read();
        return 0;
    }
    json &config = CommandManager::GetConfig();

     // 开启自动保存
    try
    {
        if (config["autosaverEnable"])
        {
            AutoSaver::start(config["database_file"],
                             config["autosave_file"],
                             config["intervalSeconds"],
                             config["slotCount"]);
        buffer("当前文件将会保存在路径: " + config["autosave_file"].get<std::string>(), MessageType::Info);
        }
    }
    catch (const std::exception &e)
    {
        buffer("自动保存开启失败: " + std::string(e.what()), MessageType::Error);
    }

    while (true)
    {
        buffer("a.获取物品 b.获取服务器状态 c.获取玩家信息 d.退出程序", Command);
        string input = ConsoleInputManager::read();
        switch (input[0])
        {
        case 'a':
        case 'A':
        {
            GetItemMenu();
            break;
        }
        case 'b':
        case 'B':
        {
            buffer("功能紧急编写中ing……", Warn);
            break;
        }
        case 'c':
        case 'C':
        {
            buffer("功能紧急编写中ing……", Warn);
            break;
        }
        case 'd':
        case 'D':
        {
            return 0;
            break;
        }
        default:
        {
            buffer("请输入正确的指令", Warn);
            break;
        }
        }
    }

    return 0;
}