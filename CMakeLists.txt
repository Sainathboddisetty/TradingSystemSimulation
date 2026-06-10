cmake_minimum_required(VERSION 3.16)
project(TradingSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ──────────────────────────────────────────
# -O3          : aggressive optimisation (auto-vectorise, inline everything)
# -march=native: generate instructions for the current CPU (enables AVX, BMI2, etc.)
# -fno-rtti    : disable RTTI — we don't need dynamic_cast in ULL code
# -fno-exceptions: disable C++ exceptions — they add hidden overhead on every call frame
# -Wall -Wextra: catch common mistakes
# ─────────────────────────────────────────────────────────────
add_compile_options(
    -O3
    -march=native
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -Wpedantic
)

# ── Sources ──────────────────────────────────────────────────
add_executable(trading_system
    src/main.cpp
)

target_include_directories(trading_system PRIVATE include src)

# ── Threading ────────────────────────────────────────────────
find_package(Threads REQUIRED)
target_link_libraries(trading_system PRIVATE Threads::Threads)

# ── Copy data directory to build location ────────────────────
add_custom_command(TARGET trading_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:trading_system>/data
    COMMENT "Copying reference data to build directory"
)

# ── Build type helper messages ───────────────────────────────
message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")