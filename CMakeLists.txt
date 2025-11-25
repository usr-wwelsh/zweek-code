cmake_minimum_required(VERSION 3.20)
project(zweek VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
include(FetchContent)

# FTXUI for TUI
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)

# JSON for data handling (updated for CMake 3.20+)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# llama.cpp for GGUF model inference (OPTIMIZED: GBNF support)
FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG master
)

# Configure llama.cpp options
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Make dependencies available
FetchContent_MakeAvailable(ftxui json llama)

# Source files
set(SOURCES
    src/main.cpp
    src/ui/tui.cpp
    src/ui/branding.cpp
    src/pipeline/orchestrator.cpp
    src/pipeline/router.cpp
    src/chat/chat_mode.cpp
    src/models/model_loader.cpp
    src/models/model_downloader.cpp
    src/tools/tool_executor.cpp
    src/tools/compiler_check.cpp
    src/commands/command_handler.cpp
)

# Create executable
add_executable(zweek ${SOURCES})

# Include directories
target_include_directories(zweek PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link libraries (OPTIMIZED: added llama)
target_link_libraries(zweek 
    PRIVATE 
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        llama
)

# Installation
install(TARGETS zweek DESTINATION bin)
