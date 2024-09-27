#pragma once

#include <functional>
#include <memory>

class MainLoopPrivate;
class MainLoop
{
    static MainLoop &instance() noexcept;

    explicit MainLoop() noexcept;

public:
    MainLoop(const MainLoop &) = delete;
    MainLoop &operator=(const MainLoop &) = delete;
    MainLoop(MainLoop &&) = delete;
    MainLoop &operator=(MainLoop &&) = delete;
    ~MainLoop() noexcept;

    static void start() noexcept;
    static bool isRunning() noexcept;
    static int wait() noexcept;
    static void quit(int code = 0) noexcept;
    static void postEvent(std::function<void()> &&event) noexcept;
    static void addOnQuit(std::function<void()> &&callback) noexcept;

private:
    std::unique_ptr<MainLoopPrivate> d_ptr;
};
