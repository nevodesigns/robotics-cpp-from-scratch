cmake_minimum_required(VERSION 3.16)
project(steplib VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# TODO: build this header only library and install it as a package that another
# project can find by name.
#
# There are six pieces, and each one has a specific failure when it is missing.
#
#   1. An INTERFACE library, since there is nothing to compile, plus an ALIAS
#      with a namespace so consumers write steplib::steplib.
#
#   2. Its requirements, as INTERFACE, so they arrive with the target rather
#      than having to be repeated by everyone who links it. The language
#      standard is one of them.
#
#   3. The include directory, described twice: where the headers are now, and
#      where they will be after installation. Two generator expressions,
#      BUILD_INTERFACE and INSTALL_INTERFACE. A plain path is refused at
#      configure time; only the first of the two installs cleanly and produces
#      a package with no include directory at all.
#
#   4. install() for the target into an export set, and for the headers.
#
#   5. install(EXPORT ...) to turn the export set into a file find_package can
#      read, with NAMESPACE so steplib::steplib exists on the other side.
#
#   6. A file named exactly steplibConfig.cmake, which for a header only
#      library only has to include the targets file, and a version file, or
#      find_package(steplib 1.0) reports the package as version unknown and
#      refuses it however correct the rest is.
