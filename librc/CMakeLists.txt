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
