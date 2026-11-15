#include "QtScriptInterpreter.hpp"
#include "FunctionValidation.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "UtilityException.hpp"
#include "ScriptSourceCapture.hpp"
#include <memory>
#include <optional>

namespace {
struct CapturedMissionSource
{
   QString mainSource;
   QtScriptSource source;
};
thread_local std::shared_ptr<const CapturedMissionSource> capturedMissionSource;
}

std::optional<QtScriptSource> qtCapturedScriptSource(const QString &mainSource)
{
   if (!capturedMissionSource || capturedMissionSource->mainSource!=mainSource) return std::nullopt;
   return capturedMissionSource->source;
}

bool qtInterpretScript(std::istringstream &script)
{
   capturedMissionSource.reset();
   auto *files=FileManager::Instance();
   std::optional<std::string> fovPath;
   try { fovPath=files->GetPathDefinition("FOV_DATA_PATH"); }
   catch (const UtilityException &) { } // No configured path: retain engine defaults.
   const auto restore=[&] { if (fovPath) files->AddFileType("FOV_DATA_PATH",*fovPath); };
   try {
      // CustomFOV construction installs its default path. Restore only this
      // existing setting, retaining aliases and all other interpreter effects.
      bool result;
      std::shared_ptr<CapturedMissionSource> candidate;
      {
         ScriptSourceCapture capture;
         result=Moderator::Instance()->InterpretScript(&script,true);
         if (result) {
            candidate=std::make_shared<CapturedMissionSource>();
            candidate->mainSource=QString::fromStdString(script.str());
            for (const auto &block:capture.GetBlocks()) {
               auto &source=candidate->source;
               const auto begin=source.text.size();
               source.text+=QString::fromStdString(block.text);
               if (!source.text.endsWith('\n')) source.text+='\n';
               source.blocks.append({begin,source.text.size(),QString::fromStdString(block.rootBlock),block.included});
            }
         }
      } // End observation before function-body validation or subsequent rebuilds.
      if (result) {
         try { qtValidateFunctionBodies(script.str()); }
         catch (const std::exception &error) { throw QtFunctionValidationError(error.what()); }
      }
      restore();
      if (result) capturedMissionSource=std::move(candidate);
      return result;
   } catch (...) {
      restore();
      throw;
   }
}
