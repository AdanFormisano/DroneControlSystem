cmake_minimum_required(VERSION 3.10)
project(DroneControlSystem)

set(CMAKE_CXX_STANDARD 17)

add_executable(DroneControlSystem
        src/main.cpp
        src/DroneControl/DroneControl.cpp
        src/DroneControl/DroneControl.h
        src/Drone/Drone.cpp
        src/ChargeBase/ChargeBase.cpp
        src/ChargeBase/ChargeBase.h
        src/Drone/DroneZone.cpp
        src/Drone/DroneManager.cpp
        src/Drone/DroneManager.h
        src/globals.cpp
        src/globals.h
        src/db/Database.cpp
        src/db/Database.h
        src/GUI/GuiLite.h
        utils/RedisUtils.cpp
        utils/RedisUtils.h
        utils/utils.cpp
        utils/utils.h

)

# Add spdlog dependency
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
target_link_libraries(DroneControlSystem ${LIBPQXX_LIB})
