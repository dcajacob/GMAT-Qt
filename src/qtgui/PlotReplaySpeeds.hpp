#pragma once
#include <array>

// Shared by local animation, global animation and simulation-clock multipliers.
inline constexpr std::array<double,11> plotReplaySpeeds{
   .001,.005,.01,.025,.05,.1,.25,.5,1.,2.,4.
};
