#include "FolderRun.hpp"
#include "FileComparison.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QRegularExpression>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include <future>
#include <filesystem>
#include <limits>
#include <cmath>

static QString save(const QString &path,const QByteArray &bytes)
{
   QSaveFile file(path);
   if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) return file.errorString();
   return {};
}
QStringList folderRunScripts(const QString &directory)
{
   QStringList result;
   for (const auto &file:QDir(directory).entryInfoList(QDir::Files,QDir::Name)) {
      if (file.suffix().compare("script",Qt::CaseInsensitive) && file.suffix().compare("m",Qt::CaseInsensitive)) continue;
      QFile input(file.absoluteFilePath());
      if (input.open(QIODevice::ReadOnly)) {
         // Function definitions are not standalone missions. Ignore comments
         // and a UTF-8 BOM before checking the first actual token.
         auto text=QString::fromUtf8(input.read(65536)); text.remove(QChar(0xfeff));
         text.remove(QRegularExpression("(?m)^\\s*%[^\\n]*(?:\\n|$)"));
         if (QRegularExpression("^\\s*function\\b").match(text).hasMatch()) continue;
      }
      result.append(file.absoluteFilePath());
   }
   return result;
}
QString folderRunBaseline(const FolderRunOptions &options,const QString &report)
{
   QString name=QFileInfo(report).fileName();
   return QDir(options.baselineDirectory).absoluteFilePath(name.replace("GMAT",options.replacement));
}

bool sameFolderRunFile(const QString &left,const QString &right)
{
   const QFileInfo a(left), b(right);
   if (a == b) return true;
   std::error_code error;
   if (std::filesystem::equivalent(a.filesystemAbsoluteFilePath(),b.filesystemAbsoluteFilePath(),error))
      return true;
   // A new destination can still be reached through two names for its parent.
   return QFileInfo(a.absolutePath()) == QFileInfo(b.absolutePath()) &&
#ifdef Q_OS_WIN
         a.fileName().compare(b.fileName(),Qt::CaseInsensitive)==0;
#else
         a.fileName()==b.fileName();
#endif
}

