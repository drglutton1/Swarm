cmake_minimum_required(VERSION 3.16)
project(swarm_poker_ecosystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(swarm_poker_sim src/main.cpp)
target_include_directories(swarm_poker_sim PRIVATE src)

add_executable(test_poker test/test_poker.cpp)
target_include_directories(test_poker PRIVATE src)
