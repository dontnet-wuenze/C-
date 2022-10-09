# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (TTCP)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# find a boost install with the libraries filesystem and system
find_package(Boost 1.46.1 REQUIRED COMPONENTS program_options)

# check if boost was found
if(Boost_FOUND)
    message ("boost found")
else()
    message (FATAL_ERROR "Cannot find Boost")
endif()

# Create a sources variable with a link to all cpp files to compile
set( LIBS
    lib/Acceptor.cpp
    lib/InetAddress.cpp
    lib/Socket.cpp
    lib/TcpStream.cpp
)

# Add an executable with the above sources
add_executable( ttcp bin/ttcp.cpp ${LIBS})

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories( ttcp
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries( ttcp
    PRIVATE
        Boost::program_options
)