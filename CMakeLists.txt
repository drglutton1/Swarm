cmake_minimum_required(VERSION 3.16)
project(swarm_poker_ecosystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORE_SOURCES
    src/core/ocean.cpp
    src/core/genome.cpp
    src/core/decoder.cpp
    src/core/agent.cpp
    src/core/chromosome.cpp
    src/core/swarm.cpp
    src/economy/chip_manager.cpp
    src/economy/transfer.cpp
    src/economy/inheritance.cpp
    src/evolution/lifecycle.cpp
    src/evolution/reproduction.cpp
    src/evolution/mutation.cpp
    src/evolution/crossover.cpp
    src/social/face.cpp
    src/social/info_exchange.cpp
    src/social/mate_selection.cpp
    src/scheduler/time_manager.cpp
    src/scheduler/activity_scheduler.cpp
    src/scheduler/table_manager.cpp
    src/sim/population.cpp
    src/sim/statistics.cpp
    src/sim/simulation.cpp)

add_library(swarm_core ${CORE_SOURCES})
target_include_directories(swarm_core PUBLIC src)

add_executable(swarm_poker_sim src/main.cpp)
target_include_directories(swarm_poker_sim PRIVATE src)
target_link_libraries(swarm_poker_sim PRIVATE swarm_core)

add_executable(test_poker test/test_poker.cpp)
target_include_directories(test_poker PRIVATE src)
target_link_libraries(test_poker PRIVATE swarm_core)
