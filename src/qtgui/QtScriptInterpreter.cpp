#include "QtScriptInterpreter.hpp"
#include "FunctionValidation.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "UtilityException.hpp"
#include <optional>

bool qtInterpretScript(std::istringstream &script)
{
   auto *files=FileManager::Instance();
   std::optional<std::string> fovPath;
   try { fovPath=files->GetPathDefinition("FOV_DATA_PATH"); }
   catch (const UtilityException &) { } // No configured path: retain engine defaults.
   const auto restore=[&] { if (fovPath) files->AddFileType("FOV_DATA_PATH",*fovPath); };
   try {
      // CustomFOV construction installs its default path. Restore only this
      // existing setting, retaining aliases and all other interpreter effects.
      const bool result=Moderator::Instance()->InterpretScript(&script,true);
      if (result) {
         try { qtValidateFunctionBodies(script.str()); }
         catch (const std::exception &error) { throw QtFunctionValidationError(error.what()); }
      }
      restore();
      return result;
   } catch (...) {
      restore();
      throw;
   }
}
