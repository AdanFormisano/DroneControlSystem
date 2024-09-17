cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 20)

# Optimization flags used for profiling
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(DroneControlSystem
        src/main.cpp
        src/ChargeBase/ChargeBase.cpp
        src/ChargeBase/ChargeBase.h
        src/DroneControl/DroneControl.cpp
        src/DroneControl/DroneControl.h
        src/globals.h
        src/Database/Database.cpp
        src/Database/Database.h
        src/Database/Buffer.cpp
        src/Database/Buffer.h
        src/TestGenerator/TestGenerator.cpp
        src/TestGenerator/TestGenerator.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
        src/Monitors/Monitor.cpp
        src/Monitors/Monitor.h
        src/Monitors/RechargeTimeMonitor.cpp
        src/Monitors/CoverageMonitor.cpp
        src/Monitors/DroneChargeMonitor.cpp
        src/Monitors/TimeToReadDataMonitor.cpp
        src/Scanner/ScannerManager.cpp
        src/Scanner/ScannerManager.h
        src/Scanner/ThreadUtils.h
        src/Scanner/ThreadUtils.cpp
        src/Scanner/Drone.cpp
        src/Scanner/Drone.h
        src/Scanner/DroneState.h
        src/Scanner/DroneState.cpp
)

option(DEBUG "Enable debug mode" OFF)

if (DEBUG)
    target_compile_definitions(DroneControlSystem PRIVATE DEBUG)
endif ()

target_compile_definitions(DroneControlSystem PRIVATE BOOST_DATE_TIME_NO_LIB)

# Add spdlog dependency
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

find_package(Boost REQUIRED COMPONENTS thread)
find_package(Boost REQUIRED COMPONENTS chrono)
find_package(Boost REQUIRED COMPONENTS system)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(DroneControlSystem PUBLIC ${HIREDIS_HEADER})
# target_include_directories(Drone PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(DroneControlSystem ${HIREDIS_LIB})
# target_link_libraries(Drone ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(DroneControlSystem PUBLIC ${REDIS_PLUS_PLUS_HEADER})
# target_include_directories(Drone PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(DroneControlSystem ${REDIS_PLUS_PLUS_LIB})
# target_link_libraries(Drone ${REDIS_PLUS_PLUS_LIB})

target_link_libraries(DroneControlSystem spdlog::spdlog)
# target_link_libraries(Drone spdlog::spdlog)

# <------------ libpqxx dependency -------------->
find_path(LIBPQXX_HEADER pqxx)
target_include_directories(DroneControlSystem PUBLIC ${LIBPQXX_HEADER})

find_library(LIBPQXX_LIB pqxx)
find_library(PQ_LIB pq REQUIRED)
target_link_libraries(DroneControlSystem ${LIBPQXX_LIB} ${PQ_LIB})

target_include_directories(DroneControlSystem PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(DroneControlSystem ${Boost_LIBRARIES})
target_link_libraries(DroneControlSystem ${Boost_SYSTEM_LIBRARY})
target_link_libraries(DroneControlSystem ${Boost_THREAD_LIBRARY})
target_link_libraries(DroneControlSystem ${Boost_CHRONO_LIBRARY})

