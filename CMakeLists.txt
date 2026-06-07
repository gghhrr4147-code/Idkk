cmake_minimum_required(VERSION 3.21)
project(BowNetherPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME} SHARED
    src/Plugin.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $ENV{LL_SDK}/include
    $ENV{LL_SDK}/src
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -fPIC
    -O2
    -fvisibility=hidden
    -fno-rtti
    -fno-exceptions
    -Wno-unused-parameter
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "BowNetherPlugin"
    PREFIX ""
    SUFFIX ".so"
)
