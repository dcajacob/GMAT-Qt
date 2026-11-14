#pragma once
#include "PathSettings.hpp"
#include <QDialog>
#include <functional>
class QtMessageReceiver;
class QListWidget;
class QLineEdit;
class QLabel;
class QPlainTextEdit;
class PathSettingsDialog : public QDialog
{
public:
   using Apply=std::function<QString(const PathSettings &)>;
   PathSettingsDialog(PathSettings settings,QtMessageReceiver &receiver,Apply apply,const QString &missionFile,QWidget *parent=nullptr);
   QString exportStartup(const QString &path) const;
private:
   PathSettings pending() const;
   void loadControls();
   PathSettings settings;
   QString missionFile;
   QListWidget *functions,*pythonModules,*matlabFunctions;
   QLineEdit *output,*entry,*pythonEntry,*matlabEntry,*startup;
   QMap<QString,QLineEdit *> scientificFiles;
   QLabel *status,*saved;
   QPlainTextEdit *preview;
};
