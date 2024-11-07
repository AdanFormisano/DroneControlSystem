cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 20)

# Enable debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -g3 -fvar-tracking-assignments -fsanitize=address")

add_executable(DroneBaseSystem
        src/main_system.cpp
        src/globals.h
        src/Database/Database.cpp
        src/Database/Database.h
        src/TestGenerator/TestGenerator.cpp
        src/TestGenerator/TestGenerator.h
        utils/LogUtils.cpp
        utils/LogUtils.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
        src/Monitors/Monitor.cpp
        src/Monitors/Monitor.h
        src/Monitors/CoverageMonitor.cpp
        src/Monitors/DroneChargeMonitor.cpp
        src/Monitors/RechargeTimeMonitor.cpp
        src/Monitors/SystemPerformanceMonitor.cpp
        src/Monitors/AreaCoverage.cpp
)

add_executable(DroneControl
        src/DroneControl/main_dronecontrol.cpp
        src/globals.h
        src/DroneControl/DroneControl.cpp
        src/DroneControl/DroneControl.h
        src/Database/Database.cpp
        src/Database/Database.h
        src/Database/NewBuffer.cpp
        src/Database/NewBuffer.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
        utils/LogUtils.cpp
        utils/LogUtils.h)

add_executable(Scanner
        src/Scanner/main_scanner.cpp
        src/globals.h
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
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
        utils/LogUtils.cpp
        utils/LogUtils.h)

add_executable(ChargeBase
        src/ChargeBase/main_chargebase.cpp
        src/globals.h
        src/ChargeBase/ChargeBase.cpp
        src/ChargeBase/ChargeBase.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h
        utils/LogUtils.cpp
        utils/LogUtils.h
        src/Monitors/AreaCoverage.cpp)

target_compile_definitions(DroneBaseSystem PRIVATE BOOST_DATE_TIME_NO_LIB)
target_compile_definitions(DroneControl PRIVATE BOOST_DATE_TIME_NO_LIB)
target_compile_definitions(Scanner PRIVATE BOOST_DATE_TIME_NO_LIB)
target_compile_definitions(ChargeBase PRIVATE BOOST_DATE_TIME_NO_LIB)

include_directories(${CMAKE_SOURCE_DIR}/libs)


# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(DroneBaseSystem PUBLIC ${HIREDIS_HEADER})
target_include_directories(DroneControl PUBLIC ${HIREDIS_HEADER})
target_include_directories(Scanner PUBLIC ${HIREDIS_HEADER})
target_include_directories(ChargeBase PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(DroneBaseSystem ${HIREDIS_LIB})
target_link_libraries(DroneControl ${HIREDIS_LIB})
target_link_libraries(Scanner ${HIREDIS_LIB})
target_link_libraries(ChargeBase ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(DroneBaseSystem PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(DroneControl PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(Scanner PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(ChargeBase PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(DroneBaseSystem ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(DroneControl ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(Scanner ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(ChargeBase ${REDIS_PLUS_PLUS_LIB})

# <------------ libpqxx dependency -------------->
find_path(LIBPQXX_HEADER pqxx)
target_include_directories(DroneBaseSystem PUBLIC ${LIBPQXX_HEADER})
target_include_directories(DroneControl PUBLIC ${LIBPQXX_HEADER})
target_include_directories(Scanner PUBLIC ${LIBPQXX_HEADER})
target_include_directories(ChargeBase PUBLIC ${LIBPQXX_HEADER})

find_library(LIBPQXX_LIB pqxx)
find_library(PQ_LIB pq REQUIRED)
target_link_libraries(DroneBaseSystem ${LIBPQXX_LIB} ${PQ_LIB})
target_link_libraries(DroneControl ${LIBPQXX_LIB} ${PQ_LIB})
target_link_libraries(Scanner ${LIBPQXX_LIB} ${PQ_LIB})
target_link_libraries(ChargeBase ${LIBPQXX_LIB} ${PQ_LIB})

