#include "ResourcePreview.hpp"
#include "ReportViewer.hpp"
#include "ComparisonPanel.hpp"
#include "MainWindow.hpp"
#include "PlotPlaybackControls.hpp"
#include <QPainter>
#include "MissionNavigation.hpp"
#include "WelcomeDialog.hpp"
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include "InspectionDialog.hpp"
#include "FunctionFileDialog.hpp"
#include "AboutDialog.hpp"
#include "HelpDialog.hpp"
#include "FolderRunDialog.hpp"
#include "GmatGlobal.hpp"
#include "UserParameter.hpp"
#include "SolarSystemPanel.hpp"
#include "SolarSystem.hpp"
#include "CelestialBodyPanel.hpp"
#include "CelestialBody.hpp"
#include "PathSettingsDialog.hpp"
#include "GmatCommand.hpp"
#include "CommandUtil.hpp"
#include "CoordinateSystem.hpp"
#include "SpacePoint.hpp"
#include "TrackingConfigDialog.hpp"
#include "EpochIntervalDialog.hpp"
#include "QtMessageReceiver.hpp"
#include "QtInterpreter.hpp"
#include "ResourceEditor.hpp"
#include "ResourceDraft.hpp"
#include "ResourceRename.hpp"
#include "ResourceProperties.hpp"
#include "CustomFovProperties.hpp"
#include "QtScriptInterpreter.hpp"
#include "SpacecraftOrbit.hpp"
#include "AtmosphereDialog.hpp"
#include "GroundStationDialog.hpp"
#include "OrbitViewDialog.hpp"
#include "ThrusterDialog.hpp"
#include "BurnDialog.hpp"
#include "EphemerisDialog.hpp"
#include "DynamicDataDialog.hpp"
#include "CalculatedPoint.hpp"
#include "EphemerisFile.hpp"
#include "EventLocatorDialog.hpp"
#include "EventLocator.hpp"
#include "CommandEditor.hpp"
#include "MissionModel.hpp"
#include "ScriptStatements.hpp"
#include "SolverBranchCommand.hpp"
#include "Vary.hpp"
#include "Solver.hpp"
#include <cmath>
#include "Debugger.hpp"
#include "ExternalForceDialog.hpp"
#include "PolyhedronDialog.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTreeWidgetItemIterator>
#include "StartupCompatibility.hpp"
#include "ScriptCompatibility.hpp"
#include "ScriptEditor.hpp"
#include "ScriptDocument.hpp"
#include "FindReplaceDialog.hpp"
#include <QDialog>
#include <QDir>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QRegularExpression>
#include "QtPlotReceiver.hpp"
#include "QtSolverListener.hpp"
#include "ListenerManagerInterface.hpp"
#include "PlotInterface.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include "GmatType.hpp"
#include <vector>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMdiArea>
#include <QLayout>
#include <QTimer>
#include <QMdiSubWindow>
#include <QOpenGLWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStringDecoder>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QKeyEvent>
#include <QStyle>
#include <sstream>
#include <set>
#include <stdexcept>

namespace {
class NavigationTree final : public QTreeWidget
{
public:
   using QTreeWidget::QTreeWidget;
protected:
   void keyPressEvent(QKeyEvent *event) override
   {
      if ((event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) && (event->modifiers()==Qt::NoModifier || event->modifiers()==Qt::KeypadModifier)) {
         // Use the same guarded route as mouse activation and existing callers.
         // Consuming the key avoids a second platform itemActivated dispatch.
         if (!event->isAutoRepeat() && currentItem()) emit itemDoubleClicked(currentItem(),currentColumn());
         event->accept(); return;
      }
      QTreeWidget::keyPressEvent(event);
   }
};
bool setGmatScriptDirectory(const QString &directory)
{
   // FileManager stores this path verbatim and FindPath joins it directly to
   // relative asset names. Match the engine filename interpreter's trailing
   // separator even though QFileInfo::absolutePath() omits it.
   auto *manager=FileManager::Instance();
   auto path=QDir::toNativeSeparators(directory);
   const auto separator=QString::fromStdString(manager->GetPathSeparator());
   if (!path.endsWith(separator)) path+=separator;
   return manager->SetGmatWorkingDirectory(path.toStdString());
}
void keepWorkspaceWindowReachable(QMdiArea *workspace,QMdiSubWindow *child)
{
   if (child->isMinimized() || child->isMaximized()) return;
   // Embedded forms and restored MDI windows can acquire a larger minimum
   // after their initial placement. Settle only this window's layouts before
   // measuring it; do not move any other window or pump the global event loop.
   child->ensurePolished();
   if (auto *content=child->widget()) {
      content->ensurePolished();
      if (content->layout()) content->layout()->activate();
   }
   if (child->layout()) child->layout()->activate();
   const auto bounds=workspace->viewport()->rect();
   child->resize(child->size().expandedTo(child->minimumSizeHint()).boundedTo(bounds.size()));
   child->move(qBound(bounds.left(),child->x(),qMax(bounds.left(),bounds.right()-child->width()+1)),
               qBound(bounds.top(),child->y(),qMax(bounds.top(),bounds.bottom()-child->height()+1)));
}
void showNewWorkspaceWindow(QMdiArea *workspace,QMdiSubWindow *child)
{
   child->show();
   // Configuration panels are created with a normal size. Do not inherit the
   // active script's maximized state and later restore an unclamped position.
   if (child->property("configurationPanel").toBool() && child->isMaximized()) child->showNormal();
   // Keep new controls reachable when Qt's cascade position leaves the viewport.
   keepWorkspaceWindowReachable(workspace,child);
   // An active maximized script can briefly maximize the new child, then Qt
   // restores its original geometry during deferred initialization. Bound that
   // final normal geometry too, without altering an explicit min/max state.
   const QPointer<QMdiSubWindow> guarded=child;
   QTimer::singleShot(0,child,[workspace,guarded] {
      if (guarded && guarded->isVisible()) keepWorkspaceWindowReachable(workspace,guarded);
   });
}
// wx assigns display names while populating the mission tree. Qt keeps its
// snapshots independent of engine objects; give summaries those names only
// while reading them, then restore every original name.
class SummaryNames
{
public:
   explicit SummaryNames(GmatCommand *first) {
      try { collect(first,nullptr,0); }
      catch (...) { restore(); throw; }
   }
   ~SummaryNames() { restore(); }
private:
   std::vector<std::pair<GmatCommand *,std::string>> originals;
   std::set<GmatCommand *> visited;
   QMap<QString,int> counts;
   void restore() { for (const auto &entry:originals) entry.first->SetSummaryName(entry.second); }
   void collect(GmatCommand *command,GmatCommand *stop,int depth)
   {
      if (depth>100) throw std::runtime_error("Mission nesting is too deep to summarize");
      for (;command && command!=stop;command=command->GetNext()) {
         if (!visited.insert(command).second) return;
         originals.emplace_back(command,command->GetSummaryName());
         auto name=command->GetName();
         if (name.empty()) {
            const auto type=QString::fromStdString(command->GetTypeName());
            name=(type+QString::number(++counts[type])).toStdString();
         }
         command->SetSummaryName(name);
         for (int branch=0;auto *child=command->GetChildCommand(branch);++branch) collect(child,command,depth+1);
      }
   }
};
QString userParameterValueError(const QString &name,const QString &type,const QString &value)
{
   auto *parameter=Moderator::Instance()->GetConfiguredObject(name.toStdString());
   if (!parameter || parameter->GetTypeName()!=type.toStdString()) return "The parameter was not created with the requested type.";
   const bool matches=type=="Variable" ? parameter->GetRealParameter("Value")==userParameterLiteral(type,value).toDouble() :
      parameter->GetStringParameter("Expression")==value.toStdString() && parameter->GetStringParameter("Value")==value.toStdString();
   return matches ? QString() : "GMAT could not preserve this initializer exactly. Edit its script settings or choose a representable value.";
}
QString omitUnsetHardwareFovs(QString script,GmatBase *replacement=nullptr)
{
   // Imager's empty optional FOV is serialized as a diagnostic placeholder.
   // Reinterpreting that output would turn an unset reference into a missing
   // object error, including for antennas that do not use a field of view.
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::HARDWARE)) {
      auto *object=Moderator::Instance()->GetConfiguredObject(name);
      if (replacement && replacement->GetName()==name) object=replacement;
      if (!object || !object->IsOfType("Imager") || !object->GetRefObjectName(Gmat::FIELD_OF_VIEW).empty()) continue;
      const auto key=QString::fromStdString(name)+".FieldOfView";
      script.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+"[ \\t]*=[ \\t]*'?UndefinedFieldOfView'?[ \\t]*;",QRegularExpression::MultilineOption));
   }
   return script;
}
QString orderPropagatorType(const QString &source,const QString &name,const QString &firstMissionStatement)
{
   // A changed Type recreates the owned propagator. Place only that selector
   // before existing numeric settings so interpretation cannot discard them.
   const auto parsed=scriptStatements(source);
   const QRegularExpression boundary("^\\s*BeginMissionSequence\\b");
   const bool explicitMission=std::any_of(parsed.cbegin(),parsed.cend(),[&](const auto &statement) { return boundary.match(statement.code).hasMatch(); });
   auto normalize=[](QString code) { code=code.trimmed(); code.remove(QRegularExpression("^GMAT\\s+")); code.remove(QRegularExpression("\\s+")); if (code.endsWith(';')) code.chop(1); return code; };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) { const auto commands=scriptStatements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
   const QRegularExpression property("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\.([A-Za-z][A-Za-z0-9_.]*)\\s*=");
   qsizetype firstSetting=-1,typeStart=-1,typeEnd=-1; int types=0;
   for (const auto &statement:parsed) {
      if (boundary.match(statement.code).hasMatch() || (!explicitMission && !firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) break;
      const auto match=property.match(statement.code); if (!match.hasMatch()) continue;
      qsizetype first=0,last=statement.code.size()-1;
      while (first<=last && statement.code[first].isSpace()) ++first;
      while (last>=first && statement.code[last].isSpace()) --last;
      if (first>last) continue;
      const auto start=statement.positions[first],end=statement.positions[last]+1;
      if (firstSetting<0) firstSetting=start;
      if (match.captured(1)=="Type") {
         ++types; typeStart=start; typeEnd=end;
         for (auto i=first+1;i<=last;++i) if (statement.positions[i]!=statement.positions[i-1]+1) throw std::runtime_error("Cannot safely order the changed propagator type.");
      }
   }
   if (types!=1 || firstSetting<0) throw std::runtime_error("Cannot locate the changed propagator type safely.");
   if (typeStart<=firstSetting) return source;
   const auto lineStart=typeStart ? source.lastIndexOf('\n',typeStart-1)+1 : 0;
   auto lineEnd=source.indexOf('\n',typeEnd); if (lineEnd<0) lineEnd=source.size();
   if (!source.mid(lineStart,typeStart-lineStart).trimmed().isEmpty() || !source.mid(typeEnd,lineEnd-typeEnd).trimmed().isEmpty())
      throw std::runtime_error("Cannot safely move the changed propagator type with adjacent source.");
   const auto insertionLine=firstSetting ? source.lastIndexOf('\n',firstSetting-1)+1 : 0;
   const auto insertion=source.mid(insertionLine,firstSetting-insertionLine).trimmed().isEmpty() ? insertionLine : firstSetting;
   auto selector=source.mid(lineStart,lineEnd-lineStart); if (!selector.endsWith('\n')) selector+='\n';
   if (insertion>0 && source[insertion-1]!='\n') selector.prepend('\n');
   auto result=source; result.remove(lineStart,lineEnd-lineStart+(lineEnd<source.size() ? 1 : 0)); result.insert(insertion,selector); return result;
}

QStringList creatableResourceTypes()
{
   QStringList result;
   for (const auto category : {Gmat::SPACECRAFT,Gmat::FORMATION,Gmat::SPACE_POINT,Gmat::REGION,Gmat::HARDWARE,Gmat::BURN,Gmat::PROP_SETUP,
         Gmat::ODE_MODEL,Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER,Gmat::FUNCTION,Gmat::EVENT_LOCATOR,
         Gmat::CALCULATED_POINT,Gmat::CELESTIAL_BODY,Gmat::MEASUREMENT_MODEL,Gmat::ERROR_MODEL,Gmat::INTERFACE,Gmat::DATA_FILTER,Gmat::FIELD_OF_VIEW})
      for (const auto &type : Moderator::Instance()->GetListOfViewableItems(category))
         result.append(QString::fromStdString(type));
   for (const auto *name:{"Smoother","ProcessNoiseModel","EstimatedParameter"}) {
      const auto category=GmatType::GetTypeId(name);
      if (category==Gmat::UNKNOWN_OBJECT) continue;
      for (const auto &type:Moderator::Instance()->GetListOfViewableItems(category)) result.append(QString::fromStdString(type));
   }
   result.append({"Variable","String","Array"});
   // The engine fails in CreateCelestialBody/SetUpBody for a new Star before
   // script properties can supply its missing central body. Keep that factory
   // item out of creation menus; the existing Sun remains editable.
   result.removeAll("Star");
   result.removeDuplicates(); result.sort(); return result;
}
bool resourceCanBeCloned(GmatBase *object)
{
   if (!object || object->IsOfType("SolarSystem") || object->IsOfType("CelestialBody") || object->IsOfType("PropSetup")) return false;
   if (auto *frame=dynamic_cast<CoordinateSystem *>(object);frame && frame->IsBuiltIn()) return false;
   if (auto *point=dynamic_cast<CalculatedPoint *>(object);point && point->IsBuiltIn()) return false;
   return true;
}
QString cloneResourceSource(const QString &source,const QString &original,const QString &name,const QString &firstMissionStatement)
{
   // Copy only explicit configuration. Canonical engine regeneration would
   // materialize unrelated defaults and lose expressions, spelling and comments.
   const QRegularExpression create("^\\s*Create\\s+([A-Za-z][A-Za-z0-9_]*)\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression definition("\\b("+QRegularExpression::escape(original)+"(?:\\s*\\[\\s*[1-9][0-9]*\\s*,\\s*[1-9][0-9]*\\s*\\])?)(?![A-Za-z0-9_])");
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?("+QRegularExpression::escape(original)+")(?:\\.[A-Za-z0-9_.]+(?:\\([^;=]*\\))?|\\([^;=]*\\))?\\s*=");
   const auto normalize=[](QString code) {
      code=code.trimmed(); if (code.endsWith(';')) code.chop(1); code.remove(QRegularExpression("^GMAT\\s+"));
      QString result; bool quoted=false; for (auto ch:code) { if (ch=='\'') quoted=!quoted; if (quoted || !ch.isSpace()) result+=ch; } return result;
   };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) { const auto commands=scriptStatements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
   const auto statements=scriptStatements(source);
   const QRegularExpression explicitBoundary("^\\s*BeginMissionSequence\\b");
   const bool explicitMission=std::any_of(statements.cbegin(),statements.cend(),[&](const auto &statement) { return explicitBoundary.match(statement.code).hasMatch(); });
   QString declaration,settings; qsizetype boundary=source.size(); bool located=firstCommand.isEmpty();
   for (const auto &statement:statements) {
      qsizetype first=0,last=statement.code.size()-1;
      while (first<=last && statement.code[first].isSpace()) ++first;
      while (last>=first && statement.code[last].isSpace()) --last;
      if (first>last) continue;
      if (explicitBoundary.match(statement.code).hasMatch() || (!explicitMission && !firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) {
         boundary=statement.positions[first]; located=true; break;
      }
      const auto declared=create.match(statement.code);
      if (declared.hasMatch()) {
         const auto member=definition.match(declared.captured(2)); if (!member.hasMatch()) continue;
         if (!declaration.isEmpty()) throw std::runtime_error("Multiple declarations for the resource to clone.");
         declaration="Create "+declared.captured(1)+' '+name+member.captured(1).mid(original.size())+";\n";
         continue;
      }
      const auto configured=assignment.match(statement.code); if (!configured.hasMatch()) continue;
      auto start=statement.positions[first],end=statement.positions[last]+1;
      const auto lineStart=start ? source.lastIndexOf('\n',start-1)+1 : 0;
      if (source.mid(lineStart,start-lineStart).trimmed().isEmpty()) start=lineStart;
      auto lineEnd=source.indexOf('\n',end); if (lineEnd<0) lineEnd=source.size();
      const auto tail=source.mid(end,lineEnd-end).trimmed();
      if (tail.isEmpty() || tail.startsWith('%')) end=lineEnd+(lineEnd<source.size() ? 1 : 0);
      auto copied=source.mid(start,end-start);
      // Like wx Clone, keep references to other resources, including references
      // in array expressions and quoted filenames. Change only assignment owner.
      const auto position=statement.positions[configured.capturedStart(1)]-start;
      copied.replace(position,original.size(),name);
      if (!copied.endsWith('\n')) copied+='\n';
      settings+=copied;
   }
   if (declaration.isEmpty()) throw std::runtime_error("Cannot locate this resource's declaration safely. Clone it in the script that defines it.");
   if (!located) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before cloning this resource.");
   auto block=declaration+settings;
   const auto cameras=qtCameraSettings(source);
   if (cameras.contains(original)) block+=qtCameraDirective(name,cameras.value(original));
   if (boundary>0 && source[boundary-1]!='\n') block.prepend('\n');
   auto result=source; result.insert(boundary,block);
   if (declaration.startsWith("Create Array ") && arrayExpressions(source,original)!="[]") {
      // The GUI's bounded array initializer is kept after BeginMissionSequence.
      // Copy it after the original block, retaining expression/dependency order;
      // other mission assignments to the original array are never duplicated.
      const auto marker="^[ \\t]*% GMAT-Qt-Array-Expressions "+QRegularExpression::escape(original);
      const auto begin=QRegularExpression(marker+" begin[ \\t]*(?:\\n|$)",QRegularExpression::MultilineOption).match(result);
      const auto end=QRegularExpression(marker+" end[ \\t]*(?:\\n|$)",QRegularExpression::MultilineOption).match(result);
      auto copied=result.mid(begin.capturedStart(),end.capturedEnd()-begin.capturedStart());
      copied.replace("% GMAT-Qt-Array-Expressions "+original+" begin","% GMAT-Qt-Array-Expressions "+name+" begin");
      copied.replace("% GMAT-Qt-Array-Expressions "+original+" end","% GMAT-Qt-Array-Expressions "+name+" end");
      copied.replace(QRegularExpression("^([ \\t]*(?:GMAT[ \\t]+)?)"+QRegularExpression::escape(original)+"(?=\\()",QRegularExpression::MultilineOption),"\\1"+name);
      if (!copied.endsWith('\n')) copied+='\n';
      result.insert(end.capturedEnd(),copied);
   }
   return result;
}
QString qtConfiguredScript()
{
   auto *moderator=Moderator::Instance();
   auto script=QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING));
   auto *system=moderator->GetSolarSystemInUse();
   if (system && system->GetStringParameter("EphemerisSource")=="SPICE") {
      // Engine parameter order writes SPICE before a modified DEFilename.
      // That filename setter needs a selected DE source. Recreate the ordered
      // fallback pair so subsequent resource/mission edits can rebuild it.
      const auto settings=solarSystemSettings(*system);
      script=solarSystemScript(script,settings,settings.values,{});
   }
   return script;
}
}

