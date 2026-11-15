cmake_minimum_required(VERSION 3.24)
project(GmatPythonRange LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(Python3 ${GMAT_PYTHON_VERSION} EXACT REQUIRED COMPONENTS Interpreter Development.Module)
find_package(SWIG 4.5 REQUIRED)
include(UseSWIG)

# Link new wrappers to the exact libraries extracted from the qualified archive.
# No GMAT engine or desktop targets are compiled here.
foreach(native GmatUtil GmatBase Station GmatEstimation)
  add_library(${native} SHARED IMPORTED)
  set_target_properties(${native} PROPERTIES IMPORTED_LOCATION "${GMAT_LIBRARY_${native}}")
  if(WIN32)
    set_target_properties(${native} PROPERTIES IMPORTED_IMPLIB "${GMAT_IMPLIB_${native}}")
  endif()
endforeach()

set(wrapper_includes "${GMAT_SOURCE}/swig")
file(GLOB_RECURSE headers LIST_DIRECTORIES FALSE
  "${GMAT_SOURCE}/src/base/*.hpp" "${GMAT_SOURCE}/src/gmatutil/*.hpp"
  "${GMAT_SOURCE}/plugins/StationPlugin/src/*.hpp"
  "${GMAT_SOURCE}/plugins/EstimationPlugin/src/*.hpp")
foreach(header IN LISTS headers)
  get_filename_component(directory "${header}" DIRECTORY)
  list(APPEND wrapper_includes "${directory}")
endforeach()
list(REMOVE_DUPLICATES wrapper_includes)
list(APPEND wrapper_includes "${GMAT_SOURCE}/plugins/StationPlugin/swig"
  "${GMAT_SOURCE}/plugins/EstimationPlugin/swig")
include_directories(${wrapper_includes} "${CSPICE_INCLUDE_DIR}")
add_compile_definitions(_DYNAMICLINK EXP_STL __USE_SPICE__ NO_GCC_PRAGMA UNICODE _UNICODE USE_64_BIT_LONGS)
if(WIN32)
  add_compile_definitions(NOMINMAX XERCES_STATIC_LIBRARY)
  add_compile_options(/bigobj /Zc:__cplusplus)
else()
  add_compile_definitions(LINUX_MAC)
endif()

foreach(module gmat station navigation)
  if(module STREQUAL "gmat")
    set(interface "${GMAT_SOURCE}/swig/gmat_py.i")
    set(native GmatBase)
  elseif(module STREQUAL "station")
    set(interface "${GMAT_SOURCE}/plugins/StationPlugin/swig/station_py.i")
    set(native Station)
  else()
    set(interface "${GMAT_SOURCE}/plugins/EstimationPlugin/swig/navigation_py.i")
    set(native GmatEstimation)
  endif()
  set_source_files_properties("${interface}" PROPERTIES CPLUSPLUS ON USE_SWIG_DEPENDENCIES TRUE
    SWIG_FLAGS "-doxygen;-w516,509" USE_TARGET_INCLUDE_DIRECTORIES TRUE)
  set(CMAKE_SWIG_OUTDIR "${GMAT_OUTPUT}")
  set(SWIG_OUTFILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${module}-wrapper")
  swig_add_library(${module}_py LANGUAGE python SOURCES "${interface}")
  target_link_libraries(${module}_py PRIVATE Python3::Module GmatUtil GmatBase ${native})
  set_target_properties(${module}_py PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${GMAT_OUTPUT}"
    RUNTIME_OUTPUT_DIRECTORY "${GMAT_OUTPUT}" LIBRARY_OUTPUT_DIRECTORY_RELEASE "${GMAT_OUTPUT}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${GMAT_OUTPUT}")
  if(APPLE)
    set_target_properties(${module}_py PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "@loader_path/../../../Frameworks;@loader_path/../../../PlugIns")
  elseif(UNIX)
    set_target_properties(${module}_py PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "$ORIGIN/../../;$ORIGIN/../../../plugins;$ORIGIN/../../../lib")
  endif()
endforeach()
