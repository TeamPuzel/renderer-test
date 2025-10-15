set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

cmake_minimum_required(VERSION 3.21)
project(Raytracer C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -Wno-unused -Wpedantic -Wno-logical-op-parentheses")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftrivial-auto-var-init=pattern")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(SDL3_DIR "lib/SDL3-3.2.20/cmake")
    file(COPY "lib/SDL3-3.2.20/lib/x64/SDL3.dll" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Depencencies ---------------------------------------------------------------------------------------------------------

find_package(SDL3 REQUIRED)

file(GLOB_RECURSE RAYTRACER_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE RAYTRACER_HEADERS CONFIGURE_DEPENDS src/*.hpp)

add_executable(raytracer ${RAYTRACER_SOURCES} ${RAYTRACER_HEADERS})
target_link_libraries(raytracer PRIVATE SDL3::SDL3)
target_include_directories(raytracer PRIVATE include)

# Resources ------------------------------------------------------------------------------------------------------------

file(GLOB_RECURSE RESOURCES res/*)

foreach(RESOURCE ${RESOURCES})
    file(COPY ${RESOURCE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res")
endforeach()
