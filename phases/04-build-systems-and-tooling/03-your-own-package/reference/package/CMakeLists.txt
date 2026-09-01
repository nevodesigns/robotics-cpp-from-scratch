cmake_minimum_required(VERSION 3.16)
project(steplib VERSION 1.0.0 LANGUAGES CXX)

# Where headers, libraries and cmake files belong on this platform. Hard coding
# "lib" and "include" works until somebody installs on a system that uses
# lib64, and this module is two lines to include.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# A header only library is an INTERFACE target: it compiles nothing itself and
# exists to carry requirements to whoever links it.
add_library(steplib INTERFACE)

# The namespaced name, so a consumer writes steplib::steplib and gets an error
# at configure time if the package is missing, rather than a link error later
# from a plain name that happened to match something else.
add_library(steplib::steplib ALIAS steplib)

# INTERFACE means "whoever links this needs it", which is the whole point of a
# package: a consumer should not have to be told the standard or the include
# path, it should arrive with the target.
target_compile_features(steplib INTERFACE cxx_std_17)

# The two generator expressions are the heart of this file.
#
# BUILD_INTERFACE is where the headers are while this project is being built.
# INSTALL_INTERFACE is where they will be after installation, relative to
# wherever the package lands.
#
# A plain path here is refused by CMake at configure time, because it would
# export an absolute path into the source tree of the machine that built it.
# Giving only the first installs cleanly and produces a package with no include
# directory at all, which fails in the consumer's compiler rather than here.
target_include_directories(steplib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Record the target in an export set, and copy the headers.
install(TARGETS steplib EXPORT steplibTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Turn the export set into a file a consumer's find_package can read. The
# namespace here is what makes steplib::steplib exist on the other side.
install(EXPORT steplibTargets
        FILE steplibTargets.cmake
        NAMESPACE steplib::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/steplib)

# find_package(steplib) looks for a file called exactly steplibConfig.cmake.
# For a header only library it has one job: include the targets file.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/steplibConfig.cmake
     "include(\"\${CMAKE_CURRENT_LIST_DIR}/steplibTargets.cmake\")\n")

# And a version file, or find_package(steplib 1.0) reports the package as
# version unknown and refuses it, however correct everything else is.
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/steplibConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/steplibConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/steplibConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/steplib)
