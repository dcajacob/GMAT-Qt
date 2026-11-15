#ifndef ScriptSourceCapture_hpp
#define ScriptSourceCapture_hpp

#include "gmatdefs.hpp"
#include <string>
#include <vector>

// Optional, scoped observation of the logical blocks read by the interpreter.
// The capture owns source text only; it does not own or alter configured objects.
class GMAT_API ScriptSourceCapture
{
public:
   struct Block
   {
      std::string text;
      std::string rootBlock;
      bool included;
   };

   ScriptSourceCapture() noexcept;
   ~ScriptSourceCapture() noexcept;
   ScriptSourceCapture(const ScriptSourceCapture &) = delete;
   ScriptSourceCapture &operator=(const ScriptSourceCapture &) = delete;
   ScriptSourceCapture(ScriptSourceCapture &&) = delete;
   ScriptSourceCapture &operator=(ScriptSourceCapture &&) = delete;

   const std::vector<Block> &GetBlocks() const noexcept { return blocks; }
   static void Record(const std::string &text, bool root);

private:
   ScriptSourceCapture *previous;
   std::string rootBlock;
   std::vector<Block> blocks;
};

#endif
