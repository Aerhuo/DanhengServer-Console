#include <fstream>
#include "CommandManager.hpp"
#include "ConsoleOutputManager.hpp"
#include "SessionManager.hpp"


json CommandManager::config;

bool CommandManager::loadConfig(const std::string& filename)
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

json& CommandManager::GetConfig(){
    return config;
}

json CommandManager::GetServerStatus()
{
    return SessionManager::GetServerStatus(config["dispatchUrl"], config["adminKey"]);
}

json CommandManager::GetPlayerMessageInfo(const std::string& playerUid)
{
    return SessionManager::GetPlayerInfo(config["dispatchUrl"], config["adminKey"], playerUid);
}

json CommandManager::SubmitCommand(const std::string& commandText, const std::string& playerUid)
{
    return SessionManager::SubmitCommand(config["dispatchUrl"], config["adminKey"], commandText, playerUid);
}

std::string CommandManager::CommandGive(const std::string& itemId, int count, const std::string& playerUid)
{
    std::string commandText = "give " + itemId + " x" + std::to_string(count);
    SubmitCommand(commandText, playerUid);
    return commandText;
}

std::string CommandManager::CommandRelic(const std::string& relicId,
                                         const std::string& tagMain,
                                         const std::string& tagSub,
                                         const std::string& level,
                                         int count,
                                         const std::string& playerUid)
{
    std::string commandText = "relic " + relicId + " " + tagMain + " " + tagSub + " " + level + " x" + std::to_string(count);
    SubmitCommand(commandText, playerUid);
    return commandText;
}
