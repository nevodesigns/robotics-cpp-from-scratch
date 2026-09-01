cmake_minimum_required(VERSION 3.16)
project(consumer LANGUAGES CXX)

# By name and by version. REQUIRED turns a missing package into an error here
# rather than into a confusing compile failure later.
find_package(steplib 1.0 REQUIRED)

add_executable(consumer main.cpp)

# The include directory and the language standard both arrive with the target.
# Nothing in this file mentions either, which is what a package is for.
target_link_libraries(consumer PRIVATE steplib::steplib)
