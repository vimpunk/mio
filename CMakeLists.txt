cmake_minimum_required(VERSION 3.8)

#
# Here we check whether mio is being configured in isolation or as a component
# of a larger project. To do so, we query whether the `PROJECT_NAME` CMake
# variable has been defined. In the case it has, we can conclude mio is a
# subproject.
#
# This convention has been borrowed from the Catch C++ unit testing library.
#
if(DEFINED PROJECT_NAME)
  set(subproject ON)
  if(NOT DEFINED INSTALL_SUBPROJECTS)
    set(INSTALL_SUBPROJECTS ON CACHE BOOL "Install subproject dependencies")
  endif()
else()
  set(subproject OFF)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

project(mio VERSION 1.1.0 LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

# Generate 'compile_commands.json' for clang_complete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# mio requires C++ 11 support, at a minimum. The `CMAKE_CXX_STANDARD` variable
# is referenced when a target is created to initialize the CXX_STANDARD target
# property.
#
# ** NOTE **
# This is a directory scope variable. This has several implicitations.
#
# 1. It DOES NOT propegate to parent scopes (such as parent projects)
# 2. It hides cache variables of the same name for this directory and below
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# The `mio.testing` options only appear as cmake-gui and ccmake options iff
# mio is the highest level project. In the case that mio is a subproject, these
# options are hidden from the user interface and set to `OFF`
#
# Iff mio is the highest level project, this option is defaulted to the value
# of the traditional course grain testing option `BUILD_TESTING` established by
# the CTest module
#
CMAKE_DEPENDENT_OPTION(mio.tests
  "Build the mio tests and integrate with ctest"
   ON "BUILD_TESTING; NOT subproject" OFF)

#
# On Windows, so as to be a "good citizen", mio offers two mechanisms to control
# the imported surface area of the Windows API. The default `mio` target sets
# the necessary flags for a minimal Win API (`WIN32_LEAN_AND_MEAN`, etc.) on
# linking targets. This is, in our view, the conservative behavior.
#
# However, an option is published in the cache allowing client to opt out of
# these compiler definintions. This preference will persist in the installed
# cmake configuration file, but can be modified by downstream users by way of
# the same cmake cache variable. This allows intermediate consumers (e.g. other
# libraries) to defer this decision making to downstream clients.
#
# Beyond the option-based mechanism, two additional targets,
# mio::mio_min_winapi and mio::mio_full_winapi, are specified below for those
# that expressly requiring either the minimal or full windows API, respectively.
#
CMAKE_DEPENDENT_OPTION(mio.windows.full_api
  "Configure mio without WIN32_LEAN_AND_MEAN and NOMINMAX definitions"
  OFF "WIN32" ON)

#
# When the end user is consuming mio as a nested subproject, an option
# is provided such that the user may exlude mio from the set of installed
# cmake projects. This accomodates end users building executables or
# compiled libraries which privately link to mio, but wish to only ship their
# artifacts in an installation
#
CMAKE_DEPENDENT_OPTION(mio.installation
  "Include mio in the install set"
  "${INSTALL_SUBPROJECTS}" "subproject" ON)
mark_as_advanced(mio.installation)

#
# mio has no compiled components. As such, we declare it as an `INTERFACE`
# library, which denotes a collection of target properties to be applied
# transitively to linking targets. In our case, this amounts to an include
# directory and (potentially) some preprocessor definitions.
#
add_library(mio INTERFACE)
add_library(mio::mio ALIAS mio)

#
# The include directory for mio can be expected to vary between build
# and installaion. Here we use a CMake generator expression to dispatch
# on how the configuration under which this library is being consumed.
#
# We define the generator expression as a variable, such that the logic
# need not be repeated when populating source file paths.
#
string(CONCAT prefix
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_include_directories(mio INTERFACE ${prefix})

if(NOT mio.windows.full_api)
  target_compile_definitions(mio INTERFACE
    $<BUILD_INTERFACE:WIN32_LEAN_AND_MEAN>
    $<BUILD_INTERFACE:NOMINMAX>)
endif()

if(WIN32)
  add_library(mio_full_winapi INTERFACE)
  add_library(mio::mio_full_winapi ALIAS mio_full_winapi)
  target_include_directories(mio_full_winapi INTERFACE ${prefix})

  add_library(mio_min_winapi INTERFACE)
  add_library(mio::mio_min_winapi ALIAS mio_full_winapi)
  target_compile_definitions(mio INTERFACE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(mio_min_winapi INTERFACE ${prefix})
endif()

#
# In order to collect mio's header files in IDE tools such as XCode or Visual
# Studio, there must exist a target adding any such header files as source files.
#
# Given mio is an interface target, source files may only be added with the
# INTERFACE keyword, which consequently propegate to linking targets. This
# behavior isn't desirable to all clients.
#
# To accomodate, a second target is declared which collects the header files,
# which links to the primary mio target. As such, the header files are available
# in IDE tools.
#
add_library(mio-headers INTERFACE)
add_library(mio::mio-headers ALIAS mio-headers)
target_link_libraries(mio-headers INTERFACE mio)

add_subdirectory(include/mio)

if(mio.tests)
  add_subdirectory(test)
endif()

if(mio.installation)
  #
  # Non-testing header files (preserving relative paths) are installed to the
  # `include` subdirectory of the `$INSTALL_DIR/${CMAKE_INSTALL_PREFIX}`
  # directory. Source file permissions preserved.
  #
  install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.*pp")

  #
  # As a header-only library, there are no target components to be installed
  # directly (the PUBLIC_HEADER property is not white listed for INTERFACE
  # targets for some reason).
  #
  # However, it is worthwhile export our target description in order to later
  # generate a CMake configuration file for consumption by CMake's `find_package`
  # intrinsic
  #
  install(TARGETS mio mio-headers EXPORT mioTargets)

  if(WIN32)
    install(TARGETS mio_full_winapi mio_min_winapi EXPORT mioTargets)
  endif()

  install(EXPORT mioTargets
    FILE mio-targets.cmake
    NAMESPACE mio::
    DESTINATION share/cmake/mio)

  write_basic_package_version_file("mio-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/mio-config.cmake.in"
    "${PROJECT_BINARY_DIR}/mio-config.cmake"
    @ONLY)

  install(FILES
    "${PROJECT_BINARY_DIR}/mio-config-version.cmake"
    "${PROJECT_BINARY_DIR}/mio-config.cmake"
    DESTINATION share/cmake/mio)

  #
  # Rudimentary CPack support.
  #
  # CPack provides a mechanism to generate installation packaging for a project,
  # e.g., self-extracting shell scripts, compressed tarballs, Debian Package files,
  # RPM Package Manager files, Windows NSIS installation wizards,
  # Apple Disk Images (.dmg), etc.
  #
  # A packaged installation can be generated by calling
  #
  # ```sh
  # cpack -G <packaging type> --config CPackConfig.cmake
  # ```
  #
  # See `cpack --help` or the CPack documentation for more information.
  #
  if(NOT subproject)
    set(CPACK_PACKAGE_VENDOR "mandreyel")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
      "Cross-platform C++11 header-only library for memory mapped file IO")
    set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/mandreyel/mio")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
    include(CPack)
  endif()
endif()
