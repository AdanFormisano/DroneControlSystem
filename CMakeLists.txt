cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 20)

# Optimization flags used for profiling
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(DroneControlSystem
        src/main.cpp
        src/globals.h
        src/ChargeBase/ChargeBase.cpp
        src/ChargeBase/ChargeBase.h
        src/DroneControl/DroneControl.cpp
        src/DroneControl/DroneControl.h
        src/Database/NewBuffer.cpp
        src/Database/NewBuffer.h
        src/Database/Database.cpp
        src/Database/Database.h
        src/TestGenerator/TestGenerator.cpp
        src/TestGenerator/TestGenerator.h
        src/Monitors/Monitor.cpp
        src/Monitors/Monitor.h
        src/Monitors/RechargeTimeMonitor.cpp
        src/Monitors/CoverageMonitor.cpp
        src/Monitors/DroneChargeMonitor.cpp
        src/Monitors/SystemPerformanceMonitor.cpp
        src/Scanner/ScannerManager.cpp
        src/Scanner/ScannerManager.h
        src/Scanner/ThreadUtils.h
        src/Scanner/ThreadUtils.cpp
        src/Scanner/Drone.cpp
        src/Scanner/Drone.h
        src/Scanner/DroneState.h
        src/Scanner/DroneState.cpp
        src/Scanner/Wave.cpp
        src/Scanner/Wave.h
        utils/LogUtils.cpp
        utils/LogUtils.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
)

target_compile_definitions(DroneControlSystem PRIVATE BOOST_DATE_TIME_NO_LIB)

include_directories(${CMAKE_SOURCE_DIR}/libs)


# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(DroneControlSystem PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(DroneControlSystem ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(DroneControlSystem PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(DroneControlSystem ${REDIS_PLUS_PLUS_LIB})

# <------------ libpqxx dependency -------------->
find_path(LIBPQXX_HEADER pqxx)
target_include_directories(DroneControlSystem PUBLIC ${LIBPQXX_HEADER})

find_library(LIBPQXX_LIB pqxx)
find_library(PQ_LIB pq REQUIRED)
target_link_libraries(DroneControlSystem ${LIBPQXX_LIB} ${PQ_LIB})

