#include "Worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

namespace {
constexpr auto sleepTime { 500 };
constexpr auto waitForFinishedTime { 10 };
} // namespace

///> Start Private
struct WorkerPrivate
{
    std::string workerName;
    std::atomic_bool running { false };
    std::atomic_bool finished { false };
    std::thread workerThread;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable waitCondition;

    bool isRunning() const noexcept;
    void stop();
    void run();
    void waitForFinished() noexcept;
    void joinWorkerThread() noexcept;
};

inline bool WorkerPrivate::isRunning() const noexcept
{
    return running.load();
}

void WorkerPrivate::stop()
{
    running.store(false);
    waitCondition.notify_one();
    waitForFinished();
}

void WorkerPrivate::run()
{
    std::stringstream ss;
    ss << workerName << '(' << std::this_thread::get_id() << ')';
    {
        std::unique_lock<std::mutex> lock { mutex };
        workerName = ss.str();
    }

    while (isRunning()) {
        std::function<void()> task { nullptr };
        {
            std::unique_lock<std::mutex> lock { mutex };
            if (!tasks.empty()) {
                task = tasks.front();
                tasks.pop();
            } else {
                waitCondition.wait(lock, [this] { return !isRunning() || !tasks.empty(); });
                continue;
            }
        }

        if (task != nullptr) {
            task();
        }
    }

    finished.store(true);
    waitCondition.notify_all();
}

void WorkerPrivate::waitForFinished() noexcept
{
    std::unique_lock<std::mutex> lock { mutex };
    waitCondition.wait(lock, [this] { return finished.load(); });
}

void WorkerPrivate::joinWorkerThread() noexcept
{
    if (workerThread.joinable()) {
        workerThread.join();
    }
}
///< End Private

Worker::Worker() noexcept : d_ptr(std::make_unique<WorkerPrivate>()) { }

Worker::~Worker() noexcept
{
    std::stringstream ss;
    ss << __PRETTY_FUNCTION__ << ' ' << d_ptr->workerName << '\n';

    d_ptr->stop();
    d_ptr->joinWorkerThread();
    d_ptr.reset(nullptr);
    std::cout << ss.str();
}

void Worker::start() noexcept
{
    if (d_ptr->running.load()) {
        return;
    }

    d_ptr->running.store(true);
    d_ptr->workerThread = std::thread(&WorkerPrivate::run, d_ptr.get());
    d_ptr->workerThread.detach();
}

void Worker::stop() noexcept
{
    d_ptr->stop();
}

void Worker::waitForFinished() noexcept
{
    d_ptr->waitForFinished();
}

void Worker::addTask(const std::function<void()> &task) noexcept
{
    try {
        {
            std::unique_lock<std::mutex> const lock { d_ptr->mutex };
            d_ptr->tasks.push(task);
        }
        d_ptr->waitCondition.notify_one();
    } catch (const std::exception & /*e*/) {
    }
}

void Worker::addTask(std::function<void()> &&task) noexcept
{
    try {
        {
            std::unique_lock<std::mutex> const lock { d_ptr->mutex };
            d_ptr->tasks.push(std::move(task));
        }

        d_ptr->waitCondition.notify_one();
    } catch (const std::exception & /*e*/) {
    }
}

size_t Worker::tasksCount() const noexcept
{
    std::scoped_lock lock(d_ptr->mutex);
    return d_ptr->tasks.size();
}

std::string Worker::workerName() const noexcept
{
    return d_ptr->workerName;
}

void Worker::setWorkerName(const std::string &name) noexcept
{
    std::scoped_lock lock(d_ptr->mutex);
    d_ptr->workerName = name;
}
