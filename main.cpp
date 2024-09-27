#include <iostream>
#include <random>
#include <thread>

#include <csignal>

#include "MainLoop.h"
#include "Worker.h"

namespace {
constexpr auto startTimeoutTask { 500 };
constexpr auto endTimeoutTask { 2000 };
constexpr auto startTimeoutGen { 10 };
constexpr auto endTimeoutGen { 500 };
constexpr auto index0 { 0 };
const auto workersNum { std::thread::hardware_concurrency() };
} // namespace

class SingleShotTimer
{
public:
    explicit SingleShotTimer(std::chrono::milliseconds timeout, std::function<void()> &&callback)
    {
        std::thread([timeout, callback = std::move(callback)]() {
            std::this_thread::sleep_for(timeout);
            callback();
        }).detach();
    }
};

class MakeTasks
{
public:
    explicit MakeTasks(std::vector<Worker> *workers) : workers(workers) { }

    void generateTasks() noexcept
    {
        if (workers == nullptr) {
            return;
        }

        std::mt19937 gen(std::random_device {}());
        std::uniform_int_distribution<> timeoutTask(startTimeoutTask, endTimeoutTask);
        std::uniform_int_distribution<> timeoutGen(startTimeoutGen, endTimeoutGen);
        constexpr int n1 { 1 };

        const int workersSize = static_cast<int>(workers->size()) - n1;
        std::uniform_int_distribution<> distWorker(index0, workersSize - n1);

        while (MainLoop::isRunning()) {
            if (workers == nullptr) {
                break;
            }

            auto index = distWorker(gen);
            workers->at(index).addTask([index, &timeoutTask, &gen]() {
                auto waitFor = timeoutTask(gen) * std::chrono::milliseconds(1);
                std::stringstream ss;
                ss << "\tProcessing task in worker " << index << " in thread "
                   << std::this_thread::get_id() << " for " << waitFor.count() << "ms" << '\n';
                std::cout << ss.str();
                std::this_thread::sleep_for(waitFor);
            });

            auto waitFor = timeoutGen(gen) * std::chrono::milliseconds(1);
            std::this_thread::sleep_for(waitFor);
        }
    }

private:
    std::vector<Worker> *workers;
};

void sigHandler(int signo)
{
    std::stringstream ss;
    ss << "Signal (" << signo << ") received\n";
    std::cout << ss.str();

    MainLoop::quit(signo);
}

int main(int /*argc*/, char * /*argv*/[])
{
    std::signal(SIGINT, sigHandler);
    std::signal(SIGABRT, sigHandler);
    std::signal(SIGTERM, sigHandler);

    MainLoop::start();

    std::vector<Worker> workers(workersNum);
    MakeTasks generator(&workers);

    int index = 0;
    for (auto &worker : workers) {
        std::stringstream ss;
        ss << "Worker(" << index++ << ")";
        worker.setWorkerName(ss.str());

        worker.start();
        MainLoop::addOnQuit([&worker]() { worker.stop(); });
    }

    generator.generateTasks();

    return MainLoop::wait();
}