MainWindow::MainWindow()
{
   debugger=new Debugger(this);
   debugger->resumeRequested=[this] { resumeMission(); };
   debugger->stopRequested=[this] { stopMission(); };
   debugger->helpRequested=[this] { contextHelp->show("Breakpoint"); };
   debugger->paused=[this](const QString &command) {
      paused=true; runAction->setText("&Resume mission"); runAction->setEnabled(true);
      pauseAction->setEnabled(false); stepAction->setEnabled(true);
      statusBar()->showMessage("Debug paused before "+command);
   };
   // Establish OpenGL composition before the top-level window is first shown.
   // Otherwise Qt replaces the native surface when the first OrbitView is
   // added during Run, which can disrupt Wayland input/focus on the old surface.
   // Keep this hidden child alive even when all plot windows have been closed.
   if (QGuiApplication::platformName()!="offscreen" && QGuiApplication::platformName()!="minimal") {
      auto *compositionAnchor=new QOpenGLWidget(this);
      compositionAnchor->setObjectName("compositionAnchor");
      compositionAnchor->resize(1,1);
      compositionAnchor->hide();
   }
   resize(1280, 850);
   workspace = new QMdiArea(this);
   workspace->setObjectName("workspace");
   workspace->setBackground(palette().mid());
   setCentralWidget(workspace);
   plots = std::make_unique<QtPlotReceiver>(workspace);
   solverListeners=std::make_unique<QtSolverListenerManager>(workspace);
   auto *navigation = new QDockWidget("Mission workspace", this);
   navigation->setObjectName("navigation");
   auto *tabs = new QTabWidget(navigation);
   auto makeTree = [tabs](const QString &name) {
      auto *tree = new NavigationTree(tabs);
      tree->setObjectName(name);
      tree->setHeaderHidden(true);
      tree->setMinimumWidth(230);
      tree->setAlternatingRowColors(false);
      tabs->addTab(tree, name);
      return tree;
   };
   resources = makeTree("Resources");
   mission = makeTree("Mission");
   output = makeTree("Output");
   missionNavigation=new MissionNavigation(mission,tabs,this);
   missionNavigation->commandTypes=[] { QStringList types; for (const auto &type:Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND)) types.append(QString::fromStdString(type)); return types; };
   mission->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(mission, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      if (item->data(0,Qt::UserRole).isValid()) openCommandEditor(item->data(0,Qt::UserRole).toInt(),MissionEdit::Replace);
   });
   connect(mission, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &position) {
      if (!ready || running || !modelValid) return;
      auto *item=mission->itemAt(position);
      const int index=item && item->data(0,Qt::UserRole).isValid() ? item->data(0,Qt::UserRole).toInt() : -1;
      QMenu menu(this);
      QAction *edit=nullptr,*before=nullptr,*after=nullptr,*inside=nullptr,*remove=nullptr,*summary=nullptr,*breakpoint=nullptr;
      const int branchEnd=missionBranchEnd(missionState,index);
      if (index>=0 && index<missionState.nodes.size()) {
         edit=menu.addAction("Edit command…"); edit->setEnabled(missionState.nodes[index].editable);
         before=menu.addAction("Insert before…"); before->setEnabled(missionState.nodes[index].type!="BeginMissionSequence");
         after=menu.addAction("Insert after…");
         if (branchEnd>=0) {
            inside=menu.addAction("Append inside branch…"); inside->setObjectName("appendInsideMissionBranch");
            inside->setToolTip("Add a command before this branch's closing command");
         }
         remove=menu.addAction("Delete command"); remove->setEnabled(missionState.nodes[index].editable);
         summary=menu.addAction("Command summary…");
         breakpoint=menu.addAction("Breakpoint before command"); breakpoint->setObjectName("missionBreakpoint");
         breakpoint->setCheckable(true); breakpoint->setChecked(breakpoints.contains(index)); breakpoint->setEnabled(!running && modelValid && editor->toPlainText()==builtScript && missionState.nodes[index].type!="BeginMissionSequence");
         menu.addSeparator();
      }
      auto *missionSummary=menu.addAction("Mission summary…");
      auto *append=menu.addAction("Append command…");
      auto *chosen=menu.exec(mission->viewport()->mapToGlobal(position));
      if (!chosen) return;
      if (chosen==missionSummary) showSummary();
      else if (summary && chosen==summary) showSummary(index);
      else if (breakpoint && chosen==breakpoint) setBreakpoint(index,breakpoint->isChecked());
      else if (chosen==append) openCommandEditor(-1,MissionEdit::Append);
      else if (inside && chosen==inside) openCommandEditor(branchEnd,MissionEdit::InsertBefore);
      else if (chosen==edit) openCommandEditor(index,MissionEdit::Replace);
      else if (chosen==before) openCommandEditor(index,MissionEdit::InsertBefore);
      else if (chosen==after) openCommandEditor(index,MissionEdit::InsertAfter);
      else if (chosen==remove) {
         const auto snapshot=missionState;
         if (QMessageBox::question(this,"Delete command","Delete this command and any commands inside its branch?",
             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes) {
            const auto error=applyMissionChange(snapshot,index,MissionEdit::Remove,{});
            if (!error.isEmpty()) statusBar()->showMessage(error);
         }
      }
   });
   plots->changed = [this] { refreshOutput(); if (plotPlayback) plotPlayback->syncFromReceiver(); };
   plots->replayChanged = [this] { if (plotPlayback) plotPlayback->syncFromReceiver(); };
   plots->saveProjection=[this](const QString &name,bool perspective,double fov) { return savePlotProjection(name,perspective,fov); };
   plots->protectedPaths=[this] {
      QStringList paths{scriptPath,startupFile};
      for (const auto &document:scriptDocuments) paths.append(document->path);
      paths.append(reportFiles.values());
      for (auto it=ephemerisFiles.cbegin();it!=ephemerisFiles.cend();++it) paths.append(it.value().first);
      return paths;
   };
   connect(output, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      const auto name = item->data(0, Qt::UserRole).toString();
      const auto type=item->data(0,Qt::UserRole+1).toString(),format=item->data(0,Qt::UserRole+2).toString();
      if (type=="ephemeris" && format!="CCSDS-OEM" && format!="STK-TimePosVel") {
         if (running) { statusBar()->showMessage("Wait until the mission stops before opening its ephemeris file"); return; }
         const QFileInfo file(name); if (!file.isFile()) { statusBar()->showMessage("Ephemeris file is not available: "+name); return; }
         auto *viewer=new QWidget; viewer->setObjectName("ephemerisFileDetails"); auto *layout=new QVBoxLayout(viewer);
         auto *details=new QLabel(QString("Binary ephemeris file\nFormat: %1\nPath: %2\nSize: %3 bytes").arg(format,name).arg(file.size()),viewer); details->setObjectName("ephemerisFileInfo"); details->setWordWrap(true); details->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(details);
         auto *folder=new QPushButton("Open folder",viewer); folder->setObjectName("ephemerisOpenFolder"); layout->addWidget(folder); connect(folder,&QPushButton::clicked,viewer,[file] { QDesktopServices::openUrl(QUrl::fromLocalFile(file.absolutePath())); });
         auto *copy=new QPushButton("Copy path",viewer); copy->setObjectName("ephemerisCopyPath"); layout->addWidget(copy); connect(copy,&QPushButton::clicked,viewer,[name] { QApplication::clipboard()->setText(name); }); layout->addStretch();
         auto *child=workspace->addSubWindow(viewer); child->setAttribute(Qt::WA_DeleteOnClose); child->setWindowTitle(item->text(0)+" — "+format); child->resize(650,250); showNewWorkspaceWindow(workspace,child); workspace->setActiveSubWindow(child); child->raise(); viewer->setFocus();
      } else if (type=="report" || type=="ephemeris") {
         if (running) { statusBar()->showMessage("Wait until the mission stops before opening its report"); return; }
         if (dynamic_cast<EventLocator *>(Moderator::Instance()->GetConfiguredObject(item->text(0).toStdString()))) {
            bool written=false;
            if (modelValid && summaryAvailable && editor->toPlainText()==builtScript) {
               try { if (auto *locator=dynamic_cast<EventLocator *>(Moderator::Instance()->GetInternalObject(item->text(0).toStdString()))) written=locator->FileWasWritten(); }
               catch (BaseException &) { } // Initialization may fail before a running locator exists.
            }
            if (!written) {
               // An older file may exist even though this build/run has not
               // located events. Match wx's FileWasWritten guard before I/O.
               auto *viewer=new QPlainTextEdit;
               viewer->setObjectName("report:"+item->text(0)); viewer->setReadOnly(true);
               viewer->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
               viewer->setLineWrapMode(QPlainTextEdit::NoWrap);
               viewer->setPlainText("No event report was written for the current mission.\n\nEnable WriteReport and run the mission with the locator enabled.\nIn Manual mode, include FindEvents, then reopen this report.");
               viewer->setToolTip(name);
               auto *child=workspace->addSubWindow(viewer); child->setAttribute(Qt::WA_DeleteOnClose);
               child->setWindowTitle(item->text(0)+" — "+name); child->resize(750,500); showNewWorkspaceWindow(workspace,child); workspace->setActiveSubWindow(child); child->raise(); viewer->setFocus(); return;
            }
         }
         QFile file(name);
         if (!file.open(QIODevice::ReadOnly)) { statusBar()->showMessage("Report is not available: " + file.errorString()); return; }
         auto *viewer = new ReportViewer(name,item->text(0),nullptr,[this,name] { showFileComparison(name); });
         auto *child = workspace->addSubWindow(viewer);
         child->setAttribute(Qt::WA_DeleteOnClose);
         child->setWindowTitle(item->text(0) + " — " + name);
         viewer->setToolTip(name);
         child->resize(750,500); showNewWorkspaceWindow(workspace,child); workspace->setActiveSubWindow(child); child->raise(); viewer->setFocus();
      } else if (!name.isEmpty()) plots->show(name);
   });
   navigation->setWidget(tabs);
   addDockWidget(Qt::LeftDockWidgetArea, navigation);
   auto *console = new QDockWidget("Message Window", this);
   console->setObjectName("messages");
   messages = new QPlainTextEdit(console);
   messages->setReadOnly(true);
   messages->setMaximumBlockCount(10000);
   messages->setObjectName("messageWindow");
   messages->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   console->setWidget(messages);
   addDockWidget(Qt::BottomDockWidgetArea, console);
   resizeDocks({navigation}, {270}, Qt::Horizontal);
   resizeDocks({console}, {160}, Qt::Vertical);
   activeDocument=createScriptDocument({},{}); editor=activeDocument->editor;
   editor->setObjectName("scriptEditor"); activeDocument->window->showMaximized();
   connect(workspace,&QMdiArea::subWindowActivated,this,[this](QMdiSubWindow *) { updateScriptDocuments(); });
   auto *file = menuBar()->addMenu("&File");
   auto *edit = menuBar()->addMenu("&Edit");
   auto *view = menuBar()->addMenu("&View");
   auto *showScript=view->addAction("Active script"); showScript->setObjectName("showActiveScript");
   connect(showScript,&QAction::triggered,this,[this] { if (activeDocument->window->isMinimized() || !activeDocument->window->isVisible()) activeDocument->window->showNormal(); workspace->setActiveSubWindow(activeDocument->window); editor->setFocus(); });
   auto *run = menuBar()->addMenu("&Mission");
   auto *windows = menuBar()->addMenu("&Window");
   auto *help = menuBar()->addMenu("&Help");
   contextHelp=new HelpController(this);
   auto *contents=help->addAction("GMAT &Help"); contents->setObjectName("helpContentsAction");
   connect(contents,&QAction::triggered,this,[this] { contextHelp->show("index"); });
   auto *usingGmat=help->addAction("Using GMAT"); usingGmat->setObjectName("helpUsingGmat");
   connect(usingGmat,&QAction::triggered,this,[this] { contextHelp->show("UsingGmat"); });
   auto *tutorials=help->addAction("Tutorials"); tutorials->setObjectName("helpTutorials");
   connect(tutorials,&QAction::triggered,this,[this] { contextHelp->show("Tutorials"); });
   help->addSeparator();
   auto *toolbar = addToolBar("Standard");
   toolbar->setObjectName("standardToolbar");
   auto add = [this, toolbar](QMenu *menu, const QString &label, QStyle::StandardPixmap icon, const QKeySequence &key, auto callback) {
      auto *action = menu->addAction(style()->standardIcon(icon), label);
      action->setShortcut(key);
      connect(action, &QAction::triggered, this, callback);
      toolbar->addAction(action);
      editingActions.append(action);
      return action;
   };
   add(file, "&New mission", QStyle::SP_FileIcon, QKeySequence::New, [this] { if (confirmDiscard()) newMission(); });
   add(file, "&Open script…", QStyle::SP_DialogOpenButton, QKeySequence::Open, [this] {
      const auto path = QFileDialog::getOpenFileName(this, "Open GMAT script", scriptPath, "GMAT scripts (*.script);;All files (*)");
      if (!path.isEmpty()) openMissionFile(path);
   });
   auto *openAnother=file->addAction("Open another script window…"); openAnother->setObjectName("openScriptDocument"); openAnother->setShortcut(QKeySequence("Ctrl+Shift+O")); editingActions.append(openAnother);
   connect(openAnother,&QAction::triggered,this,[this] { const auto path=QFileDialog::getOpenFileName(this,"Open another GMAT script",scriptPath,"GMAT scripts (*.script);;All files (*)"); if (!path.isEmpty()) openScriptDocument(path); });
   auto *newScript=file->addAction("New script window"); newScript->setObjectName("newScriptDocument"); newScript->setShortcut(QKeySequence("Ctrl+Shift+N")); editingActions.append(newScript);
   connect(newScript,&QAction::triggered,this,[this] { if (!running) { auto document=createScriptDocument({},{}); document->window->show(); workspace->setActiveSubWindow(document->window); document->editor->setFocus(); updateScriptDocuments(); } });
   auto *reload=file->addAction("Reload selected script…"); reload->setObjectName("reloadScriptDocument"); editingActions.append(reload); connect(reload,&QAction::triggered,this,&MainWindow::reloadScriptDocument);
   auto *closeScript=file->addAction("Close selected script"); closeScript->setObjectName("closeScriptDocument"); closeScript->setShortcut(QKeySequence("Ctrl+W")); editingActions.append(closeScript); connect(closeScript,&QAction::triggered,this,[this] { const auto document=selectedScriptDocument(); if (document && document->window) document->window->close(); });
   recentMenu=file->addMenu("Recent missions"); recentMenu->setObjectName("recentMissionsMenu"); connect(recentMenu,&QMenu::aboutToShow,this,&MainWindow::refreshRecentMenu); refreshRecentMenu();
   add(file, "&Save", QStyle::SP_DialogSaveButton, QKeySequence::Save, [this] { saveScript(); })->setObjectName("saveScript");
   auto *saveAs = file->addAction("Save &As…"); saveAs->setObjectName("saveScriptAs");
   saveAs->setShortcut(QKeySequence::SaveAs);
   editingActions.append(saveAs);
   connect(saveAs, &QAction::triggered, this, [this] { saveScript(true); });
   file->addSeparator();
   auto *compareFiles=file->addAction("Compare files…"); compareFiles->setObjectName("compareFiles");
   editingActions.append(compareFiles);
   connect(compareFiles,&QAction::triggered,this,[this] { showFileComparison(); });
   file->addSeparator();
   connect(file->addAction("E&xit"), &QAction::triggered, this, &QWidget::close);
   connect(qApp,&QApplication::focusChanged,this,[this](QWidget *,QWidget *focused) {
      // Menus temporarily take focus; keep the editor they were opened from.
      if (!focused || qobject_cast<QMenu *>(focused) || focused==menuBar()) return;
      // QWidget::isAncestorOf stops at an owned dialog's window boundary.
      // Follow ownership so its fields retain their local editing history.
      bool owned=false;
      for (auto *parent=focused;parent;parent=parent->parentWidget())
         if (parent==this) { owned=true; break; }
      if (owned && (qobject_cast<QPlainTextEdit *>(focused) ||
          qobject_cast<QTextEdit *>(focused) || qobject_cast<QLineEdit *>(focused))) textEditTarget=focused;
      else textEditTarget.clear();
   });
   auto editAction = [edit, this](const QString &label, const QKeySequence &key, const char *slot) {
      auto *action = edit->addAction(label); action->setShortcut(key);
      action->setObjectName(QString("edit_")+slot);
      connect(action, &QAction::triggered, this, [this,slot] {
         auto *modal=QApplication::activeModalWidget();
         if (textEditTarget) {
            // A field keeps its own history, including fields in modal editors.
            // Never reuse a target behind an active modal dialog.
            if (textEditTarget->isEnabled() && (!modal || modal==textEditTarget || modal->isAncestorOf(textEditTarget)))
               QMetaObject::invokeMethod(textEditTarget,slot,Qt::DirectConnection);
            return;
         }
         // GUI Apply records one script edit even when a tree, plot or button
         // has focus. Preserve selected inactive documents; clipboard actions
         // and modal controls must never fall back to the mission source.
         if (modal || (qstrcmp(slot,"undo") && qstrcmp(slot,"redo"))) return;
         const auto document=selectedScriptDocument();
         if (document && document->editor && document->editor->isEnabled() && !document->editor->isReadOnly())
            QMetaObject::invokeMethod(document->editor,slot,Qt::DirectConnection);
      });
      editingActions.append(action);
   };
   editAction("&Undo", QKeySequence::Undo, "undo");
   editAction("&Redo", QKeySequence::Redo, "redo");
   edit->addSeparator();
   editAction("Cu&t", QKeySequence::Cut, "cut");
   editAction("&Copy", QKeySequence::Copy, "copy");
   editAction("&Paste", QKeySequence::Paste, "paste");
   edit->addSeparator();
   auto searchAction=[&](const QString &label,const QString &name,QKeySequence key,auto callback) {
      auto *action=edit->addAction(label); action->setObjectName(name); action->setShortcut(key);
      connect(action,&QAction::triggered,this,[this,callback] {
         if (QApplication::activeModalWidget()) return;
         if (auto *child=workspace->activeSubWindow()) {
            auto *report=dynamic_cast<ReportViewer *>(child->widget());
            // Comparison results embed the same read-only report viewer.
            if (!report) for (auto *widget:child->widget()->findChildren<QWidget *>())
               if ((report=dynamic_cast<ReportViewer *>(widget))) break;
            if (report) { callback(report->searchDialog()); return; }
         }
         const auto document=selectedScriptDocument(); if (!document || !document->window) return;
         if (document->window->isMinimized() || !document->window->isVisible()) document->window->showNormal();
         workspace->setActiveSubWindow(document->window); callback(document->search);
      });
   };
   searchAction("&Find…","scriptFind",QKeySequence::Find,[](FindReplaceDialog *search) { search->openSearch(); });
   searchAction("&Replace…","scriptReplace",QKeySequence::Replace,[](FindReplaceDialog *search) { search->openSearch(); });
   searchAction("Find next","scriptFindNext",QKeySequence::FindNext,[](FindReplaceDialog *search) { search->findNext(); });
   searchAction("Find previous","scriptFindPrevious",QKeySequence::FindPrevious,[](FindReplaceDialog *search) { search->findNext(true); });
   auto *goToLine=edit->addAction("Go to line…"); goToLine->setObjectName("scriptGoToLine"); goToLine->setShortcut(QKeySequence("Ctrl+L"));
   connect(goToLine,&QAction::triggered,this,[this] {
      if (QApplication::activeModalWidget()) return;
      const auto document=selectedScriptDocument(); if (!document || !document->window) return;
      if (document->window->isMinimized() || !document->window->isVisible()) document->window->showNormal();
      workspace->setActiveSubWindow(document->window);
      auto *editor=document->editor.data(); bool accepted=false;
      const int line=QInputDialog::getInt(this,"Go to line","Line number",editor->textCursor().blockNumber()+1,
         1,editor->document()->blockCount(),1,&accepted);
      if (accepted) {
         editor->setTextCursor(QTextCursor(editor->document()->findBlockByNumber(line-1)));
         editor->centerCursor(); editor->setFocus();
      }
   });

   edit->addSeparator();
   auto *paths=edit->addAction("Set paths…"); paths->setObjectName("setPaths"); editingActions.append(paths);
   connect(paths,&QAction::triggered,this,&MainWindow::showPathSettings);
   auto *create=edit->addAction("New &resource…");
   create->setObjectName("createResource"); editingActions.append(create);
   connect(create,&QAction::triggered,this,[this] { showCreateResource(); });
   auto *clone=edit->addAction("Clone resource…"); clone->setObjectName("cloneResource");
   clone->setShortcut(QKeySequence("Ctrl+Shift+C")); clone->setShortcutContext(Qt::WidgetWithChildrenShortcut);
   resources->addAction(clone); editingActions.append(clone);
   connect(clone,&QAction::triggered,this,[this] {
      const auto *item=resources->currentItem();
      showCloneResource(item && !item->data(0,Qt::UserRole).toString().isEmpty() ? item->text(0) : QString());
   });
   auto *rename=edit->addAction("Rename resource…"); rename->setObjectName("renameResource");
   rename->setShortcut(QKeySequence("F2")); rename->setShortcutContext(Qt::WidgetWithChildrenShortcut);
   resources->addAction(rename); editingActions.append(rename);
   connect(rename,&QAction::triggered,this,[this] {
      const auto *item=resources->currentItem();
      showRenameResource(item && !item->data(0,Qt::UserRole).toString().isEmpty() ? item->text(0) : QString());
   });
   auto *convert=edit->addAction("Convert OpenFrames views for Qt");
   convert->setObjectName("convertOpenFramesViews"); editingActions.append(convert);
   connect(convert,&QAction::triggered,this,[this] { convertOpenFramesScript(); });
   resources->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(resources,&QTreeWidget::customContextMenuRequested,this,[this,create](const QPoint &position) {
      auto *item=resources->itemAt(position);
      const QString name=item && !item->data(0,Qt::UserRole).toString().isEmpty() ? item->text(0) : QString();
      const QString snapshot=builtScript;
      enum class Operation { None,Create,Clone,Rename,Delete }; Operation operation=Operation::None; QString type;
      {
         QMenu menu(this); menu.setObjectName("resourceContextMenu");
         auto *category=item;
         while (category && !category->data(0,Qt::UserRole+1).isValid()) category=category->parent();
         if (category) {
            const auto group=category->data(0,Qt::UserRole+1).toUInt();
            QStringList types;
            if (group==Gmat::PARAMETER) types={"Variable","Array","String"};
            else if (group==Gmat::GROUND_STATION) types={"GroundStation"};
            else for (const auto &type:Moderator::Instance()->GetListOfViewableItems(group)) types.append(QString::fromStdString(type));
            // Smoother is registered in its own factory but displayed with solvers.
            if (group==Gmat::SOLVER) types.append("Smoother");
            const auto available=creatableResourceTypes();
            types.removeDuplicates(); types.sort();
            for (const auto &type:types) if (available.contains(type)) {
               auto *add=menu.addAction("Add "+type+"…"); add->setObjectName("addResource_"+type); add->setData(type);
               add->setEnabled(create->isEnabled());
            }
            if (menu.actions().isEmpty()) { auto *unavailable=menu.addAction("No resource types available"); unavailable->setEnabled(false); }
         } else {
            auto *generic=menu.addAction(create->text()); generic->setObjectName("createResource"); generic->setEnabled(create->isEnabled());
         }
         menu.addSeparator();
         auto *selected=name.isEmpty() ? nullptr : Moderator::Instance()->GetConfiguredObject(name.toStdString());
         auto *clone=menu.addAction("Clone resource…"); clone->setObjectName("cloneResource");
         clone->setEnabled(ready && !running && modelValid && editor->toPlainText()==builtScript && resourceCanBeCloned(selected));
         auto *rename=menu.addAction("Rename resource…"); rename->setObjectName("renameResource");
         rename->setEnabled(ready && !running && modelValid && editor->toPlainText()==builtScript && resourceCanBeRenamed(selected));
         auto *remove=menu.addAction("Delete resource…");
         remove->setEnabled(ready && !running && modelValid && selected && !selected->IsOfType("CelestialBody") && !selected->IsOfType("SolarSystem"));
         const auto *chosen=menu.exec(resources->viewport()->mapToGlobal(position));
         if (chosen && (chosen->objectName().startsWith("addResource_") || chosen->objectName()=="createResource")) { operation=Operation::Create; type=chosen->data().toString(); }
         else if (chosen==clone) operation=Operation::Clone;
         else if (chosen==rename) operation=Operation::Rename;
         else if (chosen==remove) operation=Operation::Delete;
      } // Destroy the popup before opening another modal surface; retain values only.
      if (operation==Operation::Create) showCreateResource(type);
      else if (operation==Operation::Clone) showCloneResource(name);
      else if (operation==Operation::Rename) showRenameResource(name);
      else if (operation==Operation::Delete &&
          QMessageBox::question(this,"Delete resource","Delete "+name+" from this mission?",
             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes) {
         const auto error=deleteResource(name,snapshot);
         if (!error.isEmpty()) QMessageBox::warning(this,"Cannot delete resource",error);
      }
   });
   view->addAction(navigation->toggleViewAction());
   view->addAction(console->toggleViewAction());
   view->addAction(missionNavigation->detachAction());
   toolbar->addSeparator();
   add(run, "&Build script", QStyle::SP_BrowserReload, QKeySequence("F7"), [this] { buildScript(); });
   runAction = add(run, "&Run mission", QStyle::SP_MediaPlay, QKeySequence("F5"), [this] {
      if (paused) resumeMission(); else runMission();
   });
   runAction->setObjectName("runMission");
   editingActions.removeOne(runAction);
   pauseAction = add(run, "&Pause", QStyle::SP_MediaPause, QKeySequence("F6"), [this] { pauseMission(); });
   pauseAction->setObjectName("pauseMission");
   editingActions.removeOne(pauseAction);
   stopAction = add(run, "&Stop", QStyle::SP_MediaStop, QKeySequence("Shift+F5"), [this] { stopMission(); });
   stopAction->setObjectName("stopMission");
   editingActions.removeOne(stopAction);
   auto *debug=add(run,"Debug mission…",QStyle::SP_MediaPlay,QKeySequence("Ctrl+F5"),[this] { debugMission(); }); debug->setObjectName("debugMission");
   // A bug pictogram distinguishes Debug from Run even without a theme icon.
   QPixmap bug(32,32); bug.fill(Qt::transparent);
   {
      QPainter painter(&bug); painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(QPen(palette().buttonText().color(),2.5,Qt::SolidLine,Qt::RoundCap));
      for (int y:{14,20,26}) { painter.drawLine(5,y-2,11,y); painter.drawLine(21,y,27,y-2); }
      painter.drawLine(12,7,9,3); painter.drawLine(20,7,23,3);
      painter.setBrush(palette().buttonText()); painter.drawEllipse(QRectF(12,5,8,8));
      painter.setBrush(QColor(230,150,40)); painter.drawEllipse(QRectF(10,10,12,19));
      painter.drawLine(16,12,16,27);
   }
   debug->setIcon(QIcon(bug));
   toolbar->addSeparator();
   plotPlayback=new PlotPlaybackControls(*plots,toolbar);
   toolbar->addWidget(plotPlayback);
   stepAction=run->addAction("Step command"); stepAction->setObjectName("stepMission"); stepAction->setShortcut(QKeySequence("F10")); stepAction->setShortcutContext(Qt::ApplicationShortcut);
   connect(stepAction,&QAction::triggered,this,[this] { stepMission(); });
   auto *clearBreakpoints=run->addAction("Clear breakpoints"); clearBreakpoints->setObjectName("clearBreakpoints"); editingActions.append(clearBreakpoints);
   connect(clearBreakpoints,&QAction::triggered,this,[this] { breakpoints.clear(); refreshBreakpoints(); });
   run->addSeparator();
   auto *folder=run->addAction("Run scripts from folder…"); folder->setObjectName("runScriptFolder"); editingActions.append(folder);
   connect(folder,&QAction::triggered,this,[this] {
      FolderRunDialog dialog(QString::fromStdString(FileManager::Instance()->GetFullPathname("OUTPUT_PATH")),
         [this](const FolderRunOptions &options,QtPlotReceiver &views,const std::atomic_bool &cancel,const auto &progress) { return runFolderScripts(options,views,cancel,progress); },
         [this] { stopMission(); },this); dialog.exec();
   });
   auto *activate=run->addAction("Make selected script active"); activate->setObjectName("activateScriptDocument"); activate->setShortcut(QKeySequence("Ctrl+Alt+F7")); editingActions.append(activate);
   connect(activate,&QAction::triggered,this,[this] { activateScriptDocument(selectedScriptDocument()); });
   auto *saveBuild=run->addAction("Save and build script"); saveBuild->setObjectName("saveBuildScript");
   saveBuild->setShortcut(QKeySequence("Ctrl+Shift+F7")); editingActions.append(saveBuild);
   connect(saveBuild,&QAction::triggered,this,[this] { saveAndBuildScript(false); });
   auto *saveRun=run->addAction("Save, build and run mission"); saveRun->setObjectName("saveRunMission");
   saveRun->setShortcut(QKeySequence("Ctrl+Shift+F5")); editingActions.append(saveRun);
   connect(saveRun,&QAction::triggered,this,[this] { saveAndBuildScript(true); });
   auto *summary=run->addAction("Mission summary…"); summary->setObjectName("missionSummary"); editingActions.append(summary);
   connect(summary,&QAction::triggered,this,[this] { showSummary(); });
   setRunning(false);
   windows->setObjectName("windowMenu");
   connect(windows,&QMenu::aboutToShow,this,[this,windows] {
      windows->clear();
      connect(windows->addAction("&Tile"),&QAction::triggered,workspace,&QMdiArea::tileSubWindows);
      connect(windows->addAction("&Cascade"),&QAction::triggered,workspace,&QMdiArea::cascadeSubWindows);
      connect(windows->addAction("&Next window"),&QAction::triggered,workspace,&QMdiArea::activateNextSubWindow);
      connect(windows->addAction("&Previous window"),&QAction::triggered,workspace,&QMdiArea::activatePreviousSubWindow);
      windows->addSeparator();
      for (auto *child:workspace->subWindowList()) {
         if (!child->isVisible()) continue;
         auto *action=windows->addAction(child->windowTitle().replace("&","&&"));
         action->setObjectName("windowEntry"); action->setCheckable(true);
         action->setChecked(child==workspace->activeSubWindow());
         action->setData(child->windowTitle());
         const QPointer<QMdiSubWindow> guarded=child;
         connect(action,&QAction::triggered,this,[this,guarded] {
            if (!guarded) return;
            if (guarded->isMinimized()) guarded->showNormal();
            workspace->setActiveSubWindow(guarded);
            if (guarded->property("configurationPanel").toBool()) keepWorkspaceWindowReachable(workspace,guarded);
            guarded->raise(); guarded->widget()->setFocus();
         });
      }
   });
   auto *welcomeAction=help->addAction("Welcome…"); welcomeAction->setObjectName("showWelcome"); editingActions.append(welcomeAction); connect(welcomeAction,&QAction::triggered,this,[this] { showWelcome(); });
   auto *about=help->addAction("&About GMAT"); about->setObjectName("aboutGMAT");
   connect(about, &QAction::triggered, this, [this] {
      AboutDialog dialog(QString::fromStdString(FileManager::Instance()->GetRootPath()),this); dialog.exec();
   });
   auto *capabilities=help->addAction("Available engine types…");
   capabilities->setObjectName("engineCapabilities");
   connect(capabilities,&QAction::triggered,this,[this] {
      QDialog dialog(this); dialog.setWindowTitle("Available engine types");
      auto *layout=new QVBoxLayout(&dialog);
      auto *description=new QLabel("These types are registered by this runtime and its loaded plugins. They are available to scripts; some properties still require the script editor. OpenFrames viewer definitions can be converted from the Edit menu. wx-only plugin panels are not loaded by Qt.",&dialog);
      description->setWordWrap(true); layout->addWidget(description);
      auto *types=new QPlainTextEdit(availableEngineTypes().join('\n'),&dialog); types->setReadOnly(true); layout->addWidget(types);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog); layout->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      dialog.resize(650,550); dialog.exec();
   });
   connect(resources, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      if (!ready || running || item->data(0, Qt::UserRole).toString().isEmpty()) return;
      auto *object = Moderator::Instance()->GetConfiguredObject(item->text(0).toStdString());
      if (!object) return;
      if (!modelValid || editor->toPlainText() != builtScript) {
         statusBar()->showMessage("Build the edited script before opening a resource panel");
         return;
      }
      const QString name = item->text(0);
      const QString snapshot = builtScript;
      for (auto *child:workspace->subWindowList()) {
         if (child->property("resourceName").toString()!=name) continue;
         auto *panel=dynamic_cast<EditablePanel *>(child->widget());
         if (!panel) continue;
         if (child->property("sourceScript").toString()==snapshot || panel->hasChanges()) {
            if (child->isMinimized()) child->showNormal();
            workspace->setActiveSubWindow(child);
            keepWorkspaceWindowReachable(workspace,child);
            child->raise(); panel->setFocus();
            if (child->property("sourceScript").toString()!=snapshot)
               statusBar()->showMessage("This panel has changes from an older mission. Close and discard them before reopening.");
            return;
         }
         child->close();
      }
      auto *panel=makeResourcePanel(*object,snapshot);
      auto *child = new EditorSubWindow;
      child->setWidget(panel);
      workspace->addSubWindow(child);
      child->setAttribute(Qt::WA_DeleteOnClose);
      child->setProperty("configurationPanel", true);
      child->setProperty("resourceName",name);
      child->setProperty("sourceScript",snapshot);
      child->setWindowTitle(name); child->resize(680,(object->GetTypeName()=="Variable" || object->GetTypeName()=="String") ? 240 : 540); showNewWorkspaceWindow(workspace,child);
   });
   QSettings settings;
   restoreGeometry(settings.value("geometry").toByteArray());
   restoreState(settings.value("windowState").toByteArray());
   missionNavigation->restorePlacement();
   updateTitle();
}

