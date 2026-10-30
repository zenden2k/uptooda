cmake_minimum_required(VERSION 4.0)

project(squirrel)

# Squirrel 3.0.0 defines a global function-like macro named `type`.
# With C++20 that macro collides with libstdc++'s <compare> implementation.
# The library does not require C++20, so compile it as C++17 regardless of
# the standard requested by a consuming Conan profile.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_definitions(-D_LIB -DGARBAGE_COLLECTOR -D_SQ64)

add_subdirectory(squirrel)	
add_subdirectory(sqstdlib)	


			 
			 
