#pragma once

// One owner for the non-thread-safe MATLAB C libraries in the Linux Qt build.
// The caller remains synchronous while its UI services GMAT callbacks. A callback
// cannot issue another MATLAB request while the pending evaluation owns MATLAB.
#include "InterfaceException.hpp"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

class MatlabLibraryExecutor
{
public:
   MatlabLibraryExecutor();
   ~MatlabLibraryExecutor();
   bool OnWorker() const { return std::this_thread::get_id() == worker.get_id(); }
   void EnsureIdle() const;

   template<class F> auto Invoke(F function) -> std::invoke_result_t<F>
   {
      EnsureIdle();
      using Result = std::invoke_result_t<F>;
      auto task = std::make_shared<std::packaged_task<Result()>>(std::move(function));
      auto future = task->get_future();
      struct ActiveGuard { bool &flag; explicit ActiveGuard(bool &f):flag(f) {flag=true;} ~ActiveGuard(){flag=false;} } guard(active);
      {
         std::lock_guard<std::mutex> lock(mutex);
         pending = [task] { (*task)(); };
      }
      ready.notify_one();
      while (future.wait_for(std::chrono::milliseconds(2)) != std::future_status::ready)
         ServiceUi();
      return future.get();
   }
private:
   void ServiceUi();
   void Work();
   const std::thread::id caller;
   std::mutex mutex;
   std::condition_variable ready;
   std::function<void()> pending;
   bool stopping = false;
   bool active = false; // Accessed only by the calling/UI thread.
   std::thread worker;
};

MatlabLibraryExecutor &MatlabLibraries();
