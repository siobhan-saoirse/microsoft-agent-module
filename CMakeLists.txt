cmake_minimum_required(VERSION 3.15)
project(msagent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(gmod-module-base)

add_library(msagent SHARED
    src/main.cpp
    src/agent.cpp
    src/lua_api.cpp
)

target_include_directories(msagent PRIVATE
    include
)

target_link_libraries(msagent
    gmod-module-base
    ole32
    oleaut32
)

set_gmod_suffix_prefix(msagent)