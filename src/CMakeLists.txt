# Source and header setups
set(SOURCES
    granitesearch.cpp
    graniteembedder.cpp
)
set(HEADERPATHS
    include/graniteembedder.hpp
    include/granitesearch.hpp
)

add_library(GraniteSearchCore STATIC ${SOURCES} ${HEADERPATHS})
add_executable(GraniteSearch GraniteUI.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)

# =====================================================================
# SYSTEM PACMAN LLAMA LINKING RULES (Arch Linux)
# =====================================================================

# 1. Look up the pacman system shared library file natively inside /usr/lib
find_library(LLAMA_LIB
    NAMES llama
    PATHS /usr/lib
    NO_DEFAULT_PATH
    REQUIRED
)

# 2. Add search parameters for your local project headers and system-wide headers
target_include_directories(GraniteSearchCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/include
)

# 3. Explicitly link everything, clean and warning-free
target_link_libraries(GraniteSearchCore PUBLIC
    ${LLAMA_LIB}   # Resolves to /usr/lib/libllama.so exactly
    pthread        # POSIX threads
    dl             # Dynamic linking loader
)

target_link_libraries(GraniteSearch PRIVATE GraniteSearchCore PkgConfig::RAYLIB)

# 4. Compiler optimizations
target_compile_options(GraniteSearchCore PRIVATE -O3 -Wall -Wextra -march=native)
target_compile_options(GraniteSearch PRIVATE -O3 -Wall -Wextra -march=native)
