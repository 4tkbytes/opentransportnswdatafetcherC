cmake_minimum_required(VERSION 3.10)
project(opentransportnswdatafetcher C)

# Set MSYS2 UCRT64 path - adjust if needed
set(MSYS2_PATH "C:/msys64/ucrt64")
set(CURL_ROOT "${MSYS2_PATH}")

# Add CURL and cJSON include and library paths
include_directories("${MSYS2_PATH}/include")
link_directories("${MSYS2_PATH}/lib")

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add the executable
add_executable(transportnsw_fetcher src/main.c)

# Link against libcurl and cjson
target_link_libraries(transportnsw_fetcher PRIVATE 
    curl
    cjson
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler warnings
if(MSVC)
    target_compile_options(transportnsw_fetcher PRIVATE /W4)
else()
    target_compile_options(transportnsw_fetcher PRIVATE -Wall -Wextra)
endif()