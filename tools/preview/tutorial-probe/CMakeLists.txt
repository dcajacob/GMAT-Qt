cmake_minimum_required(VERSION 3.21)
project(GmatPackagedTutorialProbe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
get_filename_component(GMAT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(GMAT_INSTALLATION "" CACHE PATH "Extracted GMAT Qt release installation")
set(CSPICE_DIR "" CACHE PATH "CSPICE headers matching the release")
if(NOT IS_DIRECTORY "${GMAT_INSTALLATION}")
   message(FATAL_ERROR "GMAT_INSTALLATION must name an extracted release")
endif()

if(APPLE)
   set(gmat_binary_dir "${GMAT_INSTALLATION}/Contents/MacOS")
else()
   set(gmat_binary_dir "${GMAT_INSTALLATION}/bin")
endif()

file(GLOB gmat_base_directories LIST_DIRECTORIES true "${GMAT_SOURCE_DIR}/src/base/*")
file(GLOB gmat_util_directories LIST_DIRECTORIES true "${GMAT_SOURCE_DIR}/src/gmatutil/*")
set(gmat_header_directories "${GMAT_SOURCE_DIR}/src/base/forcemodel/harmonic"
   "${GMAT_SOURCE_DIR}/src/base/factory/guicomponents"
   "${GMAT_SOURCE_DIR}/src/gmatutil/util/datawriter"
   "${GMAT_SOURCE_DIR}/src/gmatutil/util/interpolator"
   "${GMAT_SOURCE_DIR}/src/gmatutil/util/matrixoperations")
list(APPEND gmat_header_directories "${CSPICE_DIR}/include")
foreach(directory IN LISTS gmat_base_directories gmat_util_directories)
   if(IS_DIRECTORY "${directory}")
      list(APPEND gmat_header_directories "${directory}")
   endif()
endforeach()

foreach(name GmatBase GmatUtil)
   if(WIN32)
      find_library(gmat_${name} NAMES ${name} lib${name} PATHS "${GMAT_IMPORT_LIBRARIES}" NO_DEFAULT_PATH REQUIRED)
   else()
      find_library(gmat_${name} NAMES ${name} PATHS "${gmat_binary_dir}" NO_DEFAULT_PATH REQUIRED)
   endif()
   add_library(${name} UNKNOWN IMPORTED)
   set_target_properties(${name} PROPERTIES
      IMPORTED_LOCATION "${gmat_${name}}"
      INTERFACE_INCLUDE_DIRECTORIES "${gmat_header_directories}")
endforeach()

add_compile_definitions(__USE_SPICE__ USE_64_BIT_LONGS UNICODE _UNICODE)
if(WIN32)
   add_compile_options(/Zc:__cplusplus)
else()
   add_compile_definitions(LINUX_MAC NO_GCC_PRAGMA)
endif()
set(GMAT_QT_TUTORIAL_ONLY ON)
add_subdirectory("${GMAT_SOURCE_DIR}/src/qtgui" frontend)
