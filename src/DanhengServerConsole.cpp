//=============================================================================
// DanhengServer-Console 主入口及菜单模块
//=============================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <map>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "SessionManager.hpp"
#include "AutoSaver.hpp"
#include "ConsoleOutputManager.hpp"
#include "ConsoleInputManager.hpp"
#include "ConsoleManager.hpp"

using json = nlohmann::json;
using namespace std;

// 方便引用不同类型的消息常量
constexpr MessageType Error   = MessageType::Error;
constexpr MessageType Info    = MessageType::Info;
constexpr MessageType Success = MessageType::Success;
constexpr MessageType Warn    = MessageType::Warning;
constexpr MessageType Command = MessageType::Command;
constexpr MessageType Newline = MessageType::Newline;

// 全局配置与玩家 UID 引用
json&   config    = ConsoleManager::config;
string& playerUid = ConsoleManager::playerUid;

////////////////////////////////////////////////////////////////////////////////
//                            自动保存启动
////////////////////////////////////////////////////////////////////////////////

/// 如果配置开启，则启动自动保存
static void StartAutoSave()
{
    if (!config.value("autosave", false))
        return;

    try
    {
        AutoSaver::start(
            config.at("database_file").get<string>(),
            config.at("autosave_file").get<string>(),
            config.at("intervalSeconds").get<int>(),
            config.at("slotCount").get<int>()
        );
        buffer("自动保存已启动，保存路径: " +
               config.at("autosave_file").get<string>(),
               Info);
    }
    catch (const exception& ex)
    {
        buffer("自动保存开启失败: " + string(ex.what()), Error);
    }
}

////////////////////////////////////////////////////////////////////////////////
//                          通用辅助函数
////////////////////////////////////////////////////////////////////////////////

/// 反复提示用户输入一个合法的大写选项
static char askChoice(const string& prompt, const vector<char>& allowed)
{
    if(!prompt.empty())
        buffer(prompt, Command);

    while (true)
    {
        string line = read();
        if (line.empty())
        {
            buffer("请输入有效选项", Warn);
            continue;
        }
        char c = toupper(line[0]);
        if (find(allowed.begin(), allowed.end(), c) != allowed.end())
            return c;

        buffer("无效选项，请重新输入", Warn);
    }
}

/// 尝试读取一个整数，否则返回 defaultValue 并提示
static int readIntOrDefault(int defaultValue = 1)
{
    try
    {
        return stoi(read());
    }
    catch (...)
    {
        buffer("格式错误，使用默认值 " + to_string(defaultValue), Warn);
        return defaultValue;
    }
}

inline void RandomRelics(Relic::Type type,
                         int starRank,
                         const std::string& relicBaseId,
                         int partId,
                         int count,
                         const std::string& subTag = "",
                         const std::string& level  = "l0")
{
    // 根据部位确定主词条范围
    int minTag = 1;
    int maxTag = 1;
        switch (partId) {
            case 1: case 2: maxTag = 1; break;    // 头/手
            case 3:         maxTag = 7; break;    // 躯
            case 4:         maxTag = 4; break;    // 脚
            case 5:         maxTag = 10; break;   // 位面球
            case 6:         maxTag = 5;  break;   // 连接绳
            default: throw std::invalid_argument("Invalid partId");
        }

    static thread_local std::mt19937 gen{ std::random_device{}() };
    std::uniform_int_distribution<> dist(minTag, maxTag);

    for (int i = 0; i < count; ++i) {
        std::string mainTag = std::to_string(dist(gen));
        auto relic = Relic::Random(
            type,
            starRank,
            relicBaseId,
            partId,
            /*minTag=*/minTag,
            /*maxTag=*/maxTag,
            subTag,
            level
        );
            try
            {
                json resp = ConsoleManager::CommandRelic(relic, 1, ConsoleManager::playerUid);

                if (!resp.contains("data") ||
                    !resp["data"].contains("message"))
                    throw runtime_error("响应数据格式错误，返回信息：" + resp["message"].get<string>());

                auto mt = resp["message"].get<string>() == "Success"
                            ? Success
                            : Error;
                buffer(SessionManager::base64Decode(resp["data"]["message"].get<string>()), mt);
            }
            catch (const exception& ex)
            {
                buffer("命令执行异常: " + string(ex.what()), Error);
            }
    }
}

////////////////////////////////////////////////////////////////////////////////
//                          自定义物品与遗器
////////////////////////////////////////////////////////////////////////////////

/// A1. 读取并提交“普通物品”指令
static void handleCustomItem()
{
    buffer("请依次输入: 物品ID 数量", Command);
    string itemId = read();
    int    count  = readIntOrDefault(1);

    ConsoleManager::CommandGive(itemId, count, playerUid);
}

/// A2. 读取并提交“自定义遗器”指令
static void handleCustomRelic()
{
    buffer(
      "请输入：遗器类型(T隧洞 / P位面)  星级  relicId(两位)  部位ID  主词条  副词条  强化等级  数量",
      Command
    );

    // 遗器类型
    char tc = toupper(read()[0]);
    Relic::Type type = (tc == 'P') ? Relic::Type::Plane : Relic::Type::Tunnel;

    // 星级 (实际品级)
    int    starRank = readIntOrDefault(1);

    // 遗器 ID & 部位
    string relicId = read();          
    int    partId  = readIntOrDefault(1);

    // 主词条 / 副词条 / 强化等级
    string mainTag = read();
    string subTag  = read();
    string level   = read();

    // 数量
    int count = readIntOrDefault(1);

    // 构造并提交遗器指令
    Relic relic(type, starRank, relicId, partId, mainTag, subTag, level);
    ConsoleManager::CommandRelic(relic, count, playerUid);
}

