# Optional exact NASA MSIS-86 adapter. Existing models retain their identities.
message(STATUS "GMAT MSISE86 Plugin setup")
include(CheckLanguage)
check_language(Fortran)
if(NOT CMAKE_Fortran_COMPILER)
  message(FATAL_ERROR "PLUGIN_MSISE86ATMOSPHERE requires a Fortran compiler")
endif()
enable_language(Fortran)
if(NOT CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "MSISE86 legacy local storage is currently qualified only with GNU Fortran")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(TargetName Msise86)
set(PLUGIN_DIRS atmosphere factory plugin)
set(model_source "${CMAKE_CURRENT_BINARY_DIR}/msis86_isolated.f")
add_custom_command(OUTPUT "${model_source}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/model/PrepareMsis86.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/model/msis86.for" "${CMAKE_CURRENT_SOURCE_DIR}/model/msis86.dat" "${model_source}"
  DEPENDS model/PrepareMsis86.py model/msis86.for model/msis86.dat VERBATIM)
set_source_files_properties("${model_source}" PROPERTIES COMPILE_OPTIONS "-std=legacy;-fno-automatic;-ffixed-line-length-none")
set(PLUGIN_SRCS atmosphere/Msise86Atmosphere.cpp plugin/GmatPluginFunctions.cpp "${model_source}")
_SETUPPLUGIN(${TargetName} "${PLUGIN_DIRS}" "${PLUGIN_SRCS}" plugins)

if(GMAT_QT_BUILD_TESTS)
  add_test(NAME QtAcceptance.NasaMsise86Reference
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/src/qtgui/tests/NasaMsise86ModelTests.py"
      --plugin "$<TARGET_FILE:Msise86>" --base "$<TARGET_FILE:GmatBase>"
      --reference "${PROJECT_SOURCE_DIR}/src/qtgui/tests/fixtures/msise86/m86tes.dat")
  set_tests_properties(QtAcceptance.NasaMsise86Reference PROPERTIES LABELS "qt-support;atmosphere" TIMEOUT 30)
endif()
