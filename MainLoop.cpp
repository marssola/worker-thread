#include "MainLoop.h"

#include <condition_variable>
#include <future>
#include <iostream>

namespace {
constexpr auto sleepTime { 250 };
} // namespace

///> Start Private
struct MainLoopPrivate
{
    int returnCode { 0 };
    std::atomic_bool running { false };
    std::future<void> future;

    std::queue<std::function<void()>> events;
    std::queue<std::function<void()>> onQuit;

    std::mutex mutex;
    std::mutex mutexOnQuit;
    std::condition_variable waitCondition;

    bool isRunning() const noexcept;
    void run();
};

inline bool MainLoopPrivate::isRunning() const noexcept
{
    return running.load();
}

void MainLoopPrivate::run()
{
    std::cout << "Starting Event Loop\n";

    while (isRunning()) {
        {
            std::unique_lock<std::mutex> lock { mutex };
            if (events.empty()) {
                waitCondition.wait(lock);
                continue;
            }
        }

        auto event = events.front();
        events.pop();
        if (event != nullptr) {
            event();
        }
    }
}
///< End Private

MainLoop::MainLoop() noexcept : d_ptr(std::make_unique<MainLoopPrivate>()) { }

MainLoop::~MainLoop() noexcept = default;

MainLoop &MainLoop::instance() noexcept
{
    static MainLoop app;
    return app;
}

void MainLoop::start() noexcept
{
    auto &d = MainLoop::instance().d_ptr;
    d->running.store(true);
    d->future = std::async(std::launch::async, &MainLoopPrivate::run, d.get());
}

bool MainLoop::isRunning() noexcept
{
    auto &d = MainLoop::instance().d_ptr;
    return d->isRunning();
}

int MainLoop::wait() noexcept
{
    auto &d = MainLoop::instance().d_ptr;
    d->future.wait();

    std::cout << "MainLoop exited!\n";
    return d->returnCode;
}

void MainLoop::quit(int code) noexcept
{
    try {
        auto &d = MainLoop::instance().d_ptr;
        d->returnCode = code;

        bool queueEmpty { false };
        {
            std::unique_lock<std::mutex> const lock { d->mutex };
            queueEmpty = d->onQuit.empty();
        }

        while (!queueEmpty) {
            std::function<void()> callback { nullptr };
            {
                std::unique_lock<std::mutex> const lock { d->mutexOnQuit };
                callback = d->onQuit.front();
                d->onQuit.pop();
            }

            if (callback != nullptr) {
                callback();
            }

            {
                std::unique_lock<std::mutex> const lock { d->mutexOnQuit };
                queueEmpty = d->onQuit.empty();
            }
        }

        d->running.store(false);
        d->waitCondition.notify_one();
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "Error: " << e.what() << '\n';
        std::cout << ss.str();
    }
}

void MainLoop::postEvent(std::function<void()> &&event) noexcept
{
    try {
        auto &d = MainLoop::instance().d_ptr;
        {
            std::unique_lock<std::mutex> const lock { d->mutex };
            d->events.push(std::move(event));
        }
        d->waitCondition.notify_one();
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "Error: " << e.what() << '\n';
    }
}

void MainLoop::addOnQuit(std::function<void()> &&callback) noexcept
{
    try {
        auto &d = MainLoop::instance().d_ptr;
        {
            std::unique_lock<std::mutex> const lock { d->mutexOnQuit };
            d->onQuit.push(std::move(callback));
        }
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "Error: " << e.what() << '\n';
    }
}
