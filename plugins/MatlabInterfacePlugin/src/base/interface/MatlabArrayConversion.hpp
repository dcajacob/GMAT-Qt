#pragma once
#include "matlabinterface_defs.hpp"
#include "MatlabValue.hpp"
#include "matrix.h"

// Returns an owning native array. The caller must destroy it on the MATLAB
// library worker in Qt builds. This routine neither opens nor uses an engine.
MATLAB_API mxArray *CreateMatlabArray(const MatlabValue &value);
