#pragma once
#include <QMainWindow>
#include <memory>
#include <optional>
#include <functional>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>
#include "MissionModel.hpp"
#include "FolderRun.hpp"
class QMdiArea;
class QPlainTextEdit;
class QTreeWidget;
class QCloseEvent;
class QtMessageReceiver;
class QtInterpreter;
class QtPlotReceiver;
class PlotPlaybackControls;
class QtSolverListenerManager;
class EditablePanel;
class HelpController;
class CommandEditor;
class MissionNavigation;
class WelcomeDialog;
class Debugger;
class QMenu;
class QMdiSubWindow;
class GmatBase;
class GmatCommand;
class QAction;
struct ScriptDocument;
class MainWindow : public QMainWindow
{
public:
   MainWindow();
   ~MainWindow() override;
   bool initialize(const QString &startup);
   bool loadScript(const QString &path);
   bool openScriptDocument(const QString &path);
   void showWelcome(bool startup=false);
   bool saveScriptTo(const QString &path);
   bool buildScript();
   bool convertOpenFramesScript();
   QStringList availableEngineTypes() const;
   enum class RunResult { Completed, Stopped, Failed, Busy };
   RunResult runMission();
   RunResult debugMission();
   bool setBreakpoint(int index,bool enabled);
   QSet<int> breakpointIndices() const { return breakpoints; }
   void stepMission();
   FolderRunResult runFolderScripts(const FolderRunOptions &options,QtPlotReceiver &batchPlots,
      const std::atomic_bool &cancel,const std::function<void(int,int,const FolderRunItem &)> &progress={});
   void pauseMission();
   void resumeMission();
   void stopMission();
   QStringList summaryFrames() const;
   QString summaryText(int index,const QString &frame,bool physicsOnly=false,bool includeA1=false);
   void showSummary(int index=-1);
   bool isRunning() const { return running; }
   QtPlotReceiver *plotReceiver() const { return plots.get(); }
   const MissionSnapshot &missionSnapshot() const { return missionState; }
   QString applyMissionChange(const MissionSnapshot &snapshot, int index,
                              MissionEdit operation, const QString &replacement,const QMap<QString,QString> &pendingFiniteResources={});
   QString applySolverCorrections(const MissionSnapshot &snapshot,int index);
   QString applyResourceChanges(const QString &name, const QMap<QString, QString> &changes,
                                const QString &expectedScript);
   enum class ResourceCreationPolicy { RequireReferences, AllowIncomplete };
   QString createResource(const QString &type, const QString &name, const QString &expectedScript, int rows=1, int columns=1, const std::optional<QString> &initialValue=std::nullopt, const QString &functionPath={}, const QString &spacecraft={}, const QMap<QString,QString> &settings={}, ResourceCreationPolicy policy=ResourceCreationPolicy::RequireReferences);
   QString cloneResource(const QString &original, const QString &name, const QString &expectedScript, const QMap<QString,QString> &settings={});
   QString renameResource(const QString &original,const QString &name,const QString &expectedScript);
   QString deleteResource(const QString &name, const QString &expectedScript);
   // SolarSystem-owned bodies use source reconstruction,not ConfigManager removal.
   QString deleteUserDefinedBody(const QString &name,const QString &expectedScript);
protected:
   void closeEvent(QCloseEvent *event) override;
private:
   QString applyResourceSettings(GmatBase &object,const QMap<QString,QString> &changes,const QString &source);
   QString createResourceUnderBody(const QString &type,const QString &name,const QString &expectedScript,int rows,int columns,const std::optional<QString> &initialValue,const QString &functionPath,const QString &spacecraft,const QMap<QString,QString> &settings,ResourceCreationPolicy policy,const QString &centralBody);
   QString savePlotProjection(const QString &name,bool perspective,double fov);
   void refreshTrees();
   void refreshOutput();
   EditablePanel *makeResourcePanel(GmatBase &object,const QString &snapshot);
   void refreshAppliedResourcePanels();
   void openCommandEditor(int index, MissionEdit operation);
   void showRenameCommand(int index);
   CommandEditor *makeCommandPanel(int index,MissionEdit operation);
   void showCreateResource(const QString &initialType={},const QString &centralBody={});
   void showCloneResource(const QString &original);
   void showRenameResource(const QString &original);
   void showPathSettings();
   void openPathSettings(bool outputTab);
   void showFileComparison(const QString &baseline={});
   QString applyModelScript(const QString &candidate,const std::function<QString()> &validate={},const QString &removedCamera={});
   bool restoreBuiltModel();
   void setScriptDirectory();
   void newMission();
   void refreshRecentMenu();
   bool openMissionFile(const QString &path);
   bool saveScript(bool saveAs = false);
   void saveAndBuildScript(bool run);
   bool confirmDiscard();
   std::shared_ptr<ScriptDocument> createScriptDocument(const QString &path,const QString &text);
   std::shared_ptr<ScriptDocument> selectedScriptDocument() const;
   bool activateScriptDocument(const std::shared_ptr<ScriptDocument> &document);
   bool saveScriptDocument(const std::shared_ptr<ScriptDocument> &document,bool saveAs=false);
   bool saveScriptDocumentTo(const std::shared_ptr<ScriptDocument> &document,const QString &path);
   bool confirmScriptClose(const std::shared_ptr<ScriptDocument> &document,bool discard);
   bool closeScriptDocument(const std::shared_ptr<ScriptDocument> &document);
   bool resolveDocumentPanels();
   void reloadScriptDocument();
   void updateScriptDocuments();
   QList<std::shared_ptr<ScriptDocument>> scriptDocuments;
   std::shared_ptr<ScriptDocument> activeDocument;
   void updateTitle();
   void setRunning(bool value);
   bool setMatlabConnection(bool open);
   RunResult executeMission(bool debug);
   void refreshBreakpoints();
   QMdiArea *workspace;
   QPlainTextEdit *editor;
   QPlainTextEdit *messages;
   QString *folderMessages=nullptr;
   QPointer<QWidget> textEditTarget;
   HelpController *contextHelp;
   QTreeWidget *resources;
   QTreeWidget *mission;
   MissionNavigation *missionNavigation;
   Debugger *debugger;
   QSet<int> breakpoints;
   QString breakpointSource;
   QPointer<WelcomeDialog> welcome;
   QMenu *recentMenu;
   QTreeWidget *output;
   std::unique_ptr<QtMessageReceiver> receiver;
   std::unique_ptr<QtInterpreter> interpreter;
   std::unique_ptr<QtPlotReceiver> plots;
   PlotPlaybackControls *plotPlayback=nullptr;
   std::unique_ptr<QtSolverListenerManager> solverListeners;
   QList<QAction *> editingActions;
   QAction *runAction = nullptr;
   QAction *pauseAction = nullptr;
   QAction *stopAction = nullptr;
   QAction *stepAction = nullptr;
   QString scriptPath;
   QString startupDirectory;
   QString startupFile;
   QString builtScript;
   QMap<QString, QString> reportFiles;
   QMap<QString,QPair<QString,QString>> ephemerisFiles;
   MissionSnapshot missionState;
   MissionSnapshot runCorrectionState;
   QVector<GmatCommand *> runCommands,runCorrectionCommands;
   quint64 modelGeneration=0;
   bool summaryAvailable=false;
   RunResult lastRunResult=RunResult::Failed;
   bool modelValid = false;
   bool ready = false;
   bool running = false;
   bool paused = false;
   bool stopRequested = false;
};
