cmake_minimum_required(VERSION 3.16)

project(WorkerThread LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads)

add_executable(WorkerThread
  main.cpp
  Worker.h Worker.cpp
  MainLoop.h MainLoop.cpp
)

include(GNUInstallDirs)
install(TARGETS WorkerThread
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
