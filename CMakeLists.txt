cmake_minimum_required(VERSION 3.25)

project(OptixCSP LANGUAGES CXX CUDA)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CUDA architectures 
set(CMAKE_CUDA_ARCHITECTURES 75;86;89)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

file(COPY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/bin/data/)
message(STATUS "Data directory copied to: ${CMAKE_BINARY_DIR}/bin/data/")

# Add subdirectories
add_subdirectory(src)
add_subdirectory(demos)

# Optionally, message user
message(STATUS "Configured OptixCSP project.")