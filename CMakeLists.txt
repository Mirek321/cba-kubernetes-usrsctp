cmake_minimum_required(VERSION 3.28)

project(UsrSctpServer)

# Global C++ Standard Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put all executables into build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add subdirectories
add_subdirectory(client)
add_subdirectory(server)
