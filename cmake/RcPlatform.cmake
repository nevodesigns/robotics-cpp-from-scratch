# Works out which toolchain identifier this configuration matches. Lessons
# declare the identifiers they support in lesson.json, and that declaration is
# what CI enforces, so the naming has to be decided in exactly one place.

function(rc_detect_platform out_var)
  if(DEFINED RC_PLATFORM_ID AND NOT RC_PLATFORM_ID STREQUAL "")
    set(${out_var} "${RC_PLATFORM_ID}" PARENT_SCOPE)
    return()
  endif()

  set(id "unknown")
  if(MSVC)
    set(id "windows-msvc2022")
  elseif(APPLE)
    set(id "macos-clang")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12)
      set(id "ubuntu-22.04-gcc11")
    else()
      set(id "ubuntu-24.04-gcc13")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
      set(id "ubuntu-22.04-clang14")
    else()
      set(id "ubuntu-24.04-clang18")
    endif()
  endif()

  set(${out_var} "${id}" PARENT_SCOPE)
endfunction()
