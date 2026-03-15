cmake_minimum_required(VERSION 4.1)

# Define project name and language (C++)
project(MyRacingGame VERSION 1.0 LANGUAGES CXX)

# Enforce C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

# Find SDL3 and SDL3_image packages provided by Homebrew or system package managers
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)

include(FetchContent)
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG        v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Include header files directory
include_directories(include)

# Gather all source files from the src directory
file(GLOB SOURCES "src/*.cpp")

# Create the executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Link the SDL3 libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image httplib::httplib)