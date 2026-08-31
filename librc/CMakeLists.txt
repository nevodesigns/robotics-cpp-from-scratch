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

# The same two checks for the Qt headers, with Qt linked and AUTOMOC on so that
# a Q_OBJECT class is compiled the way a real target compiles it. A header
# declaring Q_OBJECT and never reaching moc links fine until somebody connects
# to one of its signals, which is E-QT-0001 and is worth catching here rather
# than there.
if(rc_qt_headers)
  find_package(Qt6 QUIET COMPONENTS Core Widgets)
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
    target_link_libraries(rc_qt_headers_standalone PRIVATE rc::core Qt6::Core Qt6::Widgets)
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