EditablePanel *MainWindow::makeResourcePanel(GmatBase &object,const QString &snapshot)
{
   const auto name=QString::fromStdString(object.GetName());
   const auto apply=[this,name,snapshot](const QMap<QString,QString> &changes) {
      return applyResourceChanges(name,changes,snapshot);
   };
   EditablePanel *panel;
   if (auto *system=dynamic_cast<SolarSystem *>(&object)) panel=new SolarSystemPanel(*system,apply);
   else if (auto *body=dynamic_cast<CelestialBody *>(&object)) panel=new CelestialBodyPanel(*body,apply);
   else panel=new ResourceEditor(object,apply,nullptr,snapshot);
   if (auto *buttons=panel->findChild<QDialogButtonBox *>()) {
      if (auto *button=buttons->button(QDialogButtonBox::Apply)) button->setObjectName("applyPanel");
      if (auto *button=buttons->button(QDialogButtonBox::Close)) button->setObjectName("closePanel");
   }
   panel->onApplied=[this] { refreshAppliedResourcePanels(); };
   contextHelp->attach(panel,object.IsOfType("CelestialBody") ? "CelestialBody" : QString::fromStdString(object.GetTypeName()));
   return panel;
}

void MainWindow::refreshAppliedResourcePanels()
{
   for (auto *child:workspace->subWindowList()) {
      const auto name=child->property("resourceName").toString();
      auto *old=dynamic_cast<EditablePanel *>(child->widget());
      if (name.isEmpty() || !old || old->hasChanges() || child->property("sourceScript").toString()==builtScript) continue;
      auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
      if (!object) continue;
      EditablePanel *replacement;
      try { replacement=makeResourcePanel(*object,builtScript); }
      catch (BaseException &error) {
         messages->appendPlainText(QString::fromStdString(error.GetFullMessage()));
         statusBar()->showMessage("Changes applied; close and reopen "+name+" to refresh its controls"); continue;
      } catch (const std::exception &error) {
         messages->appendPlainText(QString::fromUtf8(error.what()));
         statusBar()->showMessage("Changes applied; close and reopen "+name+" to refresh its controls"); continue;
      }
      // Preserve the visible page, search, adjusted columns and table position.
      for (auto *tabs:old->findChildren<QTabWidget *>()) if (!tabs->objectName().isEmpty())
         if (auto *fresh=replacement->findChild<QTabWidget *>(tabs->objectName())) fresh->setCurrentIndex(tabs->currentIndex());
      for (auto *tabs:old->findChildren<QTabBar *>()) if (!tabs->objectName().isEmpty())
         if (auto *fresh=replacement->findChild<QTabBar *>(tabs->objectName())) fresh->setCurrentIndex(tabs->currentIndex());
      if (auto *filter=old->findChild<QLineEdit *>("propertyFilter"))
         if (auto *fresh=replacement->findChild<QLineEdit *>("propertyFilter")) fresh->setText(filter->text());
      for (auto *table:old->findChildren<QTableWidget *>()) if (!table->objectName().isEmpty())
         if (auto *fresh=replacement->findChild<QTableWidget *>(table->objectName())) {
            for (int c=0;c<qMin(table->columnCount(),fresh->columnCount());++c) fresh->setColumnWidth(c,table->columnWidth(c));
            if (table->currentRow()>=0 && table->currentRow()<fresh->rowCount()) fresh->setCurrentCell(table->currentRow(),qMax(0,table->currentColumn()));
            fresh->verticalScrollBar()->setValue(table->verticalScrollBar()->value());
         }
      const auto *focus=QApplication::focusWidget();
      const bool focused=focus && (focus==old || old->isAncestorOf(focus));
      const auto focusName=focused ? focus->objectName() : QString();
      child->setWidget(replacement); child->setProperty("sourceScript",builtScript);
      old->hide(); old->deleteLater(); replacement->show();
      if (focused) {
         auto *target=focusName.isEmpty() ? nullptr : replacement->findChild<QWidget *>(focusName);
         (target ? target : replacement)->setFocus();
      }
   }
}

