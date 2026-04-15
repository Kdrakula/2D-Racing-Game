cmake_minimum_required(VERSION 4.1)

# Define project name and language (C++)
project(MyRacingGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

include(FetchContent)

# ─────────────────────────────────────────────────────────────────────────────
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG        v0.14.1
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
# ─────────────────────────────────────────────────────────────────────────────

# Include header files directory
include_directories(include)

# Gather all source files from the src directory
file(GLOB SOURCES "src/*.cpp")

# Create the executable target (as a macOS App Bundle so it can be clicked)
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})

# ── Auto-detect version from the latest git tag ──────────────────────────────
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
# 1. Fallback to a valid default version if GIT_TAG is empty
if(NOT GIT_TAG OR GIT_TAG STREQUAL "")
    set(GIT_TAG "v1.0.0")
endif()

message(STATUS "Game version: ${GIT_TAG}")

# 2. Safely remove the "v" prefix. Quotes around "${GIT_TAG}" prevent argument errors.
string(REGEX REPLACE "^v" "" PROJECT_VER "${GIT_TAG}")

# 3. Initialize the project FIRST. This defines ${PROJECT_NAME} and the VERSION.
project(MyRacingGame VERSION "${PROJECT_VER}" LANGUAGES CXX)

# NOTE: target_compile_definitions MUST be placed further down in your file,
# strictly AFTER you declare: add_executable(${PROJECT_NAME} ...)
# If you want it global before add_executable, use add_compile_definitions instead:
add_compile_definitions(GAME_VERSION="${GIT_TAG}")
# ─────────────────────────────────────────────────────────────────────────────

# Collect all individual asset files so cmake tracks changes to each one
file(GLOB_RECURSE ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

# Compute the destination directory inside the .app bundle
set(ASSETS_DEST "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/assets")

# Dedicated target that ALWAYS copies assets, depends on each file
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${ASSETS_DEST}"
    DEPENDS ${ASSET_FILES}
    COMMENT "Syncing assets/ into app bundle..."
)

# Make sure copy_assets runs BEFORE the binary is linked
add_dependencies(${PROJECT_NAME} copy_assets)

# Link the SDL3 libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf httplib::httplib nlohmann_json::nlohmann_json)

# ── macOS Portability: Use RPATHs for easier bundling in CI ────────────────────
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()