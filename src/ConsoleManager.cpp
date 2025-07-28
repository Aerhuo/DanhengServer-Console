#include <fstream>
#include <sstream>
#include "ConsoleManager.hpp"
#include "ConsoleOutputManager.hpp"
#include "SessionManager.hpp"


json ConsoleManager::config;
std::string ConsoleManager::playerUid = "10001";

bool ConsoleManager::loadConfig(const std::string& filename)
{
    std::ifstream configFile(filename);
    if (!configFile.is_open())
    {
        buffer("无法打开配置文件: " + filename, MessageType::Error);
        return false;
    }

    try
    {
        configFile >> config;
        buffer("成功读取配置文件: " + filename, MessageType::Success);
        return true;
    }
    catch (const std::exception& e)
    {
        buffer("配置文件解析失败: " + std::string(e.what()), MessageType::Error);
        return false;
    }
}

json ConsoleManager::GetServerStatus()
{
    return SessionManager::GetServerStatus(config["dispatchUrl"], config["adminKey"]);
}

json ConsoleManager::GetPlayerMessageInfo(const std::string& playerUid)
{
    return SessionManager::GetPlayerInfo(config["dispatchUrl"], config["adminKey"], playerUid);
}

json ConsoleManager::SubmitCommand(const std::string& commandText, const std::string& playerUid)
{
    return SessionManager::SubmitCommand(config["dispatchUrl"], config["adminKey"], commandText, playerUid);
}

json ConsoleManager::CommandGive(const std::string& itemId, int count, const std::string& playerUid)
{
    std::string commandText = "give " + itemId + " x" + std::to_string(count);
    return SubmitCommand(commandText, playerUid);
}

json ConsoleManager::CommandRelic(
    const Relic& relic,
    int           count,
    const std::string& playerUid)
{
    // 拼接子词条，若为空则跳过
    const auto& sub = relic.getSubTag();
    std::string subPart = sub.empty() ? "" : (" " + sub);

    // 构造命令文本
    std::ostringstream oss;
    oss  << "relic "
         << relic.getId()
         << " " << relic.getMainTag()
         << subPart
         << " " << relic.getLevel()
         << " x" << count;
    std::string commandText = oss.str();

    // 提交命令
    return SubmitCommand(commandText, playerUid);
}