void MainWindow::showFileComparison(const QString &baseline)
{
   if (running) { statusBar()->showMessage("Wait until the mission stops before comparing its output files"); return; }
   auto *panel=new ComparisonPanel(baseline);
   auto *child=workspace->addSubWindow(panel); child->setAttribute(Qt::WA_DeleteOnClose);
   child->setWindowTitle("Compare files");
   child->resize(qBound(600,workspace->width(),980),qBound(480,workspace->height(),780)); child->show();
}
MainWindow::~MainWindow()
{
   disconnect(workspace,nullptr,this,nullptr);
   disconnect(qApp,nullptr,this,nullptr);
   for (const auto &document:scriptDocuments) if (document->editor) disconnect(document->editor->document(),nullptr,this,nullptr);
   for (const auto &document:scriptDocuments) if (auto *window=dynamic_cast<ScriptSubWindow *>(document->window.data())) window->mayClose={};
   plots->changed = {};
   plots->replayChanged = {};
   plotPlayback->stop();
   Moderator::SetUiInterpreter(nullptr);
   if (ready) Moderator::Instance()->Finalize();
   ListenerManagerInterface::SetListenerManager(nullptr);
   PlotInterface::SetPlotReceiver(nullptr);
   MessageInterface::SetMessageReceiver(nullptr);
}
bool MainWindow::initialize(const QString &startup)
{
   const auto compatibilityError=qtStartupCompatibilityError(startup);
   if (!compatibilityError.isEmpty()) {
      messages->appendPlainText(compatibilityError);
      statusBar()->showMessage("Runtime initialization failed");
      return false;
   }
   receiver = std::make_unique<QtMessageReceiver>();
   receiver->SetMessageCallback(this, [this](const QString &text) {
      messages->moveCursor(QTextCursor::End); messages->insertPlainText(text);
      if (folderMessages) *folderMessages+=text;
   });
   MessageInterface::SetMessageReceiver(receiver.get());
   PlotInterface::SetPlotReceiver(plots.get());
   ListenerManagerInterface::SetListenerManager(solverListeners.get());
   try {
      startupDirectory = QFileInfo(startup).absolutePath();
      startupFile = QFileInfo(startup).absoluteFilePath();
      // FileManager's default executable name is GMAT.exe. Supply the Qt
      // runtime explicitly so Linux paths do not depend on the working directory.
      QString runtime=QCoreApplication::applicationDirPath()+"/GmatQt";
      if (!QFileInfo::exists(runtime)) runtime=QDir::current().filePath("GmatQt");
      if (!QFileInfo::exists(runtime)) runtime=QCoreApplication::applicationFilePath();
      FileManager::Instance()->SetBinDirectory(QFileInfo(runtime).fileName().toStdString(),runtime.toStdString());
      ready = Moderator::Instance()->Initialize(startup.toStdString(), true);
      if (ready) {
         QStringList keywords;
         for (const auto &word:Moderator::Instance()->GetListOfAllFactoryItems()) keywords.append(QString::fromStdString(word));
         static_cast<ScriptEditor *>(editor)->setKeywords(keywords);
         interpreter = std::make_unique<QtInterpreter>();
         Moderator::SetUiInterpreter(interpreter.get());
         newMission();
      }
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   statusBar()->showMessage(ready ? "Ready" : "Runtime initialization failed");
   return ready;
}
void MainWindow::newMission()
{
   if (!ready || running) return;
   ++modelGeneration; summaryAvailable=false;
   scriptPath.clear();
   setScriptDirectory();
   plots->cameraSettings.clear();
   plots->clear(true);
   Moderator::Instance()->LoadDefaultMission();
   editor->setPlainText(qtConfiguredScript());
   builtScript = editor->toPlainText(); modelValid = true;
   scriptPath.clear(); activeDocument->path.clear(); activeDocument->savedText=editor->toPlainText(); editor->document()->setModified(false); refreshTrees(); updateTitle();
   activeDocument->window->show(); workspace->setActiveSubWindow(activeDocument->window); editor->setFocus();
}
bool MainWindow::loadScript(const QString &path)
{
   if (running) return false;
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) return false;
   }
   const auto identity=scriptDocumentIdentity(path);
   for (const auto &document:scriptDocuments) if (document!=activeDocument && !document->path.isEmpty() && scriptDocumentIdentity(document->path)==identity) return activateScriptDocument(document);
   QString text; if (!readScriptDocumentFile(this,path,text)) return false;
   ++modelGeneration; summaryAvailable=false;
   plots->clear(true);
   editor->setPlainText(text);
   scriptPath = QFileInfo(path).absoluteFilePath(); activeDocument->path=scriptPath; activeDocument->savedText=text; editor->document()->setModified(false); updateTitle(); rememberMissionFile(scriptPath); refreshRecentMenu(); activeDocument->window->show(); workspace->setActiveSubWindow(activeDocument->window); editor->setFocus(); return true;
}
bool MainWindow::saveScript(bool saveAs)
{
   return saveScriptDocument(selectedScriptDocument(),saveAs);
}
void MainWindow::saveAndBuildScript(bool run)
{
   if (!ready || running) return;
   const auto document=selectedScriptDocument(); if (!document) return;
   if (document->editor->toPlainText().trimmed().isEmpty()) {
      statusBar()->showMessage("Enter a mission script before saving and building"); return;
   }
   if (!saveScriptDocument(document)) return;
   const bool switching=document!=activeDocument;
   if (switching && !activateScriptDocument(document)) return;
   if (run) runMission(); else if (!switching) buildScript();
}
bool MainWindow::saveScriptTo(const QString &path)
{
   return saveScriptDocumentTo(activeDocument,path);
}
bool MainWindow::confirmDiscard()
{
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) {
         if (QMessageBox::question(this, "Unapplied panel changes",
             "Panels contain unapplied changes. Discard them and continue?",
             QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Discard)
            return false;
         break;
      }
   }
   if (editor->document()->isModified()) {
      const auto answer = QMessageBox::question(this, "Unsaved script", "Save your changes before continuing?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      if (answer != QMessageBox::Discard && !(answer == QMessageBox::Save && saveScriptDocument(activeDocument))) return false;
   }
   for (auto *child : workspace->subWindowList()) {
      if (auto *panel = dynamic_cast<EditablePanel *>(child->widget())) {
         panel->discardChanges();
         child->close();
      }
   }
   return true;
}
void MainWindow::refreshRecentMenu()
{
   recentMenu->clear();
   for (const auto &path:recentMissionFiles()) {
      auto *action=recentMenu->addAction(QFileInfo(path).fileName().replace("&","&&")); action->setToolTip(path); action->setData(path); action->setObjectName("recentMission");
      connect(action,&QAction::triggered,this,[this,path] { openMissionFile(path); });
   }
   if (recentMenu->actions().isEmpty()) { auto *empty=recentMenu->addAction("No recent missions"); empty->setEnabled(false); }
   else { recentMenu->addSeparator(); connect(recentMenu->addAction("Clear recent list"),&QAction::triggered,this,[this] { QSettings().remove("RecentFiles"); refreshRecentMenu(); }); }
   if (welcome) welcome->refreshRecent();
}
bool MainWindow::openMissionFile(const QString &path)
{
   if (!ready || running || !confirmDiscard()) return false;
   const bool opened=loadScript(path) && buildScript();
   if (opened && welcome) welcome->close();
   return opened;
}
void MainWindow::showWelcome(bool startup)
{
   if (!ready || running || (startup && !QSettings().value("Welcome/showOnStartup",true).toBool())) return;
   if (!welcome) welcome=new WelcomeDialog(QString::fromStdString(FileManager::Instance()->GetRootPath()),
      [this](const QString &path) { return openMissionFile(path); },
      [this] { if (!ready || running || !confirmDiscard()) return false; newMission(); return true; },
      [this](const QString &topic) { contextHelp->show(topic); },this);
   welcome->refreshRecent(); welcome->show(); welcome->raise(); welcome->activateWindow();
}
QStringList MainWindow::availableEngineTypes() const
{
   QStringList result;
   if (ready) for (const auto &name:Moderator::Instance()->GetListOfAllFactoryItems()) result.append(QString::fromStdString(name));
   result.removeDuplicates(); result.sort(); return result;
}
bool MainWindow::convertOpenFramesScript()
{
   if (!ready || running) return false;
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges()) {
         statusBar()->showMessage("Apply or discard the open panel changes before converting views"); return false;
      }
   const auto converted=convertOpenFramesViews(editor->toPlainText());
   if (!converted.error.isEmpty()) { messages->appendPlainText(converted.error); return false; }
   if (converted.script==editor->toPlainText()) { statusBar()->showMessage("No OpenFrames viewer definitions to convert"); return true; }
   const auto error=applyModelScript(converted.script);
   if (!error.isEmpty()) { messages->appendPlainText("View conversion rejected: "+error); return false; }
   messages->appendPlainText(QString("Converted %1 OpenFrames viewer(s) for Qt. Review the visual differences listed in the script before saving.").arg(converted.plots));
   for (const auto &note:converted.notes) messages->appendPlainText(note);
   statusBar()->showMessage("Views converted — review the script; Undo restores the original");
   return true;
}
bool MainWindow::buildScript()
{
   if (!ready || running) return false;
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) {
         statusBar()->showMessage("Apply or discard the open panel changes before building or running");
         return false;
      }
   }
   // File > Open and command-line loading both build through here, as does Run.
   // Offer conversion before the engine rejects unavailable OpenFrames types.
   const auto conversion=convertOpenFramesViews(editor->toPlainText());
   if (!conversion.error.isEmpty()) {
      QMessageBox::warning(this,"OpenFrames views need manual conversion",
         "This script uses OpenFrames features that cannot be converted automatically.\n\n"+conversion.error);
      statusBar()->showMessage("Build stopped — OpenFrames views need manual conversion");
      return false;
   }
   if (conversion.script!=editor->toPlainText()) {
      QMessageBox prompt(QMessageBox::Question,"Convert OpenFrames views?",
         "This script uses OpenFrames views, which are unavailable in the Qt interface. "
         "Convert them to Qt views and continue?\n\n"
         "Mission calculations will be preserved. Some visual settings differ. "
         "Changes stay unsaved, and Undo restores the original script.",
         QMessageBox::Yes|QMessageBox::No,this);
      prompt.setObjectName("openFramesConversionPrompt");
      prompt.button(QMessageBox::Yes)->setText("Convert views");
      prompt.button(QMessageBox::No)->setText("Keep original");
      prompt.setDefaultButton(QMessageBox::Yes);
      prompt.setEscapeButton(QMessageBox::No);
      if (!conversion.notes.isEmpty()) prompt.setDetailedText(conversion.notes.join("\n"));
      if (prompt.exec()!=QMessageBox::Yes) {
         statusBar()->showMessage("Build canceled — original OpenFrames script kept");
         return false;
      }
      if (!convertOpenFramesScript()) {
         QMessageBox::warning(this,"Conversion could not be completed",
            "The converted script could not be built. The original script is unchanged. See the Message Window for details.");
         return false;
      }
      return true; // Conversion already validated and rebuilt the mission.
   }
   ++modelGeneration; summaryAvailable=false;
   bool success = false;
   plots->clear();
   try {
      setScriptDirectory();
      const auto cameras=qtCameraSettings(editor->toPlainText());
      std::istringstream stream(editor->toPlainText().toStdString());
      success = qtInterpretScript(stream);
      if (success) { QtPlotReceiver::validateCameraReferences(cameras); plots->cameraSettings=cameras; }
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { success=false; messages->appendPlainText(QString::fromUtf8(error.what())); }
   catch (...) { messages->appendPlainText("Unexpected error while building the script."); }
   modelValid = success;
   if (success) builtScript = editor->toPlainText();
   refreshTrees(); statusBar()->showMessage(success ? "Build succeeded" : "Build failed — see Message Window"); return success;
}
void MainWindow::refreshTrees()
{
   if (breakpointSource!=builtScript) { breakpoints.clear(); breakpointSource=builtScript; }
   resources->clear(); mission->clear(); output->clear();
   reportFiles.clear();
   ephemerisFiles.clear();
   auto *root = new QTreeWidgetItem(resources, {"Resources"});
   auto *solarSystem=new QTreeWidgetItem(root,{"SolarSystem"});
   solarSystem->setData(0,Qt::UserRole,"SolarSystem");
   solarSystem->setToolTip(0,"Planetary ephemeris source, files and update settings");
   std::vector<std::pair<const char *, UnsignedInt>> groups = {
      {"Spacecraft", Gmat::SPACECRAFT}, {"Hardware", Gmat::HARDWARE},
      {"Formations", Gmat::FORMATION}, {"Ground Stations", Gmat::GROUND_STATION},
      {"Regions", Gmat::REGION},
      {"Propagators", Gmat::PROP_SETUP}, {"Burns", Gmat::BURN},
      {"Force Models", Gmat::ODE_MODEL},
      {"Coordinate Systems", Gmat::COORDINATE_SYSTEM}, {"Solvers", Gmat::SOLVER},
      {"Output", Gmat::SUBSCRIBER}, {"Variables, Arrays, Strings", Gmat::PARAMETER},
      {"Functions", Gmat::FUNCTION}, {"Event Locators",Gmat::EVENT_LOCATOR},
      {"Calculated Points",Gmat::CALCULATED_POINT}, {"Celestial Bodies",Gmat::CELESTIAL_BODY},
      {"Measurement Models",Gmat::MEASUREMENT_MODEL}, {"Error Models",Gmat::ERROR_MODEL},
      {"Interfaces",Gmat::INTERFACE}, {"Data Filters",Gmat::DATA_FILTER}, {"Fields of View",Gmat::FIELD_OF_VIEW}};
   for (const auto &entry:{std::pair{"Process Noise Models","ProcessNoiseModel"},std::pair{"Estimated Parameters","EstimatedParameter"}}) {
      const auto type=GmatType::GetTypeId(entry.second);
      if (type!=Gmat::UNKNOWN_OBJECT && !Moderator::Instance()->GetListOfFactoryItems(type).empty()) groups.push_back({entry.first,type});
   }
   for (const auto &group : groups) {
      auto *category = new QTreeWidgetItem(root, {group.first});
      category->setData(0,Qt::UserRole+1,group.second);
      for (const auto &name : Moderator::Instance()->GetListOfObjects(group.second)) {
         if (group.second == Gmat::SUBSCRIBER) {
            auto *object = Moderator::Instance()->GetConfiguredObject(name);
            if (object && object->IsOfType(Gmat::REPORT_FILE))
               reportFiles.insert(QString::fromStdString(name), QString::fromStdString(object->GetStringParameter("FullPathFileName")));
            if (auto *ephemeris=dynamic_cast<EphemerisFile *>(object)) {
               const auto format=ephemeris->GetStringParameter("FileFormat");
               const auto resolved=ephemeris->GetStringParameter("FullPathFileName");
               // Engine writing retains custom extensions except for SPK.
               const auto path=format!="SPK" && !QFileInfo(QString::fromStdString(resolved)).suffix().isEmpty() ? resolved : ephemeris->GetProperFileName(resolved,format,false);
               ephemerisFiles.insert(QString::fromStdString(name),{QString::fromStdString(path),QString::fromStdString(format)});
            }
         }
         if (group.second==Gmat::EVENT_LOCATOR || group.second==Gmat::SOLVER) {
            auto *object=Moderator::Instance()->GetConfiguredObject(name);
            try {
               const auto filename=QString::fromStdString(object->GetStringParameter(group.second==Gmat::SOLVER ? "ReportFile" : "Filename"));
               if (!filename.isEmpty()) reportFiles.insert(QString::fromStdString(name),QFileInfo(filename).isAbsolute() ? filename :
                  QDir(QString::fromStdString(FileManager::Instance()->GetFullPathname("OUTPUT_PATH"))).filePath(filename));
            } catch (BaseException &) {} // Plugins need not provide a report file.
         }
         if (group.second == Gmat::PARAMETER) {
            auto *parameter = dynamic_cast<Parameter *>(Moderator::Instance()->GetConfiguredObject(name));
            if (!parameter || parameter->GetKey() == GmatParam::SYSTEM_PARAM) continue;
         }
         auto *item = new QTreeWidgetItem(category, {QString::fromStdString(name)});
         item->setData(0, Qt::UserRole, QString::fromStdString(name));
      }
   }
   root->setExpanded(true); resources->expandToDepth(1);
   auto *sequence = new QTreeWidgetItem(mission, {"Mission Sequence"});
   missionState = {};
   if (modelValid) {
      try {
         missionState = snapshotMission(Moderator::Instance()->GetFirstCommand(),
            omitUnsetHardwareFovs(qtConfiguredScript()), builtScript);
         std::function<void(int,QTreeWidgetItem *)> addNode = [&](int index, QTreeWidgetItem *parent) {
            const auto &node = missionState.nodes[index];
            auto *item = new QTreeWidgetItem(parent, {node.label});
            item->setData(0, Qt::UserRole, index);
            item->setData(0, Qt::UserRole+1, node.type);
            item->setToolTip(0, node.statement);
            for (const auto child : node.children) addNode(child,item);
         };
         for (const auto index : missionState.roots) addNode(index,sequence);
         refreshBreakpoints();
      } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
      catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); }
   }
   missionNavigation->refresh();
   refreshOutput();
}
void MainWindow::refreshOutput()
{
   output->clear();
   auto *reports = new QTreeWidgetItem(output, {"Reports"});
   for (auto it = reportFiles.cbegin(); it != reportFiles.cend(); ++it) {
      auto *item = new QTreeWidgetItem(reports, {it.key()});
      item->setData(0, Qt::UserRole, it.value());
      item->setData(0, Qt::UserRole+1, "report");
   }
   reports->setExpanded(true);
   auto *ephemerides=new QTreeWidgetItem(output,{"Ephemeris files"});
   for (auto it=ephemerisFiles.cbegin();it!=ephemerisFiles.cend();++it) {
      auto *item=new QTreeWidgetItem(ephemerides,{it.key()}); item->setData(0,Qt::UserRole,it.value().first); item->setData(0,Qt::UserRole+1,"ephemeris"); item->setData(0,Qt::UserRole+2,it.value().second); item->setToolTip(0,it.value().first);
   }
   ephemerides->setExpanded(true);
   auto *folder = new QTreeWidgetItem(output, {"Plots"});
   for (const auto &name : plots->names()) {
      auto *item = new QTreeWidgetItem(folder, {name});
      item->setData(0, Qt::UserRole, name);
   }
   folder->setExpanded(true);
}
void MainWindow::updateTitle()
{
   setWindowTitle(QString("%1%2 — GMAT Qt 6").arg(scriptPath.isEmpty() ? "Untitled" : QFileInfo(scriptPath).fileName(), editor->document()->isModified() ? " *" : ""));
   updateScriptDocuments();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
   if (running) {
      stopMission();
      statusBar()->showMessage("Stopping mission — close again after execution ends");
      event->ignore();
      return;
   }
   if (!confirmDiscard()) { event->ignore(); return; }
   for (const auto &document:scriptDocuments) if (document!=activeDocument && !confirmScriptClose(document,false)) { event->ignore(); return; }
   QSettings settings; settings.setValue("geometry", saveGeometry()); settings.setValue("windowState", saveState());
   event->accept();
}

