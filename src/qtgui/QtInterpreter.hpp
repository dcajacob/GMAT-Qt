#pragma once

#include "ScriptInterpreter.hpp"
#include <QElapsedTimer>
#include <memory>
class QtMatlabCallbackServer;

// GMAT calls SetInputFocus at its existing interruption checkpoints. Keeping
// execution and event delivery on this thread avoids concurrent engine access.
class QtInterpreter final : public ScriptInterpreter
{
public:
   QtInterpreter();
   ~QtInterpreter() override;
   void SetInputFocus() override;
   void StartMatlabServer() override;
private:
   QElapsedTimer eventClock;
   std::unique_ptr<QtMatlabCallbackServer> matlabCallbacks;
};
