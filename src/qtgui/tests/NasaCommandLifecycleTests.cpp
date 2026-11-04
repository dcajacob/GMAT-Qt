#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "InspectionDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const QString &message) { if (!ok) throw std::runtime_error(message.toStdString()); }
class CommandCase {
public:
   MainWindow window;
   QJsonObject data;
   QPlainTextEdit *script;
   QTreeWidget *tree;
   QMdiArea *area;
   int delay;
   CommandCase(const QString &startup,QJsonObject data) : data(data) {
      delay=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 120 : 0;
      window.resize(1280,1000); window.show(); require(window.initialize(startup),"Engine initialization failed");
      script=window.findChild<QPlainTextEdit *>("scriptEditor"); tree=window.findChild<QTreeWidget *>("Mission"); area=window.findChild<QMdiArea *>("workspace");
      require(script && tree && area,"Mission workspace absent");
      auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs && i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   }
   ~CommandCase() { for (auto *child:area->subWindowList()) if (auto *p=dynamic_cast<CommandEditor *>(child->widget())) p->discardChanges(); }
   void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
   void later(const std::function<void()> &action,std::exception_ptr &failure) {
      QTimer::singleShot(delay,&window,[&,action] { try { action(); } catch (...) { failure=std::current_exception(); if (auto *modal=qobject_cast<QDialog *>(QApplication::activeModalWidget())) modal->reject(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); } });
   }
   int index(const QString &label) {
      const auto snapshot=window.missionSnapshot();
      for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==label) return i;
      throw std::runtime_error(("Mission label missing: "+label).toStdString());
   }
   QTreeWidgetItem *item(int index) {
      for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) return *it;
      throw std::runtime_error("Mission tree entry absent");
   }
   void context(int index,const QString &action,const std::function<void()> &after={}) {
      auto *entry=item(index); tree->expandAll(); tree->scrollToItem(entry); tree->setCurrentItem(entry); events();
      std::exception_ptr failure;
      later([&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Mission context menu absent");
         QAction *choice=nullptr; for (auto *a:menu->actions()) if (a->text()==action || a->objectName()==action) choice=a;
         require(choice && choice->isEnabled(),"Mission action unavailable: "+action); if (after) later(after,failure);
         menu->setActiveAction(choice); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
      },failure);
      tree->customContextMenuRequested(tree->visualItemRect(entry).center()); if (failure) std::rethrow_exception(failure); events();
   }
   QMdiSubWindow *child() { auto *child=area->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Actual command panel absent"); return child; }
   QPlainTextEdit *preview() { auto *view=child()->findChild<QPlainTextEdit *>("commandSource"); require(view,"Command preview absent"); return view; }
   QLineEdit *name() { auto *view=child()->findChild<QLineEdit *>("commandName"); require(view,"Command name absent"); return view; }
   void apply() { auto *buttons=child()->findChild<QDialogButtonBox *>(); require(buttons && buttons->button(QDialogButtonBox::Apply),"Apply absent"); buttons->button(QDialogButtonBox::Apply)->click(); events(); }
   void close() { auto *buttons=child()->findChild<QDialogButtonBox *>(); require(buttons,"Close absent"); buttons->button(QDialogButtonBox::Close)->click(); events(); }
   void panelButton(QDialogButtonBox::StandardButton which) {
      auto *buttons=child()->findChild<QDialogButtonBox *>();
      auto *button=buttons ? buttons->button(which) : nullptr;
      require(button && button->isEnabled() && button->isVisible(),"Panel OK/Cancel action absent or disabled");
      button->click(); events();
   }
   void panelButtons(const QString &original,const QString &edited,const QString &currentLabel) {
      context(index(currentLabel),"Edit command…");
      QPointer<QMdiSubWindow> previous=child();
      const auto cleanSteps=script->document()->availableUndoSteps();
      panelButton(QDialogButtonBox::Ok);
      require(!previous && script->toPlainText()==edited && script->document()->availableUndoSteps()==cleanSteps,
         "Clean command OK changed source/history or did not close");

      context(index(currentLabel),"Edit command…"); previous=child();
      preview()->setPlainText(data["before"].toString());
      require(dynamic_cast<CommandEditor *>(previous->widget())->hasChanges(),"Cancel fixture has no pending edit");
      panelButton(QDialogButtonBox::Cancel);
      require(!previous && script->toPlainText()==edited && script->document()->availableUndoSteps()==cleanSteps,
         "Command Cancel committed pending edits, lost prior Apply or changed history");
      proof("cancel",{"mission-tree","discard-pending","retain-prior-apply","exact-source","no-extra-undo","closed-owner"});

      context(index(currentLabel),"Edit command…"); previous=child();
      preview()->setPlainText("UnknownNASACommand;"); panelButton(QDialogButtonBox::Ok);
      require(previous && previous==child() && preview()->toPlainText()=="UnknownNASACommand;" &&
         script->toPlainText()==edited && script->document()->availableUndoSteps()==cleanSteps,
         "Invalid command OK closed the panel, changed source or discarded correction input");
      auto *status=previous->findChild<QLabel *>("commandStatus");
      require(status && !status->text().isEmpty() && status->text()!=
         "Apply validates the complete mission and updates the script. Branches include their enclosed commands.",
         "Invalid command OK omitted its diagnostic");
      preview()->setPlainText(data["before"].toString()); panelButton(QDialogButtonBox::Ok);
      require(!previous && script->toPlainText()==original,"Valid command OK did not commit the exact original command then close");
      require(window.missionSnapshot().nodes[index("NASA original")].statement.trimmed()==data["before"].toString(),
         "Command OK did not update the mission snapshot");
      script->undo(); require(script->toPlainText()==edited,"Command OK Undo was not exact");
      script->redo(); require(script->toPlainText()==original,"Command OK Redo was not exact");
      script->undo(); require(script->toPlainText()==edited && window.buildScript(),"Command OK test did not restore the applied fixture");
      proof("ok",{"mission-tree","validation-rejection","valid-commit-close","clean-noop","exact-source","undo-redo","closed-owner"});
   }
   void edit() {
      const auto field=data["field"].toString();
      if (field.isEmpty()) { name()->setText("NASA edited"); return; }
      if (field=="source") { preview()->setPlainText(data["after"].toString()); return; }
      auto *control=child()->findChild<QLineEdit *>("commandField_"+field); require(control,"Typed command input absent: "+field);
      control->setText(data["value"].toString()); events();
   }
   void proof(const QString &operation,const QStringList &facets={"mission-tree","exact-source","pending-protection"}) {
      QJsonArray values; for (const auto &f:facets) values.append(f);
      QJsonObject record={{"checkpoint","command."+data["key"].toString()+"."+operation},{"family",data["key"]},{"operation",operation},{"outcome","passed"},{"facets",values}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
};
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaCommands");
   try {
      require(argc==4,"Supply startup, command recipes and case key"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary fixtures unavailable");
      QFile recipes(QString::fromLocal8Bit(argv[2])); require(recipes.open(QIODevice::ReadOnly),"Recipes unavailable");
      QJsonObject data; for (const auto &entry:QJsonDocument::fromJson(recipes.readAll()).array()) if (entry.toObject()["key"].toString()==QString::fromLocal8Bit(argv[3])) data=entry.toObject();
      if (QString::fromLocal8Bit(argv[3])=="IfNoElse") {
         // Separate the historical no-Else branch context from the existing
         // If recipe, which deliberately exercises an Else branch.
         data={{"key","IfNoElse"},{"template","If"},{"field","Condition"},{"value","V > 0"},
            {"before","If 'NASA original' V == 0; % keep header\n   V = 2; % NASA body\nEndIf; % keep footer"},
            {"after","If 'NASA original' V > 0; % keep header\n   V = 2; % NASA body\nEndIf; % keep footer"},
            {"expected",QJsonArray{QJsonArray{0,7}}}};
      }
      require(!data.isEmpty(),"Command case missing"); const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      CommandCase test(startup,data); auto *script=test.script;
      const auto reportPath=files.filePath("truth.txt");
      const QString solverFixtures=data["key"].toString()=="RunSimulator" ? "Create Simulator Sim OtherSim;\n" : data["key"].toString()=="RunEstimator" ? "Create BatchEstimator Est OtherEst;\n" : QString();
      const bool plotCommand=QStringList{"PenUp","PenDown","MarkPoint","ClearPlot"}.contains(data["key"].toString());
      const QString plotFixtures=plotCommand ? "Create OrbitView ThreeD;\nThreeD.Add = {Sat};\nThreeD.ShowPlot = false;\nCreate GroundTrack Surface;\nSurface.Add = {Sat};\nSurface.ShowPlot = false;\n" : QString();
      const QString prefix="% NASA exact source sentinel Δ\nCreate Variable V i;\nV = 0;\ni = 7;\nCreate Spacecraft Sat Other;\nCreate ImpulsiveBurn Burn OtherBurn;\nCreate FiniteBurn FB OtherFB;\nCreate DifferentialCorrector DC OtherDC;\nCreate Yukon Opt OtherOpt;\nCreate Propagator Prop;\n"+solverFixtures+plotFixtures+"Create XYPlot Idle OtherIdle;\nIdle.XVariable = V;\nIdle.YVariables = {i};\nIdle.ShowPlot = false;\nOtherIdle.XVariable = V;\nOtherIdle.YVariables = {i};\nOtherIdle.ShowPlot = false;\nCreate ReportFile Truth;\nTruth.Filename = '"+reportPath+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 16;\nBeginMissionSequence;\n";
      const QString wrapper=data["wrapper"].toString(),footer=data["footer"].toString();
      const QString tail="Report 'NASA anchor' Truth V i;\n% Preserve final comment Ω\n";
      const auto empty=prefix+wrapper+footer+tail;
      script->setPlainText(empty); require(test.window.buildScript(),"Insertion fixture failed to build");
      // The actual Mission tree insertion menu and template are required, even
      // for commands whose fixture needs a complete branch rather than defaults.
      const QString insertionLabel=wrapper.isEmpty() ? "NASA anchor" : "NASA wrapper";
      if (wrapper.isEmpty()) test.context(test.index(insertionLabel),"Insert before…");
      else test.context(test.index(insertionLabel),"appendInsideMissionBranch");
      auto *templates=test.child()->findChild<QComboBox *>("commandTemplate"); const auto type=data["template"].toString();
      require(templates && templates->findText(type)>=0,"Required mission insertion template absent: "+type);
      templates->setCurrentText(type); templates->textActivated(type); test.events();
      test.preview()->setPlainText(data["before"].toString()); require(script->toPlainText()==empty,"Pending insertion changed source"); test.apply();
      const auto original=prefix+wrapper+data["before"].toString()+"\n"+footer+tail;
      require(script->toPlainText()==original,"Insertion differs from literal source oracle"); require(test.item(test.index("NASA original"))->text(0).contains("NASA original"),"Mission tree omits applied command name"); test.proof("insert");
      const auto steps=script->document()->availableUndoSteps(); test.apply(); require(script->toPlainText()==original && steps==script->document()->availableUndoSteps(),"Second insertion Apply duplicated or changed command"); test.close();
      // Insert after a selected branch means after the whole branch, whereas
      // inserting after a leaf inside a solver keeps the new command inside it.
      const QString sibling="'NASA sibling' V = V + 0; % preserve sibling";
      test.context(test.index("NASA original"),"Insert after…"); test.preview()->setPlainText(sibling); test.apply();
      require(script->toPlainText()==prefix+wrapper+data["before"].toString()+"\n"+sibling+"\n"+footer+tail,"Insert After crossed a branch boundary or changed unrelated source"); test.close();
      test.context(test.index("NASA sibling"),"Delete command",[&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(box && box->button(QMessageBox::Yes),"Sibling delete confirmation absent"); box->button(QMessageBox::Yes)->click(); });
      require(script->toPlainText()==original,"Sibling removal did not restore exact source"); test.proof("insert-after");
      if (data["before"].toString().contains("NASA body") && type!="Script event") {
         auto *branch=test.item(test.index("NASA original")); require(branch->childCount()>0,"Branch children missing from Mission tree");
         test.tree->collapseItem(branch); test.events(); require(!branch->isExpanded(),"Branch did not collapse");
         test.tree->expandItem(branch); test.events(); require(branch->isExpanded(),"Branch did not expand");
         auto *depth=test.window.findChild<QComboBox *>("missionDepth"); require(depth,"Mission expansion control absent");
         depth->setCurrentIndex(depth->findData(0)); test.events();
         for (QTreeWidgetItemIterator it(test.tree);*it;++it) require(!(*it)->isExpanded(),"Collapse All left an expanded branch");
         depth->setCurrentIndex(depth->findData(-1)); test.events();
         for (QTreeWidgetItemIterator it(test.tree);*it;++it) if ((*it)->childCount()) require((*it)->isExpanded(),"Expand All left a collapsed branch");
         require(script->toPlainText()==original,"Tree expansion changed script"); test.proof("expand-collapse");
      }
      test.context(test.index("NASA original"),"Edit command…"); require(test.preview()->toPlainText().trimmed()==data["before"].toString(),"Context Open differs from applied command"); test.proof("open-context"); test.close(); test.proof("close");
      test.tree->itemDoubleClicked(test.item(test.index("NASA original")),0); test.events(); require(test.preview()->toPlainText().trimmed()==data["before"].toString(),"Double-click Open differs from applied command"); test.proof("open-double-click");
      if (QStringList{"PenUp","PenDown","MarkPoint","ClearPlot"}.contains(data["key"].toString())) {
         std::exception_ptr choiceFailure;
         test.later([&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); require(dialog,"Plot selector absent");
            auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList"); QStringList allowed={"Idle","OtherIdle"}; if (data["key"].toString()=="PenUp" || data["key"].toString()=="PenDown") allowed.append({"ThreeD","Surface"});
            require(list && list->count()==allowed.size(),"Plot selector includes unsupported outputs or omits supported plot types");
            for (int i=0;i<list->count();++i) require(allowed.removeOne(list->item(i)->text()),"Plot selector lists duplicate/unsupported resource");
            require(allowed.isEmpty(),"Plot selector omitted a supported type");
            auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons,"Plot selector buttons absent");
            for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked);
            require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Empty plot selection accepted");
            for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Checked);
            require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Multiple plot selection disabled"); buttons->button(QDialogButtonBox::Cancel)->click();
         },choiceFailure);
         auto *choose=test.child()->findChild<QPushButton *>("commandChoose_Subscribers"); require(choose,"Typed plot selector absent"); choose->click(); if (choiceFailure) std::rethrow_exception(choiceFailure);
         require(test.preview()->toPlainText().trimmed()==data["before"].toString() && script->toPlainText()==original,"Plot selector Cancel changed pending or applied source"); test.proof("plot-selector");
      }
      test.edit(); const auto pending=test.preview()->toPlainText(); require(pending.trimmed()==data["after"].toString(),"Typed field changed unexpected command text"); require(script->toPlainText()==original,"Pending edit changed script");
      test.preview()->setPlainText("UnknownNASACommand;"); test.apply(); require(script->toPlainText()==original && test.preview()->toPlainText()=="UnknownNASACommand;", "Invalid command changed applied source or lost correction input");
      require(!test.child()->findChild<QLabel *>("commandStatus")->text().isEmpty(),"Invalid command lacks diagnostic"); test.preview()->setPlainText(pending); test.proof("invalid-command");
      test.name()->setText("Invalid'Name"); test.apply(); require(script->toPlainText()==original && test.preview()->toPlainText()==pending,"Invalid name changed model or discarded pending command");
      require(!test.child()->findChild<QLabel *>("commandStatus")->text().isEmpty(),"Invalid label lacks error");
      test.name()->setText(data["field"].toString().isEmpty() ? "NASA edited" : "NASA original"); test.proof("invalid-label");
      // Show Script must describe applied source even while edits are pending.
      std::exception_ptr failure;
      test.later([&] { auto *dialog=test.window.findChild<QDialog *>("inspectionDialog"); require(dialog,"Show Script dialog absent"); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text && text->isReadOnly() && text->toPlainText().trimmed()==data["before"].toString(),"Show Script exposed pending or wrong command source"); dialog->accept(); },failure);
      auto *show=test.child()->findChild<QPushButton *>("showScript"); require(show,"Show Script absent"); show->click(); if (failure) std::rethrow_exception(failure); test.proof("show-script");
      for (const auto answer:{QMessageBox::Cancel,QMessageBox::Discard}) {
         QPointer<QMdiSubWindow> previous=test.child(); test.later([&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(box && box->button(answer),"Pending close choice absent"); box->button(answer)->click(); },failure); test.close(); if (failure) std::rethrow_exception(failure);
         require(script->toPlainText()==original,"Pending close modified applied source");
         if (answer==QMessageBox::Cancel) { require(previous && test.preview()->toPlainText()==pending,"Cancel lost pending command"); test.proof("exit-no"); }
         else { require(previous.isNull(),"Discard did not close command panel"); test.context(test.index("NASA original"),"Edit command…"); require(test.preview()->toPlainText().trimmed()==data["before"].toString(),"Discard persisted pending edit"); test.proof("exit-yes"); }
      }
      test.edit(); test.apply(); const auto edited=prefix+wrapper+data["after"].toString()+"\n"+footer+tail;
      require(script->toPlainText()==edited,"Applied command differs from exact literal source oracle");
      const auto appliedSteps=script->document()->availableUndoSteps(); test.apply(); require(script->toPlainText()==edited && appliedSteps==script->document()->availableUndoSteps(),"Repeated Apply changed source or undo history"); test.close();
      script->undo(); require(script->toPlainText()==original,"Undo changed unrelated command bytes"); script->redo(); require(script->toPlainText()==edited,"Redo lost exact command source"); require(test.window.buildScript(),"Edited script rebuild failed"); test.proof("apply"); test.proof("undo-redo");
      test.panelButtons(original,edited,data["field"].toString().isEmpty() ? "NASA edited" : "NASA original");
      const auto saved=files.filePath(QString::fromUtf8("NASA 命令 Δ.script")); require(test.window.saveScriptTo(saved),"Unicode save failed"); require(test.window.loadScript(saved) && script->toPlainText()==edited && test.window.buildScript(),"Unicode reopen lost command source"); test.proof("save-reopen",{"mission-tree","exact-source","unicode-save-reopen"});
      if (data.contains("expected")) {
         const auto result=test.window.runMission(); require(result==MainWindow::RunResult::Completed,"Known-result command execution failed: "+test.window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()); QFile report(reportPath); require(report.open(QIODevice::ReadOnly),"Execution report missing"); const auto rows=QString::fromUtf8(report.readAll()).trimmed().split('\n');
         const auto expected=data["expected"].toArray(); require(rows.size()==expected.size(),"Execution report row count differs from literal truth");
         for (int r=0;r<rows.size();++r) { const auto values=rows[r].split(','); const auto truth=expected[r].toArray(); require(values.size()==truth.size(),"Execution report column count differs from truth"); for (int c=0;c<values.size();++c) { bool ok=false; const auto v=values[c].trimmed().toDouble(&ok); require(ok && std::isfinite(v) && v==truth[c].toDouble(),"Execution report differs from independently calculated literal truth"); } }
         QJsonArray observed; for (const auto &row:rows) { QJsonArray values; for (const auto &value:row.split(',')) values.append(value.trimmed().toDouble()); observed.append(values); }
         std::cout<<"NASA_RESULT "<<QJsonDocument(QJsonObject{{"family",data["key"]},{"expected",expected},{"observed",observed}}).toJson(QJsonDocument::Compact).constData()<<std::endl;
         test.proof("execution",{"mission-tree","exact-source","literal-execution-report"});
      }
      const auto currentLabel=data["field"].toString().isEmpty() ? "NASA edited" : "NASA original";
      const int beforeNodes=test.window.missionSnapshot().nodes.size();
      auto remove=[&](QMessageBox::StandardButton answer) { test.context(test.index(currentLabel),"Delete command",[&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(box && box->button(answer),"Delete confirmation absent"); box->button(answer)->click(); }); };
      remove(QMessageBox::Cancel); require(script->toPlainText()==edited && test.window.missionSnapshot().nodes.size()==beforeNodes,"Delete Cancel changed mission");
      remove(QMessageBox::Yes); require(script->toPlainText()==empty,"Delete failed to remove exactly the selected command and its branch");
      for (const auto &node:test.window.missionSnapshot().nodes) require(!node.statement.contains("NASA original") && !node.statement.contains("NASA edited") && !node.statement.contains("NASA body"),"Delete left orphaned branch commands");
      test.proof("delete"); std::cout<<"PASS NASA command lifecycle "<<data["key"].toString().toStdString()<<'\n';
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
