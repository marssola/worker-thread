#pragma once

#include <functional>
#include <memory>

class WorkerPrivate;
class Worker
{
public:
    explicit Worker() noexcept;
    ~Worker() noexcept;

    void start() noexcept;
    void stop() noexcept;
    void waitForFinished() noexcept;
    void addTask(const std::function<void()> &task) noexcept;
    void addTask(std::function<void()> &&task) noexcept;

    void setWorkerName(const std::string &name) noexcept;

private:
    std::unique_ptr<WorkerPrivate> d_ptr;
};