void MainWindow::setRunning(bool value)
{
   running = value;
   if (welcome) welcome->setEnabled(!value);
   recentMenu->setEnabled(!value);
   for (auto *action : editingActions) action->setEnabled(!value);
   for (const auto &document:scriptDocuments) { document->editor->setReadOnly(value); if (document->search) document->search->setEnabled(!value); }
   for (auto *child : workspace->subWindowList())
      if (child->property("configurationPanel").toBool()) child->widget()->setEnabled(!value);
   resources->setEnabled(!value);
   mission->setEnabled(!value);
   runAction->setEnabled(!value);
   runAction->setText("&Run mission");
   pauseAction->setEnabled(value);
   stopAction->setEnabled(value);
   stepAction->setEnabled(false);
}
MainWindow::RunResult MainWindow::runMission()
{
   return executeMission(false);
}
MainWindow::RunResult MainWindow::debugMission()
{
   return executeMission(true);
}
bool MainWindow::setBreakpoint(int index,bool enabled)
{
   if (running || !modelValid || editor->toPlainText()!=builtScript || index<0 || index>=missionState.nodes.size() || missionState.nodes[index].type=="BeginMissionSequence") return false;
   if (enabled) breakpoints.insert(index); else breakpoints.remove(index);
   breakpointSource=builtScript; refreshBreakpoints(); return true;
}
void MainWindow::refreshBreakpoints()
{
   for (QTreeWidgetItemIterator it(mission);*it;++it) {
      const auto index=(*it)->data(0,Qt::UserRole);
      if (!index.isValid() || index.toInt()<0 || index.toInt()>=missionState.nodes.size()) continue;
      const bool selected=breakpoints.contains(index.toInt()); (*it)->setData(0,Qt::UserRole+2,selected);
      (*it)->setText(0,(selected ? QString("● ") : QString())+missionState.nodes[index.toInt()].label);
   }
}
void MainWindow::stepMission()
{
   if (running && paused && debugger->isWaiting()) debugger->step();
}
MainWindow::RunResult MainWindow::executeMission(bool debug)
{
   if (running) return RunResult::Busy;
   if (!buildScript()) return RunResult::Failed;
   debug=debug || !breakpoints.isEmpty();
   solverListeners->missionStarted();
   paused = false;
   stopRequested = false;
   setRunning(true);
   statusBar()->showMessage("Running mission…");
   RunResult result = RunResult::Failed;
   struct ObserverScope {
      CommandExecutionObserver *previous;
      explicit ObserverScope(CommandExecutionObserver *observer) : previous(GmatCommand::SetExecutionObserver(observer)) {}
      ~ObserverScope() { GmatCommand::SetExecutionObserver(previous); }
   } observer(debug ? debugger : nullptr);
   try {
      // Capture before execution: GetNext on a stopped/incomplete solver can
      // return itself or throw for ExitMode=Stop. Post-run corrections must
      // not traverse that execution state to find source-owned Vary commands.
      snapshotMission(Moderator::Instance()->GetFirstCommand(),missionState.canonicalScript,builtScript,&runCommands);
      runCorrectionState=snapshotMission(Moderator::Instance()->GetFirstCommand(),missionState.canonicalScript,builtScript,&runCorrectionCommands,true);
      if (debug) {
         debugger->begin(runCommands,breakpoints);
      }
      const auto status = Moderator::Instance()->RunMission();
      if (status == 1) result = RunResult::Completed;
      else if (status == -4) result = RunResult::Stopped;
   } catch (BaseException &error) {
      messages->appendPlainText(QString::fromStdString(error.GetFullMessage()));
   } catch (const std::exception &error) {
      messages->appendPlainText(QString::fromUtf8(error.what()));
   } catch (...) {
      messages->appendPlainText("Unexpected error during mission execution.");
   }
   plots->missionFinished();
   debugger->finish(); paused = false;
   summaryAvailable=true; lastRunResult=result;
   setRunning(false);
   solverListeners->missionFinished(result==RunResult::Stopped,result==RunResult::Failed);
   statusBar()->showMessage(result == RunResult::Completed ? "Mission completed" :
      result == RunResult::Stopped ? "Mission stopped" : "Mission failed — see Message Window");
   return result;
}
FolderRunResult MainWindow::runFolderScripts(const FolderRunOptions &options,QtPlotReceiver &batchPlots,
   const std::atomic_bool &cancel,const std::function<void(int,int,const FolderRunItem &)> &progress)
{
   FolderRunResult result;
   if (!ready || running) { result.error="Finish the current mission before running a folder."; return result; }
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges()) {
      result.error="Apply or discard pending panel changes before running a folder."; return result;
   }
   result.error=validateFolderRun(options); if (!result.error.isEmpty()) return result;
   auto *fm=FileManager::Instance(); auto *global=GmatGlobal::Instance();
   const auto originalOutput=fm->GetFullPathname("OUTPUT_PATH"),originalEphem=fm->GetFullPathname("VEHICLE_EPHEM_PATH"),originalWorking=fm->GetGmatWorkingDirectory();
   const auto originalLog=receiver->GetLogFileName(); const bool originalLogging=receiver->GetLogEnable(),originalBatch=global->IsBatchMode(),originalValid=modelValid;
   // Clean panels can hold references to engine objects replaced by a folder
   // build. Pending panels were protected above; reopen clean ones afterward.
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); }
   ++modelGeneration; summaryAvailable=false; paused=false; stopRequested=false; setRunning(true); global->SetBatchMode(true);
   PlotInterface::SetPlotReceiver(&batchPlots);
   QtSolverListenerManager batchListeners(batchPlots.workspaceArea());
   ListenerManagerInterface::SetListenerManager(&batchListeners);
   QString previous;
   QString buildFailureCategory,buildFailureDetails;
   bool built=false;
   try {
      result=runScriptFolder(options,[&](const QString &path,const QString &assetBase,int repeat,const QString &output) {
         QApplication::processEvents();
         FolderRunItem item; QString diagnostics; QScopedValueRollback<QString *> capture(folderMessages,&diagnostics); bool solverRun=false;
         if (cancel) { item.category="Interrupted"; return item; }
         try {
            fm->SetAbsPathname("OUTPUT_PATH",(output+"/").toStdString()); fm->SetAbsPathname("VEHICLE_EPHEM_PATH",(output+"/").toStdString());
            receiver->SetLogPath(output.toStdString(),true); receiver->SetLogEnable(true);
            if (repeat==1 || previous!=path) {
               previous=path; built=false; buildFailureCategory.clear(); buildFailureDetails.clear(); batchPlots.clear(true);
               if (!setGmatScriptDirectory(assetBase)) throw std::runtime_error("The source script folder is unavailable.");
               QFile file(path);
               if (!file.open(QIODevice::ReadOnly)) { item.category=buildFailureCategory="Read error"; item.details=buildFailureDetails=file.errorString(); return item; }
               const auto bytes=file.readAll(); QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless); QString source=decoder(bytes);
               if (file.error()!=QFileDevice::NoError || decoder.hasError()) { item.category=buildFailureCategory="Read error"; item.details=buildFailureDetails="Script is unreadable or is not UTF-8."; return item; }
               const auto converted=convertOpenFramesViews(source);
               if (!converted.error.isEmpty()) { item.category=buildFailureCategory="Build error"; item.details=buildFailureDetails=converted.error; return item; }
               if (converted.plots) {
                  QMessageBox prompt(QMessageBox::Question,"OpenFrames folder views",
                     "Convert this mission's OpenFrames views for the Qt viewer? The source file and saved copy are retained.",
                     QMessageBox::Yes|QMessageBox::No,QApplication::activeModalWidget());
                  prompt.setObjectName("openFramesFolderConversionPrompt"); prompt.button(QMessageBox::Yes)->setText("Convert views"); prompt.button(QMessageBox::No)->setText("Skip script"); prompt.setDefaultButton(QMessageBox::Yes); prompt.setDetailedText(converted.notes.join('\n'));
                  if (prompt.exec()!=QMessageBox::Yes) { item.category=buildFailureCategory="Conversion skipped"; item.details=buildFailureDetails="Original OpenFrames script preserved."; return item; }
                  source=converted.script; item.details=converted.notes.join('\n')+"\n";
               }
               const auto cameras=qtCameraSettings(source); std::istringstream stream(source.toStdString()); built=qtInterpretScript(stream);
               if (built) { QtPlotReceiver::validateCameraReferences(cameras); batchPlots.cameraSettings=cameras; }
            }
            if (!built) { item.category=buildFailureCategory.isEmpty() ? "Build error" : buildFailureCategory; item.details=buildFailureDetails; }
            else {
               // Subscribers cache the resolved filename when configured.
               // Repeats reuse objects, so re-resolve the same output setting
               // against this repeat's directory before cloning the sandbox.
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SUBSCRIBER)) {
                  auto *object=Moderator::Instance()->GetConfiguredObject(name);
                  if (object && (object->IsOfType("ReportFile") || object->IsOfType("EphemerisFile")))
                     object->SetStringParameter("Filename",object->GetStringParameter("Filename"));
               }
               batchListeners.missionStarted(); solverRun=true;
               item.engineStatus=Moderator::Instance()->RunMission();
               switch(item.engineStatus) {
               case 1: item.category="Completed"; break;
               case -2: item.category="Initialization error"; break;
               case -3: item.category="Unknown initialization error"; break;
               case -4: item.category="Interrupted"; break;
               case -5: item.category="Runtime error"; break;
               default: item.category="Unknown runtime error"; break;
               }
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SUBSCRIBER)) {
                  auto *object=Moderator::Instance()->GetConfiguredObject(name); if (!object || !object->IsOfType("ReportFile")) continue;
                  const auto filename=QString::fromStdString(object->GetStringParameter("Filename"));
                  item.reports.insert(QString::fromStdString(name),QFileInfo(filename).isAbsolute() ? filename : QDir(output).filePath(filename));
               }
            }
         } catch (BaseException &error) { item.category=built ? "Runtime error" : "Build error"; item.details=QString::fromStdString(error.GetFullMessage()); }
         catch (const std::exception &error) { item.category="Unknown error"; item.details=QString::fromUtf8(error.what()); }
         catch (...) { item.category="Unknown error"; item.details="An unexpected exception interrupted this script."; }
         if (solverRun) {
            batchPlots.missionFinished();
            batchListeners.missionFinished(item.category=="Interrupted",item.category!="Completed");
         }
         QApplication::processEvents(); item.details+=diagnostics;
         if (!built) { buildFailureCategory=item.category; buildFailureDetails=item.details; }
         return item;
      },cancel,progress);
   } catch (BaseException &error) { result.error=QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); }
   catch (...) { result.error="An unexpected exception interrupted the folder run."; }
   // Restore paths even when a build, initialization, run or comparison fails.
   // Suppress subscriber callbacks while replacing the batch engine model so
   // neither the ordinary viewer histories nor the final batch scenes vanish.
   PlotInterface::SetPlotReceiver(nullptr);
   ListenerManagerInterface::SetListenerManager(solverListeners.get());
   const auto restore=[&](const char *name,const std::function<void()> &action) {
      try { action(); }
      catch (BaseException &error) { result.error+="\n"+QString(name)+": "+QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { result.error+="\n"+QString(name)+": "+QString::fromUtf8(error.what()); }
      catch (...) { result.error+="\n"+QString(name)+": unexpected restoration error."; }
   };
   restore("Output restore",[&] { fm->SetAbsPathname("OUTPUT_PATH",originalOutput); });
   restore("Ephemeris restore",[&] { fm->SetAbsPathname("VEHICLE_EPHEM_PATH",originalEphem); });
   restore("Script-folder restore",[&] { if (!fm->SetGmatWorkingDirectory(originalWorking)) throw std::runtime_error("The original script folder is unavailable."); });
   restore("Log restore",[&] { receiver->RestoreLog(originalLog,originalLogging); }); global->SetBatchMode(originalBatch);
   try {
      std::istringstream stream(builtScript.toStdString()); const bool restored=qtInterpretScript(stream);
      modelValid=restored && originalValid;
      if (!restored) result.error+="\nThe original engine configuration could not be restored. Rebuild the open mission before running it.";
   } catch (BaseException &error) { modelValid=false; result.error+="\nRestore failed: "+QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { modelValid=false; result.error+="\nRestore failed: "+QString::fromUtf8(error.what()); }
   catch (...) { modelValid=false; result.error+="\nRestore failed unexpectedly. Rebuild the open mission before running it."; }
   PlotInterface::SetPlotReceiver(plots.get()); paused=false; stopRequested=false; setRunning(false); refreshTrees();
   return result;
}
void MainWindow::pauseMission()
{
   if (!running || paused || stopRequested) return;
   if (debugger->isActive()) { debugger->requestPause(); pauseAction->setEnabled(false); statusBar()->showMessage("Pausing at the next command…"); return; }
   Moderator::Instance()->ChangeRunState("Pause");
   paused = true;
   runAction->setText("&Resume mission");
   runAction->setEnabled(true);
   pauseAction->setEnabled(false);
   statusBar()->showMessage("Mission paused");
}
void MainWindow::resumeMission()
{
   if (!running || !paused || stopRequested) return;
   if (debugger->isWaiting()) debugger->resume(); else Moderator::Instance()->ChangeRunState("Resume");
   paused = false;
   runAction->setText("&Run mission");
   runAction->setEnabled(false);
   pauseAction->setEnabled(true);
   stepAction->setEnabled(false);
   statusBar()->showMessage("Running mission…");
}
void MainWindow::stopMission()
{
   if (!running || stopRequested) return;
   stopRequested = true;
   Moderator::Instance()->ChangeRunState("Stop");
   if (debugger->isActive()) debugger->stop();
   runAction->setEnabled(false);
   pauseAction->setEnabled(false);
   stopAction->setEnabled(false);
   stepAction->setEnabled(false);
   statusBar()->showMessage("Stopping mission…");
}

QString MainWindow::applyResourceChanges(const QString &name,
      const QMap<QString, QString> &requested, const QString &expectedScript)
{
   QMap<QString,QString> changes=requested;
   if (running) return "Stop the mission before editing resources.";
   if (!modelValid || expectedScript != builtScript || editor->toPlainText() != builtScript)
      return "The mission has changed. Build the current script and reopen this panel.";
   if (changes.isEmpty()) return {};
   auto *moderator = Moderator::Instance();
   auto *object = moderator->GetConfiguredObject(name.toStdString());
   if (!object) return "This resource no longer exists. Reopen the panel.";
   return applyResourceSettings(*object,changes,expectedScript);
}
QString MainWindow::applyResourceSettings(GmatBase &resource,const QMap<QString,QString> &requested,const QString &expectedScript)
{
   auto *object=&resource;
   const auto name=QString::fromStdString(resource.GetName());
   QMap<QString,QString> changes=requested;
   if (changes.isEmpty()) return applyModelScript(expectedScript);
   const QStringList objectDrawingKeys={"@QtObjectLabels","@QtObjectTrajectories","@QtObjectCenters","@QtObjectEndpoints","@QtObjectMarkerSizes","@QtObjectLineWidths","@QtObjectFontSizes","@QtObjectFontPositions","@QtObjectAxes","@QtObjectGrids","@QtObjectXYPlanes","@QtObjectVelocities"};
   const bool objectDrawing=std::any_of(objectDrawingKeys.cbegin(),objectDrawingKeys.cend(),[&](const auto &key) { return changes.contains(key); });
   if (objectDrawing && !object->IsOfType("OrbitView")) return "Object drawing settings belong to an OrbitView.";
   QMap<QString,QString> external;
   for (auto it=changes.cbegin();it!=changes.cend();++it) if (it.key().startsWith("@ExternalForce.")) external[it.key()]=it.value();
   bool structuralExternal=false;
   if (!external.isEmpty()) {
      if (!object->IsOfType("ODEModel")) return "External forces belong to a force model.";
      const auto error=validateExternalForceSettings(external); if (!error.isEmpty()) return error;
      const auto original=externalForceSettings(*object);
      structuralExternal=original.value("@ExternalForce.Enabled")!=external.value("@ExternalForce.Enabled");
      if (!structuralExternal && external.value("@ExternalForce.Enabled")=="true") {
         // Existing contributors use the ordinary source-preserving field
         // patcher, retaining implicit settings and declaration ordering.
         const QMap<QString,QString> fields={{"@ExternalForce.Module","External.ScriptFileName"},{"@ExternalForce.Function","External.DerivativesFunction"},{"@ExternalForce.ExcludeOtherForces","External.ExcludeOtherForces"}};
         for (auto it=fields.cbegin();it!=fields.cend();++it) if (original.value(it.key())!=external.value(it.key())) changes[it.value()]=external.value(it.key());
         for (auto it=external.cbegin();it!=external.cend();++it) changes.remove(it.key());
      }
   }
   QMap<QString,QString> polyhedronMoves;
   if (object->IsOfType("ODEModel") && !changes.contains("@PolyhedronForces")) {
      const auto original=polyhedronSettings(*object); auto pending=original; QStringList consumed;
      for (int index=0;index<original.size();++index) {
         auto row=original[index].toObject(); const auto body=row.value("body").toString(); const auto prefix="PolyhedronGravityModel."+body+'.';
         if (changes.contains(prefix+"CreateForceBody") && changes.value(prefix+"CreateForceBody").trimmed()!=body) {
            polyhedronMoves[body]=changes.value(prefix+"CreateForceBody").trimmed();
            row["body"]=polyhedronMoves.value(body); consumed.append(prefix+"CreateForceBody");
         }
         for (const auto &field:QStringList{"ShapeFileName","BodyDensity"}) if (changes.contains(prefix+field)) {
            row[field=="ShapeFileName" ? "shape" : "density"]=changes.value(prefix+field); consumed.append(prefix+field);
         }
         pending[index]=row;
      }
      if (!polyhedronMoves.isEmpty()) {
         changes["@PolyhedronForces"]=QString::fromUtf8(QJsonDocument(pending).toJson(QJsonDocument::Compact));
         for (const auto &field:consumed) changes.remove(field);
      }
   }
   const bool replacePolyhedron=changes.contains("@PolyhedronForces"); QJsonArray polyhedron;
   if (replacePolyhedron) {
      if (!object->IsOfType("ODEModel")) return "Polyhedron contributors belong to a force model.";
      QJsonParseError parse; const auto value=QJsonDocument::fromJson(changes.value("@PolyhedronForces").toUtf8(),&parse);
      if (parse.error!=QJsonParseError::NoError || !value.isArray()) return "Invalid polyhedron contributor settings.";
      polyhedron=value.array(); const auto error=validatePolyhedronSettings(polyhedron); if (!error.isEmpty()) return error;
      QStringList originalBodies; for (const auto &entry:polyhedronSettings(*object)) originalBodies.append(entry.toObject().value("body").toString());
      for (const auto &entry:polyhedron) {
         const auto row=entry.toObject(); const auto origin=row.value("previousBody").toString();
         if (origin.isEmpty()) continue;
         if (!originalBodies.removeOne(origin)) return "The original polyhedron contributor is unavailable. Reopen the panel.";
         polyhedronMoves[origin]=row.value("body").toString();
      }
   }
   if (changes.isEmpty()) return external.isEmpty() ? QString() : externalForceSettingsError(*object,external);
   const bool shadows=object->IsOfType("SolarPowerSystem") && changes.size()==1 && changes.contains("ShadowBodies");
   if (shadows) {
      try {
         // Keep implicit defaults and unrelated configuration unchanged.
         // Whole-mission serialization can apply a previously implicit power
         // epoch and change its decay calculation during a body-list edit.
         QMap<QString,QStringList> expected;
         for (auto it=changes.cbegin();it!=changes.cend();++it) expected[it.key()]=splitResourceReferences(it.value());
         QString block;
         const QStringList fields={"ShadowBodies"};
         for (const auto &field:fields) {
            if (!changes.contains(field)) continue;
            block+=replaceResourceList(*object,"GMAT "+name+"."+field+" = {};\n",field,changes.value(field));
         }
         QString firstCommand;
         for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         return applyModelScript(setConfigurationBlock(expectedScript,name,changes.keys(),block,firstCommand),[name,expected] {
            auto *updated=Moderator::Instance()->GetConfiguredObject(name.toStdString());
            if (!updated) return QString("The resource was not retained.");
            for (auto it=expected.cbegin();it!=expected.cend();++it) { QStringList actual; for (const auto &body:updated->GetStringArrayParameter(it.key().toStdString())) actual.append(QString::fromStdString(body)); if (actual!=it.value()) return QString("The selected bodies were not retained. The previous configuration was restored."); }
            return QString();
         });
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   if (auto *body=dynamic_cast<CelestialBody *>(object)) {
      try {
         QString firstCommand;
         for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         return applyModelScript(celestialBodyScript(*body,expectedScript,changes,firstCommand),[name,changes] {
            auto *updated=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));
            return updated ? celestialBodySettingsError(*updated,changes) : QString("This body was not retained. The previous configuration was restored.");
         });
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   if (auto *system=dynamic_cast<SolarSystem *>(object)) {
      try {
         QString firstCommand;
         for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         const auto values=solarSystemSettings(*system);
         auto pending=values.values;
         for (auto it=changes.cbegin();it!=changes.cend();++it) {
            if (!pending.contains(it.key())) return "This solar-system setting is not editable here.";
            pending[it.key()]=it.value();
         }
         const auto selected=pending.value("EphemerisSource");
         if (changes.contains("EphemerisSource") && !changes.contains("DEFilename") && values.deFiles.contains(selected)) {
            pending["DEFilename"]=values.deFiles.value(selected); pending["@DEFileSource"]=selected;
         }
         return applyModelScript(solarSystemScript(expectedScript,values,pending,firstCommand),[pending] {
            return solarSystemSettingsError(*Moderator::Instance()->GetSolarSystemInUse(),pending);
         });
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   if (object->GetTypeName()=="Variable" || object->GetTypeName()=="String") {
      if (changes.size()!=1 || !changes.contains("Value")) return "Edit the parameter's initial value.";
      try {
         QString firstCommand;
         for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         return applyModelScript(setUserParameterValue(expectedScript,name,QString::fromStdString(object->GetTypeName()),changes.value("Value"),firstCommand),[name,type=QString::fromStdString(object->GetTypeName()),value=changes.value("Value")] { return userParameterValueError(name,type,value); });
      }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   auto *calculatedPoint=dynamic_cast<CalculatedPoint *>(object);
   const bool builtInPoint=calculatedPoint && calculatedPoint->IsBuiltIn();
   if (builtInPoint) for (auto it=changes.cbegin();it!=changes.cend();++it)
      if (it.key()!="OrbitColor" && it.key()!="TargetColor") return "Only orbit and target colors can be changed on this built-in calculated point.";
   if (changes.contains("@DynamicData") && !object->IsOfType("DynamicDataDisplay")) return "Grid settings require a dynamic data display.";
   if (changes.contains("@TrackingConfigs") && !object->IsOfType("TrackingFileSet")) return "Tracking configurations require a TrackingFileSet.";
   if (changes.contains("@ArrayExpressions") && !object->IsOfType("Array")) return "Cell expressions require an Array.";
   if (changes.contains("@ArrayExpressions") && changes.size()==1) {
      try {
         return applyModelScript(setArrayExpressions(expectedScript,name,changes["@ArrayExpressions"],
            object->GetIntegerParameter("NumRows"),object->GetIntegerParameter("NumCols")));
      } catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   QString candidate;
   try {
      setScriptDirectory();
      QtResourcePreview proposed(object->Clone());
      if (!proposed) return "This resource cannot be edited.";
      if (changes.contains("@DynamicData")) applyDynamicDataSettings(*proposed,changes.value("@DynamicData"));
      const bool pairedMixture=object->IsOfType("Thruster") && changes.contains("Tank") && changes.contains("MixRatio");
      const QString mixture=changes.value("MixRatio");
      const auto propChanges=applyPropSetupProperties(*proposed,changes);
      const auto orbitChanges=applySpacecraftOrbitProperties(*proposed,changes);
      const auto attitudeChanges=applyAttitudeProperties(*proposed,changes);
      const auto gravityChanges=applyGravityBodyProperties(*proposed,changes);
      const auto tankChanges=applyChemicalTankProperties(*proposed,changes);
      const auto regionChanges=applyRegionProperties(*proposed,changes);
      const auto customFovChanges=applyCustomFovProperties(*proposed,changes,customFovCoordinates(*object,expectedScript));
      const auto atmosphereChanges=applyAtmosphereProperties(*proposed,changes);
      const auto stationChanges=applyGroundStationLocation(*proposed,changes);
      const auto eventChanges=applyEventLocatorProperties(*proposed,changes);
      const auto viewChanges=applyOrbitViewProperties(*proposed,changes);
      const auto burnChanges=applyBurnProperties(*proposed,changes);
      const auto ephemerisChanges=applyEphemerisProperties(*proposed,changes);
      const auto intervalChanges=applyEpochIntervalProperties(*proposed,changes);
      const auto warmChanges=applyWarmStartProperties(*proposed,changes);
      const QString modelField=proposed->IsOfType("ProcessNoiseModel") ? "Type" : proposed->IsOfType("EstimatedParameter") ? "Model" : QString();
      if (!modelField.isEmpty() && changes.contains(modelField)) setResourceProperty(*proposed,modelField,changes.value(modelField));
      for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
         if (propChanges.contains(it.key()) || orbitChanges.contains(it.key()) || attitudeChanges.contains(it.key()) || gravityChanges.contains(it.key()) || tankChanges.contains(it.key()) || regionChanges.contains(it.key()) || customFovChanges.contains(it.key()) || atmosphereChanges.contains(it.key()) || stationChanges.contains(it.key()) || eventChanges.contains(it.key()) || viewChanges.contains(it.key()) || burnChanges.contains(it.key()) || ephemerisChanges.contains(it.key()) || objectDrawingKeys.contains(it.key()) || it.key()=="@ArrayExpressions" || it.key()=="@DynamicData" || it.key()=="@TrackingConfigs" || it.key().startsWith("@ExternalForce.") || it.key()=="@PolyhedronForces" || (pairedMixture && it.key()=="MixRatio")) continue;
         if (intervalChanges.contains(it.key()) || warmChanges.contains(it.key()) || it.key()==modelField || isResourceList(*proposed,it.key())) continue;
         try { setResourceProperty(*proposed, it.key(), it.value()); }
         catch (BaseException &error) { return it.key() + ": " + QString::fromStdString(error.GetFullMessage()); }
         catch (const std::exception &error) { return it.key() + ": " + QString::fromUtf8(error.what()); }
      }
      validateResourceProperties(*proposed,replacePolyhedron);
      if (proposed->IsOfType("ODEModel") && (changes.contains("PrimaryBodies") || changes.contains("PointMasses"))) {
         const auto effective=[&](const QString &field) {
            if (changes.contains(field)) return splitResourceReferences(changes.value(field));
            QStringList names; for (const auto &name:proposed->GetStringArrayParameter(field.toStdString())) names.append(QString::fromStdString(name)); return names;
         };
         const auto primary=effective("PrimaryBodies"),points=effective("PointMasses");
         for (const auto &body:primary) if (points.contains(body)) return "A body cannot be both primary gravity and point mass. Adjust both lists before Apply.";
      }
      if (proposed->GetTypeName()=="XYPlot" && proposed->GetBooleanParameter("ShowPlot")) {
         const bool emptyY=changes.contains("YVariables") ? splitResourceReferences(changes.value("YVariables")).isEmpty() : proposed->GetStringArrayParameter("YVariables").empty();
         if (emptyY) return "Select at least one Y parameter, or turn off Show plot.";
      }
      if ((proposed->GetTypeName()=="GroundTrackPlot" || proposed->GetTypeName()=="GroundTrack") && proposed->GetBooleanParameter("ShowPlot")) {
         const bool empty=changes.contains("Add") ? splitResourceReferences(changes.value("Add")).isEmpty() : proposed->GetStringArrayParameter("Add").empty();
         if (empty) return "Select an object, or turn off Show plot.";
      }
      if (!proposed->Validate()) return "The resource rejected these settings.";
      candidate = expectedScript;
      if (builtInPoint) {
         // Default built-in points are cloaked and have no block to replace.
         // Write only their supported appearance settings; never create or
         // change their fixed body definition.
         QString block,firstCommand;
         for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         for (auto it=changes.cbegin();it!=changes.cend();++it) {
            const QString key=name+"."+it.key();
            const QString assignment="GMAT "+key+" = "+QString::fromStdString(proposed->GetStringParameter(it.key().toStdString()))+";";
            block+=assignment+'\n';
         }
         return applyModelScript(setConfigurationBlock(expectedScript,name,changes.keys(),block,firstCommand));
      }
      auto serialize=[](GmatBase &resource) {
         if (resource.IsOfType("PlanetographicRegion")) return regionResourceScript(resource);
         if (!resource.IsOfType(Gmat::PROP_SETUP)) return QString::fromStdString(resource.GetGeneratingString(Gmat::SCRIPTING));
         // The full script writes force models in their own section. Match that
         // convention without changing the configured object's output flags.
         QtResourcePreview copy(resource.Clone());
         copy->TakeAction("ExcludeODEModel");
         return QString::fromStdString(copy->GetGeneratingString(Gmat::SCRIPTING));
      };
      const auto snapshot=[&](GmatBase &resource) { return resource.IsOfType("Spacecraft") ? spacecraftOrbitScript(resource) : resource.IsOfType("OrbitView") ? orbitViewScript(resource) : (resource.IsOfType("ImpulsiveBurn") || resource.IsOfType("FiniteBurn")) ? burnResourceScript(resource) : serialize(resource); };
      const auto oldBlock = omitUnsetHardwareFovs(snapshot(*object),object);
      auto newBlock = snapshot(*proposed);
      if (!customFovChanges.isEmpty()) newBlock=customFovResourceScript(*proposed,customFovEditedCoordinates(changes,customFovCoordinates(*object,expectedScript)));
      for (auto it=changes.cbegin();it!=changes.cend();++it)
         if (!objectDrawingKeys.contains(it.key()) && it.key()!="@ArrayExpressions" && it.key()!="@DynamicData" && it.key()!="@TrackingConfigs" && !it.key().startsWith("@ExternalForce.") && it.key()!="@PolyhedronForces" && !orbitChanges.contains(it.key()) && !regionChanges.contains(it.key()) && !customFovChanges.contains(it.key()) && !atmosphereChanges.contains(it.key()) && !eventChanges.contains(it.key()) && !viewChanges.contains(it.key()) && !burnChanges.contains(it.key()) && isResourceList(*proposed,it.key())) newBlock=replaceResourceList(*proposed,newBlock,it.key(),it.value(),pairedMixture && it.key()=="Tank" ? &mixture : nullptr);
      if (changes.contains("@TrackingConfigs")) newBlock=replaceTrackingConfigurations(*proposed,newBlock,changes.value("@TrackingConfigs"));
      if (changes.contains("FieldOfView") && object->IsOfType("Imager")) {
         // Imager's getter can still read the clone's old FOV pointer after
         // its string setter changes the reference name. Serialize the pending
         // reference explicitly; the interpreter resolves it on rebuild.
         const auto key=name+".FieldOfView";
         newBlock.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+"[ \\t]*=[^;\\n]*;",QRegularExpression::MultilineOption));
         const auto value=changes.value("FieldOfView").trimmed();
         if (!value.isEmpty()) newBlock+="\nGMAT "+key+" = "+value+";\n";
      }
      if (changes.contains("Propagator") && (object->IsOfType("Simulator") || object->IsOfType("Estimator"))) {
         // These setters replace the first propagator name without removing
         // its spacecraft map. The serializer then omits that map because it
         // iterates only the new name list. Retain every original explicit
         // mapping; the newly serialized scalar assignment sets the default.
         const QRegularExpression mapping("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(name+".Propagator")+"[ \\t]*=[ \\t]*\\{[^;\\n]*\\}[ \\t]*;[^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption);
         QString originalMappings;
         auto matches=mapping.globalMatch(oldBlock);
         while (matches.hasNext()) { const auto line=matches.next().captured(); originalMappings+=line; if (!line.endsWith('\n')) originalMappings+='\n'; }
         newBlock.remove(mapping);
         if (!newBlock.endsWith('\n')) newBlock+='\n';
         newBlock+=originalMappings;
      }
      QString firstCommand;
      for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
      candidate=patchResourceConfiguration(expectedScript,name,oldBlock,omitUnsetHardwareFovs(newBlock,proposed.get()),firstCommand,object->IsOfType("ODEModel"));
      if (object->IsOfType("PropSetup") && changes.contains("Type") && object->GetStringParameter("Type")!=changes.value("Type").trimmed().toStdString()) candidate=orderPropagatorType(candidate,name,firstCommand);
      if (object->IsOfType("Array") && (changes.contains("@ArrayExpressions") || changes.contains("RmatValue")))
         candidate=setArrayExpressions(candidate,name,changes.value("@ArrayExpressions",arrayExpressions(expectedScript,name)),
            proposed->GetIntegerParameter("NumRows"),proposed->GetIntegerParameter("NumCols"));
      // Explicit camera controls take precedence over imported primary-camera
      // behavior. Named secondary views remain independent.
      const QStringList cameraFields={"ViewPointReference","ViewPointVector","ViewScaleFactor","ViewDirection","ViewUpAxis","ViewUpCoordinateSystem"};
      const bool cameraEdited=std::any_of(cameraFields.cbegin(),cameraFields.cend(),[&](const QString &field) { return changes.contains(field); });
      if (object->IsOfType("OrbitView") && cameraEdited) {
         const auto settings=qtCameraSettings(expectedScript);
         if (settings.contains(name)) {
            auto setting=settings.value(name);
            setting.segmentFrame.clear(); setting.reset.reset();
            if (!setting.automaticTrajectory.isEmpty() || !setting.automaticBody.isEmpty()) {
               if (!setting.automaticBody.isEmpty()) setting.bodyRelative=false;
               setting.automaticTrajectory.clear(); setting.automaticBody.clear(); setting.centerOffset.reset(); setting.lookAtRotation=false;
            }
            if (changes.contains("ViewUpAxis") || changes.contains("ViewUpCoordinateSystem")) setting.up.reset();
            if (changes.contains("ViewDirection")) { setting.centerOffset.reset(); setting.lookAtRotation=false; }
            candidate=setQtCameraSetting(candidate,name,setting);
         }
      }
      if (object->IsOfType("OrbitView") && changes.contains("Add")) {
         const auto settings=qtCameraSettings(candidate);
         if (settings.contains(name)) {
            auto setting=settings.value(name); const auto &objects=proposed->GetStringArrayParameter("Add");
            auto prune=[&](auto &flags) {
               for (auto it=flags.begin();it!=flags.end();) {
                  if (std::find(objects.begin(),objects.end(),it.key().toStdString())==objects.end()) it=flags.erase(it); else ++it;
               }
            };
            prune(setting.objectLabels); prune(setting.objectTrajectories); prune(setting.objectCenters); prune(setting.objectEndpoints); prune(setting.objectMarkerSizes); prune(setting.objectLineWidths); prune(setting.objectFontSizes); prune(setting.objectFontPositions); prune(setting.objectAxes); prune(setting.objectGrids); prune(setting.objectXYPlanes); prune(setting.objectVelocities);
            candidate=setQtCameraSetting(candidate,name,setting);
         }
      }
      if (objectDrawing) {
         auto setting=qtCameraSettings(candidate).value(name);
         if (changes.contains("@QtObjectLabels")) setting.objectLabels=qtObjectFlags(changes.value("@QtObjectLabels"));
         if (changes.contains("@QtObjectTrajectories")) setting.objectTrajectories=qtObjectFlags(changes.value("@QtObjectTrajectories"));
         if (changes.contains("@QtObjectCenters")) setting.objectCenters=qtObjectFlags(changes.value("@QtObjectCenters"));
         if (changes.contains("@QtObjectEndpoints")) setting.objectEndpoints=qtObjectFlags(changes.value("@QtObjectEndpoints"));
         if (changes.contains("@QtObjectAxes")) setting.objectAxes=qtObjectFlags(changes.value("@QtObjectAxes"));
         if (changes.contains("@QtObjectGrids")) setting.objectGrids=qtObjectFlags(changes.value("@QtObjectGrids"));
         if (changes.contains("@QtObjectXYPlanes")) setting.objectXYPlanes=qtObjectFlags(changes.value("@QtObjectXYPlanes"));
         if (changes.contains("@QtObjectVelocities")) setting.objectVelocities=qtObjectFlags(changes.value("@QtObjectVelocities"));
         if (changes.contains("@QtObjectMarkerSizes")) setting.objectMarkerSizes=qtObjectSizes(changes.value("@QtObjectMarkerSizes"));
         if (changes.contains("@QtObjectLineWidths")) setting.objectLineWidths=qtObjectWidths(changes.value("@QtObjectLineWidths"));
         if (changes.contains("@QtObjectFontSizes")) setting.objectFontSizes=qtObjectFontSizes(changes.value("@QtObjectFontSizes"));
         if (changes.contains("@QtObjectFontPositions")) setting.objectFontPositions=qtObjectFontPositions(changes.value("@QtObjectFontPositions"));
         candidate=setQtCameraSetting(candidate,name,setting);
      }
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   const auto gravityCheck=[name,changes] {
      if (changes.contains("Type") || changes.contains("FM")) {
         auto *resource=Moderator::Instance()->GetConfiguredObject(name.toStdString());
         if (resource && resource->IsOfType("PropSetup")) for (const auto &field:QStringList{"Type","FM"}) if (changes.contains(field))
            if (QString::fromStdString(resource->GetStringParameter(field.toStdString()))!=changes.value(field).trimmed()) return QString("The selected propagator type or force model was not retained. The previous configuration was restored.");
      }
      if (!changes.contains("PrimaryBodies") && !changes.contains("PointMasses")) return QString();
      auto *model=Moderator::Instance()->GetConfiguredObject(name.toStdString()); if (!model) return QString("The force model was not retained.");
      for (const auto &field:QStringList{"PrimaryBodies","PointMasses"}) if (changes.contains(field)) {
         QStringList actual; for (const auto &body:model->GetStringArrayParameter(field.toStdString())) actual.append(QString::fromStdString(body));
         auto wanted=splitResourceReferences(changes.value(field)); actual.sort(); wanted.sort();
         if (actual!=wanted) return QString("The selected gravity bodies were not retained. The previous configuration was restored.");
      }
      return QString();
   };
   if (!external.isEmpty() || replacePolyhedron) {
      try {
         QString firstCommand; for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
         if (structuralExternal) candidate=externalForceScript(candidate,name,external,firstCommand);
         if (replacePolyhedron) candidate=polyhedronScript(candidate,*object,polyhedron,firstCommand,polyhedronMoves);
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
      return applyModelScript(candidate,[name,external,replacePolyhedron,polyhedron,gravityCheck] {
         const auto gravityError=gravityCheck(); if (!gravityError.isEmpty()) return gravityError;
         auto *model=Moderator::Instance()->GetConfiguredObject(name.toStdString()); if (!model) return QString("The force model was not retained.");
         if (!external.isEmpty()) { const auto error=externalForceSettingsError(*model,external); if (!error.isEmpty()) return error; }
         return replacePolyhedron ? polyhedronSettingsError(*model,polyhedron) : QString();
      });
   }
   return applyModelScript(candidate,gravityCheck);
}

QString MainWindow::createResource(const QString &type,const QString &name,const QString &expectedScript,int rows,int columns,const std::optional<QString> &initialValue,const QString &functionPath,const QString &spacecraft,const QMap<QString,QString> &settings)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before creating a resource.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard the open panel changes before creating a resource.";
   static const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   if (!identifier.match(name).hasMatch()) return "Use a name starting with a letter, followed by letters, digits or underscores.";
   if (!creatableResourceTypes().contains(type)) return "Select an available resource type.";
   if (Moderator::Instance()->GetConfiguredObject(name.toStdString())) return "That resource name is already in use.";
   const auto commands=Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND);
   if (std::find(commands.begin(),commands.end(),name.toStdString())!=commands.end() || name=="GMAT") return "Choose a name that is not a mission command or reserved keyword.";
   if (type=="Array" && (rows<1 || columns<1 || rows>1000 || columns>1000))
      return "Choose array dimensions from 1 to 1000.";
   const auto dimensions=type=="Array" ? QString("[%1,%2]").arg(rows).arg(columns) : QString();
   QString initializer,forceModelDeclaration;
   auto changes=settings;
   // Untouched creation forms omit original values. Region has no valid empty
   // geometry; retain its input mode so the pending helper validates it before
   // the generic empty-change fast paths can register a blank resource.
   if (type=="PlanetographicRegion" && changes.isEmpty()) changes.insert("@RegionSource","Vertices");
   if (type=="CustomFOV" && !changes.contains("@CustomFovSource"))
      changes.insert("@CustomFovSource",changes.value("FOVFileName").trimmed().isEmpty() ? "Vertices" : "File");
   if (type=="PropSetup") {
      try {
         const auto selectedType=settings.value("Type","RungeKutta89");
         const auto selectedModel=settings.value("FM").trimmed();
         const bool unregisteredImplicit=(selectedModel=="InternalODEModel" || selectedModel=="InternalForceModel") &&
            !Moderator::Instance()->GetConfiguredObject(selectedModel.toStdString());
         if (propagatorUsesForceModel(selectedType) && (selectedModel.isEmpty() || unregisteredImplicit)) {
            const auto forceModel=defaultPropagatorForceModelName(name);
            // Only newly created GUI propagators get a local default model.
            // Imported implicit models and explicit existing selections retain
            // their original sharing and source semantics.
            forceModelDeclaration="Create ForceModel "+forceModel+";\n";
            initializer="GMAT "+name+".FM = "+forceModel+";\n";
            changes.remove("FM"); // The source transaction creates this model.
         }
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   if (type=="EphemerisFile") {
      auto selected=settings.value("Spacecraft",spacecraft);
      if (selected.isEmpty()) {
         const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT);
         if (configured.empty()) return "Create a Spacecraft before adding an EphemerisFile.";
         selected=QString::fromStdString(configured.front());
      }
      auto *object=Moderator::Instance()->GetConfiguredObject(selected.toStdString());
      if (!object || !object->IsOfType(Gmat::SPACECRAFT)) return "Select a configured Spacecraft for the EphemerisFile.";
      initializer="GMAT "+name+".Spacecraft = "+selected+";\n";
   }
   const auto locatorTypes=Moderator::Instance()->GetListOfFactoryItems(Gmat::EVENT_LOCATOR);
   if (std::find(locatorTypes.begin(),locatorTypes.end(),type.toStdString())!=locatorTypes.end()) {
      try {
         const auto draft=resourceDraft(type,name);
         const bool contact=draft->IsOfType("ContactLocator");
         const QString field=contact ? "Target" : "Spacecraft";
         const auto selected=settings.value(field,QString::fromStdString(draft->GetStringParameter(field.toStdString()))).trimmed();
         if (selected.isEmpty()) return contact ? "Create a Spacecraft or select a configured PlanetographicRegion before adding a ContactLocator." : "Create a Spacecraft before adding an event locator.";
         auto *target=Moderator::Instance()->GetConfiguredObject(selected.toStdString());
         if (!target || (!target->IsOfType(Gmat::SPACECRAFT) && !(contact && target->IsOfType(Gmat::REGION))))
            return contact ? "Select a configured Spacecraft or PlanetographicRegion for the ContactLocator." : "Select a configured Spacecraft for the event locator.";
         // The creator omits unchanged fields. Persist the displayed default
         // explicitly so interpreter reconstruction and Save/reopen retain it.
         initializer="GMAT "+name+"."+field+" = "+selected+";\n";
      } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   if (type=="GmatFunction") {
      if (functionPath.trimmed().isEmpty()) return "Choose a function file, or use New file to create one before adding this function.";
      const QFileInfo file(functionPath);
      if (!file.isFile() || !file.isReadable()) return "Choose a readable GMAT function file.";
      auto path=file.absoluteFilePath(); path.replace("'","''");
      initializer="GMAT "+name+".FunctionPath = '"+path+"';\n";
   }
   if (initialValue) {
      try { initializer="GMAT "+name+" = "+userParameterLiteral(type,*initialValue)+";\n"; }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   const auto source=forceModelDeclaration+"Create "+type+" "+name+dimensions+";\n"+initializer+builtScript;
   if ((!changes.isEmpty() || type=="PlanetographicRegion") && type!="Variable" && type!="String" && type!="GmatFunction") {
      try { auto draft=resourceDraft(type,name); return applyResourceSettings(*draft,changes,source); }
      catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   return applyModelScript(source,initialValue ? std::function<QString()>([name,type,value=*initialValue] { return userParameterValueError(name,type,value); }) : std::function<QString()>());
}

QString MainWindow::cloneResource(const QString &original,const QString &requestedName,const QString &expectedScript,const QMap<QString,QString> &settings)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before cloning a resource.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard the open panel changes before cloning a resource.";
   auto *moderator=Moderator::Instance(); auto *object=moderator->GetConfiguredObject(original.toStdString());
   if (!object) return "Select a configured resource to clone.";
   if (!resourceCanBeCloned(object)) return "Built-in resources, celestial bodies and propagators cannot be cloned from this panel.";
   const auto name=requestedName.trimmed().isEmpty() ? QString::fromStdString(moderator->GetNewName(original.toStdString(),2)) : requestedName.trimmed();
   if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch()) return "Use a name starting with a letter, followed by letters, digits or underscores.";
   if (moderator->GetConfiguredObject(name.toStdString())) return "That resource name is already in use.";
   const auto commands=moderator->GetListOfFactoryItems(Gmat::COMMAND);
   if (name=="GMAT" || std::find(commands.begin(),commands.end(),name.toStdString())!=commands.end()) return "Choose a name that is not a mission command or reserved keyword.";
   try {
      QString firstCommand; for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
      const auto candidate=cloneResourceSource(expectedScript,original,name,firstCommand);
      QtResourcePreview draft(object->Clone()); draft->SetName(name.toStdString(),name.toStdString());
      auto changes=settings;
      // Scalar forms submit Value even unchanged. Keep the original initializer's
      // source spelling when cloning without an edit, just like ordinary fields.
      if (changes.contains("Value") && (object->GetTypeName()=="Variable" || object->GetTypeName()=="String")) {
         const auto value=object->GetTypeName()=="Variable" ? QString::number(object->GetRealParameter("Value"),'g',17) : QString::fromStdString(object->GetStringParameter("Expression"));
         if (changes.value("Value")==value) changes.remove("Value");
      }
      return changes.isEmpty() ? applyModelScript(candidate) : applyResourceSettings(*draft,changes,candidate);
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
}

void MainWindow::showCloneResource(const QString &original)
{
   if (!ready || running || !modelValid || editor->toPlainText()!=builtScript) { statusBar()->showMessage("Build the current script before cloning a resource"); return; }
   auto *object=Moderator::Instance()->GetConfiguredObject(original.toStdString());
   if (!resourceCanBeCloned(object)) { statusBar()->showMessage("Select a resource that can be cloned"); return; }
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges()) {
      statusBar()->showMessage("Apply or discard the open panel changes before cloning a resource"); return;
   }
   const auto snapshot=builtScript;
   try {
      const auto suggested=QString::fromStdString(Moderator::Instance()->GetNewName(original.toStdString(),2));
      QString firstCommand; for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
      const auto preview=cloneResourceSource(snapshot,original,suggested,firstCommand);
      QtResourcePreview draft(object->Clone()); draft->SetName(suggested.toStdString(),suggested.toStdString());
      QDialog dialog(this); dialog.setObjectName("cloneResourceDialog"); dialog.setWindowTitle("Clone "+original);
      const auto helpTopic=QString::fromStdString(object->GetTypeName()); dialog.setProperty("helpTopic",helpTopic);
      auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout; layout->addLayout(form);
      form->addRow("Source",new QLabel(original+" — "+QString::fromStdString(object->GetTypeName()),&dialog));
      auto *name=new QLineEdit(&dialog); name->setObjectName("resourceName"); name->setPlaceholderText(suggested);
      name->setToolTip("Leave blank to use the suggested available name."); form->addRow("Name (optional)",name);
      auto *status=new QLabel("Configure the copy here. Clone commits its name and settings together.",&dialog); status->setObjectName("resourceCloneStatus"); status->setWordWrap(true);
      auto *panel=new ResourceEditor(*draft,[&](const QMap<QString,QString> &changes) {
         if (name->text().trimmed().isEmpty()) name->setText(QString::fromStdString(Moderator::Instance()->GetNewName(original.toStdString(),2)));
         return cloneResource(original,name->text().trimmed(),snapshot,changes);
      },&dialog,preview,true,true);
      panel->onStatus=[status](const QString &message) { status->setText(message); };
      panel->onApplied=[&] { dialog.accept(); };
      panel->findChild<QDialogButtonBox *>("resourceButtons")->hide();
      if (auto *message=panel->findChild<QLabel *>("resourceStatus")) message->hide();
      contextHelp->attach(panel,QString::fromStdString(object->GetTypeName()));
      layout->addWidget(panel,1); layout->addWidget(status);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help,&dialog); buttons->setObjectName("resourceCloneButtons"); buttons->button(QDialogButtonBox::Ok)->setText("Clone"); layout->addWidget(buttons);
      auto *help=buttons->button(QDialogButtonBox::Help); help->setObjectName("resourceCloneHelp"); help->setToolTip("Help for this resource (F1)");
      connect(buttons,&QDialogButtonBox::helpRequested,&dialog,[this,helpTopic] { contextHelp->show(helpTopic); });
      connect(buttons,&QDialogButtonBox::accepted,panel,&ResourceEditor::requestApply);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      dialog.resize(850,(object->GetTypeName()=="Variable" || object->GetTypeName()=="String") ? 330 : 740); name->setFocus();
      if (dialog.exec()==QDialog::Accepted) {
         refreshAppliedResourcePanels();
         auto *navigation=findChild<QDockWidget *>("navigation"); navigation->show(); navigation->findChild<QTabWidget *>()->setCurrentWidget(resources);
         const auto items=resources->findItems(name->text().trimmed(),Qt::MatchExactly|Qt::MatchRecursive);
         if (items.size()==1) { resources->setCurrentItem(items.first()); resources->scrollToItem(items.first()); resources->itemDoubleClicked(items.first(),0); }
      }
   } catch (BaseException &error) { statusBar()->showMessage(QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { statusBar()->showMessage(QString::fromUtf8(error.what())); }
}

QString MainWindow::renameResource(const QString &original,const QString &requestedName,const QString &expectedScript)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before renaming a resource.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard the open panel changes before renaming a resource.";
   auto *moderator=Moderator::Instance(); auto *object=moderator->GetConfiguredObject(original.toStdString());
   if (!resourceCanBeRenamed(object)) return "Select a configured resource; built-in resources and celestial bodies cannot be renamed.";
   const auto name=requestedName.trimmed();
   if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch()) return "Use a name starting with a letter, followed by letters, digits or underscores.";
   if (name==original) return {};
   if (moderator->GetConfiguredObject(name.toStdString())) return "That resource name is already in use.";
   const auto commands=moderator->GetListOfFactoryItems(Gmat::COMMAND);
   if (name=="GMAT" || std::find(commands.begin(),commands.end(),name.toStdString())!=commands.end()) return "Choose a name that is not a mission command or reserved keyword.";
   try {
      QString relatedOld,relatedNew;
      if (object->IsOfType("PropSetup") && object->GetStringParameter("FM")==original.toStdString()+"_ForceModel" &&
          moderator->GetConfiguredObject((original+"_ForceModel").toStdString()) && !moderator->GetConfiguredObject((name+"_ForceModel").toStdString())) {
         relatedOld=original+"_ForceModel"; relatedNew=name+"_ForceModel";
      }
      const auto candidate=renameResourceSource(expectedScript,*object,name);
      const auto error=applyModelScript(candidate,[original,name] {
         auto *engine=Moderator::Instance();
         return engine->GetConfiguredObject(original.toStdString()) || !engine->GetConfiguredObject(name.toStdString()) ? QString("Rename did not replace the configured resource; the original mission was restored.") : QString();
      },original);
      if (!error.isEmpty()) return error;
      for (auto *child:workspace->subWindowList()) {
         const auto previous=child->property("resourceName").toString();
         const auto current=previous==original ? name : (!relatedOld.isEmpty() && previous==relatedOld ? relatedNew : previous);
         if (current!=previous) { child->setProperty("resourceName",current); child->setWindowTitle(current); }
      }
      refreshAppliedResourcePanels();
      statusBar()->showMessage("Resource renamed — Undo restores the original name; save to keep changes"); return {};
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
}
void MainWindow::showRenameResource(const QString &original)
{
   if (!ready || running || !modelValid || editor->toPlainText()!=builtScript) { statusBar()->showMessage("Build the current script before renaming a resource"); return; }
   if (!resourceCanBeRenamed(Moderator::Instance()->GetConfiguredObject(original.toStdString()))) { statusBar()->showMessage("Select a resource that can be renamed"); return; }
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges()) {
      statusBar()->showMessage("Apply or discard the open panel changes before renaming a resource"); return;
   }
   const auto snapshot=builtScript;
   QDialog dialog(this); dialog.setObjectName("renameResourceDialog"); dialog.setWindowTitle("Rename "+original);
   auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout; layout->addLayout(form);
   auto *name=new QLineEdit(original,&dialog); name->setObjectName("resourceName"); form->addRow("New name",name);
   auto *status=new QLabel("Rename updates this mission’s resource and command references together.",&dialog); status->setObjectName("resourceRenameStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); buttons->setObjectName("resourceRenameButtons"); buttons->button(QDialogButtonBox::Ok)->setText("Rename"); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] { const auto error=renameResource(original,name->text(),snapshot); if (error.isEmpty()) dialog.accept(); else status->setText(error); });
   name->selectAll(); name->setFocus(); dialog.resize(520,180);
   if (dialog.exec()==QDialog::Accepted) {
      const auto items=resources->findItems(name->text().trimmed(),Qt::MatchExactly|Qt::MatchRecursive);
      if (items.size()==1) { resources->setCurrentItem(items.first()); resources->scrollToItem(items.first()); }
   }
}

void MainWindow::showPathSettings()
{
   if (!ready || running) { statusBar()->showMessage("Stop the mission before changing paths"); return; }
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget()))
      if (panel->hasChanges()) { statusBar()->showMessage("Apply or discard panel changes before changing paths"); return; }
   try {
      PathSettingsDialog dialog(capturePathSettings(startupFile),*receiver,[this](const PathSettings &settings) {
         if (running) return QString("Stop the mission before changing paths.");
         const auto error=applyPathSettings(settings,*receiver); if (!error.isEmpty()) return error;
         startupFile=settings.startupFile;
         ++modelGeneration; summaryAvailable=false; modelValid=false;
         for (auto *child:workspace->subWindowList()) if (dynamic_cast<EditablePanel *>(child->widget())) child->close();
         plots->clear(true); resources->clear(); mission->clear(); missionState={};
         reportFiles.clear(); ephemerisFiles.clear(); refreshOutput();
         statusBar()->showMessage("Paths changed — rebuild the mission");
         return QString();
      },scriptPath,this);
      dialog.exec();
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); }
}

void MainWindow::showCreateResource(const QString &initialType)
{
   if (!ready || running || !modelValid || editor->toPlainText()!=builtScript) {
      statusBar()->showMessage("Build the current script before creating a resource"); return;
   }
   const QString snapshot=builtScript;
   QDialog dialog(this); dialog.setWindowTitle(initialType.isEmpty() ? "New resource" : "New "+initialType); dialog.setObjectName("newResourceDialog");
   auto *outer=new QVBoxLayout(&dialog); auto *layout=new QFormLayout; outer->addLayout(layout);
   auto *type=new QComboBox(&dialog); type->setObjectName("resourceType");
   const auto available=creatableResourceTypes();
   if (!initialType.isEmpty() && !available.contains(initialType)) { statusBar()->showMessage("This resource type is no longer available"); return; }
   type->addItems(initialType.isEmpty() ? available : QStringList{initialType}); type->setCurrentText(initialType.isEmpty() ? "Spacecraft" : initialType);
   type->setEnabled(initialType.isEmpty());
   dialog.setProperty("helpTopic",type->currentText());
   connect(type,&QComboBox::currentTextChanged,&dialog,[&dialog](const QString &topic) { dialog.setProperty("helpTopic",topic); });
   auto *name=new QLineEdit(&dialog); name->setObjectName("resourceName");
   const auto suggestName=[type,name] {
      name->setPlaceholderText(QString::fromStdString(Moderator::Instance()->GetNewName(type->currentText().toStdString(),1)));
   };
   const auto resolveName=[type,name] {
      if (name->text().trimmed().isEmpty())
         name->setText(QString::fromStdString(Moderator::Instance()->GetNewName(type->currentText().toStdString(),1)));
      return name->text().trimmed();
   };
   name->setToolTip("Leave blank to use the suggested available name.");
   connect(type,&QComboBox::currentTextChanged,&dialog,suggestName); suggestName();
   auto *status=new QLabel("Name and configure the resource here. Create commits all settings together.",&dialog); status->setObjectName("resourceCreationStatus"); status->setWordWrap(true);
   layout->addRow("Type",type); layout->addRow("Name (optional)",name);
   auto *rows=new QSpinBox(&dialog); rows->setObjectName("arrayRows"); rows->setRange(1,1000);
   auto *columns=new QSpinBox(&dialog); columns->setObjectName("arrayColumns"); columns->setRange(1,1000);
   layout->addRow("Rows",rows); layout->addRow("Columns",columns);
   auto *functionFiles=new QWidget(&dialog); auto *functionRow=new QHBoxLayout(functionFiles); functionRow->setContentsMargins(0,0,0,0);
   auto *functionPath=new QLineEdit(functionFiles); functionPath->setObjectName("resourceFunctionPath"); functionRow->addWidget(functionPath,1);
   auto *browseFunction=new QPushButton("Browse…",functionFiles); browseFunction->setObjectName("resourceFunctionBrowse"); functionRow->addWidget(browseFunction);
   auto *newFunction=new QPushButton("New file…",functionFiles); newFunction->setObjectName("resourceFunctionNew"); functionRow->addWidget(newFunction); layout->addRow("Function file",functionFiles);
   auto *functionNote=new QLabel("Cancel keeps any function file you already saved.",&dialog); functionNote->setWordWrap(true); layout->addRow(functionNote);
   connect(browseFunction,&QPushButton::clicked,&dialog,[&] {
      const auto path=QFileDialog::getOpenFileName(&dialog,"Choose GMAT function",functionPath->text(),"GMAT functions (*.gmf);;All files (*)");
      if (!path.isEmpty()) functionPath->setText(path);
   });
   connect(newFunction,&QPushButton::clicked,&dialog,[&] {
      const auto functionName=resolveName();
      if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(functionName).hasMatch()) { status->setText("Enter a valid function name before creating its file."); name->setFocus(); return; }
      QFileDialog chooser(&dialog,"New GMAT function file",functionName+".gmf","GMAT functions (*.gmf);;All files (*)");
      chooser.setObjectName("newFunctionFileDialog"); chooser.setAcceptMode(QFileDialog::AcceptSave); chooser.setDefaultSuffix("gmf");
      if (chooser.exec()!=QDialog::Accepted || chooser.selectedFiles().isEmpty()) return;
      const auto path=chooser.selectedFiles().first();
      if (QFileInfo::exists(path)) { status->setText("That file already exists. Use Browse or choose a new name."); return; }
      QString source;
      try { source=functionFileTemplate(path); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); return; }
      FunctionFileDialog function(path,&dialog,source);
      if (function.exec()==QDialog::Accepted) functionPath->setText(function.savedPath());
   });
   auto *host=new QWidget(&dialog); auto *hostLayout=new QVBoxLayout(host); hostLayout->setContentsMargins(0,0,0,0); outer->addWidget(host,1);
   auto panels=std::make_shared<QMap<QString,ResourceEditor *>>();
   auto drafts=std::make_shared<QMap<QString,std::shared_ptr<GmatBase>>>();
   auto active=std::make_shared<ResourceEditor *>(nullptr);
   const auto buildForm=[&,panels,drafts,active] {
      if (*active) (*active)->hide(); *active=nullptr;
      const auto selected=type->currentText();
      if (selected=="GmatFunction") return;
      if (!panels->contains(selected)) try {
         const auto draftName=name->text().trimmed().isEmpty() ? QString::fromStdString(Moderator::Instance()->GetNewName(selected.toStdString(),1)) : name->text().trimmed();
         auto draft=qtResourcePreviewShared(resourceDraft(selected,draftName)); (*drafts)[selected]=draft;
         auto *panel=new ResourceEditor(*draft,[&,selected](const QMap<QString,QString> &settings) {
            const auto error=createResource(selected,resolveName(),snapshot,rows->value(),columns->value(),
               (selected=="Variable" || selected=="String") ? std::optional<QString>(settings.value("Value")) : std::nullopt,{},settings.value("Spacecraft"),settings);
            return error;
         },host,{},false,true);
         panel->setProperty("helpTopic",selected);
         if (selected=="EphemerisFile" || selected=="ReportFile") {
            auto *filename=panel->findChild<QLineEdit *>(selected=="EphemerisFile" ? "ephemeris_Filename" : "resource_Filename");
            if (filename) {
               auto previous=std::make_shared<QString>(draftName);
               connect(name,&QLineEdit::textChanged,panel,[name,filename,selected,previous] {
                  const auto next=name->text().trimmed().isEmpty() ? QString::fromStdString(Moderator::Instance()->GetNewName(selected.toStdString(),1)) : name->text().trimmed();
                  if (filename->text().startsWith(*previous+".")) filename->setText(next+filename->text().mid(previous->size()));
                  *previous=next;
               });
            }
         }
         if (selected=="Array") {
            panel->onArrayDimensions=[rows,columns](int rowCount,int columnCount) {
               const QSignalBlocker rowBlock(rows),columnBlock(columns);
               rows->setValue(rowCount); columns->setValue(columnCount);
            };
            panel->resizeArray(rows->value(),columns->value());
         }
         panel->onStatus=[status](const QString &message) {
            QStringList lines;
            for (const auto &line:message.split('\n',Qt::SkipEmptyParts)) if (!lines.contains(line.trimmed())) lines.append(line.trimmed());
            status->setText(lines.join(' ')); status->setToolTip(message);
         };
         if (auto *message=panel->findChild<QLabel *>("resourceStatus")) message->hide();
         panel->onApplied=[&] { dialog.accept(); };
         panel->findChild<QDialogButtonBox *>("resourceButtons")->hide();
         (*panels)[selected]=panel; hostLayout->addWidget(panel);
      } catch (BaseException &error) { status->setText(QString::fromStdString(error.GetFullMessage())); return; }
      catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); return; }
      *active=panels->value(selected); (*active)->show();
   };
   const auto dimensions=[=] {
      const bool array=type->currentText()=="Array";
      layout->setRowVisible(rows,array); layout->setRowVisible(columns,array);
      const bool function=type->currentText()=="GmatFunction";
      layout->setRowVisible(functionFiles,function); layout->setRowVisible(functionNote,function);
   };
   connect(type,&QComboBox::currentTextChanged,&dialog,dimensions);
   connect(type,&QComboBox::currentTextChanged,&dialog,[&] { buildForm(); }); dimensions(); buildForm();
   const auto resizeArray=[active,type,rows,columns] { if (*active && type->currentText()=="Array") (*active)->resizeArray(rows->value(),columns->value()); };
   connect(rows,&QSpinBox::valueChanged,&dialog,resizeArray); connect(columns,&QSpinBox::valueChanged,&dialog,resizeArray);
   outer->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help,&dialog);
   auto *help=buttons->button(QDialogButtonBox::Help); help->setObjectName("resourceCreationHelp"); help->setToolTip("Help for the selected resource (F1)");
   connect(buttons,&QDialogButtonBox::helpRequested,&dialog,[this,type] { contextHelp->show(type->currentText()); });
   buttons->setObjectName("resourceCreationButtons"); buttons->button(QDialogButtonBox::Ok)->setText("Create"); outer->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
      if (type->currentText()=="GmatFunction") {
         const auto error=createResource(type->currentText(),resolveName(),snapshot,1,1,std::nullopt,functionPath->text());
         if (error.isEmpty()) dialog.accept(); else status->setText(error);
      } else if (*active) (*active)->requestApply();
   });
   dialog.resize(850,740); name->setFocus();
   if (dialog.exec()==QDialog::Accepted) {
      auto *navigation=findChild<QDockWidget *>("navigation");
      navigation->show(); navigation->findChild<QTabWidget *>()->setCurrentWidget(resources);
      const auto items=resources->findItems(name->text().trimmed(),Qt::MatchExactly|Qt::MatchRecursive);
      if (items.size()==1) { resources->setCurrentItem(items.first()); resources->scrollToItem(items.first()); resources->itemDoubleClicked(items.first(),0); }
   }
}

