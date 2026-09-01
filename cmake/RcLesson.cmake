# rc_add_lesson()
#
# Every lesson directory contains exactly one line of CMake: rc_add_lesson().
# This function reads lesson.json, decides whether the lesson applies to the
# toolchain being configured, and creates two test executables from the same
# test sources:
#
#   <id>.exercise    the learner's code, which starts out failing
#   <id>.reference   the worked implementation, which must always pass
#
# The tests include "solution.hpp". Which one they get is decided by the include
# directory, so a single test suite grades both.

function(rc_add_lesson)
  set(lesson_dir "${CMAKE_CURRENT_SOURCE_DIR}")
  set(manifest "${lesson_dir}/lesson.json")

  if(NOT EXISTS "${manifest}")
    message(FATAL_ERROR "rc_add_lesson: no lesson.json in ${lesson_dir}")
  endif()

  file(READ "${manifest}" manifest_text)
  string(JSON lesson_id ERROR_VARIABLE json_error GET "${manifest_text}" "id")
  if(json_error)
    message(FATAL_ERROR "rc_add_lesson: ${manifest} is not valid JSON: ${json_error}")
  endif()

  string(JSON lesson_title GET "${manifest_text}" "title")
  string(JSON platform_count ERROR_VARIABLE ignore LENGTH "${manifest_text}" "platforms")

  # Does this lesson claim the toolchain we are configuring?
  set(supported FALSE)
  if(platform_count)
    math(EXPR last "${platform_count} - 1")
    foreach(index RANGE ${last})
      string(JSON entry GET "${manifest_text}" "platforms" ${index})
      if(entry STREQUAL "${RC_PLATFORM}")
        set(supported TRUE)
      endif()
    endforeach()
  endif()

  if(NOT supported)
    message(STATUS "lesson ${lesson_id}: skipped, does not claim ${RC_PLATFORM}")
    return()
  endif()

  # Qt lessons declare the modules they need. A missing Qt is a skip, not a
  # failure, so the curriculum still configures on a machine without it.
  set(qt_modules "")
  string(JSON qt_block ERROR_VARIABLE no_qt GET "${manifest_text}" "qt")
  if(NOT no_qt AND NOT qt_block STREQUAL "null")
    string(JSON qt_count ERROR_VARIABLE ignore LENGTH "${manifest_text}" "qt" "modules")
    if(qt_count)
      math(EXPR qt_last "${qt_count} - 1")
      foreach(index RANGE ${qt_last})
        string(JSON qt_module GET "${manifest_text}" "qt" "modules" ${index})
        list(APPEND qt_modules ${qt_module})
      endforeach()
    endif()
  endif()

  if(qt_modules)
    find_package(Qt6 QUIET COMPONENTS ${qt_modules})
    if(NOT Qt6_FOUND)
      # A learner without Qt should still be able to build everything else, so
      # locally this is a skip. In continuous integration it must be an error:
      # the lesson claims this toolchain, and a lesson that is silently skipped
      # reports success without a single line having been compiled. A skip that
      # looks like a pass is the worst outcome a gate can produce.
      if(RC_STRICT_CLAIMS)
        message(FATAL_ERROR
          "lesson ${lesson_id} claims ${RC_PLATFORM} and needs Qt6 (${qt_modules}), "
          "which was not found. Either install Qt on this lane or remove the claim "
          "from lesson.json. See rule L019.")
      endif()
      message(STATUS "lesson ${lesson_id}: skipped, Qt6 (${qt_modules}) was not found")
      return()
    endif()
  endif()

  string(REPLACE "-" "_" target_base "lesson_${lesson_id}")

  file(GLOB test_sources CONFIGURE_DEPENDS "${lesson_dir}/tests/*.cpp")
  if(NOT test_sources)
    message(FATAL_ERROR "rc_add_lesson: ${lesson_id} has no tests, every lesson ships a test suite")
  endif()

  foreach(variant exercise reference)
    # Headers are listed as sources on purpose. Qt's AUTOMOC only processes a
    # header that belongs to the target, and a lesson that declares Q_OBJECT in
    # a header would otherwise fail to link with an undefined vtable.
    file(GLOB variant_sources CONFIGURE_DEPENDS
      "${lesson_dir}/${variant}/*.cpp"
      "${lesson_dir}/${variant}/*.hpp")
    set(target "${target_base}_${variant}")

    # The learner's copy is deliberately allowed to be broken: some lessons ship
    # an exercise that does not even link, because the linker error is the point.
    # Excluding it from the default build keeps a plain "cmake --build" green and
    # fast, while rcpp verify builds it by name on demand.
    if(variant STREQUAL "exercise")
      add_executable(${target} EXCLUDE_FROM_ALL ${test_sources} ${variant_sources})
    else()
      add_executable(${target} ${test_sources} ${variant_sources})
    endif()
    target_include_directories(${target} PRIVATE "${lesson_dir}/${variant}")

    # Where the lesson lives on disk, for the few tests that need more than
    # their own source: a fixtures directory of recorded data, or, in lesson
    # 04-03, a project to hand to CMake and build.
    target_compile_definitions(${target} PRIVATE
      RC_LESSON_DIR="${lesson_dir}"
      RC_LESSON_VARIANT_DIR="${lesson_dir}/${variant}")
    target_link_libraries(${target} PRIVATE rc::core Threads::Threads)
    target_compile_features(${target} PRIVATE cxx_std_17)

    if(qt_modules)
      set_target_properties(${target} PROPERTIES AUTOMOC ON)
      foreach(module ${qt_modules})
        target_link_libraries(${target} PRIVATE Qt6::${module})
      endforeach()
    endif()

    if(MSVC)
      target_compile_options(${target} PRIVATE /W4)
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()

    if(RC_SANITIZE_ADDRESS AND NOT MSVC)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    if(RC_SANITIZE_THREAD AND NOT MSVC)
      target_compile_options(${target} PRIVATE -fsanitize=thread)
      target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()

    add_test(NAME "${lesson_id}.${variant}" COMMAND ${target})
    set_tests_properties("${lesson_id}.${variant}" PROPERTIES
      LABELS "${variant};phase${RC_CURRENT_PHASE}"
      TIMEOUT 60
    )

    # A Qt test must not require a display. Setting the platform on the test
    # itself means it runs the same way on a laptop, over SSH, and on a
    # continuous integration machine that has no screen at all, without the
    # learner having to know that.
    if(qt_modules)
      set_property(TEST "${lesson_id}.${variant}" APPEND PROPERTY
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
  endforeach()

  # The learner's copy is allowed to fail. The worked implementation is not, so
  # it carries the label CI gates on.
  set_tests_properties("${lesson_id}.reference" PROPERTIES LABELS "reference;gate")

  set_property(GLOBAL APPEND PROPERTY RC_REGISTERED_LESSONS "${lesson_id}")
endfunction()
