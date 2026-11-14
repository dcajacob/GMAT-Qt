#include "MainWindow.hpp"
#include "EditablePanel.hpp"
#include "CommandEditor.hpp"
#include "ResourceEditor.hpp"
#include "AcceptanceView.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QString read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Report missing"); return QString::fromUtf8(f.readAll()).trimmed(); }
static void proof(const char *family,const char *operation) {
   const auto name=QString::fromLatin1(family),action=QString::fromLatin1(operation);
   const QStringList facets=action=="ok" ? QStringList{"validation-rejection","valid-commit-close","clean-noop","exact-source","undo-redo","numeric-result","sibling-isolation"}
      : QStringList{"discard-pending","retain-prior-apply","exact-source","no-extra-undo","sibling-isolation"};
   QJsonArray values; for (const auto &facet:facets) values.append(facet);
   const QJsonObject data{{"checkpoint","panel."+name+"."+action},{"family",name},{"operation",action},{"outcome","passed"},{"facets",values}};
   std::cout<<"NASA_CHECK "<<QJsonDocument(data).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaPanelButtons");
   try {
      require(argc==2,"Supply startup file"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Initialization failed"); window.show();
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *script=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(area && script,"Workspace missing"); const auto report=files.filePath("panel results.txt");
      auto source=[&](int value=2,int increment=3,int precision=16,const QString &reported="V") {
         return QString("% retain unrelated Unicode α\nCreate Variable V W;\nV = ")+QString::number(value)+";\nW = 17;\nCreate ReportFile R;\nR.Filename = '"+report+"';\nR.WriteHeaders = false;\nR.Precision = "+QString::number(precision)+";\nBeginMissionSequence;\nV = V + "+QString::number(increment)+"; % keep command\nReport R "+reported+";\n";
      };
      auto reset=[&] {
         for (auto *child:area->subWindowList()) if (auto *p=dynamic_cast<EditablePanel *>(child->widget())) { p->discardChanges(); child->close(); }
         events(); script->setPlainText(source()); require(window.buildScript(),"Fixture build failed"); script->document()->setModified(false); acceptanceView(window);
      };
      auto button=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton which) {
         auto *box=child->widget()->findChild<QDialogButtonBox *>(); require(box,"Panel buttons missing");
         auto *b=box->button(which); require(b && b->isEnabled(),"Panel action missing or disabled"); return b;
      };
      auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton which) {
         area->setActiveSubWindow(child); acceptanceView(*child); auto *b=button(child,which);
         require(b->isVisible(),"Panel action is hidden"); QTest::mouseClick(b,Qt::LeftButton); events();
      };
      auto resource=[&](const QString &name) {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Resource missing");
         tree->itemDoubleClicked(items.front(),0); events(); auto *child=area->activeSubWindow(); require(child && dynamic_cast<EditablePanel *>(child->widget()),"Resource panel missing"); acceptanceView(*child); return child;
      };
      auto scalar=[&](QMdiSubWindow *child) { auto *v=child->widget()->findChild<QLineEdit *>("parameterValue"); require(v,"Variable value field missing"); return v; };
      auto value=[] { return Moderator::Instance()->GetConfiguredObject("V")->GetRealParameter("Value"); };
      auto command=[&](const QString &needle="V = V +") {
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains(needle)) index=i;
         require(index>=0,"Assignment missing"); auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItem *entry=nullptr;
         for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) entry=*it;
         require(entry,"Assignment tree item missing"); tree->itemDoubleClicked(entry,0); events(); auto *child=area->activeSubWindow();
         require(child && dynamic_cast<CommandEditor *>(child->widget()),"Command panel missing"); acceptanceView(*child); return child;
      };
      auto text=[](QMdiSubWindow *child) { auto *v=child->widget()->findChild<QPlainTextEdit *>("commandSource"); require(v,"Command source missing"); return v; };
      auto run=[&](const QString &expected) {
         if (QFileInfo::exists(report)) require(QFile::remove(report),"Could not remove the prior report");
         require(window.runMission()==MainWindow::RunResult::Completed,"Mission failed"); require(read(report)==expected,"Independent numerical result differs");
      };

      reset(); QPointer<QMdiSubWindow> sibling=resource("R"),child=resource("V");
      const auto original=source(); const auto undoBefore=script->document()->availableUndoSteps();
      scalar(child)->setText("9");
      acceptanceView(*child);
      bool prompted=false; QTimer promptCloser;
      QObject::connect(&promptCloser,&QTimer::timeout,&window,[&] {
         if (auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); box && box->windowTitle()=="Unapplied changes") {
            prompted=true; promptCloser.stop(); box->button(QMessageBox::Cancel)->click();
         }
      });
      promptCloser.start(5);
      QTest::mouseClick(button(child,QDialogButtonBox::Close),Qt::LeftButton); events();
      promptCloser.stop();
      require(prompted && child && child->isVisible() && scalar(child)->text()=="9" && script->toPlainText()==original,"Close no longer preserves Cancel and pending edits");
      click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && sibling->isVisible() && script->toPlainText()==original && value()==2 && script->document()->availableUndoSteps()==undoBefore,"Resource Cancel committed data or closed its sibling");

      child=resource("V"); scalar(child)->setText("7"); click(child,QDialogButtonBox::Apply);
      require(child && child->isVisible() && scalar(child)->text()=="7" && script->toPlainText()==source(7),"Apply did not retain a clean refreshed resource panel");
      const auto afterApply=script->document()->availableUndoSteps(); scalar(child)->setText("11"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==source(7) && value()==7 && script->document()->availableUndoSteps()==afterApply,"Cancel lost a prior Apply or saved pending data");
      script->undo(); require(script->toPlainText()==original,"Resource Apply Undo changed unrelated source"); script->redo(); require(script->toPlainText()==source(7),"Resource Apply Redo differs");

      reset(); sibling=resource("R"); child=resource("V"); const auto cleanUndo=script->document()->availableUndoSteps();
      click(child,QDialogButtonBox::Ok); require(!child && sibling && script->toPlainText()==original && script->document()->availableUndoSteps()==cleanUndo,"Clean resource OK created an edit or closed a sibling");
      child=resource("V"); auto *status=child->widget()->findChild<QLabel *>("resourceStatus");
      require(status,"Resource diagnostic control missing"); const auto initialStatus=status->text();
      scalar(child)->setText("not a number"); click(child,QDialogButtonBox::Ok);
      require(child && child->isVisible() && scalar(child)->text()=="not a number" && value()==2 && script->toPlainText()==original && script->document()->availableUndoSteps()==cleanUndo,"Invalid resource OK closed, committed or lost pending input");
      require(!status->text().isEmpty() && status->text()!=initialStatus,"Invalid resource OK omitted its diagnostic");
      scalar(child)->setText("8"); click(child,QDialogButtonBox::Apply);
      require(child && child->isVisible() && script->toPlainText()==source(8),"Failed OK made the next Apply close its resource panel");
      const auto appliedUndo=script->document()->availableUndoSteps(); click(child,QDialogButtonBox::Ok);
      require(!child && sibling && script->document()->availableUndoSteps()==appliedUndo,"Clean OK after Apply changed history"); run("11");
      reset(); sibling=resource("R"); child=resource("V"); scalar(child)->setText("9");
      auto *resourcePanel=dynamic_cast<EditablePanel *>(child->widget()); const auto refreshResource=resourcePanel->onApplied;
      resourcePanel->onApplied=[&,refreshResource] { refreshResource(); area->setActiveSubWindow(sibling); };
      click(child,QDialogButtonBox::Ok);
      require(!child && sibling && sibling->isVisible() && script->toPlainText()==source(9) && value()==9,"Resource OK failed to commit then close only its owning window");
      script->undo(); require(script->toPlainText()==original,"Resource OK Undo was not exact"); script->redo(); require(script->toPlainText()==source(9),"Resource OK Redo was not exact"); run("12");

      reset(); sibling=resource("R"); child=command(); const auto commandUndo=script->document()->availableUndoSteps();
      text(child)->setPlainText("V = ; % keep command"); click(child,QDialogButtonBox::Ok);
      require(child && child->isVisible() && script->toPlainText()==original && script->document()->availableUndoSteps()==commandUndo,"Invalid command OK changed source or closed its panel");
      auto *commandStatus=child->widget()->findChild<QLabel *>("commandStatus");
      require(commandStatus && !commandStatus->text().isEmpty() && commandStatus->text()!=
         "Apply validates the complete mission and updates the script. Branches include their enclosed commands.","Invalid command OK omitted its diagnostic");
      text(child)->setPlainText("V = V + 4; % keep command"); click(child,QDialogButtonBox::Apply);
      require(child && child->isVisible() && script->toPlainText()==source(2,4),"Failed command OK made later Apply close or lose the command");
      const auto commandAppliedUndo=script->document()->availableUndoSteps(); text(child)->setPlainText("V = V + 9; % keep command"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==source(2,4) && script->document()->availableUndoSteps()==commandAppliedUndo,"Command Cancel lost prior Apply or saved pending changes");
      child=command(); click(child,QDialogButtonBox::Ok);
      require(!child && sibling && script->document()->availableUndoSteps()==commandAppliedUndo,"Clean command OK created a no-op edit"); run("6");
      reset(); sibling=resource("R"); child=command(); text(child)->setPlainText("V = V + 10; % keep command"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==original,"Command Cancel committed a pending command or closed a sibling");
      child=command(); text(child)->setPlainText("V = V + 4; % keep command");
      auto *commandPanel=dynamic_cast<EditablePanel *>(child->widget()); const auto refreshCommand=commandPanel->onApplied;
      commandPanel->onApplied=[&,refreshCommand] { refreshCommand(); area->setActiveSubWindow(sibling); };
      click(child,QDialogButtonBox::Ok);
      require(!child && sibling && script->toPlainText()==source(2,4),"Command OK did not commit then close its owning window");
      script->undo(); require(script->toPlainText()==original,"Command OK Undo was not exact"); script->redo(); require(script->toPlainText()==source(2,4),"Command OK Redo was not exact"); run("6");

      auto precision=[](QMdiSubWindow *owner) {
         auto *table=owner->widget()->findChild<QTableWidget *>("resourceProperties"); require(table,"Report property table missing");
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="Precision") return table->item(row,1);
         throw std::runtime_error("Report precision field missing");
      };
      reset(); sibling=resource("V"); child=resource("R"); const auto reportUndo=script->document()->availableUndoSteps();
      precision(child)->setText("-1"); click(child,QDialogButtonBox::Ok);
      require(child && child->isVisible() && precision(child)->text()=="-1" && script->toPlainText()==original && script->document()->availableUndoSteps()==reportUndo,"Invalid ReportFile OK changed source or discarded input");
      auto *reportStatus=child->widget()->findChild<QLabel *>("resourceStatus"); require(reportStatus && !reportStatus->text().isEmpty() && reportStatus->text()!= "Apply validates changes and updates the mission script.","ReportFile OK validation omitted its diagnostic");
      precision(child)->setText("15"); click(child,QDialogButtonBox::Apply);
      require(child && child->isVisible() && script->toPlainText()==source(2,3,15),"ReportFile Apply after invalid OK did not remain open");
      const auto precisionUndo=script->document()->availableUndoSteps(); precision(child)->setText("14"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==source(2,3,15) && Moderator::Instance()->GetConfiguredObject("R")->GetIntegerParameter("Precision")==15 && script->document()->availableUndoSteps()==precisionUndo,"ReportFile Cancel lost prior Apply or saved pending precision");
      child=resource("R"); click(child,QDialogButtonBox::Ok); require(!child && sibling && script->document()->availableUndoSteps()==precisionUndo,"Clean ReportFile OK added an edit");
      reset(); sibling=resource("V"); child=resource("R"); precision(child)->setText("14"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==original,"ReportFile Cancel committed pending precision");
      child=resource("R"); precision(child)->setText("14"); click(child,QDialogButtonBox::Ok);
      require(!child && sibling && script->toPlainText()==source(2,3,14) && Moderator::Instance()->GetConfiguredObject("R")->GetIntegerParameter("Precision")==14,"ReportFile OK failed to commit and close");
      script->undo(); require(script->toPlainText()==original,"ReportFile OK Undo was not exact"); script->redo(); require(script->toPlainText()==source(2,3,14),"ReportFile OK Redo was not exact"); run("5");

      reset(); sibling=resource("V"); child=command("Report R"); const auto reportCommandUndo=script->document()->availableUndoSteps();
      text(child)->setPlainText("Report R MissingPanelVariable;"); click(child,QDialogButtonBox::Ok);
      require(child && child->isVisible() && script->toPlainText()==original && script->document()->availableUndoSteps()==reportCommandUndo,"Invalid Report command OK discarded input or committed changes");
      auto *reportCommandStatus=child->widget()->findChild<QLabel *>("commandStatus"); require(reportCommandStatus && !reportCommandStatus->text().isEmpty() && reportCommandStatus->text()!= "Apply validates the complete mission and updates the script. Branches include their enclosed commands.","Report command validation diagnostic missing");
      text(child)->setPlainText("Report R W;"); click(child,QDialogButtonBox::Apply);
      require(child && child->isVisible() && script->toPlainText()==source(2,3,16,"W"),"Report command Apply after invalid OK closed or lost the panel");
      const auto reportAppliedUndo=script->document()->availableUndoSteps(); text(child)->setPlainText("Report R V;"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==source(2,3,16,"W") && script->document()->availableUndoSteps()==reportAppliedUndo,"Report command Cancel lost previous Apply");
      child=command("Report R"); click(child,QDialogButtonBox::Ok); require(!child && sibling && script->document()->availableUndoSteps()==reportAppliedUndo,"Clean Report command OK added a no-op"); run("17");
      reset(); sibling=resource("V"); child=command("Report R"); text(child)->setPlainText("Report R W;"); click(child,QDialogButtonBox::Cancel);
      require(!child && sibling && script->toPlainText()==original,"Report command Cancel committed pending parameters");
      child=command("Report R"); text(child)->setPlainText("Report R W;"); click(child,QDialogButtonBox::Ok);
      require(!child && sibling && script->toPlainText()==source(2,3,16,"W"),"Report command OK did not commit and close its panel");
      script->undo(); require(script->toPlainText()==original,"Report command OK Undo was not exact"); script->redo(); require(script->toPlainText()==source(2,3,16,"W"),"Report command OK Redo was not exact"); run("17");

      // Special resource factories share the same clean OK/Cancel lifecycle.
      reset(); for (const auto &name:{QString("SolarSystem"),QString("Earth")}) {
         child=resource(name); const auto before=script->toPlainText(); const auto history=script->document()->availableUndoSteps();
         click(child,QDialogButtonBox::Ok); require(!child && script->toPlainText()==before && script->document()->availableUndoSteps()==history,"Special resource clean OK changed source/history");
         child=resource(name); click(child,QDialogButtonBox::Cancel); require(!child && script->toPlainText()==before,"Special resource Cancel changed source");
      }
      proof("ReportFile","ok"); proof("ReportFile","cancel");
      proof("Report","ok"); proof("Report","cancel");
      std::cout<<"PASS NASA panel OK/Cancel: invalid input retained, clean OK is a no-op, valid OK commits before close, Cancel preserves prior Apply, owning-window isolation, exact Undo/Redo, and numerical results 11/12/6/5/17.\n";
      return 0;
   } catch (BaseException &e) { std::cerr<<e.GetFullMessage()<<'\n'; }
     catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; }
   return 1;
}
