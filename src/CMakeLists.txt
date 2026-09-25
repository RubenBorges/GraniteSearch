# Source and header setups
set(SOURCES
    GraniteSearch.cpp
    graniteembedder.cpp
)
set(HEADERPATHS
    include/graniteembedder.hpp
)

add_executable(GraniteSearch ${SOURCES} ${HEADERPATHS})

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
target_include_directories(GraniteSearch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/include
)

# 3. Explicitly link everything, clean and warning-free
target_link_libraries(GraniteSearch PRIVATE
    ${LLAMA_LIB}   # Resolves to /usr/lib/libllama.so exactly
    pthread        # POSIX threads
    dl             # Dynamic linking loader
)

# 4. Compiler optimizations
target_compile_options(GraniteSearch PRIVATE -O3 -Wall -Wextra -march=native)
