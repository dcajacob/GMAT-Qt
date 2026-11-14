#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Resource name artifact missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(data)==data.size(),"Resource name artifact write failed"); }

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaResourceNames");
   try {
      require(argc==2&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Resource names need native startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Resource name fixtures unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto output=files.filePath("resource names Ω.csv"),saved=files.filePath("resource names Ω.script");
      const auto artifact=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &data) { if (!artifact.isEmpty()) write(artifact+"/"+name,data); };
      const QString mission="% preserve name validation Ω\nCreate Variable SourceValue Keep;\nSourceValue = 13.25;\nKeep = 41;\nCreate ReportFile Witness;\nWitness.Filename = '"+output+"';\nWitness.WriteHeaders = false;\nWitness.FixedWidth = false;\nWitness.Delimiter = ',';\nWitness.Precision = 25;\nBeginMissionSequence;\nReport Witness SourceValue Keep;\n";
      MainWindow window; window.resize(1200,850); window.show(); require(window.initialize(startup),"Resource name runtime unavailable"); acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); require(source&&tree,"Resource name workspace unavailable");
      source->setPlainText(mission); save("initial.script",mission.toUtf8()); QJsonArray reports,attempts,accepted;
      auto run=[&](const QString &phase) {
         require(window.runMission()==MainWindow::RunResult::Completed,"Resource name witness failed"); const auto raw=read(output); const auto fields=QString::fromUtf8(raw).trimmed().split(','); require(fields.size()==2,"Resource name report cardinality differs"); QJsonArray values;
         for (const auto &field:fields) { bool ok=false; const auto value=field.toDouble(&ok); require(ok&&std::isfinite(value),"Resource name report nonfinite"); values.append(value); }
         require(values[0].toDouble()==13.25&&values[1].toDouble()==41,"Resource naming changed independent witness results"); reports.append(QJsonObject{{"phase",phase},{"values",values}}); save(phase+".csv",raw); save("reports.json",QJsonDocument(reports).toJson()); return raw;
      };
      const auto baseline=run("baseline"); require(window.saveScriptTo(saved),"Resource name baseline save failed");
      QStringList folders;
      for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1).isValid()&&(*it)->data(0,Qt::UserRole).toString().isEmpty()&&QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match((*it)->text(0)).hasMatch()) folders.append((*it)->text(0));
      folders.removeDuplicates(); folders.sort(); require(folders.contains("Hardware")&&folders.contains("Spacecraft"),"Native resource-folder census incomplete");
      save("folder-census.json",QJsonDocument(QJsonObject{{"identifier_folder_names",QJsonArray::fromStringList(folders)}}).toJson());
      auto item=[&](const QString &name) {
         for (auto *entry:tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive)) if (!entry->data(0,Qt::UserRole).toString().isEmpty()) return entry;
         throw std::runtime_error("Named resource tree entry missing");
      };
      auto invoke=[&](const QString &name,const QString &actionName,const QString &dialogName,const std::function<void(QDialog *)> &answer) {
         auto *entry=item(name); tree->expandAll(); tree->scrollToItem(entry); tree->setCurrentItem(entry); tree->setFocus(); events();
         require(tree->isVisible()&&tree->viewport()->rect().intersects(tree->visualItemRect(entry)),"Resource row not visible"); acceptanceView(window);
         std::exception_ptr failure; bool menuSeen=false,dialogSeen=false; QTimer timer,watchdog; timer.setInterval(20); watchdog.setSingleShot(true);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            try {
               if (!menuSeen) {
                  auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); if (!menu) return;
                  require(menu->objectName()=="resourceContextMenu"&&menu->windowHandle()&&QTest::qWaitForWindowExposed(menu->windowHandle()),"Resource context native surface unavailable");
                  auto *action=menu->findChild<QAction *>(actionName); require(action&&action->isEnabled(),"Resource context action unavailable"); acceptanceView(*menu); menuSeen=true;
                  QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center()); return;
               }
               auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog||dialog->objectName()!=dialogName) return;
               timer.stop(); dialogSeen=true; require(dialog->windowHandle()&&QTest::qWaitForWindowExposed(dialog->windowHandle()),"Resource name dialog native surface unavailable"); acceptanceView(*dialog); answer(dialog);
            } catch (...) {
               failure=std::current_exception(); timer.stop(); if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close();
            }
         });
         QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { failure=std::make_exception_ptr(std::runtime_error("Resource name dialog did not finish")); if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); });
         timer.start(); watchdog.start(180000); const auto point=tree->visualItemRect(entry).center(); QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point)); QApplication::sendEvent(tree->viewport(),&event); timer.stop(); watchdog.stop(); if (failure) std::rethrow_exception(failure); require(menuSeen&&dialogSeen,"Resource context/modal route not reached"); events();
      };
      auto operation=[&](const QString &mode,const QString &original,const QString &next) {
         const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps();
         const auto action=mode=="Rename" ? "renameResource" : mode=="Clone" ? "cloneResource" : "addResource_Variable";
         const auto dialog=mode=="Rename" ? "renameResourceDialog" : mode=="Clone" ? "cloneResourceDialog" : "newResourceDialog";
         invoke(original,action,dialog,[&](QDialog *base) {
            QPointer<QDialog> guarded=base; auto *name=base->findChild<QLineEdit *>("resourceName"); const auto buttonName=mode=="Rename" ? "resourceRenameButtons" : mode=="Clone" ? "resourceCloneButtons" : "resourceCreationButtons"; auto *buttons=base->findChild<QDialogButtonBox *>(buttonName);
            const auto label=mode=="Rename" ? "resourceRenameStatus" : mode=="Clone" ? "resourceCloneStatus" : "resourceCreationStatus";
            auto *status=base->findChild<QLabel *>(label); auto *ok=buttons ? buttons->button(QDialogButtonBox::Ok) : nullptr; require(name&&name->isVisible()&&status&&ok,"Resource name form controls missing");
            QStringList invalid{"Hardware","function","bad name","1Bad","GMAT","Propagate","Keep"}; invalid.append(folders); invalid.removeDuplicates();
            for (const auto &candidate:invalid) {
               auto *previousCandidate=object(candidate);
               name->setFocus(); name->selectAll(); QTest::keyClicks(name,candidate); require(name->text()==candidate,"Resource invalid name input differs");
               if (ok->isEnabled()) QTest::mouseClick(ok,Qt::LeftButton); events();
               const bool stayed=guarded&&guarded->isVisible(); const auto diagnostic=stayed ? status->text() : QString("Accepted or closed unexpectedly");
               attempts.append(QJsonObject{{"operation",mode},{"name",candidate},{"dialog_stayed_open",stayed},{"source_unchanged",source->toPlainText()==before},{"diagnostic",diagnostic}}); save("attempts.json",QJsonDocument(attempts).toJson()); save("latest.script",source->toPlainText().toUtf8());
               require(stayed&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&object(original)&&object(candidate)==previousCandidate,"Reserved folder/keyword name accepted or rejection changed source/model/history");
               if (folders.contains(candidate)) require(diagnostic.contains("folder",Qt::CaseInsensitive),"Folder-name diagnostic does not identify the conflict");
               if (candidate=="function") require(diagnostic.contains("reserved",Qt::CaseInsensitive),"Function keyword diagnostic does not identify the conflict");
               acceptanceView(*guarded);
            }
            name->setFocus(); name->selectAll(); QTest::keyClicks(name,next); require(ok->isEnabled(),"Valid corrected name disabled"); QTest::mouseClick(ok,Qt::LeftButton); require(guarded&&guarded->result()==QDialog::Accepted&&!guarded->isVisible(),"Corrected valid resource name did not commit");
         });
         require(object(next)&&(!object(original)||mode!="Rename"),"Valid resource name did not update native objects");
         const auto after=source->toPlainText(); const auto value=object(next)->GetRealParameter("Value"); require(object(next)->GetTypeName()=="Variable"&&value==(mode=="Create" ? 0 : 13.25),"Corrected resource name lost typed native value"); accepted.append(QJsonObject{{"operation",mode},{"name",next},{"source_changed",after!=before},{"native_value",value}}); save("accepted.json",QJsonDocument(accepted).toJson());
         require(after!=before,"Valid resource naming did not change source"); require(run(mode.toLower())==baseline,"Valid naming changed witness report"); source->undo(); require(source->toPlainText()==before&&window.buildScript(),"Resource name Undo was not exact"); source->redo(); require(source->toPlainText()==after&&window.buildScript(),"Resource name Redo was not exact");
      };
      operation("Rename","SourceValue","Renamed_Source"); operation("Clone","Renamed_Source","Independent_Copy"); operation("Create","Renamed_Source","Created_Value");
      const auto final=source->toPlainText(); require(window.saveScriptTo(saved)&&window.loadScript(saved)&&source->toPlainText()==final,"Resource naming Unicode reopen changed source"); require(run("reopened")==baseline,"Reopened naming changed witness report"); save("applied.script",final.toUtf8());
      const QJsonObject proof{{"checkpoint","native-resource-names"},{"outcome","passed"},{"family","resource-names"},{"operations",3},{"folder_names",folders.size()},{"invalid_rejections",attempts.size()},{"raw_report_values",10},{"facets",QJsonArray{"actual-native-resource-context-and-name-dialog-pointer-routes","all-current-identifier-folder-names-and-function-keyword-rejected","atomic-correction-exact-history-and-typed-native-objects","independent-literal-reports-and-unicode-reopen"}},{"scope","Current identifier-like Resource Tree folder labels plus selected syntax/duplicate/keyword names across Variable Rename/Clone/Create dialogs, correction, exact history, native values and witness reports. Context menu is opened through a Qt viewport ContextMenu event, not OS input injection. Other resource types/name encodings/domain bounds, dependencies, protected/formation/optional-backend semantics, platform/hardware/stress/science and full NASA/current-build qualification remain open."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; source->document()->setModified(false); return 0;
   } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
     catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
