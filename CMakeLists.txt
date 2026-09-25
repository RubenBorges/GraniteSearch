cmake_minimum_required(VERSION 3.20)
project(GraniteSearch CXX)

# Force C++26 for modern features
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Process the source subdirectory first
add_subdirectory(src)

# =====================================================================
# INSTALL & DISTRIBUTION TARGETS (dist/)
# =====================================================================

set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")

# 1. Install the compiled binary executable
install(TARGETS GraniteSearch
    RUNTIME DESTINATION "${DIST_DIR}"
)

# 2. Copy the text documentation assets folder contents
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/"
    DESTINATION "${DIST_DIR}/docs"
    FILES_MATCHING PATTERN "*.txt"
)

# 3. New Rule: Copy the ibm-granite30m folder layout into dist/
# Note: Leaving off the trailing slash on "ibm-granite30m" ensures
# that the folder itself is created as dist/ibm-granite30m/
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ibm-granite30m"
    DESTINATION "${DIST_DIR}"
)
