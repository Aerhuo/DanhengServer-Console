#pragma once

#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Relic.hpp

#pragma once

#include <string>
#include <random>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Relic {
public:
    // 隧洞遗器(1) 或 位面饰品(3)
    enum class Type : int {
        Tunnel = 1,
        Plane  = 3
    };

    /**
     * 完整构造遗器
     * @param type_       遗器类型
     * @param starRank_   星级 (实际品级)，品质代码 = starRank_ + 1
     * @param relicId_    两位遗器ID ("01","02"...)
     * @param partId_     部位ID (隧洞:1–4，位面:5–6)
     * @param mainTag_    主词条ID (字符串)
     * @param subTag_     副词条ID (可选)
     * @param level_      强化等级 (默认 "l0")
     */
    Relic(Type               type_,
          int                starRank_,
          const std::string& relicId_,
          int                partId_,
          const std::string& mainTag_,
          const std::string& subTag_ = "",
          const std::string& level_  = "l0")
      : type(type_)
      , starRank(starRank_)
      , relicId(relicId_)
      , partId(partId_)
      , mainTag(mainTag_)
      , subTag(subTag_)
      , level(level_)
    {
        buildFullId();
    }

    //—— 访问器 ——//
    Type        getType()     const { return type; }
    int         getStarRank() const { return starRank; }
    const std::string& getRelicId() const { return relicId; }
    int         getPartId()   const { return partId; }
    const std::string& getMainTag() const { return mainTag; }
    const std::string& getSubTag()  const { return subTag; }
    const std::string& getLevel()   const { return level; }
    const std::string& getId()      const { return id; }

    /**
     * 随机主词条工厂
     * @param minTag 下界
     * @param maxTag 上界
     */
    static Relic Random(Type               type,
                        int                starRank,
                        const std::string& relicId,
                        int                partId,
                        int                minTag,
                        int                maxTag,
                        const std::string& subTag = "",
                        const std::string& level  = "l0")
    {
        static thread_local std::mt19937 gen{ std::random_device{}() };
        std::uniform_int_distribution<> dist(minTag, maxTag);
        std::string mainTag = std::to_string(dist(gen));
        return Relic(type, starRank, relicId, partId, mainTag, subTag, level);
    }

    //—— JSON 序列化 ——//
    friend void to_json(json& j, const Relic& r) {
        j = json{
            {"item_id",   r.id},
            {"main_tag",  r.mainTag},
            {"sub_tag",   r.subTag},
            {"level",     r.level}
        };
    }

private:
    Type        type;
    int         starRank;
    std::string relicId;
    int         partId;
    std::string mainTag;
    std::string subTag;
    std::string level;
    std::string id;          // 完整物品ID

    // 构造 id 时类型代码即枚举值
    static constexpr int typeCode(Type t) {
        return static_cast<int>(t);
    }

    // 拼接完整 ID：品质代码 + 类型代码 + relicId + 部位ID
    void buildFullId() {
        int qualityCode = starRank + 1;
        std::ostringstream oss;
        oss << qualityCode
            << typeCode(type)
            << relicId
            << partId;
        id = oss.str();
    }
};

class ConsoleManager {
public:
    static json        config;    // 全局配置
    static std::string playerUid; // 当前玩家 UID

    // 加载配置文件，成功返回 true
    static bool loadConfig(const std::string& filename = "config.json");

    // 获取服务器状态信息
    static json GetServerStatus();

    // 查询玩家消息 (日志、状态等)
    static json GetPlayerMessageInfo(const std::string& uid);

    // 发送任意命令，返回服务器响应
    static json SubmitCommand(const std::string& commandText, const std::string& uid);

    // 构造并返回“给予物品”命令执行状态
    static json CommandGive(const std::string& itemId, int count, const std::string& uid);

    // 构造并返回“给予遗器”命令执行状态
    static json CommandRelic(const Relic& relic, int count, const std::string& uid);
};