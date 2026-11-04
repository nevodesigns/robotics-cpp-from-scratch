# The repository's own tool. It builds before anything checks anything, which is
# also the first proof that the learner's toolchain works.

add_executable(rcpp
  main.cpp
  util.cpp
  json.cpp
  catalog.cpp
  atlas.cpp
  cmd_doctor.cpp
  cmd_audit.cpp
  cmd_verify.cpp
  cmd_explain.cpp
  cmd_catalog.cpp
  cmd_readme.cpp
  cmd_next.cpp
  cmd_platforms.cpp
  cmd_targets.cpp
  progress.cpp
)

target_compile_features(rcpp PRIVATE cxx_std_17)
target_include_directories(rcpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The tool reads its own command line with rc/core/args.hpp, the header lesson
# 01-13 graduates, which also means the tool is the first consumer of librc and
# would stop building if a graduated header stopped compiling. That is the
# intended trade: this curriculum teaches one language and one library, and a
# tool that does not use them is a tool that can drift from them.
#
# rc_core is an INTERFACE library, so this adds include directories and nothing
# to link, and librc is configured before this directory.
target_link_libraries(rcpp PRIVATE rc::core)

if(MSVC)
  target_compile_options(rcpp PRIVATE /W4)
else()
  target_compile_options(rcpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(rcpp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
