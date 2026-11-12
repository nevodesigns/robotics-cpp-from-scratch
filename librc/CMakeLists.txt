# librc is the library the learner accretes. Every lesson that ships an artifact
# graduates its verified implementation into one of these modules, so by the end
# the learner owns one library they wrote and understand, not a folder of
# disconnected exercises.

add_library(rc_core INTERFACE)
add_library(rc::core ALIAS rc_core)

target_include_directories(rc_core INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(rc_core INTERFACE cxx_std_17)

# The header only modules are exposed through rc::core for now. Compiled modules
# arrive with the phases that build them.
install(DIRECTORY include/ DESTINATION include)

# Every header must compile on its own, and twice.
#
# On its own, because a header that only builds after something else was
# included first is a trap for whoever includes it next. rc/rt/latest.hpp was
# written using std::atomic without including <atomic>, and every lesson that
# used it would have compiled, because rc_test.hpp had already pulled <atomic>
# in. The defect only appears for the person who includes it first.
#
# Twice, because a header only library that defines a function without inline
# links fine in one translation unit and fails with a duplicate symbol in two.
# Generating a pair per header is what makes the linker check that.
file(GLOB_RECURSE rc_public_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/rc/*.hpp")

# rc_test.hpp defines main, which is its whole point: a lesson's test file
# includes it and gets an entry point. It cannot share a link with anything
# else, and it needs no gate here anyway, because every lesson test file in the
# repository includes it as its first include and so proves it standalone on
# every toolchain, every build.
list(REMOVE_ITEM rc_public_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/rc/test/rc_test.hpp")

# The rc::qt headers need Qt to compile and moc to link, so they get their own
# gate below rather than being dropped from the checking altogether. On a
# machine without Qt they are skipped, which is the same bargain the Qt lessons
# make: Qt is optional in this curriculum and everything before phase 09 builds
# without it.
set(rc_qt_headers "")
foreach(header ${rc_public_headers})
  if(header MATCHES "/include/rc/qt/")
    list(APPEND rc_qt_headers "${header}")
  endif()
endforeach()
list(REMOVE_ITEM rc_public_headers ${rc_qt_headers})

# The rc::gz headers need Gazebo's transport and message libraries. They get their own gate below when those are installed and
# are left out of the others, as the rc::qt headers are: Gazebo is optional and the course builds without it.
set(rc_gz_headers "")
foreach(header ${rc_public_headers})
  if(header MATCHES "/include/rc/gz/")
    list(APPEND rc_gz_headers "${header}")
  endif()
endforeach()
list(REMOVE_ITEM rc_public_headers ${rc_gz_headers})

set(standalone_sources "")
foreach(header ${rc_public_headers})
  file(RELATIVE_PATH include_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
  string(REPLACE "/" "_" flat "${include_path}")
  string(REPLACE ".hpp" "" flat "${flat}")

  # leak_check.hpp replaces the global allocation functions, which no
  # translation unit may do twice in one binary. Once is the whole contract, and
  # a second copy would keep a second set of counters and report leaks that are
  # not there, so it gets the standalone check without the duplicate check.
  set(copies first second)
  if(include_path STREQUAL "rc/test/leak_check.hpp")
    set(copies first)
  endif()

  foreach(copy ${copies})
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/standalone/${flat}_${copy}.cpp")
    file(WRITE "${generated}" "#include <${include_path}>\n")
    list(APPEND standalone_sources "${generated}")
  endforeach()
endforeach()

# Every header in one translation unit as well.
#
# Compiling each header alone proves each is complete, and says nothing about two of them agreeing. Three headers
# defined `rc::core::Reading` with three different layouts, each compiled alone and each linked, which is a violation of
# the one definition rule that no compiler reports across translation units. All of them together in one file does report
# it, as a redefinition.
set(together_text "// Generated. Every header at once: two that define one name differently compile alone and fail here.\n")
foreach(header ${rc_public_headers})
  file(RELATIVE_PATH include_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
  # leak_check.hpp defines the global allocation functions, which only one translation unit of a program may do, and
  # it already has its standalone copy above.
  if(NOT include_path STREQUAL "rc/test/leak_check.hpp")
    string(APPEND together_text "#include <${include_path}>\n")
  endif()
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/together.cpp" "${together_text}")
list(APPEND standalone_sources "${CMAKE_CURRENT_BINARY_DIR}/standalone/together.cpp")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/main.cpp"
  "// Generated. The test is that this links, which means every header in\n"
  "// librc compiled alone and twice over.\n"
  "int main() { return 0; }\n")

add_executable(rc_headers_standalone
  "${CMAKE_CURRENT_BINARY_DIR}/standalone/main.cpp"
  ${standalone_sources})
target_link_libraries(rc_headers_standalone PRIVATE rc::core)
if(MSVC)
  target_compile_options(rc_headers_standalone PRIVATE /W4 /WX)
else()
  target_compile_options(rc_headers_standalone PRIVATE -Wall -Wextra -Werror)
endif()

add_test(NAME librc.headers-compile-standalone COMMAND rc_headers_standalone)
set_property(TEST librc.headers-compile-standalone PROPERTY LABELS gate)

# Tests of the library itself, apart from the lessons: each file in tests/ is a program with rc_test.hpp's main, built with the warnings and sanitizers of a lesson and run as a gate.
find_package(Threads REQUIRED)
file(GLOB rc_unit_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp")
foreach(source ${rc_unit_tests})
  get_filename_component(unit_name "${source}" NAME_WE)
  set(unit_target "rc_${unit_name}")
  add_executable(${unit_target} "${source}")
  target_link_libraries(${unit_target} PRIVATE rc::core Threads::Threads)
  target_compile_features(${unit_target} PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(${unit_target} PRIVATE /W4)
  else()
    target_compile_options(${unit_target} PRIVATE -Wall -Wextra)
  endif()
  if(RC_SANITIZE_ADDRESS AND NOT MSVC)
    target_compile_options(${unit_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${unit_target} PRIVATE -fsanitize=address,undefined)
  endif()
  if(RC_SANITIZE_THREAD AND NOT MSVC)
    target_compile_options(${unit_target} PRIVATE -fsanitize=thread)
    target_link_options(${unit_target} PRIVATE -fsanitize=thread)
  endif()
  add_test(NAME "librc.${unit_name}" COMMAND ${unit_target})
  set_tests_properties("librc.${unit_name}" PROPERTIES LABELS "reference;gate;librc" TIMEOUT 60)
endforeach()

# The same two checks for the Qt headers, with Qt linked and AUTOMOC on so that
# a Q_OBJECT class is compiled the way a real target compiles it. A header
# declaring Q_OBJECT and never reaching moc links fine until somebody connects
# to one of its signals, which is E-QT-0001 and is worth catching here rather
# than there.
if(rc_qt_headers)
  # Qml joins the list from phase 11, where rc/qt/qml_host.hpp arrives. Asking
  # for it here rather than separately keeps the rule that the Qt gate either
  # runs over every rc::qt header or does not run: a gate that quietly covers
  # some of them reports success for the ones it skipped. Network joins from
  # phase 12, where rc/qt/link_client.hpp arrives, and is asked for by name so
  # that its headers are found on a platform where Qml does not bring them.
  # Concurrent joins from lesson 12-04, where rc/qt/plan_runner.hpp arrives.
  find_package(Qt6 QUIET COMPONENTS Core Widgets Qml Network Concurrent)
  if(Qt6_FOUND)
    set(qt_standalone_sources "")
    foreach(header ${rc_qt_headers})
      file(RELATIVE_PATH include_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
      string(REPLACE "/" "_" flat "${include_path}")
      string(REPLACE ".hpp" "" flat "${flat}")

      foreach(copy first second)
        set(generated "${CMAKE_CURRENT_BINARY_DIR}/standalone/${flat}_${copy}.cpp")
        file(WRITE "${generated}" "#include <${include_path}>\n")
        list(APPEND qt_standalone_sources "${generated}")
      endforeach()
    endforeach()

    # And all of them at once, for the reason the same extra file exists above.
    set(qt_together_text "// Generated. Every Qt header at once.\n")
    foreach(header ${rc_qt_headers})
      file(RELATIVE_PATH include_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
      string(APPEND qt_together_text "#include <${include_path}>\n")
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/qt_together.cpp" "${qt_together_text}")
    list(APPEND qt_standalone_sources "${CMAKE_CURRENT_BINARY_DIR}/standalone/qt_together.cpp")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/qt_main.cpp"
      "// Generated. The test is that this links.\n"
      "int main() { return 0; }\n")

    # The headers are listed as sources on purpose: AUTOMOC only processes a
    # header that belongs to the target.
    add_executable(rc_qt_headers_standalone
      "${CMAKE_CURRENT_BINARY_DIR}/standalone/qt_main.cpp"
      ${qt_standalone_sources}
      ${rc_qt_headers})
    set_target_properties(rc_qt_headers_standalone PROPERTIES AUTOMOC ON)
    target_link_libraries(rc_qt_headers_standalone PRIVATE
      rc::core Qt6::Core Qt6::Widgets Qt6::Qml Qt6::Network Qt6::Concurrent)
    if(MSVC)
      target_compile_options(rc_qt_headers_standalone PRIVATE /W4 /WX)
    else()
      target_compile_options(rc_qt_headers_standalone PRIVATE -Wall -Wextra -Werror)
    endif()

    add_test(NAME librc.qt-headers-compile-standalone COMMAND rc_qt_headers_standalone)
    set_property(TEST librc.qt-headers-compile-standalone PROPERTY LABELS gate)
  else()
    message(STATUS "librc: rc::qt header gate skipped, Qt6 was not found")
  endif()
endif()

# The same two checks for the Gazebo headers, with its libraries linked, where they are installed.
if(rc_gz_headers)
  find_package(gz-transport13 QUIET)
  find_package(gz-msgs10 QUIET)
  if(gz-transport13_FOUND AND gz-msgs10_FOUND)
    set(gz_standalone_sources "")
    set(gz_together_text "// Generated. Every Gazebo header at once.\n")
    foreach(header ${rc_gz_headers})
      file(RELATIVE_PATH include_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
      string(REPLACE "/" "_" flat "${include_path}")
      string(REPLACE ".hpp" "" flat "${flat}")
      foreach(copy first second)
        set(generated "${CMAKE_CURRENT_BINARY_DIR}/standalone/${flat}_${copy}.cpp")
        file(WRITE "${generated}" "#include <${include_path}>\n")
        list(APPEND gz_standalone_sources "${generated}")
      endforeach()
      string(APPEND gz_together_text "#include <${include_path}>\n")
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/gz_together.cpp" "${gz_together_text}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/standalone/gz_main.cpp" "// Generated. The test is that this links.\nint main() { return 0; }\n")
    add_executable(rc_gz_headers_standalone "${CMAKE_CURRENT_BINARY_DIR}/standalone/gz_main.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/standalone/gz_together.cpp" ${gz_standalone_sources})
    target_link_libraries(rc_gz_headers_standalone PRIVATE rc::core gz-transport13::gz-transport13 gz-msgs10::gz-msgs10)
    target_compile_options(rc_gz_headers_standalone PRIVATE -Wall -Wextra)
    add_test(NAME librc.gz-headers-compile-standalone COMMAND rc_gz_headers_standalone)
    set_property(TEST librc.gz-headers-compile-standalone PROPERTY LABELS gate)
  else()
    message(STATUS "librc: rc::gz header gate skipped, Gazebo transport was not found")
  endif()
endif()
