# $Id$
# 
# GMAT: General Mission Analysis Tool.
# 
# CMAKE script file for GMAT MatlabInterface Plugin.
# (Adapted from the CInterface plugin CMake script)
# (NOTE FROM RAVI: This needs debugging due to need for Matlab Optimization Toolbox)
# This file must be installed in the plugins/MatlabInterfacePlugin/src directory
#
# Author: Ravi Mathur
#  
# DO NOT MODIFY THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING!
#

MESSAGE("==============================")
MESSAGE("GMAT MatlabInterface Plugin setup " ${VERSION}) 

# ====================================================================
# Major error checks
if(NOT Matlab_FOUND)
  MESSAGE(WARNING "Skipping MatlabInterface: Matlab missing")
  RETURN()
endif()

# The Unix wx client sources provide the legacy MATLAB-to-wx-GMAT IPC path.
# GMAT-to-MATLAB Engine calls do not depend on them. Build that Engine path
# for the Linux Qt application without adding the legacy wx application client.
SET(MATLAB_USE_WX_IPC ${UNIX})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI)
  SET(MATLAB_USE_WX_IPC FALSE)
endif()

if(MATLAB_USE_WX_IPC AND (NOT wxWidgets_FOUND))
  MESSAGE(WARNING "Skipping MatlabInterface: wxWidgets missing")
  RETURN()
endif()

SET(TargetName MatlabInterface)

# ====================================================================
# list of directories containing source/header files
SET(PLUGIN_DIRS command factory function include interface matwriter plugin subscriber)
if(MATLAB_USE_WX_IPC)
  SET(PLUGIN_DIRS ${PLUGIN_DIRS} "../matlab/gmat_mex/src")
endif()

# ====================================================================
# source files
SET(PLUGIN_SRCS
    command/CallMatlabFunction.cpp
    factory/CallMatlabFunctionFactory.cpp
    factory/MatlabFunctionFactory.cpp
    factory/MatlabInterfaceFactory.cpp
    factory/MatlabWorkspaceFactory.cpp
    function/MatlabFunction.cpp
    interface/MatlabInterface.cpp
    plugin/GmatPluginFunctions.cpp
    subscriber/MatlabWorkspace.cpp
    matwriter/MatWriter.cpp
    matwriter/MatWriterMaker.cpp
    matwriter/MatData.cpp
    matwriter/RealMatData.cpp
    matwriter/StringMatData.cpp
    )
if(MATLAB_USE_WX_IPC)
  SET(PLUGIN_SRCS
    ${PLUGIN_SRCS}
    ../matlab/gmat_mex/src/MatlabClient.cpp
    ../matlab/gmat_mex/src/MatlabConnection.cpp
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI)
  LIST(APPEND PLUGIN_SRCS interface/MatlabLibraryExecutor.cpp)
endif()

# ====================================================================
# Setup common plugin definitions, targets, etc.
_SETUPPLUGIN(${TargetName} "${PLUGIN_DIRS}" "${PLUGIN_SRCS}" plugins)

# Rename DLL export preproccessor definition
SET_TARGET_PROPERTIES(${TargetName} PROPERTIES DEFINE_SYMBOL "MATLAB_EXPORTS")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI)
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_COMPILE_DEFINITIONS(${TargetName} PRIVATE GMAT_MATLAB_QT_ENGINE)
  TARGET_LINK_LIBRARIES(${TargetName} PRIVATE Threads::Threads)
endif()

# Enforce the available Matlab architecture on macOS
SET_TARGET_PROPERTIES(${TargetName} PROPERTIES OSX_ARCHITECTURES "${Matlab_OSX_ARCHITECTURE}")

# ====================================================================
# Includes
TARGET_INCLUDE_DIRECTORIES(${TargetName} PRIVATE ${Matlab_INCLUDE_DIRS})
if(MATLAB_USE_WX_IPC)
  TARGET_INCLUDE_DIRECTORIES(${TargetName} PRIVATE
    "${PROJECT_SOURCE_DIR}/src/gui/include"
    "${PROJECT_SOURCE_DIR}/src/gui/app"
    )
  INCLUDE(${wxWidgets_USE_FILE})
endif()

# ====================================================================
# Additional link libraries
# This plugin uses the C Engine, Matrix and MAT-file APIs. Linking every
# discovered MATLAB library also pulls in the unused C++ Engine/MEX runtimes
# and their bundled Qt dependencies, which conflict with the host Qt build.
TARGET_LINK_LIBRARIES(${TargetName} PRIVATE
  ${Matlab_ENG_LIBRARY} ${Matlab_MX_LIBRARY} ${Matlab_MAT_LIBRARY})
if(MATLAB_USE_WX_IPC)
  TARGET_LINK_LIBRARIES(${TargetName} PUBLIC ${wxWidgets_LIBRARIES})
endif()

# Set RPATH to find shared libraries in default locations on Mac/Linux
if(UNIX)
  if(APPLE)
    SET(MAC_BASEPATH "../Frameworks/") # GmatBase and wxLibs in Frameworks
    SET_TARGET_PROPERTIES(${TargetName} PROPERTIES INSTALL_RPATH
      "@loader_path/${MAC_BASEPATH}"
      )
  else()
    SET_TARGET_PROPERTIES(${TargetName} PROPERTIES INSTALL_RPATH
      "\$ORIGIN/;\$ORIGIN/../lib/"
      )
  endif()
endif()