QString validateFolderRun(const FolderRunOptions &options)
{
   if (!QFileInfo(options.directory).isDir()) return "Choose an existing script folder.";
   if (options.first<1 || options.count<1 || options.repeats<1) return "Starting script, count and repeats must be positive.";
   if (options.outputDirectory.trimmed().isEmpty()) return "Choose an output folder.";
   if (options.saveCopies && options.copyDirectory.trimmed().isEmpty()) return "Choose a folder for saved script copies.";
   if (options.compare && (!QFileInfo(options.baselineDirectory).isDir() || !std::isfinite(options.tolerance) || options.tolerance<0)) return "Choose an existing comparison folder and a finite nonnegative tolerance.";
   if (options.saveComparison && (!options.compare || options.comparisonFile.trimmed().isEmpty())) return "Enable comparison and choose a comparison results filename.";
   if (options.saveCopies && QFileInfo(options.copyDirectory).canonicalFilePath()==QFileInfo(options.directory).canonicalFilePath()) return "Saved copies must use a different folder from the source scripts.";
   return {};
}
QString FolderRunResult::summary() const
{
   QString result="GMAT folder run\n";
   if (!error.isEmpty()) result+="Error: "+error+"\n";
   result+=cancelled ? "Interrupted; remaining runs were not executed.\n" : "Finished.\n";
   QMap<QString,int> categories;
   for (const auto &item:items) {
      ++categories[item.category];
      result+=QString("\n%1 — repeat %2 — %3 (engine %4)\nLoaded: %5\nOutput: %6\n").arg(item.script).arg(item.repeat).arg(item.category).arg(item.engineStatus).arg(item.loadedScript,item.outputDirectory);
      if (!item.details.isEmpty()) result+=item.details+"\n";
      if (!item.comparison.isEmpty()) result+=item.comparison+"\n";
   }
   result+="\nRun totals\n";
   for (auto it=categories.cbegin();it!=categories.cend();++it) result+=QString("%1: %2\n").arg(it.key()).arg(it.value());
   return result;
}
FolderRunResult runScriptFolder(const FolderRunOptions &options,const FolderScriptRunner &run,
   const std::atomic_bool &cancel,const std::function<void(int,int,const FolderRunItem &)> &progress)
{
   FolderRunResult result; result.error=validateFolderRun(options); if (!result.error.isEmpty()) return result;
   auto scripts=folderRunScripts(options.directory); const auto allScripts=scripts;
   if (options.first>scripts.size()) { result.error="Starting script is outside the folder's mission list."; return result; }
   scripts=scripts.mid(options.first-1,options.count);
   auto matches=[](const QString &path,const QString &filter,bool exclude) { return filter.isEmpty() || path.contains(filter)!=exclude; };
   for (int i=scripts.size()-1;i>=0;--i) if (!matches(scripts[i],options.filter,options.exclude) || !matches(scripts[i],options.secondFilter,options.secondExclude)) scripts.removeAt(i);
   if (scripts.isEmpty()) { result.error="No missions match the selected range and filters."; return result; }
   if (static_cast<long long>(scripts.size())*options.repeats>std::numeric_limits<int>::max()) { result.error="The selected folder run has too many repeats."; return result; }
   if (!QDir().mkpath(options.outputDirectory)) { result.error="Cannot create the output folder."; return result; }
   QTemporaryFile probe(QDir(options.outputDirectory).filePath(".gmat-folder-run-XXXXXX"));
   if (!probe.open()) { result.error="The output folder is not writable: "+probe.errorString(); return result; } probe.close();
   if (options.saveCopies && !QDir().mkpath(options.copyDirectory)) { result.error="Cannot create the saved-script folder."; return result; }
   QTemporaryDir comparisons;
   QStringList protectedPaths=allScripts;
   QString comparisonText;
   for (const auto &script:scripts) {
      QString loaded=script,copyError;
      if (options.saveCopies) {
         loaded=QDir(options.copyDirectory).filePath(QFileInfo(script).fileName());
         QFile source(script);
         const bool overwritesMission = std::any_of(allScripts.cbegin(), allScripts.cend(),
               [&](const QString &sourcePath) { return sameFolderRunFile(loaded, sourcePath); });
         if (overwritesMission) copyError="A saved copy would overwrite a source script.";
         else if (!source.open(QIODevice::ReadOnly)) copyError=source.errorString();
         else { const auto bytes=source.readAll(); copyError=source.error()!=QFileDevice::NoError ? source.errorString() : save(loaded,bytes); }
      }
      for (int repeat=1;repeat<=options.repeats;++repeat) {
         if (cancel) { result.cancelled=true; break; }
         auto output=options.runFolders ? QDir(options.outputDirectory).filePath("Run_"+QString::number(repeat)) : options.outputDirectory;
         FolderRunItem item; item.script=script; item.loadedScript=loaded; item.outputDirectory=output; item.repeat=repeat;
         if (!copyError.isEmpty()) { item.category="Copy error"; item.details=copyError; }
         else if (!QDir().mkpath(output)) { item.category="Output error"; item.details="Cannot create run output folder."; }
         else { item=run(loaded,QFileInfo(script).absolutePath(),repeat,output); item.script=script; item.loadedScript=loaded; item.outputDirectory=output; item.repeat=repeat; }
         if (options.compare && item.category=="Completed") {
            QVector<FileComparePair> pairs;
            for (auto it=item.reports.cbegin();it!=item.reports.cend();++it) {
               pairs.append({folderRunBaseline(options,it.value()),it.value()});
            }
            if (pairs.isEmpty()) item.comparison="Comparison: no ReportFile outputs were produced.";
            else if (!comparisons.isValid()) item.comparison="Comparison error: the temporary results folder is unavailable.";
            else {
               FileCompareOptions compare; compare.mode=FileCompareMode::NumericLines; compare.tolerance=options.tolerance;
               const auto path=comparisons.filePath("comparison.txt");
               // Comparisons stream large files on a worker; the GUI event loop
               // continues delivering Stop while the worker polls cancellation.
               auto worker=std::async(std::launch::async,[pairs,compare,path,&cancel] { return compareFiles(pairs,compare,path,cancel); });
               QEventLoop loop; QTimer timer; timer.setInterval(10);
               QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if (worker.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) loop.quit(); });
               if (worker.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) { timer.start(); loop.exec(); }
               const auto compared=worker.get();
               QFile contents(path); if (contents.open(QIODevice::ReadOnly)) item.comparison=QString::fromUtf8(contents.readAll());
               if (!compared.error.isEmpty()) item.comparison+="\nComparison error: "+compared.error;
               item.comparison+=QString("\nComparison totals: compared=%1 differences=%2 errors=%3\n").arg(compared.compared).arg(compared.different).arg(compared.errors);
            }
            comparisonText+=QString("\n%1 repeat %2\n").arg(script).arg(repeat)+item.comparison;
         }
         result.items.append(item); if (progress) progress(result.items.size(),scripts.size()*options.repeats,item);
         const auto reportPaths=item.reports.values()+item.reservedReportPaths;
         if (options.compare)
            for (const auto &report:reportPaths) protectedPaths.append(folderRunBaseline(options,report));
         protectedPaths.append(item.loadedScript); protectedPaths.append(reportPaths); protectedPaths.append(item.ephemerisPaths);
         if (item.category=="Interrupted" || cancel) { result.cancelled=true; break; }
      }
      if (result.cancelled) break;
   }
   const auto protectedFile=[&](const QString &candidate) {
      for (const auto &path:protectedPaths) if (sameFolderRunFile(candidate,path)) return true;
      return false;
   };
   result.summaryFile=QDir(options.outputDirectory).filePath("FolderRunSummary.txt");
   if (protectedFile(result.summaryFile)) {
      int suffix=1;
      do { result.summaryFile=QDir(options.outputDirectory).filePath(QString("FolderRunSummary.%1.txt").arg(suffix++)); } while (QFileInfo::exists(result.summaryFile) || protectedFile(result.summaryFile));
   }
   const auto summaryError=save(result.summaryFile,result.summary().toUtf8()); if (!summaryError.isEmpty()) result.error="Cannot save folder-run summary: "+summaryError;
   if (options.saveComparison) {
      protectedPaths.append(result.summaryFile);
      if (protectedFile(options.comparisonFile)) {
         result.error="Comparison results cannot overwrite a mission, saved copy, report or folder summary."; return result;
      }
      const auto error=save(options.comparisonFile,comparisonText.toUtf8()); if (!error.isEmpty()) result.error="Cannot save comparison results: "+error;
   }
   return result;
}
