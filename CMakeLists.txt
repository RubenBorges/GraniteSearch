cmake_minimum_required(VERSION 3.20)
project(GraniteSearch CXX)

# Force C++26 for modern features
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

# =====================================================================
# INSTALL & DISTRIBUTION TARGETS (dist/)
# =====================================================================

set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")

install(TARGETS GraniteSearch
    RUNTIME DESTINATION "${DIST_DIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/"
    DESTINATION "${DIST_DIR}/docs"
    FILES_MATCHING PATTERN "*.txt"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ibm-granite30m"
    DESTINATION "${DIST_DIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/images/"
    DESTINATION "${DIST_DIR}"
    FILES_MATCHING PATTERN "*.png"
)
