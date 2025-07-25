#include "AutoSaver.hpp"
#include "ConsoleOutputManager.hpp"
#include <fstream>
#include <chrono>
#include <ctime>

std::atomic<bool> AutoSaver::isRunning{false};
std::thread AutoSaver::workerThread;
std::mutex AutoSaver::fileMutex;

fs::path AutoSaver::source;
fs::path AutoSaver::saveRoot;
int AutoSaver::interval = 0;
int AutoSaver::slotIndex = 0;
int AutoSaver::slotCount = 0;
bool AutoSaver::saveImmediately = true;

AutoSaver::Finalizer AutoSaver::finalizer;

void AutoSaver::start(const fs::path &sourceFile,
                      const fs::path &saveRootDir,
                      int intervalSeconds,
                      int slotCount_,
                      bool saveImmediately_)
{
    if (isRunning.exchange(true))
        return;

    source = sourceFile;
    saveRoot = saveRootDir;
    interval = intervalSeconds;
    slotCount = slotCount_;
    saveImmediately = saveImmediately_;
    slotIndex = 0;

    workerThread = std::thread([]()
                               { run(); });
}

void AutoSaver::triggerSaveNow()
{
    std::lock_guard<std::mutex> lock(fileMutex);
    performSave();
}

void AutoSaver::run()
{
    if (saveImmediately)
    {
        std::lock_guard<std::mutex> lock(fileMutex);
        performSave(); // 启动立即保存
    }

    auto nextTick = std::chrono::steady_clock::now() + std::chrono::seconds(interval);
    while (isRunning.load())
    {
        std::this_thread::sleep_until(nextTick);
        nextTick += std::chrono::seconds(interval);

        std::lock_guard<std::mutex> lock(fileMutex);
        performSave();
    }
}

void AutoSaver::performSave()
{
    fs::path slotDir = saveRoot / ("slot_" + std::to_string(slotIndex));
    fs::create_directories(slotDir);
    fs::path targetFile = slotDir / source.filename();

    try
    {
        if (fs::exists(source))
        {
            fs::copy_file(source, targetFile, fs::copy_options::overwrite_existing);

            time_t now = std::time(nullptr);
            char timeStr[26];
            ctime_s(timeStr, sizeof(timeStr), &now);
            std::string timestamp(timeStr);
            timestamp.pop_back(); // 去掉换行符

            buffer("自动保存完成：#" + std::to_string(slotIndex) + " @" + timestamp,
                   MessageType::Info);
        }
        else
        {
            buffer("自动保存源文件不存在：" + source.string(), MessageType::Warning);
        }
    }
    catch (const std::exception &e)
    {
        buffer("自动保存失败 (槽 #" + std::to_string(slotIndex) + "): " + e.what(),
               MessageType::Error);
    }

    slotIndex = (slotIndex + 1) % slotCount;
}

AutoSaver::Finalizer::~Finalizer()
{
    isRunning = false;
    if (workerThread.joinable())
        workerThread.join();
}