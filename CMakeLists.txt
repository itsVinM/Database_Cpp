cmake_minimum_required(VERSION 3.10)
project(Database_Cpp)

# Set the C++ compiler (replace g++ with your compiler name)
set(CMAKE_CXX_COMPILER g++)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")


# Executable
add_executable(Database_Cpp ${SOURCES} ${HEADERS})



