#include "ConsoleInputManager.hpp"
#include "ConsoleOutputManager.hpp"
#include <conio.h>
#include <iostream>
#include <chrono>

std::atomic<bool> ConsoleInputManager::isRunning{false};
std::thread ConsoleInputManager::inputThread;
std::queue<std::string> ConsoleInputManager::inputQueue;
std::mutex ConsoleInputManager::queueMutex;
std::condition_variable ConsoleInputManager::queueCond;
std::string ConsoleInputManager::typingShadow;
std::mutex ConsoleInputManager::shadowMutex;
ConsoleInputManager::InputManagerFinalizer ConsoleInputManager::finalizer;

void ConsoleInputManager::start()
{
    if (isRunning.exchange(true))
        return;
    inputThread = std::thread([]()
                              { inputLoop(); });
}

std::string ConsoleInputManager::read()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCond.wait(lock, []
                   { return !inputQueue.empty(); });

    std::string input = inputQueue.front();
    inputQueue.pop();
    return input;
}

std::string ConsoleInputManager::getTypingShadow()
{
    std::lock_guard<std::mutex> lock(shadowMutex);
    return typingShadow;
}

void ConsoleInputManager::inputLoop()
{
    std::string buffer;
    bool lastTypingState = ConsoleOutputManager::getTyping();

    while (isRunning.load())
    {
        bool currentTyping = ConsoleOutputManager::getTyping();

        // 检测 typing 状态变化：true → false
        if (lastTypingState && !currentTyping)
        {
            std::lock_guard<std::mutex> lock(shadowMutex);
            std::cout <<"\n> ";
            if (!typingShadow.empty())
            {
                std::cout << typingShadow;
                typingShadow.clear();
            }
        }
        lastTypingState = currentTyping;

        if (_kbhit())
        {
            char ch = _getch();

            if (currentTyping)
            {
                if (ch != '\r')
                {
                    std::lock_guard<std::mutex> lock(shadowMutex);
                    typingShadow += ch;
                }
            }
            else
            {
                if (ch == '\r')
                {
                    std::cout << std::endl;
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        inputQueue.push(buffer);
                    }
                    queueCond.notify_one();
                    buffer.clear();
                }
                else if (ch == '\b')
                {
                    if (!buffer.empty())
                    {
                        buffer.pop_back();
                        std::cout << "\b \b";
                    }
                }
                else
                {
                    buffer += ch;
                    std::cout << ch;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

ConsoleInputManager::InputManagerFinalizer::~InputManagerFinalizer()
{
    isRunning = false;
    if (inputThread.joinable())
        inputThread.join();
}
