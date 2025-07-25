#pragma once
#include <thread>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ConsoleInputManager
{
public:
    static void start();
    static std::string read();
    static std::string getTypingShadow();  // 获取打字期间输入

private:
    static void inputLoop();
    static std::atomic<bool> isRunning;
    static std::thread inputThread;
    static std::queue<std::string> inputQueue;
    static std::mutex queueMutex;
    static std::condition_variable queueCond;
    static std::string typingShadow;
    static std::mutex shadowMutex;

    class InputManagerFinalizer {
    public:
        ~InputManagerFinalizer();
    };
    static InputManagerFinalizer finalizer;
};
