#pragma once
#include <string>
#include <thread>
#include <mutex>

class ConsoleInputManager
{
public:
    // 每次调用 read() 时才启动一次输入线程，直到用户敲回车后线程结束
    static std::string read();

private:
    // 实际做输入读取的函数：收集字符直到 '\r'，然后退出
    static void inputLoop(std::string& outLine);

    // 当输出线程正在打印时，临时存放用户敲击的字符
    static std::string typingShadow;
    static std::mutex   shadowMutex;
};

inline std::string read(){
    return ConsoleInputManager::read();
}