# $Id$
# 
# GMAT: General Mission Analysis Tool.
# 
# CMAKE script file for GMAT FminconOptimizer Plugin.
# (Adapted from the CInterface plugin CMake script)
# (NOTE FROM RAVI: This needs debugging due to need for Matlab Optimization Toolbox
# This file must be installed in the plugins/FminconOptimizerPlugin/src directory
#
# Author: Ravi Mathur
#  
# DO NOT MODIFY THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING!
#

MESSAGE("==============================")
MESSAGE("GMAT FminconOptimizer Plugin setup " ${VERSION}) 

# ====================================================================
# Major error checks
if(NOT WIN32 AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI))
  MESSAGE(WARNING "Skipping FminconOptimizer: Not supported on ${CMAKE_SYSTEM_NAME}")
  RETURN()
endif()

if(NOT PLUGIN_MATLABINTERFACE)
  MESSAGE(WARNING "Skipping FminconOptimizer: MatlabInterface Plugin disabled")
  RETURN()
endif()

if(NOT Matlab_FOUND)
  MESSAGE(WARNING "Skipping FminconOptimizer: Matlab missing")
  RETURN()
endif()

# ====================================================================
# Define plugin target
SET(TargetName FminconOptimizer)

# ====================================================================
# list of directories containing source/header files
SET(PLUGIN_DIRS factory include plugin solver)

# ====================================================================
# source files
SET(PLUGIN_SRCS
    factory/FminconOptimizerFactory.cpp
    plugin/GmatPluginFunctions.cpp
    solver/FminconOptimizer.cpp
)

# ====================================================================
# Setup common plugin definitions, targets, etc.
_SETUPPLUGIN(${TargetName} "${PLUGIN_DIRS}" "${PLUGIN_SRCS}" plugins)

# Rename DLL export preproccessor definition
SET_TARGET_PROPERTIES(${TargetName} PROPERTIES DEFINE_SYMBOL "FMINCON_EXPORTS")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI)
  TARGET_COMPILE_DEFINITIONS(${TargetName} PRIVATE GMAT_QT_FMINCON_CALLBACK)
endif()

# ====================================================================
# Additional includes
TARGET_INCLUDE_DIRECTORIES(${TargetName} PRIVATE ${Matlab_INCLUDE_DIRS})

# ====================================================================
# Additional link libraries
TARGET_LINK_LIBRARIES(${TargetName} PRIVATE MatlabInterface)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GMAT_INCLUDE_QT_GUI)
  TARGET_LINK_LIBRARIES(${TargetName} PRIVATE ${Matlab_ENG_LIBRARY} ${Matlab_MX_LIBRARY})
else()
  TARGET_LINK_LIBRARIES(${TargetName} PRIVATE ${Matlab_LIBRARIES})
endif()
