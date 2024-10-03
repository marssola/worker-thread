# Example application for worker threads

WorkerThreads is a simple example application that demonstrates how to create a worker thread in C++. The project has a pool of workers that insert task into the worker with least load using std::min_element algorithm. The vector of workers is initialized by the number of threads the system can handle (std::thread::hardware_concurrency()).

The project contains:

    * A worker class that represents a worker thread.
    * A worker pool class that manages the worker threads.
    * A generator of tasks that inserts tasks into the worker pool.
    * MainLoop that waits for the program to terminate or receive a signal (SIGINT or SIGTERM).
        * The Unix signal handler is configured to catch SIGINT and SIGTERM signals and invoke the MainLoop::stop() function.

### Dependencies

    * C++ 17
    * CMake 3.16
    * NCurses

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
build/WorkerThread
```

