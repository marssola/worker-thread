cmake_minimum_required(VERSION 3.16)

project(WorkerThread LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads)
find_package(Curses REQUIRED)

if (NOT Curses_FOUND)
    message(FATAL_ERROR "Curses library not found")
endif()

include_directories(${CURSES_INCLUDE_DIR})

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

target_link_libraries(WorkerThread ${CURSES_LIBRARIES} Threads::Threads)
