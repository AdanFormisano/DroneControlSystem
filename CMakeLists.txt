cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 17)

add_executable(DroneControlSystem src/main.cpp
        src/main.h)

include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
)

FetchContent_MakeAvailable(spdlog)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(DroneControlSystem PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(DroneControlSystem ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(DroneControlSystem PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(DroneControlSystem ${REDIS_PLUS_PLUS_LIB})

target_link_libraries(DroneControlSystem spdlog::spdlog)

target_include_directories(DroneControlSystem PUBLIC ${HIREDIS_HEADER})
