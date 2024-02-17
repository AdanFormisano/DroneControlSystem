cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 20)

# Maybe just add utils?
include_directories(include)

add_subdirectory(utils/spdlog)

file(GLOB SOURCES "src/*.cpp")

add_executable(DroneControlSystem src/main.cpp
        src/main.h
)

target_link_libraries(DroneControlSystem spdlog::spdlog)
