#include "Worker.h"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <queue>
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
    while (isRunning()) {
        bool hasTasks { false };
        std::function<void()> task { nullptr };
        {
            std::unique_lock<std::mutex> const lock { mutex };
            if (!tasks.empty()) {
                task = tasks.front();
                tasks.pop();
            }
        }

        if (task != nullptr) {
            std::stringstream ss;
            ss << workerName << " Running task in thread " << std::this_thread::get_id() << '\n';
            std::cout << ss.str();
            task();
        }

        {
            std::unique_lock<std::mutex> const lock { mutex };
            hasTasks = !tasks.empty();
        }
        if (hasTasks) {
            std::stringstream ss;
            ss << workerName << " Has tasks in queue: " << tasks.size() << '\n';
            std::cout << ss.str();
            continue;
        }

        std::unique_lock<std::mutex> lock { mutex };
        waitCondition.wait(lock);
    }

    std::stringstream ss;
    ss << workerName << " stopped in thread " << std::this_thread::get_id() << '\n';

    if (!tasks.empty()) {
        ss << workerName << "\n\tTasks left in queue: " << tasks.size() << '\n';
    }

    std::cout << ss.str();
    finished.store(true);
}

void WorkerPrivate::waitForFinished() noexcept
{
    while (!finished.load()) {
        waitCondition.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(waitForFinishedTime));
    }
}
///< End Private

Worker::Worker() noexcept : d_ptr(std::make_unique<WorkerPrivate>()) { }

Worker::~Worker() noexcept
{
    std::stringstream ss;
    ss << __PRETTY_FUNCTION__ << ' ' << d_ptr->workerName << '\n';

    d_ptr->stop();
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
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << d_ptr->workerName << " Error adding task: " << e.what() << '\n';
        std::cerr << ss.str();
    }
}

void Worker::addTask(std::function<void()> &&task) noexcept
{
    try {
        {
            std::unique_lock<std::mutex> const lock { d_ptr->mutex };
            d_ptr->tasks.push(std::move(task));
        }

        std::stringstream ss;
        ss << d_ptr->workerName << " Adding task, tasks in queue: " << d_ptr->tasks.size() << '\n';
        std::cout << ss.str();

        d_ptr->waitCondition.notify_one();
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << d_ptr->workerName << " Error adding task: " << e.what() << '\n';
        std::cerr << ss.str();
    }
}

void Worker::setWorkerName(const std::string &name) noexcept
{
    d_ptr->workerName = name;
}
