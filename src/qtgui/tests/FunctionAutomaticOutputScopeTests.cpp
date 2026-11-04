#include "FileUtil.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
struct Case
{
   const char *name;
   std::string body;
   StringArray outputs;
   StringArray automaticObjects;
   std::string error;
   WrapperTypeArray types;
   IntegerArray rows, cols;
};
}

int main()
{
   const Case cases[] = {
      {"existing-automatic-object", "BeginMissionSequence;\n", {"Shared"},
       {"Shared"}, "", {Gmat::OBJECT_WT}, {-1}, {-1}},
      {"mixed-local-and-automatic", "Create Variable Mark;\nBeginMissionSequence;\n",
       {"Mark", "Shared"}, {"Shared"}, "",
       {Gmat::VARIABLE_WT, Gmat::OBJECT_WT}, {-1, -1}, {-1, -1}},
      {"local-array-shadows-automatic", "Create Array Shared[2,3];\n",
       {"Shared"}, {"Shared"}, "", {Gmat::ARRAY_WT}, {2}, {3}},
      {"local-variable-shadows-automatic", "Create Variable Shared;\n",
       {"Shared"}, {"Shared"}, "", {Gmat::VARIABLE_WT}, {-1}, {-1}},
      {"duplicate-local-still-rejected",
       "Create Variable Shared;\nCreate Variable Shared;\n", {"Shared"},
       {"Shared"}, "Multiple declaration", {}, {}, {}},
      {"duplicate-output-still-rejected", "BeginMissionSequence;\n",
       {"Shared", "Shared"}, {"Shared"}, "Duplicate output", {}, {}, {}},
      {"unapproved-name-still-missing", "BeginMissionSequence;\n", {"Missing"},
       {"Shared"}, "Missing output declaration", {}, {}, {}},
      {"later-create-cannot-rescue-unapproved",
       "BeginMissionSequence;\nfunction [Missing] = Ignored(input)\nCreate Variable Missing;\n",
       {"Missing"}, {"Shared"}, "Missing output declaration", {}, {}, {}},
      {"later-global-cannot-rescue-unapproved",
       "BeginMissionSequence;\nfunction [Missing] = Ignored(input)\nGlobal Missing;\n",
       {"Missing"}, {"Shared"}, "Missing output declaration", {}, {}, {}},
      {"later-shape-cannot-change-automatic",
       "BeginMissionSequence;\nfunction [Shared] = Ignored(input)\nCreate Array Shared[4,5];\n",
       {"Shared"}, {"Shared"}, "", {Gmat::OBJECT_WT}, {-1}, {-1}}
   };
   int failed = 0;
   for (const auto &test : cases)
   {
      try
      {
         std::istringstream stream(test.body);
         std::string error;
         IntegerArray rows, cols;
         auto types = GmatFileUtil::GetFunctionOutputTypes(&stream, {}, test.outputs,
               error, rows, cols, test.automaticObjects);
         if (test.error.empty())
         {
            if (!error.empty() || types != test.types || rows != test.rows || cols != test.cols)
               throw std::runtime_error("Incorrect automatic output metadata: " + error);
         }
         else if (error.find(test.error) == std::string::npos)
            throw std::runtime_error("Expected declaration rejection absent: " + error);
         std::cout << "PASS " << test.name << '\n';
      }
      catch (const std::exception &e)
      {
         ++failed;
         std::cerr << "FAIL " << test.name << ": " << e.what() << '\n';
      }
   }
   // A formal input can be a scalar, array or object. Its runtime wrapper must
   // continue to determine the type even when the caller has an automatic
   // object with the same spelling; do not force OBJECT_WT from that object.
   for (const std::string body : {"BeginMissionSequence;\n", "Global input;\n"})
   {
      std::istringstream oldStream(body), newStream(body);
      std::string oldError, newError;
      IntegerArray oldRows, oldCols, newRows, newCols;
      auto oldTypes = GmatFileUtil::GetFunctionOutputTypes(&oldStream, {"input"},
            {"input"}, oldError, oldRows, oldCols);
      auto newTypes = GmatFileUtil::GetFunctionOutputTypes(&newStream, {"input"},
            {"input"}, newError, newRows, newCols, {"input"});
      if (oldError != newError || oldTypes != newTypes || oldRows != newRows || oldCols != newCols)
      {
         ++failed;
         std::cerr << "FAIL formal-input-precedence\n";
      }
      else
         std::cout << "PASS formal-input-precedence\n";
   }
   std::cout << "Automatic-output scope cases: " << 12 - failed << "/12 passed\n";
   return failed ? 1 : 0;
}
