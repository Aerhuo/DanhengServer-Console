#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CommandManager
{
public:
    // 加载配置文件
    static bool loadConfig(const std::string& filename = "config.json");

    //获取配置文件
    static json& GetConfig();

    // 获取服务器状态
    static json GetServerStatus();

    // 获取玩家消息（如日志类型、状态等）
    static json GetPlayerMessageInfo(const std::string& playerUid);

    // 发送通用指令
    static json SubmitCommand(const std::string& commandText, const std::string& playerUid);

    // 构造并发送物品指令
    static std::string CommandGive(const std::string& itemId, int count, const std::string& playerUid);

    // 构造并发送遗器指令
    static std::string CommandRelic(const std::string& relicId,
                                    const std::string& tagMain,
                                    const std::string& tagSub,
                                    const std::string& level,
                                    int count,
                                    const std::string& playerUid);

private:
    static json config;  // 全局配置数据
};