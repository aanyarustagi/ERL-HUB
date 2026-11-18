cmake_minimum_required(VERSION 3.10)
project(ERL_HUB CXX)

set(CMAKE_CXX_STANDARD 11)          # also builds as 14 and 17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

file(GLOB HUB_SOURCES src/*.cpp)
list(FILTER HUB_SOURCES EXCLUDE REGEX "main\\.cpp$")

add_library(erl_hub_core STATIC ${HUB_SOURCES})
target_include_directories(erl_hub_core PUBLIC include)

add_executable(erl_hub src/main.cpp)
target_link_libraries(erl_hub erl_hub_core)

add_executable(erl_hub_tests tests/test_erl_hub.cpp)
target_link_libraries(erl_hub_tests erl_hub_core)

enable_testing()
add_test(NAME erl_hub_tests COMMAND erl_hub_tests)
