#pragma once
#include "MatlabValue.hpp"
#include <cstddef>
#include <functional>
#include <memory>

struct GMAT_API MatlabCallbackRequest
{
   enum class Operation { EVALUATE = 0, FETCH = 1, STORE = 2, RESERVE = 3, FETCH_CHARACTER = 4,
      PUSH_OPTIMIZER = 5, POP_OPTIMIZER = 6, OPEN_MAT_FILE = 7, WRITE_MAT_FILE = 8 };
   Operation operation = Operation::EVALUATE;
   std::string name, expression;
   MatlabValue value;
};
struct GMAT_API MatlabCallbackReply
{
   bool found = false, reserved = false;
   MatlabValue value;
   std::string output;
};

// A callback-thread transport to the waiting MATLAB client. It never runs a
// vendor C-library task on the callback thread or reenters the busy C Engine.
class GMAT_API MatlabCallbackBridge
{
public:
   using Executor = std::function<MatlabCallbackReply(const MatlabCallbackRequest&)>;
   class GMAT_API Scope
   {
   public:
      Scope(); // Mask an enclosing transport for a legacy callback request.
      explicit Scope(Executor executor);
      ~Scope() noexcept;
      Scope(const Scope&) = delete;
      Scope& operator=(const Scope&) = delete;
   private:
      std::size_t identity = 0;
   };
   static bool IsActive();
   static MatlabCallbackReply Execute(const MatlabCallbackRequest &request);
   static std::shared_ptr<void> &LocalState();
};
