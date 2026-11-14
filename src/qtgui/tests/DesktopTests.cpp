#include "MainWindow.hpp"
#include "AboutDialog.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include "SolarSystemPanel.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QEventLoop>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QTemporaryDir>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QMessageBox>
#include <QMenu>
#include <QTreeWidgetItemIterator>
#include <QTimer>
#include <QRegularExpression>
#include <iostream>
#include <stdexcept>
#include <functional>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
// Opt-in viewing only: ordinary CTest timing and assertions are unchanged.
static int watchDelay() { return qBound(0,qEnvironmentVariableIntValue("GMAT_QT_WATCH_MS"),5000); }
static void watchPoint(const char *step)
{
   if (!watchDelay()) return;
   std::cout<<"WATCH: "<<step<<std::endl;
   QEventLoop loop; QTimer::singleShot(watchDelay(),&loop,&QEventLoop::quit); loop.exec();
}

static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Desktop fixture write failed"); }
static void escape(QWidget *widget) { QKeyEvent key(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier); QApplication::sendEvent(widget,&key); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(watchDelay(),owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtDesktop");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; MainWindow window; window.show(); require(window.initialize(startup),"Desktop runtime unavailable");
      if (watchDelay()) std::cout<<"WATCH: Qt platform="<<QGuiApplication::platformName().toStdString()<<" DISPLAY="<<qgetenv("DISPLAY").constData()<<std::endl;
      watchPoint("Main workspace and mission source");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto original=editor->toPlainText();
      editor->appendPlainText("% retain pending text while inspecting About"); const auto pending=editor->toPlainText();
      const auto root=QString::fromStdString(FileManager::Instance()->GetRootPath()); QFile license(QDir(root).filePath("License.txt"));
      require(license.open(QIODevice::ReadOnly),"Runtime license missing"); const auto expected=QString::fromUtf8(license.readAll());
      auto *action=window.findChild<QAction *>("aboutGMAT"); require(action,"About menu action missing"); std::exception_ptr failure;
      later(&window,failure,[&] {
         auto *about=dynamic_cast<AboutDialog *>(window.findChild<QDialog *>("aboutDialog")); require(about && about->isVisible(),"About menu did not open dialog");
         const auto info=about->findChild<QLabel *>("aboutInfo")->text(); auto *global=GmatGlobal::Instance();
         require(info.contains(QString::fromStdString(global->GetGmatVersion())) && info.contains(QString::fromStdString(global->GetGMATBuildDate())) && info.contains(QString::fromLatin1(qVersion())) && info.contains("64-bit"),"About build/runtime details incorrect");
         auto *credits=about->findChild<QPlainTextEdit *>("aboutCredits"); require(credits && credits->isReadOnly() && credits->toPlainText().contains("SPICE") && credits->toPlainText().contains("OpenSceneGraph"),"About credits unavailable");
         const auto links=about->findChild<QLabel *>("aboutLinks")->text(); require(links.contains("https://sourceforge.net/projects/gmat/") && links.contains("mailto:gmat@gsfc.nasa.gov"),"About project/contact actions missing");
         later(about,failure,[&] {
            auto *dialog=about->findChild<QDialog *>("licenseDialog"); require(dialog && dialog->isVisible(),"Offline license did not open");
            auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text && text->isReadOnly() && text->lineWrapMode()==QPlainTextEdit::NoWrap && text->toPlainText()==expected,"License text missing or modified");
            require(!dialog->findChild<QPushButton *>("inspectionSave") && dialog->findChild<QPushButton *>("inspectionFind"),"License inspection actions incorrect");
            if (!capture.isEmpty()) require(dialog->grab().save(capture+".license.png"),"License capture failed");
            escape(dialog);
         });
         about->findChild<QPushButton *>("showLicense")->click(); if (failure) std::rethrow_exception(failure);
         require(about->findChild<QLabel *>("aboutError")->text().isEmpty(),"Valid license produced an error");
         about->resize(460,380); QApplication::processEvents();
         auto *close=about->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close);
         require(about->rect().contains(close->mapTo(about,close->rect().bottomRight())),"Compact About hides Close");
         if (!capture.isEmpty()) require(about->grab().save(capture+".about.png"),"About capture failed");
         escape(about);
      }); action->trigger(); if (failure) std::rethrow_exception(failure);
      require(editor->toPlainText()==pending && editor->document()->isUndoAvailable(),"About/license changed pending mission source or Undo");
      editor->undo(); require(editor->toPlainText()==original,"About/license damaged pending Undo");
      QTemporaryDir files; require(files.isValid(),"Desktop fixture directory unavailable");
      AboutDialog missing(files.path()); missing.show(); auto *show=missing.findChild<QPushButton *>("showLicense");
      show->click(); auto *error=missing.findChild<QLabel *>("aboutError"); require(error->text().contains(files.filePath("License.txt")) && error->text().contains("Cannot read"),"Missing license is silently accepted");
      write(files.filePath("License.txt"),QByteArray("bad UTF-8 \xff")); show->click(); require(error->text().contains("UTF-8"),"Malformed license is silently truncated");
      write(files.filePath("License.txt"),expected.toUtf8());
      later(&missing,failure,[&] { auto *dialog=missing.findChild<QDialog *>("licenseDialog"); require(dialog && dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText()==expected,"Restored license cannot reopen"); escape(dialog); });
      show->click(); if (failure) std::rethrow_exception(failure); require(error->text().isEmpty(),"Restored license retained stale failure"); escape(&missing);
      const auto report=files.filePath("apply.txt");
      const QString prefix="% keep Apply comments α\nCreate Variable V W;\nV = 1; % first initializer\nW = 2;\nCreate Spacecraft Sat;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\n";
      const QString mission="BeginMissionSequence;\nReport 'keep label' Values V W Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(prefix+mission); require(window.buildScript(),"Apply fixture build failed");
      auto *workspace=window.findChild<QMdiArea *>("workspace");
      auto open=[&](const QString &name) {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); require(tree,"Resource tree missing");
         const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Apply resource missing");
         require(QMetaObject::invokeMethod(tree,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,items[0]),Q_ARG(int,0)),"Resource cannot open");
         for (auto *child:workspace->subWindowList()) if (child->property("resourceName").toString()==name) { watchPoint("Opened resource editor"); return child; }
         throw std::runtime_error("Apply panel missing");
      };
      auto apply=[](QMdiSubWindow *child) { watchPoint("Pending editor changes before Apply"); child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); watchPoint("Apply result or validation message"); };
      auto *v=open("V"),*w=open("W"),*sat=open("Sat");
      auto *oldSat=sat->widget(); auto *section=oldSat->findChild<QTabBar *>("propertySections"); section->setCurrentIndex(section->count()-1);
      oldSat->findChild<QLineEdit *>("propertyFilter")->setText("DryMass"); oldSat->findChild<QTableWidget *>("resourceProperties")->setColumnWidth(0,211);
      w->widget()->findChild<QLineEdit *>("parameterValue")->setText("7");
      for (const auto &value:QStringList{"5","9"}) {
         auto *previous=v->widget(); auto *input=previous->findChild<QLineEdit *>("parameterValue"); input->setText(value); input->setFocus(); apply(v);
         require(v->isVisible() && v->widget()!=previous && !dynamic_cast<EditablePanel *>(v->widget())->hasChanges() && v->widget()->findChild<QLineEdit *>("parameterValue")->text()==value,"Apply did not refresh the open resource");
         require(v->property("sourceScript").toString()==editor->toPlainText() && editor->toPlainText().endsWith(mission) && editor->toPlainText().contains("% first initializer"),"Apply lost source or its updated snapshot");
      }
      require(sat->widget()!=oldSat && sat->isVisible() && sat->widget()->findChild<QLineEdit *>("propertyFilter")->text()=="DryMass" && sat->widget()->findChild<QTableWidget *>("resourceProperties")->columnWidth(0)==211,"Clean dependent panel lost its state on refresh");
      require(dynamic_cast<EditablePanel *>(w->widget())->hasChanges() && w->widget()->findChild<QLineEdit *>("parameterValue")->text()=="7","Apply discarded another panel's pending edit");
      const auto current=editor->toPlainText(); apply(w); require(editor->toPlainText()==current && dynamic_cast<EditablePanel *>(w->widget())->hasChanges(),"Stale pending Apply changed the mission");
      dynamic_cast<EditablePanel *>(w->widget())->discardChanges(); w->close();
      require(window.runMission()==MainWindow::RunResult::Completed,"Repeated Apply mission failed");
      QFile result(report); require(result.open(QIODevice::ReadOnly),"Apply report missing"); const auto expectedReport=result.readAll(); result.close();
      watchPoint("Executed report mission; checking Undo and Redo next");
      editor->undo(); const auto firstApply=editor->toPlainText(); require(firstApply!=current,"Repeated Apply Undo missing"); editor->undo(); require(editor->toPlainText()==prefix+mission,"First Apply Undo not exact"); editor->redo(); require(editor->toPlainText()==firstApply,"First Apply Redo not exact"); editor->redo(); require(editor->toPlainText()==current,"Repeated Apply Redo not exact");
      require(window.saveScriptTo(files.filePath("Apply ü.script")) && window.loadScript(files.filePath("Apply ü.script")) && window.runMission()==MainWindow::RunResult::Completed,"Apply save/reopen failed");
      require(result.open(QIODevice::ReadOnly) && result.readAll()==expectedReport,"Apply save/reopen changed calculations"); result.close();
      auto reference=prefix; reference.replace("V = 1;","V = 9;"); editor->setPlainText(reference+mission);
      require(window.runMission()==MainWindow::RunResult::Completed && result.open(QIODevice::ReadOnly) && result.readAll()==expectedReport,"Apply report differs from independent script"); result.close();
      const QString commandMission="BeginMissionSequence;\nV = V + 1; % retain command comment\nW = W + 1;\nReport Values V W Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(reference+commandMission); require(window.buildScript(),"Command Apply fixture failed");
      auto *tree=window.findChild<QTreeWidget *>("Mission");
      auto item=[&](int index) {
         for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) return *it;
         throw std::runtime_error("Command Apply mission item missing");
      };
      auto find=[&](const QString &text) {
         const auto snapshot=window.missionSnapshot();
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains(text)) return i;
         throw std::runtime_error("Command Apply mission node missing");
      };
      auto commandWindow=[&]() {
         for (auto *child:workspace->subWindowList(QMdiArea::StackingOrder)) if (child->isVisible() && dynamic_cast<CommandEditor *>(child->widget())) {
            if (child==workspace->currentSubWindow()) return child;
         }
         throw std::runtime_error("Command Apply editor missing");
      };
      tree->itemDoubleClicked(item(find("V = V + 1")),0); auto *command=commandWindow();
      const auto originalCommandScript=editor->toPlainText(); QString firstCommandScript;
      for (const auto &step:QStringList{"2","3"}) {
         auto *old=command->widget(); auto *text=old->findChild<QPlainTextEdit *>("commandSource");
         auto statement=text->toPlainText(); statement.replace(QRegularExpression("V \\+ [123]"),"V + "+step); text->setPlainText(statement); text->setFocus(); apply(command);
         require(command->isVisible() && command->widget()!=old && !dynamic_cast<CommandEditor *>(command->widget())->hasChanges() &&
            command->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText().contains("V + "+step),"Command Apply did not keep and refresh its editor");
         require(editor->toPlainText().contains("% retain command comment"),"Command Apply lost its comment");
         if (step=="2") firstCommandScript=editor->toPlainText();
      }
      watchPoint("Accepted command edits");
      const auto acceptedCommandScript=editor->toPlainText();
      auto *bad=command->widget()->findChild<QPlainTextEdit *>("commandSource"); bad->setPlainText("V = MissingName;"); apply(command);
      require(editor->toPlainText()==acceptedCommandScript && dynamic_cast<CommandEditor *>(command->widget())->hasChanges() && bad->toPlainText()=="V = MissingName;","Failed command Apply lost accepted source or pending text");
      dynamic_cast<EditablePanel *>(command->widget())->discardChanges(); command->close();
      require(window.runMission()==MainWindow::RunResult::Completed && result.open(QIODevice::ReadOnly),"Repeated command Apply report missing"); const auto commandReport=result.readAll(); result.close();
      editor->undo(); require(editor->toPlainText()==firstCommandScript,"Command Apply Undo did not restore first Apply"); editor->undo(); require(editor->toPlainText()==originalCommandScript,"Command Apply Undo did not restore exact original");
      editor->redo(); editor->redo(); require(editor->toPlainText()==acceptedCommandScript,"Command Apply Redo not exact");
      require(window.saveScriptTo(files.filePath("command Apply ü.script")) && window.loadScript(files.filePath("command Apply ü.script")) && window.runMission()==MainWindow::RunResult::Completed && result.open(QIODevice::ReadOnly) && result.readAll()==commandReport,"Command Apply save/reopen changed calculations"); result.close();
      auto independent=reference+commandMission; independent.replace("V = V + 1;","V = V + 3;"); editor->setPlainText(independent);
      require(window.runMission()==MainWindow::RunResult::Completed && result.open(QIODevice::ReadOnly) && result.readAll()==commandReport,"Command Apply differs from independent source calculations"); result.close();
      // Exercise the actual context-menu insertion routes, including after a
      // branch: a refreshed editor must refer to the insertion, not its anchor.
      for (const auto &operation:QStringList{"Insert before…","Insert after…","Append command…"}) {
         editor->setPlainText(reference+"BeginMissionSequence;\nIf V > 0;\n   W = W + 1;\nEndIf;\nReport Values V W;\n"); require(window.buildScript(),"Insertion Apply fixture failed");
         int anchor=-1; const auto snapshot=window.missionSnapshot();
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="If") anchor=i;
         require(anchor>=0,"Insertion branch anchor missing"); tree->expandAll(); auto *selected=item(anchor); tree->scrollToItem(selected); QApplication::processEvents();
         std::exception_ptr menuFailure;
         QTimer::singleShot(watchDelay(),&window,[&] {
            try {
               auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Insertion menu did not open"); QAction *choice=nullptr;
               for (auto *action:menu->actions()) if (action->text()==operation) choice=action;
               require(choice && choice->isEnabled(),"Insertion action unavailable"); menu->setActiveAction(choice); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
            } catch (...) { menuFailure=std::current_exception(); if (auto *popup=QApplication::activePopupWidget()) popup->close(); }
         });
         tree->customContextMenuRequested(tree->visualItemRect(selected).center()); if (menuFailure) std::rethrow_exception(menuFailure);
         auto *insert=commandWindow(); insert->widget()->findChild<QPlainTextEdit *>("commandSource")->setPlainText("V = V + 5;"); apply(insert);
         require(insert->isVisible() && !dynamic_cast<CommandEditor *>(insert->widget())->hasChanges() && insert->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText().contains("V + 5"),"Accepted insertion did not become an edit panel");
         auto replacement=insert->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText(); replacement.replace("V + 5","V + 6"); insert->widget()->findChild<QPlainTextEdit *>("commandSource")->setPlainText(replacement); apply(insert);
         require(editor->toPlainText().count("V + 6")==1 && !editor->toPlainText().contains("V + 5") && editor->toPlainText().contains("If V > 0") && editor->toPlainText().contains("W = W + 1"),"Second insertion Apply duplicated the command or edited its anchor");
         insert->close();
      }
      watchPoint("Completed visible desktop checks");
      std::cout<<"PASS: About menu engine/build/runtime details, credits, exact offline license, read-only/search/Close/Escape, compact layout, missing/malformed license correction and pending mission/Undo preservation\n";
      std::cout<<"PASS: repeated workspace Apply keeps resource windows open, refreshes clean snapshots/page/filter/columns, preserves other pending edits, rejects stale Apply, exact Undo/Redo, Unicode save/reopen and independent numeric reports\n";
      std::cout<<"PASS: repeated command Apply stays open and refreshes accepted controls, rejects invalid changes without losing pending text, exact Undo/Redo, Unicode save/reopen and independent reports; before/after-branch/append insertion becomes editing without duplication\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
