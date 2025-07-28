#include "ConsoleInputManager.hpp"
#include "ConsoleOutputManager.hpp"  // 提供 getTyping()
#include <conio.h>                   // _kbhit(), _getch()
#include <iostream>
#include <chrono>
#include <thread>

// 静态成员变量在 .cpp 中初始化
std::string ConsoleInputManager::typingShadow;
std::mutex   ConsoleInputManager::shadowMutex;

std::string ConsoleInputManager::read()
{
    // 本次调用要返回的完整一行输入
    std::string line;

    // 每次 read 之前，清空旧的 shadow
    {
        std::lock_guard<std::mutex> lock(shadowMutex);
        typingShadow.clear();
    }

    // 启动输入循环线程，线程跑到敲回车为止
    std::thread t(&ConsoleInputManager::inputLoop, std::ref(line));

    // 阻塞等待输入线程结束
    t.join();

    // 返回本次用户敲入的一行文本
    return line;
}

void ConsoleInputManager::inputLoop(std::string& outLine)
{
    std::string buffer;                  // 存放本次正在输入的内容
    bool lastTyping = ConsoleOutputManager::getTyping();

    while (true)
    {
        bool currentTyping = ConsoleOutputManager::getTyping();

        // 输出线程从打印状态切换到非打印状态
        if (lastTyping && !currentTyping)
        {
            std::lock_guard<std::mutex> lock(shadowMutex);
            // 恢复行首提示符及打印中敲的内容
            std::cout << "\n> " << typingShadow;
            buffer = typingShadow;       // 将 shadow 拷贝到本地 buffer
            typingShadow.clear();
        }
        lastTyping = currentTyping;

        // 非阻塞检测键盘
        if (_kbhit())
        {
            char ch = _getch();

            if (currentTyping)
            {
                // 输出线程正在打印，所有字符先存到 shadow
                if (ch != '\r')
                {
                    std::lock_guard<std::mutex> lock(shadowMutex);
                    typingShadow.push_back(ch);
                }
            }
            else
            {
                // 普通输入模式
                if (ch == '\r')
                {
                    // 回车：结束本次输入
                    std::cout << std::endl;
                    outLine = buffer;
                    return;
                }
                else if (ch == '\b')
                {
                    // 退格：删除 buffer 最后一个字符并回退光标
                    if (!buffer.empty())
                    {
                        buffer.pop_back();
                        std::cout << "\b \b";
                    }
                }
                else
                {
                    // 普通字符：追加并回显
                    buffer.push_back(ch);
                    std::cout << ch;
                }
            }
        }

        // 降低 CPU 占用率
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}