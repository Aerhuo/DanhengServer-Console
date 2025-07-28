#include "ConsoleOutputManager.hpp"
#include "ConsoleInputManager.hpp"
#include <thread>
#include <chrono>
#include <windows.h>
#include <conio.h>

// 延迟打印每个字符的时间（毫秒）
int ConsoleOutputManager::delayMs = 2;

// 输出队列和同步机制
std::queue<ConsoleMessage> ConsoleOutputManager::queue;
std::mutex ConsoleOutputManager::queueMutex;
std::condition_variable ConsoleOutputManager::queueNotifier;
std::recursive_mutex ConsoleOutputManager::outputMutex;
std::atomic<bool> ConsoleOutputManager::isTyping{false};

static bool outputStarted = false;

// 启动后台输出线程（仅启动一次）
void ConsoleOutputManager::start()
{
    static std::mutex startMutex;
    std::lock_guard<std::mutex> lock(startMutex);

    if (!outputStarted)
    {
        std::thread(outputLoop).detach();
        outputStarted = true;
    }
}

// 添加消息到输出队列，只使用 text 和 type 两个参数
void ConsoleOutputManager::buffer(const std::string &text, MessageType type)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.emplace(text, type);
    }
    queueNotifier.notify_one();
}

// 后台线程循环消费消息队列
void ConsoleOutputManager::outputLoop()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueNotifier.wait(lock, []
                           { return !queue.empty(); });

        ConsoleMessage msg = queue.front();
        queue.pop();
        lock.unlock();

        flushSingle(msg);
    }
}

// 返回消息类型对应的颜色代码
std::string ConsoleOutputManager::getColorCode(MessageType type)
{
    switch (type)
    {
    case MessageType::Success:
        return "\x1B[32m"; // 绿色
    case MessageType::Error:
        return "\x1B[31m"; // 红色
    case MessageType::Warning:
        return "\x1B[33m"; // 黄色
    case MessageType::Info:
        return "\x1B[36m"; // 蓝色
    case MessageType::Command:
        return "\x1B[37m"; // 白色
    default:
        return "";
    }
}

// 逐字打出文本内容，带有字符延迟效果
void ConsoleOutputManager::typeWrite(const std::string &text, std::ostream &out)
{
    int counter = 0;
    for (char c : text)
    {
        while (_kbhit())
            _getch(); // 清除键盘输入缓存
        out << c << std::flush;

        if (++counter % 3 == 0 && c != '\n' && c != '\r')
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs * 3));
    }
}

// 实际输出函数，根据消息类型控制行为
void ConsoleOutputManager::flushSingle(const ConsoleMessage &msg)
{
    std::lock_guard<std::recursive_mutex> lock(outputMutex);
    isTyping.exchange(true);

    std::ostream &out = (msg.type == MessageType::Error) ? std::cerr : std::cout;

    out << std::endl;

    // 获取对应颜色代码
    std::string color = getColorCode(msg.type);
    out << color;

    // 输出前缀
    switch (msg.type)
    {
    case MessageType::Success:
        out << "[SUCCESS] ";
        break;
    case MessageType::Error:
        out << "[ERROR] ";
        break;
    case MessageType::Warning:
        out << "[WARN] ";
        break;
    case MessageType::Info:
        out << "[INFO] ";
        break;
    case MessageType::Command:
        out << "[Command] ";
    default:
        break;
    }

    // 动态打印处理（按类型自动判断）
    bool animated = (msg.type == MessageType::Success ||
                     msg.type == MessageType::Error ||
                     msg.type == MessageType::Info ||
                     msg.type == MessageType::Command ||
                    msg.type == MessageType::Warning);

    if (animated)
        typeWrite(msg.content, out);
    else
        out << msg.content;

    // 重置颜色（仅在需要时）
    if (!color.empty() && msg.type != MessageType::Newline)
        out << "\x1B[0m";
        
    isTyping.exchange(false);
}