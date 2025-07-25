#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SessionManager
{
public:
    static json GetServerStatus(const std::string &serverUrl, const std::string &adminKeyPlain)
    {
        json session = createSession(serverUrl);
        authorize(
            serverUrl,
            session["data"]["sessionId"],
            session["data"]["rsaPublicKey"],
            adminKeyPlain);
        return getServerStatus(serverUrl, session["data"]["sessionId"]);
    }
    static json GetPlayerInfo(const std::string &serverUrl, const std::string &adminKeyPlain, const std::string &playerUid)
    {
        json session = createSession(serverUrl);
        authorize(
            serverUrl,
            session["data"]["sessionId"],
            session["data"]["rsaPublicKey"],
            adminKeyPlain);
        return getPlayerInfo(serverUrl, session["data"]["sessionId"], playerUid);
    };
    static json SubmitCommand(const std::string &serverUrl, const std::string &adminKeyPlain, const std::string &commandPlain, const std::string &targetUid)
    {
        json session = createSession(serverUrl);
        authorize(
            serverUrl,
            session["data"]["sessionId"],
            session["data"]["rsaPublicKey"],
            adminKeyPlain);
        return submitCommand(
            serverUrl,
            session["data"]["sessionId"],
            session["data"]["rsaPublicKey"],
            commandPlain,
            targetUid);
    }

private:
    static json createSession(const std::string &serverUrl);
    static json authorize(
        const std::string &serverUrl,
        const std::string &sessionId,
        const std::string &rsaPublicKeyPEM,
        const std::string &adminKeyPlain);
    static json getServerStatus(const std::string &serverUrl, const std::string &sessionId);
    static json getPlayerInfo(const std::string &serverUrl, const std::string &sessionId, const std::string &playerUid);
    static json submitCommand(
        const std::string &serverUrl,
        const std::string &sessionId,
        const std::string &rsaPublicKeyPEM,
        const std::string &commandPlain,
        const std::string &targetUid);
};