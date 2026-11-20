#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <atomic>
#include <functional>
struct FolderRunOptions
{
   QString directory,outputDirectory,copyDirectory,baselineDirectory,replacement="GMAT",comparisonFile;
   QString filter,secondFilter;
   int first=1,count=1,repeats=1;
   bool exclude=false,secondExclude=false,saveCopies=false,runFolders=false,compare=false,saveComparison=false;
   double tolerance=1e-6;
};
struct FolderRunItem
{
   QString script,loadedScript,outputDirectory,category,details,comparison;
   int repeat=1,engineStatus=0;
   QMap<QString,QString> reports;
   QStringList reservedReportPaths, ephemerisPaths;
};
struct FolderRunResult
{
   QVector<FolderRunItem> items;
   QString error,summaryFile;
   bool cancelled=false;
   QString summary() const;
};
QStringList folderRunScripts(const QString &directory);
QString folderRunBaseline(const FolderRunOptions &options,const QString &report);
bool sameFolderRunFile(const QString &left,const QString &right);
QString validateFolderRun(const FolderRunOptions &options);
using FolderScriptRunner=std::function<FolderRunItem(const QString &,const QString &,int,const QString &)>;
FolderRunResult runScriptFolder(const FolderRunOptions &options,const FolderScriptRunner &run,
   const std::atomic_bool &cancel,const std::function<void(int,int,const FolderRunItem &)> &progress={});
