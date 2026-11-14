#include "MatlabLibraryExecutor.hpp"
#include "Moderator.hpp"
#include "Interpreter.hpp"

MatlabLibraryExecutor::MatlabLibraryExecutor() : caller(std::this_thread::get_id()), worker([this]{Work();}) {}

MatlabLibraryExecutor::~MatlabLibraryExecutor()
{
   {
      std::lock_guard<std::mutex> lock(mutex);
      stopping = true;
   }
   ready.notify_one();
   worker.join();
}

void MatlabLibraryExecutor::EnsureIdle() const
{
   if (OnWorker()) return;
   if (std::this_thread::get_id() != caller)
      throw InterfaceException("MATLAB requests must originate on the owning application thread");
   if (active)
      throw InterfaceException("A MATLAB evaluation is already active; a GMAT callback cannot reenter MATLAB");
}

void MatlabLibraryExecutor::ServiceUi()
{
   if (auto *ui = Moderator::GetUiInterpreter()) ui->SetInputFocus();
}

void MatlabLibraryExecutor::Work()
{
   for (;;)
   {
      std::function<void()> work;
      {
         std::unique_lock<std::mutex> lock(mutex);
         ready.wait(lock,[this]{return stopping || bool(pending);});
         if (stopping && !pending) return;
         work = std::move(pending);
         pending = {};
      }
      work(); // packaged_task transfers any exception to the synchronous caller.
   }
}

MatlabLibraryExecutor &MatlabLibraries()
{
   static MatlabLibraryExecutor executor;
   return executor;
}