QString MainWindow::deleteResource(const QString &name,const QString &expectedScript)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before deleting a resource.";
   for (auto *child:workspace->subWindowList()) {
      auto *panel=dynamic_cast<EditablePanel *>(child->widget());
      if (child->property("resourceName").toString()==name && panel && panel->hasChanges())
         return "Apply or discard this resource's panel changes before deleting it.";
   }
   auto *moderator=Moderator::Instance();
   auto *object=moderator->GetConfiguredObject(name.toStdString());
   if (!object) return "This resource no longer exists.";
   // SolarSystem owns celestial bodies rather than ConfigManager. Match wx's
   // protected body workflow instead of reporting a false 'resource is used'.
   if (object->IsOfType("CelestialBody") || object->IsOfType("SolarSystem"))
      return "Celestial bodies and the SolarSystem cannot be deleted from this panel. Edit a user-defined body's script declaration instead.";
   QString candidate;
   try {
      const auto canonical=qtConfiguredScript();
      const QRegularExpression declaration("^[ \\t]*Create[ \\t]+[A-Za-z0-9_]+[ \\t]+[^;\\n]*\\b"+
         QRegularExpression::escape(name)+"\\b[^;\\n]*;",QRegularExpression::MultilineOption);
      if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch() || !declaration.match(canonical).hasMatch())
         return "Built-in resources and generated parameters cannot be deleted here.";
      QString firstCommand;
      for (const auto &node:missionState.nodes) if (node.type!="BeginMissionSequence") { firstCommand=node.statement; break; }
      candidate=removeQtCameraSetting(removeResourceConfiguration(expectedScript,name,firstCommand),name);
      if (!moderator->RemoveObject(object->GetType(),name.toStdString(),true))
         return "This resource is used by another resource or mission command. See Message Window for details.";
   } catch (BaseException &error) {
      const auto detail=QString::fromStdString(error.GetFullMessage());
      return restoreBuiltModel() ? detail : detail+" Restoration failed; rebuild the script.";
   } catch (const std::exception &error) {
      const auto detail=QString::fromUtf8(error.what());
      return restoreBuiltModel() ? detail : detail+" Restoration failed; rebuild the script.";
   }
   const auto error=applyModelScript(candidate,{},name);
   if (error.isEmpty()) {
      for (auto *child:workspace->subWindowList()) if (child->property("resourceName").toString()==name) child->close();
      statusBar()->showMessage("Resource deleted — Undo restores it; save to keep changes");
   }
   return error;
}

