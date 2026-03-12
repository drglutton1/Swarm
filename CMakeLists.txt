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
    src/economy/inheritance.cpp)

add_library(swarm_core ${CORE_SOURCES})
target_include_directories(swarm_core PUBLIC src)

add_executable(swarm_poker_sim src/main.cpp)
target_include_directories(swarm_poker_sim PRIVATE src)
target_link_libraries(swarm_poker_sim PRIVATE swarm_core)

add_executable(test_poker test/test_poker.cpp)
target_include_directories(test_poker PRIVATE src)
target_link_libraries(test_poker PRIVATE swarm_core)
