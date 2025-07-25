#pragma once
#include <filesystem>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace fs = std::filesystem;

// 自动保存器：托管方式使用，只需调用 AutoSaver::start(...)
class AutoSaver
{
public:
    // 启动自动保存线程（仅调用一次）
    static void start(const fs::path& sourceFile,
                      const fs::path& saveRootDir,
                      int intervalSeconds,
                      int slotCount,
                      bool saveImmediately = true);

    // 手动触发保存一次
    static void triggerSaveNow();

private:
    static std::atomic<bool> isRunning;
    static std::thread workerThread;
    static std::mutex fileMutex;

    static fs::path source;
    static fs::path saveRoot;
    static int interval;
    static int slotIndex;
    static int slotCount;
    static bool saveImmediately;

    static void run();           // 后台定时保存线程
    static void performSave();   // 具体保存操作

    // 自动析构清理器：在程序结束时自动停止线程
    class Finalizer {
    public:
        ~Finalizer();
    };
    static Finalizer finalizer;
};