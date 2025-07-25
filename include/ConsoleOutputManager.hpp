#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>

enum class MessageType
{
    Success,
    Error,
    Warning,
    Info,
    Newline,
    Command
};

struct ConsoleMessage
{
    std::string content;
    MessageType type;
    bool animated;
    bool prompt;

    ConsoleMessage(const std::string &content, MessageType type)
        : content(content), type(type) {}
};

class ConsoleOutputManager
{
public:
    static void start();
    static void buffer(const std::string &text, MessageType type);
    static bool getTyping(){
        return isTyping.load();
    }

private:
    static void outputLoop();
    static void flushSingle(const ConsoleMessage &msg);
    static std::string getColorCode(MessageType type);
    static void typeWrite(const std::string &text, std::ostream &out);

    static std::queue<ConsoleMessage> queue;
    static std::mutex queueMutex;
    static std::condition_variable queueNotifier;
    static std::recursive_mutex outputMutex;
    static std::atomic<bool> isTyping;
    static int delayMs;
};

inline void buffer(const std::string &text,
                   MessageType type)
{
    ConsoleOutputManager::buffer(text, type);
}