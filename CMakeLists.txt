cmake_minimum_required(VERSION 3.22)

# 3.22 is the CMake that ships with Ubuntu 22.04. The floor of this curriculum
# is whatever a learner gets from apt on a supported LTS, with no extra
# repositories and no account to create.

project(robotics_cpp_from_scratch
  VERSION 0.1.0
  DESCRIPTION "Robotics with C++ and Qt, taught from nothing, in one language"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(RC_SANITIZE_ADDRESS "Build lesson tests with the address and undefined behaviour sanitizers" OFF)
option(RC_SANITIZE_THREAD  "Build lesson tests with the thread sanitizer" OFF)
option(RC_BUILD_LESSONS    "Build the curriculum itself, not only the tooling" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(RcPlatform)
include(RcLesson)

rc_detect_platform(RC_PLATFORM)
message(STATUS "toolchain identifier: ${RC_PLATFORM}")
if(RC_PLATFORM STREQUAL "unknown")
  message(WARNING
    "This compiler does not map to a toolchain the curriculum claims support for. "
    "Lessons will be skipped. Pass -DRC_PLATFORM_ID=... to force one.")
endif()

enable_testing()

add_subdirectory(librc)
add_subdirectory(tools/rcpp)

if(RC_BUILD_LESSONS)
  file(GLOB phase_dirs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/phases/*)
  list(SORT phase_dirs)
  foreach(phase_dir ${phase_dirs})
    if(NOT IS_DIRECTORY ${phase_dir})
      continue()
    endif()
    get_filename_component(phase_name ${phase_dir} NAME)
    string(SUBSTRING ${phase_name} 0 2 RC_CURRENT_PHASE)

    file(GLOB lesson_dirs CONFIGURE_DEPENDS ${phase_dir}/*)
    list(SORT lesson_dirs)
    foreach(lesson_dir ${lesson_dirs})
      if(IS_DIRECTORY ${lesson_dir} AND EXISTS ${lesson_dir}/CMakeLists.txt)
        add_subdirectory(${lesson_dir})
      endif()
    endforeach()
  endforeach()

  get_property(registered GLOBAL PROPERTY RC_REGISTERED_LESSONS)
  list(LENGTH registered registered_count)
  message(STATUS "lessons registered for ${RC_PLATFORM}: ${registered_count}")
endif()
