cmake_minimum_required(VERSION 3.16)
project(consumer LANGUAGES CXX)

# TODO: find the package by name and version, and link main.cpp to it.
#
# Nothing in this file should mention an include directory or a language
# standard. Both arrive with the target, and having to state them here means
# the package did not carry them, which is the thing worth noticing.

add_executable(consumer main.cpp)