void MainWindow::setScriptDirectory()
{
   // Stream interpretation does not set the script folder as the engine's
   // filename overload does. Keep input/include lookup tied to this document
   // without changing the process directory used for startup data assets.
   const auto directory = scriptPath.isEmpty() ? startupDirectory : QFileInfo(scriptPath).absolutePath();
   if (!setGmatScriptDirectory(directory))
      throw std::runtime_error("The script directory is unavailable: " + directory.toStdString());
}

bool MainWindow::restoreBuiltModel()
{
   ++modelGeneration; summaryAvailable=false;
   plots->clear(); modelValid=false;
   try {
      setScriptDirectory();
      std::istringstream previous(builtScript.toStdString());
      modelValid=qtInterpretScript(previous);
   } catch (...) { }
   refreshTrees(); return modelValid;
}

QString MainWindow::applyModelScript(const QString &requested,const std::function<QString()> &validate,const QString &removedCamera)
{
   QString candidate;
   QMap<QString,QtCameraSetting> cameras;
   try {
      candidate=retainQtCameraSettings(editor->toPlainText(),requested,removedCamera);
      cameras=qtCameraSettings(candidate);
   } catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   QString error;
   ++modelGeneration; summaryAvailable=false;
   plots->clear();
   try {
      setScriptDirectory();
      std::istringstream stream(candidate.toStdString());
      if (!qtInterpretScript(stream)) error = "The mission rejected these changes. See Message Window.";
      else { QtPlotReceiver::validateCameraReferences(cameras); if (validate) error=validate(); }
   } catch (BaseException &exception) { error = QString::fromStdString(exception.GetFullMessage()); }
   catch (const std::exception &exception) { error = QString::fromUtf8(exception.what()); }
   if (!error.isEmpty()) {
      // Interpretation reconstructs the model. Restore the entire prior model,
      // not just the last field, without touching the editor or its undo stack.
      restoreBuiltModel();
      if (!modelValid) error += " Restoration failed; rebuild the script before continuing.";
      return error;
   }
   auto cursor = editor->textCursor();
   cursor.beginEditBlock();
   cursor.select(QTextCursor::Document);
   cursor.insertText(candidate);
   cursor.endEditBlock();
   builtScript = candidate;
   plots->cameraSettings=cameras;
   modelValid = true;
   refreshTrees();
   statusBar()->showMessage("Mission updated — save the script to keep changes");
   return {};
}