/// A. 先选“普通物品”还是“自定义遗器”
static void handleCustomItemOrRelic()
{
    char kind = askChoice(
      "请选择自定义类型：A.普通物品  B.自定义遗器",
      {'A','B'}
    );

    if (kind == 'A')
        handleCustomItem();
    else
        handleCustomRelic();
}

////////////////////////////////////////////////////////////////////////////////
//                          随机遗器/饰品礼包生成
////////////////////////////////////////////////////////////////////////////////

static void handleRandomBundle(
    const string& title,
    Relic::Type   type,
    const map<char, vector<pair<int,int>>>& presets,
    int            partCount)
{
    // 1) 选子项
    char sub = askChoice(title, {'A','B','C'});
    buffer("请输入遗器ID", Command);
    string rid = read();

    auto it = presets.find(sub);
    if (it == presets.end())
    {
        buffer("无效子选项", Warn);
        return;
    }

    // 2) 按预设生成每个部位
    for (auto& rc : it->second)
    {
        int rarity = rc.first;
        int cnt    = rc.second;

        // 对于位面饰品，部位从 5 开始；隧洞遗器仍从 1 开始
        int startPart = (type == Relic::Type::Plane) ? 5 : 1;
        // 间隔连续，结束部位 = 起始部位 + 部位数量 - 1
        int endPart   = startPart + partCount - 1;

        for (int part = startPart; part <= endPart; ++part)
        {
            RandomRelics(type, rarity, rid, part, cnt);
        }
    }

}

////////////////////////////////////////////////////////////////////////////////
//                          物品菜单入口
////////////////////////////////////////////////////////////////////////////////

static void GetItemMenu()
{
    char choice = askChoice(
        "A.自定义  B.随机位面饰品  C.随机隧洞遗器",
        {'A','B','C'}
    );

    try
    {
        switch (choice)
        {
            case 'A':
                handleCustomItemOrRelic();
                break;

            case 'B': // 随机位面饰品
            {
                static const map<char, vector<pair<int,int>>> planePresets = {
                    {'A', {{2,3},{3,2},{4,1}}},
                    {'B', {{3,3},{4,2},{5,1}}},
                    {'C', {{4,4},{5,2}}}
                };
                handleRandomBundle(
                    "A.二、三、四星  B.三、四、五星  C.四、五星",
                    Relic::Type::Plane,
                    planePresets,
                    /*部位数=*/2
                );
                break;
            }

            case 'C': // 随机隧洞遗器
            {
                static const map<char, vector<pair<int,int>>> tunnelPresets = {
                    {'A', {{2,3},{3,2},{4,1}}},
                    {'B', {{3,3},{4,2},{5,1}}},
                    {'C', {{4,4},{5,2}}}
                };
                handleRandomBundle(
                    "A.二、三、四星  B.三、四、五星  C.四、五星",
                    Relic::Type::Tunnel,
                    tunnelPresets,
                    /*部位数=*/4
                );
                break;
            }
        }
    }
    catch (const exception& ex)
    {
        buffer("操作失败: " + string(ex.what()), Error);
    }
}

////////////////////////////////////////////////////////////////////////////////
//                               主函数
////////////////////////////////////////////////////////////////////////////////

int main()
{
    // 设为 UTF-8 控制台
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    buffer("欢迎使用 DanhengServer-Console！", Info);
    buffer("仅供学习交流，请勿用于商业用途", Info);

    // 加载配置
    if (!ConsoleManager::loadConfig())
    {
        buffer("配置加载失败，按任意键退出……", Newline);
        ConsoleInputManager::read();
        return 1;
    }

    // 启动 IO 管理
    ConsoleOutputManager::start();

    // 自动保存
    StartAutoSave();

    // 主循环
    while (true)
    {
        buffer("当前玩家UID: " + playerUid, Info);
        buffer("A.获取物品  B.自定义指令  C.设置UID  D.退出", Command);

        char c = askChoice("", {'A','B','C','D'});
        switch (c)
        {
            case 'A':
                GetItemMenu();
                break;

            case 'B': // 自定义命令
            {
                buffer("请输入要执行的命令: ", Command);
                string cmd = read();
                try
                {
                    json resp = ConsoleManager::SubmitCommand(cmd, playerUid);

                    if (!resp.contains("data") ||
                        !resp["data"].contains("message"))
                        throw runtime_error("响应数据格式错误，返回信息：" + resp["message"].get<string>());

                    auto mt = resp["message"].get<string>() == "Success"
                                ? Success
                                : Error;
                    buffer(SessionManager::base64Decode(resp["data"]["message"].get<string>()), mt);
                }
                catch (const exception& ex)
                {
                    buffer("命令执行异常: " + string(ex.what()), Error);
                }
                break;
            }

            case 'C': // 设置玩家 UID
                buffer("请输入新的玩家UID: ", Command);
                playerUid = read();
                buffer("玩家UID已更新为: " + playerUid, Info);
                break;

            default:  // 'D' 退出
                buffer("程序退出中……", Info);
                return 0;
        }
    }

    return 0;
}