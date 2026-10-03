#pragma once
#include <sstream>

// Interpret a Qt mission while retaining a configured FOV search expression.
bool qtInterpretScript(std::istringstream &script);