void MainWindow::openCommandEditor(int index,MissionEdit operation)
{
   if (running || !modelValid || editor->toPlainText()!=builtScript) {
      statusBar()->showMessage("Build the current script before editing mission commands"); return;
   }
   if (operation!=MissionEdit::Append && (index<0 || index>=missionState.nodes.size())) return;
   if (operation==MissionEdit::Replace && !missionState.nodes[index].editable) return;
   auto *panel=makeCommandPanel(index,operation);
   auto *child=new EditorSubWindow;
   child->setWidget(panel); workspace->addSubWindow(child);
   child->setAttribute(Qt::WA_DeleteOnClose); child->setProperty("configurationPanel",true);
   child->setProperty("commandIndex",index);
   child->setProperty("sourceScript",missionState.sourceScript);
   child->setWindowTitle(operation==MissionEdit::Replace ? missionState.nodes[index].label : "Insert mission command");
   child->resize(700,500); showNewWorkspaceWindow(workspace,child);
   workspace->setActiveSubWindow(child);
}
CommandEditor *MainWindow::makeCommandPanel(int index,MissionEdit operation)
{
   const auto snapshot=missionState;
   QString statement=operation==MissionEdit::Replace ? snapshot.nodes[index].statement : QString();
   const auto spacecraft=Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT);
   const auto propagators=Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP);
   const auto burns=Moderator::Instance()->GetListOfObjects(Gmat::IMPULSIVE_BURN);
   auto first=[](const StringArray &names,const char *fallback) { return QString::fromStdString(names.empty() ? fallback : names.front()); };
   const auto sat=first(spacecraft,"SpacecraftName"),prop=first(propagators,"PropagatorName"),burn=first(burns,"BurnName");
   auto firstType=[&](UnsignedInt category,const char *type,const char *fallback) {
      for (const auto &name:Moderator::Instance()->GetListOfObjects(category)) {
         auto *object=Moderator::Instance()->GetConfiguredObject(name);
         if (object && object->IsOfType(type) && !(QString::fromLatin1(type)=="Estimator" && object->IsOfType("Smoother"))) return QString::fromStdString(name);
      }
      return QString::fromLatin1(fallback);
   };
   const auto solver=firstType(Gmat::SOLVER,"DifferentialCorrector","SolverName");
   const auto optimizer=firstType(Gmat::SOLVER,"Optimizer","OptimizerName");
   const auto finite=firstType(Gmat::BURN,"FiniteBurn","FiniteBurnName");
   const auto report=firstType(Gmat::SUBSCRIBER,"ReportFile","ReportName");
   QMap<QString,QString> templates={
      {"Propagate",QString("Propagate %1(%2) {%2.ElapsedSecs = 600};").arg(prop,sat)},
      {"Maneuver",QString("Maneuver %1(%2);").arg(burn,sat)},
      {"BeginFiniteBurn",QString("BeginFiniteBurn %1(%2);").arg(finite,sat)},
      {"EndFiniteBurn",QString("EndFiniteBurn %1(%2);").arg(finite,sat)},
      {"Report",QString("Report %1 %2.ElapsedSecs %2.X %2.Y %2.Z;").arg(report,sat)},
      {"Assignment","VariableName = 1;"},
      {"If","If VariableName > 0;\n   % Insert commands here.\nElse;\n   % Insert alternate commands here.\nEndIf;"},
      {"While","While VariableName < 10;\n   VariableName = VariableName + 1;\nEndWhile;"},
      {"For","For VariableName = 1:1:10;\n   % Insert commands here.\nEndFor;"},
      {"Target",QString("Target %1 {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = true};\n   % Add Vary, mission commands, and Achieve here.\nEndTarget;").arg(solver)},
      {"Vary",QString("Vary %1(%2.Element1 = 0.1, {Perturbation = 0.0001, Lower = -10, Upper = 10, MaxStep = 0.2});").arg(solver,burn)},
      {"Achieve",QString("Achieve %1(%2.Earth.RMAG = 8000, {Tolerance = 0.1});").arg(solver,sat)},
      {"Optimize",QString("Optimize %1 {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = true};\n   % Add Vary, mission commands, constraints and Minimize here.\nEndOptimize;").arg(optimizer)},
      {"Minimize",QString("Minimize %1(ObjectiveVariable);").arg(optimizer)},
      {"NonlinearConstraint",QString("NonlinearConstraint %1(ConstraintVariable <= 1);").arg(optimizer)},
      {"FindEvents",QString("FindEvents %1 {Append = false};").arg(firstType(Gmat::EVENT_LOCATOR,"EventLocator","LocatorName"))},
      {"Call function",QString("[OutputVariable] = %1(InputVariable);").arg(firstType(Gmat::FUNCTION,"GmatFunction","FunctionName"))},
      {"Stop","Stop;"}, {"Script event","BeginScript;\n   % Insert commands here.\nEndScript;"}};
   if (availableEngineTypes().contains("CallPythonFunction")) templates.insert("Call Python function","[OutputVariable] = Python.ModuleName.FunctionName(InputVariable);");
   if (availableEngineTypes().contains("Toggle")) templates.insert("Toggle",QString("Toggle %1 On;").arg(first(Moderator::Instance()->GetListOfObjects(Gmat::SUBSCRIBER),"SubscriberName")));
   if (availableEngineTypes().contains("Write")) templates.insert("Write",QString("Write %1.ElapsedSecs;").arg(sat));
   if (availableEngineTypes().contains("Save")) templates.insert("Save",QString("Save %1;").arg(sat));
   if (availableEngineTypes().contains("Global")) templates.insert("Global",QString("Global %1;").arg(sat));
   if (availableEngineTypes().contains("CommandEcho")) templates.insert("CommandEcho","CommandEcho On;");
   if (availableEngineTypes().contains("Set")) templates.insert("Set (file import)",QString("Set %1 %2;").arg(sat,firstType(Gmat::INTERFACE,"DataInterface","FileInterfaceName")));
   if (availableEngineTypes().contains("RunSimulator")) templates.insert("RunSimulator",QString("RunSimulator %1;").arg(firstType(Gmat::SOLVER,"Simulator","SimulatorName")));
   if (availableEngineTypes().contains("RunEstimator")) templates.insert("RunEstimator",QString("RunEstimator %1;").arg(firstType(Gmat::SOLVER,"Estimator","EstimatorName")));
   if (availableEngineTypes().contains("RunSmoother")) templates.insert("RunSmoother",QString("RunSmoother %1;").arg(firstType(Gmat::SOLVER,"Smoother","SmootherName")));
   for (const auto &command:{QString("BeginFileThrust"),QString("EndFileThrust")}) if (availableEngineTypes().contains(command))
      templates.insert(command,QString("%1 %2(%3);").arg(command,firstType(Gmat::INTERFACE,"ThrustHistoryFile","ThrustHistoryName"),sat));
   QStringList propagationChoices,spacecraftChoices,formationChoices;
   for (const auto &value:propagators) propagationChoices.append(QString::fromStdString(value));
   for (const auto &value:spacecraft) spacecraftChoices.append(QString::fromStdString(value));
   for (const auto &value:Moderator::Instance()->GetListOfObjects(Gmat::FORMATION)) formationChoices.append(QString::fromStdString(value));
   auto *panel=new CommandEditor(statement,operation!=MissionEdit::Replace,templates,
      [this,snapshot,index,operation](const QString &replacement) {
         return applyMissionChange(snapshot,index,operation,replacement);
      },propagationChoices,spacecraftChoices,nullptr,formationChoices,
      operation==MissionEdit::Replace ? std::function<void()>([this,index,generation=modelGeneration] {
         if (generation!=modelGeneration) { statusBar()->showMessage("The mission changed. Reopen this command panel to inspect its summary."); return; }
         showSummary(index);
      }) : std::function<void()>(),
      operation==MissionEdit::Replace && (snapshot.nodes[index].type=="Target" || snapshot.nodes[index].type=="Optimize") ?
         std::function<QString()>([this,snapshot,index] { return applySolverCorrections(snapshot,index); }) : std::function<QString()>());
   contextHelp->attach(panel,panel->property("helpTopic").toString());
   panel->onApplied=[this,panel,snapshot,index,operation] {
      auto *child=qobject_cast<QMdiSubWindow *>(panel->parentWidget());
      if (!child) return;
      // Insertion becomes an editor for its first accepted command. Following
      // Apply updates that command instead of repeating the insertion.
      int accepted=index;
      if (operation==MissionEdit::Append) accepted=snapshot.nodes.size();
      else if (operation==MissionEdit::InsertAfter) {
         accepted=index+1;
         for (;accepted<snapshot.nodes.size();++accepted) {
            int parent=snapshot.nodes[accepted].parent;
            while (parent>=0 && parent!=index) parent=snapshot.nodes[parent].parent;
            if (parent!=index) break;
         }
      }
      if (accepted<0 || accepted>=missionState.nodes.size() || !missionState.nodes[accepted].editable) {
         statusBar()->showMessage("Command applied; reopen its mission entry to continue editing"); return;
      }
      CommandEditor *replacement;
      try { replacement=makeCommandPanel(accepted,MissionEdit::Replace); }
      catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); return; }
      catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); return; }
      const auto *focus=QApplication::focusWidget(); const auto name=focus && panel->isAncestorOf(focus) ? focus->objectName() : QString();
      child->setWidget(replacement); child->setProperty("commandIndex",accepted); child->setProperty("sourceScript",missionState.sourceScript);
      child->setWindowTitle(missionState.nodes[accepted].label); panel->hide(); panel->deleteLater(); replacement->show();
      if (auto *target=replacement->findChild<QWidget *>(name);!name.isEmpty() && target) target->setFocus();
      refreshAppliedResourcePanels();
   };
   return panel;
}
QString MainWindow::applyMissionChange(const MissionSnapshot &snapshot,int index,MissionEdit operation,
                                     const QString &replacement)
{
   if (running) return "Stop the mission before editing commands.";
   if (!modelValid || snapshot.sourceScript!=builtScript || editor->toPlainText()!=builtScript)
      return "The mission has changed. Build the current script and reopen this panel.";
   try { return applyModelScript(editMission(snapshot,index,operation,replacement)); }
   catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
}

QString MainWindow::applySolverCorrections(const MissionSnapshot &snapshot,int index)
{
   if (running) return "Stop the mission before applying solver corrections.";
   if (!modelValid || snapshot.sourceScript!=builtScript || editor->toPlainText()!=builtScript)
      return "The mission has changed. Build the current script and reopen this panel.";
   if (!summaryAvailable) return "Run the current mission before applying solver corrections. Rebuilding invalidates previous results.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard pending panel changes before applying solver corrections.";
   bool mutated=false;
   try {
      if (index<0 || index>=runCommands.size() || index>=snapshot.nodes.size() || runCorrectionState.sourceScript!=builtScript)
         return "This solver command no longer exists. Reopen its panel.";
      auto *branch=dynamic_cast<SolverBranchCommand *>(runCommands[index]);
      if (!branch || (snapshot.nodes[index].type!="Target" && snapshot.nodes[index].type!="Optimize"))
         return "Corrections require a Target or Optimize command.";
      auto *solver=dynamic_cast<Solver *>(branch->GetClone(0));
      if (!solver) return "Run this solver block before applying corrections.";
      const auto status=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));
      if (status==Gmat::CREATED || status==Gmat::COPIED || status==Gmat::INITIALIZED)
         return "Run this solver block before applying corrections.";
      struct Correction { Vary *vary; qsizetype start,length; QString initial; };
      QVector<Correction> corrections;
      QStringList references;
      const int branchIndex=runCorrectionCommands.indexOf(branch);
      if (branchIndex<0) return "The solver's source mapping is unavailable. Rebuild and rerun the mission.";
      static const QRegularExpression guess(R"(^\s*(?:GMAT\s+)?Vary\s+(?:'[^'\n]*'\s+)?[A-Za-z][A-Za-z0-9_]*\s*\([\s\S]*?=\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*(?=,|\)))");
      for (int i=0;i<runCorrectionState.nodes.size();++i) {
         const auto &node=runCorrectionState.nodes[i];
         if (node.parent!=branchIndex || node.type!="Vary") continue;
         if (i>=runCorrectionCommands.size() || !node.editable) return "Cannot safely locate a Vary command in the source. Rebuild and reopen this panel.";
         auto *vary=dynamic_cast<Vary *>(runCorrectionCommands[i]); if (!vary) return "The Vary commands changed. Reopen this panel.";
         const auto initial=QString::fromStdString(vary->GetStringParameter("InitialValue"));
         bool numeric=false; const auto value=initial.toDouble(&numeric);
         if (!numeric || !std::isfinite(value)) {
            references.append(QString::fromStdString(vary->GetStringParameter("Variable"))+" = "+initial);
            continue; // Engine reference guesses remain source expressions.
         }
         const auto statements=scriptStatements(builtScript.mid(node.start,node.end-node.start));
         if (statements.size()!=1) return "Cannot safely locate this Vary initial guess in the source.";
         const auto &statement=statements.first(); const auto match=guess.match(statement.code);
         if (!match.hasMatch() || match.captured(1).toDouble()!=value) return "The Vary initial guess no longer matches the source. Rebuild and reopen this panel.";
         const auto positions=statement.positions.mid(match.capturedStart(1),match.capturedLength(1));
         if (positions.isEmpty() || positions.last()-positions.first()+1!=positions.size())
            return "Cannot safely replace a continued Vary initial guess. Edit this guess in the script.";
         corrections.append({vary,node.start+positions.first(),positions.size(),initial});
      }
      QString candidate=builtScript;
      for (auto it=corrections.crbegin();it!=corrections.crend();++it) {
         mutated=true; it->vary->SetInitialValue(solver);
         const auto value=QString::fromStdString(it->vary->GetStringParameter("InitialValue"));
         bool numeric=false; const auto number=value.toDouble(&numeric);
         if (!numeric || !std::isfinite(number)) throw std::runtime_error("The solver returned an invalid initial guess. No source changes were applied.");
         if (number!=it->initial.toDouble()) candidate.replace(it->start,it->length,value);
      }
      if (candidate==builtScript) return "No numeric Vary initial guesses changed. Reference guesses are retained.";
      if (!references.isEmpty()) messages->appendPlainText("Apply Corrections retained reference guesses: "+references.join(", "));
      const auto error=applyModelScript(candidate); mutated=false;
      if (!error.isEmpty()) return error;
      // The owning solver panel refreshes through onApplied. Refresh clean
      // companions with their new source snapshots, retaining their MDI windows.
      for (auto *child:workspace->subWindowList()) {
         if (!child->property("commandIndex").isValid()) continue;
         const int other=child->property("commandIndex").toInt();
         auto *old=dynamic_cast<CommandEditor *>(child->widget());
         if (!old || other==index || other<0 || other>=missionState.nodes.size() || !missionState.nodes[other].editable) continue;
         CommandEditor *fresh=nullptr;
         try { fresh=makeCommandPanel(other,MissionEdit::Replace); }
         catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); continue; }
         catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); continue; }
         child->setWidget(fresh); child->setProperty("sourceScript",builtScript); child->setWindowTitle(missionState.nodes[other].label);
         old->hide(); old->deleteLater(); fresh->show();
      }
      return {};
   } catch (BaseException &error) {
      if (mutated) { summaryAvailable=false; restoreBuiltModel(); }
      return QString::fromStdString(error.GetFullMessage());
   } catch (const std::exception &error) {
      if (mutated) { summaryAvailable=false; restoreBuiltModel(); }
      return QString::fromUtf8(error.what());
   }
}

QString MainWindow::savePlotProjection(const QString &name,bool perspective,double fov)
{
   if (!ready || running) return "Stop the mission before saving projection settings.";
   if (!modelValid || editor->toPlainText()!=builtScript) return "Build the current script before saving projection settings.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard panel changes before saving projection settings.";
   auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
   if (!object || !object->IsOfType("OrbitView")) return "This OrbitView no longer exists in the current mission.";
   QString candidate;
   QtCameraSetting setting;
   try {
      setting=qtCameraSettings(builtScript).value(name);
      const auto model=plots->model(name);
      if (model && model->selectedCamera>0 && model->selectedCamera<=setting.views.size()) {
         auto &view=setting.views[model->selectedCamera-1]; view.perspective=perspective; view.fieldOfView=fov;
      } else { setting.perspective=perspective; setting.fieldOfView=fov; }
      candidate=setQtCameraSetting(builtScript,name,setting);
   }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   if (candidate==builtScript) return {};
   // This changes a validated comment only. Do not rebuild/delete the viewer
   // from within its own action callback or reconstruct scientific objects.
   auto cursor=editor->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document);
   cursor.insertText(candidate); cursor.endEditBlock();
   builtScript=candidate; plots->cameraSettings[name]=setting;
   refreshTrees();
   statusBar()->showMessage("Projection added to script — save to keep it; Undo restores the previous settings");
   return {};
}

QStringList MainWindow::summaryFrames() const
{
   QStringList frames;
   if (!ready || !modelValid) return frames;
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) {
      auto *coordinate=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(name));
      if (!coordinate) continue;
      auto *origin=coordinate->GetOrigin();
      GmatBase *configuredOrigin=origin;
      if (!configuredOrigin) configuredOrigin=Moderator::Instance()->GetConfiguredObject(coordinate->GetStringParameter("Origin"));
      if (!configuredOrigin || !configuredOrigin->IsOfType("SpacePoint") || configuredOrigin->IsOfType("Spacecraft") || coordinate->UsesSpacecraft()) continue;
      // Configured axes can lack initialized reference pointers. Check names as
      // well so a spacecraft-dependent frame cannot use cached states at a
      // different epoch to construct a misleading summary.
      bool usesSpacecraft=false;
      auto *axes=coordinate->GetOwnedObject(0);
      if (axes) for (const auto *property:{"Primary","Secondary","ReferenceObject"}) {
         try {
            auto *reference=Moderator::Instance()->GetConfiguredObject(axes->GetStringParameter(property));
            usesSpacecraft=usesSpacecraft || (reference && reference->IsOfType("Spacecraft"));
         } catch (BaseException &) { }
      }
      if (!usesSpacecraft) frames.append(QString::fromStdString(name));
   }
   frames.removeDuplicates(); frames.sort(); return frames;
}
QString MainWindow::summaryText(int index,const QString &frame,bool physicsOnly)
{
   if (!ready || running || !modelValid || !summaryAvailable || editor->toPlainText()!=builtScript)
      throw std::runtime_error("Run the current mission before inspecting its summary. Rebuilding or editing invalidates the previous results.");
   if (!summaryFrames().contains(frame)) throw std::runtime_error("Choose a coordinate system with a non-spacecraft origin and no spacecraft-dependent axes.");
   QVector<GmatCommand *> commands;
   snapshotMission(Moderator::Instance()->GetFirstCommand(),missionState.canonicalScript,builtScript,&commands);
   if (index<-1 || index>=commands.size()) throw std::runtime_error("This command no longer exists. Reopen its panel.");
   auto *command=index<0 ? Moderator::Instance()->GetFirstCommand() : commands[index];
   if (!command) throw std::runtime_error("The mission has no commands to summarize.");
   SummaryNames names(Moderator::Instance()->GetFirstCommand());
   const auto name=command->GetSummaryName();
   if (command->IsOfType("BeginScript")) {
      auto *end=GmatCommandUtil::GetMatchingEnd(command);
      if (!end) throw std::runtime_error("This script event has no matching EndScript.");
      command=end;
   }
   const auto previousName=command->GetSummaryName();
   struct RestoreName {
      GmatCommand *command; std::string name;
      ~RestoreName() { command->SetSummaryName(name); }
   } restore{command,previousName};
   if (index>=0) command->SetSummaryName(name);
   command->SetupSummary(frame.toStdString(),index<0,physicsOnly && index<0);
   return QString::fromStdString(command->GetStringParameter(index<0 ? "MissionSummary" : "Summary"));
}
void MainWindow::showSummary(int index)
{
   // Validate before constructing a dialog, including when invoked from an
   // editor whose original command was removed by a rebuild.
   try { summaryText(index,"EarthMJ2000Eq"); }
   catch (BaseException &error) { statusBar()->showMessage(QString::fromStdString(error.GetFullMessage())); return; }
   catch (const std::exception &error) { statusBar()->showMessage(QString::fromUtf8(error.what())); return; }
   const auto generation=modelGeneration;
   const auto originalPath=scriptPath;
   const auto context=lastRunResult==RunResult::Completed ? "Results from the completed run. States are captured at each command." :
      lastRunResult==RunResult::Stopped ? "Partial results from the stopped run. Unexecuted commands have no summary data." :
      "Partial results from the failed run. Unexecuted commands have no summary data. See the Message Window for the failure.";
   SummaryDialog dialog(index<0 ? "Mission summary" : "Command summary — "+missionState.nodes[index].label,
      summaryFrames(),index<0,[this,index,generation](const QString &frame,bool physics) {
         if (generation!=modelGeneration) throw std::runtime_error("The mission changed. Close this summary and run the current mission.");
         return summaryText(index,frame,physics);
      },context,this,[this,originalPath] { return QStringList{scriptPath,originalPath}; });
   dialog.exec();
}
