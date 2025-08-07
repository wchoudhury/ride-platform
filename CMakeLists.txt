cmake_minimum_required(VERSION 3.10)
project(RIDE_Platform)

set(CMAKE_CXX_STANDARD 14)

# Find Fast-DDS system dependencies
find_package(fastcdr REQUIRED)
find_package(foonathan_memory REQUIRED)

message(STATUS "Found fastcdr: ${fastcdr_DIR}")
message(STATUS "Found foonathan_memory: ${foonathan_memory_DIR}")
message(STATUS "Step 1: Fast-DDS Middleware dependencies configured successfully")

# Add subdirectories for the components (commented out for Step 1 testing)
# add_subdirectory(donkeycar_bridge/lcc_integration)