#pragma once
#include <string>
#include <stdexcept>
class QtFunctionValidationError : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};
// Parse every configured GMAT function in a separate engine process. Function
// commands are never executed and the live configured object map is untouched.
void qtValidateFunctionBodies(const std::string &mission);
// Internal process entry point, selected before constructing the GUI.
int qtFunctionValidationWorker(int argc,char **argv);
