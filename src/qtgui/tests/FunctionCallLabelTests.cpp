#include "CallBuiltinGmatFunction.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main()
{
   auto *global = GmatGlobal::Instance();
   struct Restore
   {
      GmatGlobal *global;
      bool previous;
      ~Restore() { global->SetWriteGmatKeyword(previous); }
   } restore{global, global->IsWritingGmatKeyword()};
   try
   {
      int cases = 0;
      for (bool keyword : {false, true})
         for (auto mode : {Gmat::NO_COMMENTS, Gmat::SCRIPTING})
            for (bool output : {false, true})
            {
               global->SetWriteGmatKeyword(keyword);
               CallBuiltinGmatFunction command;
               command.SetName("Named builtin assignment");
               command.SetStringParameter(command.GetParameterID("FunctionName"), "Helper");
               command.SetStringParameter(command.GetParameterID("AddInput"), "A");
               command.SetStringParameter(command.GetParameterID("AddInput"), "B");
               if (output)
                  command.SetStringParameter(command.GetParameterID("AddOutput"), "Result");
               const std::string expected =
                  (mode == Gmat::NO_COMMENTS ? std::string() :
                     std::string("   ") + (keyword ? "GMAT " : "")) +
                  "'Named builtin assignment' " + (output ? "[Result] = " : "") +
                  "Helper(A, B);";
               for (int repeat = 0; repeat < 2; ++repeat)
                  if (command.GetGeneratingString(mode, "   ") != expected)
                     throw std::runtime_error("Builtin label/prefix/output/argument serialization differs: " +
                        command.GetGeneratingString(mode, "   "));
               ++cases;
            }
#ifdef __linux__
      if (const char *directory = std::getenv("GMAT_QT_ACCEPTANCE_DIR"))
      {
         std::ifstream maps("/proc/self/maps");
         std::ofstream snapshot(std::string(directory) + "/function-label-self-maps.txt");
         snapshot << maps.rdbuf();
         if (!snapshot)
            throw std::runtime_error("Label support self maps were not retained");
      }
#endif
      std::cout << "NASA_CHECK {\"checkpoint\":\"builtin-function-label-serialization\","
                   "\"outcome\":\"passed\",\"cases\":" << cases <<
                   ",\"exact_serializations\":16,\"new_requirement_credit\":0}" << std::endl;
      return 0;
   }
   catch (BaseException &error)
   {
      std::cerr << error.GetFullMessage() << std::endl;
   }
   catch (std::exception &error)
   {
      std::cerr << error.what() << std::endl;
   }
   return 1;
}
