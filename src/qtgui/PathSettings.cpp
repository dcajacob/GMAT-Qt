#include "PathSettings.hpp"
#include "QtMessageReceiver.hpp"
#include "StartupCompatibility.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <functional>
#include <stdexcept>
#include <vector>
namespace {
QString read(const QString &path)
{
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(file.errorString().toStdString());
   return QString::fromUtf8(file.readAll());
}
const QStringList scientificKeys{"LEAP_SECS_FILE","EOP_FILE","NUTATION_COEFF_FILE","LSK_FILE"};
QMap<QString,QString> scientificFiles()
{
   QMap<QString,QString> result;
   for (const auto &key:scientificKeys)
      result.insert(key,QDir::cleanPath(QString::fromStdString(FileManager::Instance()->GetFullPathname(key.toStdString()))));
   return result;
}
QString withScientificFiles(const QString &text,const QMap<QString,QString> &files)
{
   if (files.isEmpty()) return text;
   const QRegularExpression assignment("^\\s*(LEAP_SECS_FILE|EOP_FILE|NUTATION_COEFF_FILE|LSK_FILE)\\s*=.*$");
   QStringList result,seen;
   for (const auto &line:text.split('\n')) {
      const auto match=assignment.match(line); const auto key=match.captured(1);
      if (!match.hasMatch() || !files.contains(key)) { result.append(line); continue; }
      if (!seen.contains(key)) result.append(key+" = "+files.value(key));
      seen.append(key);
   }
   for (auto it=files.cbegin();it!=files.cend();++it)
      if (!seen.contains(it.key())) result.append(it.key()+" = "+it.value());
   return result.join('\n');
}
QStringList functionPaths()
{
   QStringList paths,seen;
   for (const auto &path:FileManager::Instance()->GetAllGmatFunctionPaths()) {
      const auto absolute=QDir::cleanPath(QFileInfo(QString::fromStdString(path)).absoluteFilePath());
      const auto canonical=QFileInfo(absolute).canonicalFilePath();
      const auto key=canonical.isEmpty() ? absolute : canonical;
      // Engine callers may add both slash spellings of the same directory.
      // Keep its first search position when presenting and saving the list.
      if (!seen.contains(key)) { seen.append(key); paths.append(absolute); }
   }
   return paths;
}
QStringList pythonPaths()
{
   QStringList paths,seen;
   for (const auto &path:FileManager::Instance()->GetAllPythonModulePaths()) {
      const auto absolute=QDir::cleanPath(QFileInfo(QString::fromStdString(path)).absoluteFilePath());
      const auto canonical=QFileInfo(absolute).canonicalFilePath();
      const auto key=canonical.isEmpty() ? absolute : canonical;
      if (!seen.contains(key)) { seen.append(key); paths.append(absolute); }
   }
   return paths;
}
QStringList matlabPaths()
{
   QStringList paths,seen;
   for (const auto &path:FileManager::Instance()->GetAllMatlabFunctionPaths()) {
      const auto absolute=QDir::cleanPath(QFileInfo(QString::fromStdString(path)).absoluteFilePath());
      const auto canonical=QFileInfo(absolute).canonicalFilePath();
      const auto key=canonical.isEmpty() ? absolute : canonical;
      if (!seen.contains(key)) { seen.append(key); paths.append(absolute); }
   }
   return paths;
}
const QRegularExpression pythonAssignment("^\\s*PYTHON_MODULE_PATH\\s*=\\s*(.*?)\\s*$");
QStringList startupPythonPaths(const QString &text)
{
   QStringList paths,seen;
   for (const auto &line:text.split('\n')) {
      const auto match=pythonAssignment.match(line); if (!match.hasMatch()) continue;
      const auto path=QDir::cleanPath(QString::fromStdString(FileManager::Instance()->ConvertToAbsPath(match.captured(1).toStdString())));
      const auto canonical=QFileInfo(path).canonicalFilePath();
      const auto key=canonical.isEmpty() ? path : canonical;
      if (!seen.contains(key)) { seen.append(key); paths.append(path); }
   }
   return paths;
}
// FileManager parsing also changes global UI modes and the message receiver.
// Previewing another startup must restore all three, including unknown file
// aliases and ordered include/Python paths that serialization cannot restore.
class StartupTransaction
{
public:
   explicit StartupTransaction(QtMessageReceiver &receiver): receiver(receiver), state(FileManager::Instance()->CaptureState()),
      logFile(receiver.GetLogFileName()), logEnabled(receiver.GetLogEnable()), cwd(QDir::currentPath())
   {
      auto *global=GmatGlobal::Instance();
      auto remember=[&](auto getter,auto setter) {
         const auto value=(global->*getter)();
         globals.emplace_back([=] { (global->*setter)(value); });
      };
      remember(&GmatGlobal::GetRunMode,&GmatGlobal::SetRunMode);
      remember(&GmatGlobal::InTestingMode,&GmatGlobal::SetTestingMode);
      remember(&GmatGlobal::IsBatchMode,&GmatGlobal::SetBatchMode);
      remember(&GmatGlobal::GetPlotMode,&GmatGlobal::SetPlotMode);
      remember(&GmatGlobal::GetMatlabMode,&GmatGlobal::SetMatlabMode);
      remember(&GmatGlobal::IsMatlabAvailable,&GmatGlobal::SetMatlabAvailable);
      remember(&GmatGlobal::IsMatlabDebugOn,&GmatGlobal::SetMatlabDebug);
      remember(&GmatGlobal::IsMissionTreeDebugOn,&GmatGlobal::SetMissionTreeDebug);
      remember(&GmatGlobal::IsWritingParameterInfo,&GmatGlobal::SetWriteParameterInfo);
      remember(&GmatGlobal::IsWritingFilePathInfo,&GmatGlobal::SetWriteFilePathInfo);
      remember(&GmatGlobal::IsWritingGmatKeyword,&GmatGlobal::SetWriteGmatKeyword);
      remember(&GmatGlobal::GetWritePersonalizationFile,&GmatGlobal::SetWritePersonalizationFile);
      remember(&GmatGlobal::EchoCommands,&GmatGlobal::SetCommandEchoMode);
      remember(&GmatGlobal::SkipSplashMode,&GmatGlobal::SetSkipSplashMode);
      const auto hidden=global->GetHiddenCommands();
      const auto startupLog=global->GetLogfileName(GmatGlobal::STARTUP);
      globals.emplace_back([=] { global->ClearHiddenCommands(); for (const auto &command:hidden) global->AddHiddenCommand(command); global->SetLogfileName(GmatGlobal::STARTUP,startupLog); });
      receiver.SuspendLogging(true);
   }
   ~StartupTransaction()
   {
      if (!committed) {
         FileManager::Instance()->RestoreState(*state);
         for (const auto &restore:globals) restore();
         QDir::setCurrent(cwd);
         receiver.RestoreLog(logFile,logEnabled);
      }
      receiver.SuspendLogging(false);
   }
   void commit() { committed=true; }
private:
   QtMessageReceiver &receiver;
   std::shared_ptr<const FileManager::State> state;
   std::vector<std::function<void()>> globals;
   std::string logFile;
   bool logEnabled, committed=false;
   QString cwd;
};
}
PathSettings capturePathSettings(const QString &startupFile)
{
   QTemporaryDir directory;
   if (!directory.isValid()) throw std::runtime_error("Cannot prepare startup settings.");
   const auto path=directory.filePath("startup.txt");
   FileManager::Instance()->WriteStartupFile(path.toStdString());
   PathSettings result{startupFile,read(path),QFileInfo(QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"))).absoluteFilePath(),functionPaths(),pythonPaths()};
   result.matlabFunctions=matlabPaths(); result.initialMatlabFunctions=result.matlabFunctions;
   result.scientificFiles=scientificFiles(); result.initialScientificFiles=result.scientificFiles;
   return result;
}
PathSettings readPathSettings(const QString &path,QtMessageReceiver &receiver)
{
   const auto error=qtStartupCompatibilityError(path);
   if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
   const auto liveMatlab=matlabPaths();
   StartupTransaction transaction(receiver);
   const auto text=read(path);
   FileManager::Instance()->ReadStartupFile(QFileInfo(path).absoluteFilePath().toStdString());
   auto result=capturePathSettings(QFileInfo(path).absoluteFilePath());
   // Preserve the imported source and parse its Python list separately. The
   // engine keeps earlier Python paths across startup reads, so its combined
   // vector is not the pending configuration of this imported file.
   result.startupText=text; result.pythonModules=startupPythonPaths(text); result.initialMatlabFunctions=liveMatlab;
   return result;
}
QString pathSettingsScript(const PathSettings &settings)
{
   QStringList result; bool output=false,functions=false,python=false,matlab=false;
   const QRegularExpression assignment("^\\s*(OUTPUT_PATH|GMAT_FUNCTION_PATH|PYTHON_MODULE_PATH|MATLAB_FUNCTION_PATH)\\s*=.*$");
   auto paths=[](const QString &key,const QStringList &values) {
      QStringList lines; for (const auto &path:values) lines.append(key+" = "+QDir::cleanPath(path)+"/"); return lines;
   };
   for (const auto &line:settings.startupText.split('\n')) {
      const auto match=assignment.match(line);
      if (!match.hasMatch()) { result.append(line); continue; }
      if (match.captured(1)=="OUTPUT_PATH") { if (!output) result.append("OUTPUT_PATH = "+QDir::cleanPath(settings.output)+"/"); output=true; }
      else if (match.captured(1)=="GMAT_FUNCTION_PATH") { if (!functions) result.append(paths("GMAT_FUNCTION_PATH",settings.functions)); functions=true; }
      else if (match.captured(1)=="MATLAB_FUNCTION_PATH") { if (!matlab) result.append(paths("MATLAB_FUNCTION_PATH",settings.matlabFunctions)); matlab=true; }
      else { if (!python) result.append(paths("PYTHON_MODULE_PATH",settings.pythonModules)); python=true; }
   }
   if (!output) result.append("OUTPUT_PATH = "+QDir::cleanPath(settings.output)+"/");
   if (!functions) result.append(paths("GMAT_FUNCTION_PATH",settings.functions));
   if (!python) result.append(paths("PYTHON_MODULE_PATH",settings.pythonModules));
   if (!matlab) result.append(paths("MATLAB_FUNCTION_PATH",settings.matlabFunctions));
   QMap<QString,QString> changed;
   for (auto it=settings.scientificFiles.cbegin();it!=settings.scientificFiles.cend();++it)
      if (it.value()!=settings.initialScientificFiles.value(it.key())) changed.insert(it.key(),it.value());
   return withScientificFiles(result.join('\n'),changed);
}

QString validatePathSettings(const PathSettings &settings)
{
   auto validate=[](const QStringList &directories,bool python) -> QString {
      QStringList seen;
      for (const auto &path:directories) {
         if (path.isEmpty() || !QDir::isAbsolutePath(path) || path.contains('=') || path.contains(QRegularExpression("[\\x00-\\x1f]")))
            return "Choose an absolute directory without control characters or '='.";
         // The existing Python plugin interpolates these paths into a quoted
         // Python string and changes backslashes to slashes. Reject filenames
         // that it cannot represent rather than changing the Python engine.
         if (python && (path.contains('"') || path.contains('\\')))
            return "Choose a Python module directory without double quotes or backslashes.";
         const QFileInfo directory(path);
         if (!directory.isDir() || !directory.isReadable()) return "Directory is unavailable: "+path;
         const auto canonical=directory.canonicalFilePath();
         if (seen.contains(canonical)) return python ? "Each Python module directory should appear once." : "Each function directory should appear once.";
         seen.append(canonical);
      }
      return {};
   };
   for (const auto &error:{validate({settings.output},false),validate(settings.functions,false),validate(settings.pythonModules,true)})
      if (!error.isEmpty()) return error;
   // Startup files can retain unused MATLAB defaults whose directories are
   // absent when MATLAB is not installed. Do not block unrelated path edits;
   // every changed/imported MATLAB list is still validated in full.
   if (settings.matlabFunctions!=settings.initialMatlabFunctions) {
      const auto error=validate(settings.matlabFunctions,false); if (!error.isEmpty()) return error;
   }
   for (auto it=settings.scientificFiles.cbegin();it!=settings.scientificFiles.cend();++it) {
      const auto &path=it.value();
      if (!scientificKeys.contains(it.key()) || path.isEmpty() || !QDir::isAbsolutePath(path) || path!=path.trimmed() || path.contains('=') || path.contains('#') || path.contains(QRegularExpression("[\\x00-\\x1f]")))
         return "Choose an absolute scientific data filename without surrounding whitespace, controls, '#' or '='.";
      const QFileInfo file(path);
      if (!file.isFile() || !file.isReadable() || file.size()==0)
         return "Choose an existing, nonempty readable scientific data file: "+it.key();
   }
   QTemporaryFile probe(QDir(settings.output).filePath(".gmat-write-XXXXXX"));
   if (!probe.open() || probe.write("probe")!=5 || !probe.flush()) return "Cannot write to the output directory: "+probe.errorString();
   return {};
}
QString applyPathSettings(const PathSettings &settings,QtMessageReceiver &receiver)
{
   const auto error=validatePathSettings(settings); if (!error.isEmpty()) return error;
   try {
      QTemporaryDir directory; if (!directory.isValid()) return "Cannot prepare startup settings.";
      const auto path=directory.filePath("startup.txt"); QFile file(path);
      // Python imports and sys.path are cached; the engine has no public path
      // replacement API. Apply only the session's other settings. Export keeps
      // the full pending Python list for a fresh launch, including its order.
      const auto livePython=FileManager::Instance()->GetAllPythonModulePaths();
      QStringList liveLines;
      for (const auto &line:pathSettingsScript(settings).split('\n'))
         if (!pythonAssignment.match(line).hasMatch()) liveLines.append(line);
      // These readers/kernel pools are initialized for the current process.
      // Export may select new files; a session Apply must preserve every live
      // scientific location until a fresh launch initializes the readers.
      const auto liveScientific=scientificFiles();
      QMap<QString,QString> restoreScientific;
      for (auto it=liveScientific.cbegin();it!=liveScientific.cend();++it)
         if (settings.scientificFiles.value(it.key(),it.value())!=it.value()) restoreScientific.insert(it.key(),it.value());
      const auto text=withScientificFiles(liveLines.join('\n'),restoreScientific).toUtf8();
      if (!file.open(QIODevice::WriteOnly) || file.write(text)!=text.size() || !file.flush()) return "Cannot prepare startup settings: "+file.errorString();
      file.close();
      const auto compatibility=qtStartupCompatibilityError(path); if (!compatibility.isEmpty()) return compatibility;
      StartupTransaction transaction(receiver);
      FileManager::Instance()->ReadStartupFile(path.toStdString());
      QStringList actual; for (const auto &entry:functionPaths()) actual.append(QDir::cleanPath(entry));
      QStringList expected; for (const auto &entry:settings.functions) expected.append(QDir::cleanPath(entry));
      QStringList expectedMatlab; for (const auto &entry:settings.matlabFunctions) expectedMatlab.append(QDir::cleanPath(entry));
      if (matlabPaths()!=expectedMatlab) return "GMAT could not preserve the requested MATLAB function paths and order.";
      if (scientificFiles()!=liveScientific) return "Scientific data files require exporting startup settings and restarting GMAT. The previous session settings were restored.";
      if (FileManager::Instance()->GetAllPythonModulePaths()!=livePython)
         return "Python paths require exporting startup settings and restarting GMAT with that file. The previous session settings were restored.";
      if (actual!=expected || QDir::cleanPath(QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH")))!=QDir::cleanPath(settings.output))
         return "GMAT could not preserve the requested paths and order.";
      receiver.SetLogPath(settings.output.toStdString(),true);
      FileManager::Instance()->SetStartupFilePath(settings.startupFile.toStdString());
      transaction.commit();
      return {};
   } catch (BaseException &exception) { return QString::fromStdString(exception.GetFullMessage()); }
   catch (const std::exception &exception) { return QString::fromUtf8(exception.what()); }
}
